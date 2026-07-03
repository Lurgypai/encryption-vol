#!/bin/bash

cd out
make clean

BEAR=$(which bear)
if [[ ! -z $BEAR ]]; then
    bear -- make
    mv compile_commands.json ..
else
    make
fi
