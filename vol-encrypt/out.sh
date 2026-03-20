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
    -Denc_wrapper_DIR="${INS_DIR}/enc_wrapper-ins/cmake" \
    -Denc_io_DIR="${INS_DIR}/enc_io-ins/cmake" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
    -DCMAKE_BUILD_TYPE=Debug

mv compile_commands.json ..
