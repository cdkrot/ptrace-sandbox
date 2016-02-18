#!/bin/sh

if [ ! -d src/gen ]; then
    mkdir ./src/gen
fi
if [ ! -d obj/gen ]; then
    mkdir -p ./obj/gen
fi

for full_file in $(find ./src/gen-scelet -type f | grep -v copy_myself.sh); do
    file=$(basename $full_file)
    if [ -f ./src/gen/$file ]; then
        cmp -s $full_file ./src/gen/$file 
        if [ $? -eq 1 ]; then
            cp $full_file ./src/gen/$file
        fi
    else
        cp $full_file ./src/gen/$file
    fi
done
