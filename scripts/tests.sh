#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

make clean test

# Iterate over each file in the tests directory
for test_file in ./bin/tests/*; do
    if [ -f "$test_file" ]; then
        echo "Running test: $test_file"
        chmod a+x "$test_file"
        "$test_file"
        if [ $? -ne 0 ]; then
            echo -e "${RED}Test $test_file failed.${NC}"
            exit 1
        else
            echo -e "${GREEN}Test $test_file passed.${NC}"
        fi
    fi
done
