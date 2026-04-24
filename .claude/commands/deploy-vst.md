Run deploy.ps1 to copy the built VST3 plugin to the Windows VST3 folder with auto-incremented version name.

```bash
powershell -NoProfile -ExecutionPolicy Bypass -File "C:\Users\mitoufle\ClaudePRJ\AISynth\deploy.ps1" 2>&1; echo "Exit: $?"
```

Report the output, exit code, and list all `AI Synth v*.vst3` entries in `C:\Program Files\Common Files\VST3\`.
