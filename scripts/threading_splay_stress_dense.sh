#!/bin/bash

for i in $(seq 1 5000 100000); do
    sleep 0.1
    echo "./bin/userspace/threading-splay-test $i &" | /bin/sh
done
