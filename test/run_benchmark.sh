#!/bin/bash

export HDF5_PLUGIN_PATH=$(realpath ../vol-encrypt/out)
export HDF5_VOL_CONNECTOR="encrypt_vol_connector"

if [[ ! -f ${1} ]]; then
    echo "Unable to find config file \"${1}\", exiting."
    exit 1
fi

# gdb --args out/benchmark ${1}
out/benchmark ${1}

rm output.h5
