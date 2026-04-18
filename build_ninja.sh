#!/usr/bin/env bash
# Faster incremental builds with Ninja (requires ninja + clang or gcc)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
