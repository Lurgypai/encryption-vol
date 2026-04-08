#!/bin/bash

pushd dependencies > /dev/null

INSTALL_DIR=$(pwd)

export GCRYPT_ROOT_DIR=${INSTALL_DIR}/gcrypt-ins
export GPG_ERROR_ROOT_DIR=${INSTALL_DIR}/gpgerror-ins

echo "Pulling and installing encryption wrapper"
rm -rf enc_wrapper
git clone https://github.com/Lurgypai/enc_wrapper.git
pushd enc_wrapper > /dev/null
    # wrapper
    pushd wrapper
        mkdir out
        pushd out
            OUT_DIR=$(realpath ${INSTALL_DIR}/enc_wrapper-ins)

            rm -rf ${OUT_DIR}

            cmake .. \
                -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
                -DCMAKE_INSTALL_PREFIX=${OUT_DIR} \
                -DENC_WRAPPER_ENABLE_NETTLE=Off \
                -DCMAKE_BUILD_TYPE=Debug

            make -j`nproc` && make install
        popd
    popd 
    # io
    pushd io
        mkdir out
        pushd out
            WRAPPER_DIR=${INSTALL_DIR}/enc_wrapper-ins
            OUT_DIR=${INSTALL_DIR}/enc_io-ins
            rm -rf ${OUT_DIR}

            cmake .. \
                -DCMAKE_C_COMPILER=mpicc -DCMAKE_CXX_COMPILER=mpicxx \
                -DCMAKE_C_FLAGS="-DENABLE_MPI" \
                -Denc_wrapper_DIR=${WRAPPER_DIR}/cmake \
                -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
                -DCMAKE_INSTALL_PREFIX=${OUT_DIR} \
                -DCMAKE_BUILD_TYPE=Debug
                make -j`nproc` && make install
        popd
    popd
popd > /dev/null

popd
