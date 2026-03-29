#!/bin/bash

export HDF5_PLUGIN_PATH=$(realpath ../vol-encrypt/out)
export HDF5_VOL_CONNECTOR="encrypt_vol_connector"

if [[ ! -f ${1} ]]; then
    echo "Unable to find config file \"${1}\", exiting."
    exit 1
fi

if [[ ! ${2} == "write" && ! ${2} == "read" ]]; then
    echo "Invalid direction"
    echo "Usage: run_benchmarks.sh <config> <write/read>"
    exit 1
fi

EXEC=$(which srun)
if [[ -z $EXEC ]]; then
    EXEC="mpirun -n 1"
fi


# gdb --args out/benchmark ${1} ${2}
# valgrind --leak-check=full out/benchmark ${1} ${2}
${EXEC} out/benchmark ${1} ${2}

rm -r output.h5
