#!/bin/sh
for i in $(seq 1 250); do sleep 0.1 && echo "./bin/module-userspace/smdt-malloc &" | sh; done
