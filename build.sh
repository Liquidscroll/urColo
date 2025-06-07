#!/usr/bin/env bash

threads="$1"
if [ -z "$threads" ]; then
    if command -v nproc >/dev/null 2>&1; then
        threads="$(nproc)"
    elif command -v sysctl >/dev/null 2>&1; then
        threads="$(sysctl -n hw.ncpu)"
    else
        threads=1
    fi
fi

mkdir -p build && cmake -DCMAKE_BUILD_TYPE=Debug . -B build/ && \
    cmake --build build -- -j"${threads}"
