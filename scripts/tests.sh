#!/bin/bash

make clean test

# Iterate over each file in the tests directory
for test_file in ./bin/tests/*; do
    if [ -f "$test_file" ]; then
        echo "Running test: $test_file"
        chmod a+x "$test_file"
        "$test_file"
    fi
done
