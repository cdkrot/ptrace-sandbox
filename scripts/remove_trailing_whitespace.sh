#!/bin/sh

name=${1}
if [ x$name = x ]
then
    echo "No name specified"
    exit 1
fi

sed -i -r "s:\s+$::" $name
