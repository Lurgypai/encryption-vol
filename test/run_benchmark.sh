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

if [[ -z ${SUB_NODES} ]]; then
    SUB_NODES=$SLURM_JOB_NUM_NODES
fi

if [[ -z ${SUB_TASKS_PER_NODE} ]]; then
    SUB_TASKS_PER_NODE=$SLURM_NTASKS_PER_NODE
fi

EXEC=$(which srun)
if [[ -z $EXEC ]]; then
    EXEC="mpirun -n 1"
else
    EXEC="srun -N ${SUB_NODES} --ntasks-per-node=${SUB_TASKS_PER_NODE}"
fi

# gdb --args out/benchmark ${1} ${2}
# valgrind --leak-check=full out/benchmark ${1} ${2}
echo "Using ${EXEC} as runner"
${EXEC} out/benchmark ${1} ${2}
