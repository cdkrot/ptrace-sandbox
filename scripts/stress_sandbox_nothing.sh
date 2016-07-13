#!/bin/bash

for i in $(seq 1 20); do
    sleep 0.1
    echo "./scripts/run_sandbox_nothing.py &" | /bin/sh
done
