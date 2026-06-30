#!/bin/bash

pushd vol-encrypt
    ./out.sh
    pushd out
        make -j`nproc`
    popd
popd

pushd test
    ./out.sh
    ./compile.sh
popd
