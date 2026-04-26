#include "ClaudeClient.h"
#include "ApiKeyStore.h"
#include "SystemPrompt.h"
#include <juce_events/juce_events.h>

namespace
{
    juce::String buildRequestBody(const juce::String& systemPrompt,
                                  const juce::String& userPrompt,
                                  const juce::String& model,
                                  int                 numVariations,
                                  int                 maxTokens = 2048)
    {
        // Build the request as a juce::DynamicObject tree, then serialize.
        auto* sysBlock = new juce::DynamicObject();
        sysBlock->setProperty("type", "text");
        sysBlock->setProperty("text", systemPrompt);
        {
            auto* cc = new juce::DynamicObject();
            cc->setProperty("type", "ephemeral");
            sysBlock->setProperty("cache_control", juce::var(cc));
        }
        juce::Array<juce::var> systemArr;
        systemArr.add(juce::var(sysBlock));

        auto* userMsg = new juce::DynamicObject();
        userMsg->setProperty("role", "user");
        const juce::String content = "Return " + juce::String(numVariations)
            + " distinct patches via parallel set_patch tool calls for the description: "
            + userPrompt;
        userMsg->setProperty("content", content);
        juce::Array<juce::var> messages;
        messages.add(juce::var(userMsg));

        // Tool: set_patch
        auto* paramsSchema = new juce::DynamicObject();
        paramsSchema->setProperty("type", "object");
        paramsSchema->setProperty("description",
                                  "Sparse map of paramId -> normalized value (0..1) or choice index.");
        {
            auto* addProps = new juce::DynamicObject();
            addProps->setProperty("type", "number");
            paramsSchema->setProperty("additionalProperties", juce::var(addProps));
        }

        auto* toolSchema = new juce::DynamicObject();
        toolSchema->setProperty("type", "object");
        {
            auto* props = new juce::DynamicObject();
            {
                auto* nameProp = new juce::DynamicObject();
                nameProp->setProperty("type", "string");
                nameProp->setProperty("maxLength", 32);
                props->setProperty("name", juce::var(nameProp));
            }
            {
                auto* descProp = new juce::DynamicObject();
                descProp->setProperty("type", "string");
                descProp->setProperty("maxLength", 120);
                props->setProperty("description", juce::var(descProp));
            }
            {
                auto* tagsProp = new juce::DynamicObject();
                tagsProp->setProperty("type", "array");
                auto* itemsProp = new juce::DynamicObject();
                itemsProp->setProperty("type", "string");
                tagsProp->setProperty("items", juce::var(itemsProp));
                tagsProp->setProperty("maxItems", 4);
                props->setProperty("tags", juce::var(tagsProp));
            }
            props->setProperty("params", juce::var(paramsSchema));
            toolSchema->setProperty("properties", juce::var(props));
        }
        {
            juce::Array<juce::var> required;
            required.add("name");
            required.add("params");
            toolSchema->setProperty("required", required);
        }

        auto* tool = new juce::DynamicObject();
        tool->setProperty("name", "set_patch");
        tool->setProperty("description", "Apply a synth patch as a sparse map of paramId -> value.");
        tool->setProperty("input_schema", juce::var(toolSchema));
        juce::Array<juce::var> tools;
        tools.add(juce::var(tool));

        auto* toolChoice = new juce::DynamicObject();
        toolChoice->setProperty("type", "any");

        auto* root = new juce::DynamicObject();
        root->setProperty("model", model);
        root->setProperty("max_tokens", maxTokens);
        root->setProperty("system", systemArr);
        root->setProperty("messages", messages);
        root->setProperty("tools", tools);
        root->setProperty("tool_choice", juce::var(toolChoice));

        return juce::JSON::toString(juce::var(root));
    }

    // Parse Anthropic response. Returns empty vector + sets error if shape invalid.
    std::vector<Variation> parseVariations(const juce::var& v, ClaudeClientError& err)
    {
        std::vector<Variation> out;
        err = ClaudeClientError::None;

        auto content = v.getProperty("content", juce::var());
        if (! content.isArray())
        {
            err = ClaudeClientError::SchemaError;
            return out;
        }

        for (int i = 0; i < content.size(); ++i)
        {
            const auto& block = content[i];
            if (block.getProperty("type", juce::var()).toString() != "tool_use")
                continue;
            if (block.getProperty("name", juce::var()).toString() != "set_patch")
                continue;

            const auto input = block.getProperty("input", juce::var());
            Variation var;
            var.name        = input.getProperty("name", juce::var()).toString();
            var.description = input.getProperty("description", juce::var()).toString();

            if (auto tagsArr = input.getProperty("tags", juce::var()); tagsArr.isArray())
                for (int t = 0; t < tagsArr.size(); ++t)
                    var.tags.add(tagsArr[t].toString());

            const auto paramsObj = input.getProperty("params", juce::var());
            if (auto* obj = paramsObj.getDynamicObject())
            {
                for (auto& prop : obj->getProperties())
                {
                    const double d = (double) prop.value;
                    var.params[prop.name.toString()] = d;
                }
            }
            out.push_back(std::move(var));
        }

        if (out.empty())
            err = ClaudeClientError::SchemaError;
        return out;
    }
}

