#!/bin/bash

for i in $(seq 1 20); do
    sleep 0.1
    echo "./bin/userspace/threading-splay-test &" | /bin/sh
done
