#!/usr/bin/env bash

# Usage: ./run.sh [OPTIONS]
#   OPTIONS are passed directly to build.sh
#   See ./build.sh --help for available flags

./build.sh "$@"
./build/urColo
