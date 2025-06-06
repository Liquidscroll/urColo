#!/usr/bin/env bash
mkdir -p build && cmake -DCMAKE_BUILD_TYPE=Debug . -B build/ && cmake --build build
