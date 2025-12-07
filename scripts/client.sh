#!/bin/bash
set -e

# Run server in background
LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libasan.so.8 \
    stdbuf -o0 -e0 ./bin/client
