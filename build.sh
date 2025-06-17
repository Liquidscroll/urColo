#!/usr/bin/env bash

# Usage: ./build.sh [-t THREADS]
#   -t, --threads  Number of build threads (default: auto-detect)

threads=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        -t|--threads)
            threads="$2"
            shift 2
            ;;
        --help|-h)
            echo "Usage: $0 [-t THREADS]"; exit 0;;
        *)
            # Backwards compatibility: single numeric arg = threads
            if [[ -z "$threads" && "$1" =~ ^[0-9]+$ ]]; then
                threads="$1"
                shift
            else
                echo "Unknown option: $1" >&2
                echo "Usage: $0 [-t THREADS]" >&2
                exit 1
            fi
            ;;
    esac
done

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
    cmake --build build --parallel "$threads"
