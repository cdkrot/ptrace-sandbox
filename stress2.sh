#!/bin/sh
for i in $(seq 1 300); do sleep 0.1 && echo "./bin/module-userspace/sbmaxmem /bin/ls &" | sh; done