class ClaudeClient::Worker : public juce::Thread
{
public:
    Worker(ClaudeClient& owner) : juce::Thread("ClaudeClientWorker"), owner(owner) {}

    void enqueue(juce::String prompt,
                 int numVariations,
                 uint64_t myGen,
                 std::function<void(ClaudeResponse)> callback)
    {
        {
            const juce::ScopedLock lock(stateLock);
            pendingPrompt = std::move(prompt);
            pendingN      = numVariations;
            pendingGen    = myGen;
            pendingCb     = std::move(callback);
            hasPending    = true;
        }
        if (! isThreadRunning())
            startThread();
        else
            notify();   // wake an already-running worker that's currently in wait()
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            juce::String prompt;
            int n = 1;
            uint64_t myGen = 0;
            std::function<void(ClaudeResponse)> cb;
            bool hadWork = false;

            {
                const juce::ScopedLock lock(stateLock);
                if (hasPending)
                {
                    prompt     = std::move(pendingPrompt);
                    n          = pendingN;
                    myGen      = pendingGen;
                    cb         = std::move(pendingCb);
                    hasPending = false;
                    hadWork    = true;
                }
            }

            if (hadWork)
            {
                ClaudeResponse resp = doRequest(prompt, n);
                deliver(myGen, std::move(cb), std::move(resp));
            }
            else
            {
                wait(-1);   // wait until notify() or threadShouldExit
            }
        }
    }

private:
    ClaudeResponse doRequest(const juce::String& prompt, int n)
    {
        ClaudeResponse r;

        const auto apiKey = owner.keyStore.load();
        if (apiKey.isEmpty())
        {
            r.error = ClaudeClientError::MissingApiKey;
            r.message = "no API key configured";
            return r;
        }

        const auto body = buildRequestBody(SystemPrompt::build(), prompt, owner.model, n);

        juce::StringPairArray headers;
        headers.set("x-api-key", apiKey);
        headers.set("anthropic-version", "2023-06-01");
        headers.set("content-type", "application/json");

        const auto httpResp = owner.transport.post(
            juce::URL("https://api.anthropic.com/v1/messages"),
            headers, body, owner.timeoutSeconds * 1000);

        if (httpResp.timedOut)        { r.error = ClaudeClientError::Timeout; r.message = "timed out"; return r; }
        if (httpResp.status == 0)     { r.error = ClaudeClientError::NetworkFailure; r.message = "network failure"; return r; }
        if (httpResp.status < 200 || httpResp.status >= 300)
        {
            r.error = ClaudeClientError::HttpError;
            r.httpStatus = httpResp.status;
            r.message = httpResp.body.substring(0, 200);
            return r;
        }

        const auto parsed = juce::JSON::parse(httpResp.body);
        if (parsed.isVoid() || ! parsed.isObject())
        {
            r.error = ClaudeClientError::ParseError;
            r.message = "could not parse response";
            return r;
        }

        ClaudeClientError schemaErr = ClaudeClientError::None;
        r.variations = parseVariations(parsed, schemaErr);
        if (schemaErr != ClaudeClientError::None)
        {
            r.error = schemaErr;
            r.message = "no set_patch tool_use blocks";
        }
        return r;
    }

    void deliver(uint64_t myGen,
                 std::function<void(ClaudeResponse)> cb,
                 ClaudeResponse resp)
    {
        juce::WeakReference<ClaudeClient> weakOwner(&owner);
        juce::MessageManager::callAsync(
            [weakOwner, myGen, cb = std::move(cb), resp = std::move(resp)]() mutable
            {
                auto* p = weakOwner.get();
                if (p == nullptr) return;
                if (p->shutdownFlag.load()) return;
                if (myGen != p->generation.load()) return;
                cb(std::move(resp));
            });
    }

    ClaudeClient&            owner;
    juce::CriticalSection    stateLock;
    juce::String             pendingPrompt;
    int                      pendingN { 1 };
    uint64_t                 pendingGen { 0 };
    std::function<void(ClaudeResponse)> pendingCb;
    bool                     hasPending { false };
};

ClaudeClient::ClaudeClient(ApiKeyStore& ks, Transport& tr)
    : keyStore(ks), transport(tr), worker(std::make_unique<Worker>(*this)) {}

ClaudeClient::~ClaudeClient()
{
    shutdownFlag.store(true);
    if (worker)
    {
        worker->signalThreadShouldExit();
        worker->notify();   // wake the worker if it's waiting
        worker->stopThread(2000);
    }
}

void ClaudeClient::requestPatches(juce::String prompt,
                                  int numVariations,
                                  std::function<void(ClaudeResponse)> cb)
{
    numVariations = juce::jlimit(1, 8, numVariations);
    const auto myGen = ++generation;
    worker->enqueue(std::move(prompt), numVariations, myGen, std::move(cb));
}

void ClaudeClient::setModel(juce::String m) { model = std::move(m); }
void ClaudeClient::setTimeoutSeconds(int s) { timeoutSeconds = juce::jmax(1, s); }
