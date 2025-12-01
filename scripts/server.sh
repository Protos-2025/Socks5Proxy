#!/bin/bash
set -e
# Do not buffer STDIN/STDOUT/STDERR
LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libasan.so.8 exec stdbuf -o0 -e0 ./bin/server