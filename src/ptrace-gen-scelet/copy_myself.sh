#!/bin/sh

# Script to simplify building of generated code.
# Copyright (C) 2016 Vasiliy Alferov
# Licensed under GNU GPL v3 or (at your option) later.

if [ ! -d src/ptrace-gen ]; then
    mkdir ./src/ptrace-gen
fi
if [ ! -d obj/ptrace-gen ]; then
    mkdir -p ./obj/ptrace-gen
fi

for full_file in $(find ./src/ptrace-gen-scelet -type f | grep -v copy_myself.sh); do
    file=$(basename $full_file)
    if [ -f ./src/ptrace-gen/$file ]; then
        cmp -s $full_file ./src/ptrace-gen/$file 
        if [ $? -eq 1 ]; then
            cp $full_file ./src/ptrace-gen/$file
        fi
    else
        cp $full_file ./src/ptrace-gen/$file
    fi
done
