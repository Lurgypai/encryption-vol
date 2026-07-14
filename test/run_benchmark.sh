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

if [[ -z ${SLURM_PROCID} ]]; then
    echo "Using SLURM_PROCID as rank..."
    RANK=${SLURM_PROCID}
else
    echo "Using PMI_RANK as rank..."
    RANK=${PMI_RANK}
fi

output="termout/${RANK}"

date +%Y-%m-%d_%H-%M-%S >> $output
echo "---- (run_benchmark.sh) Running benchmark below ----" >> $output
out/benchmark ${1} ${2} >> $output 2>&1
echo "---- (run_benchmark.sh) completed benchmark ----" >> $output
