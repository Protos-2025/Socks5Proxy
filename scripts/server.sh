#!/bin/bash
set -e

# Run server in background
LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libasan.so.8 \
    stdbuf -o0 -e0 ./bin/server &

sleep 1

# Run client in foreground
LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libasan.so.8 \
    exec stdbuf -o0 -e0 ./bin/client
