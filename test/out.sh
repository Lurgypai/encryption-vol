#!/bin/bash

INS_DIR=$(realpath ../dependencies)

echo "Installation Directory: ${INS_DIR}"

export HDF5_DIR=${INS_DIR}/hdf5-ins
export GCRYPT_ROOT_DIR=${INS_DIR}/gcrypt-ins
export GPG_ERROR_ROOT_DIR=${INS_DIR}/gpgerror-ins

rm -r out
mkdir out
cd out

cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENC_WRAPPER_ENABLE_NETTLE=Off \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=On

mv compile_commands.json ..

