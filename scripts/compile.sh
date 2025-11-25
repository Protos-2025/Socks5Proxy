#!/bin/bash
docker compose build compiler
docker compose run --rm compiler bash -c "cd /root && make clean all"