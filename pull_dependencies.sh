#!/bin/bash

rm -rf dependencies
mkdir dependencies

pushd dependencies > /dev/null

INSTALL_DIR=$(pwd)

GPG_ERROR_CONFIG=$(which gpg-error-config)
if [[ -z ${GPG_ERROR_CONFIG} ]]; then
    echo "Downloading and unzipping gpg error"
    wget https://www.gnupg.org/ftp/gcrypt/libgpg-error/libgpg-error-1.56.tar.bz2
    tar -xvf "libgpg-error-1.56.tar.bz2"
    rm "libgpg-error-1.56.tar.bz2"
    echo "Compiling gpg error"
    pushd "libgpg-error-1.56" > /dev/null
    ./autogen.sh
    ./configure --prefix=${INSTALL_DIR}/gpgerror-ins --enable-install-gpg-error-config
    make -j`nproc` && make install
    popd > /dev/null
else
    echo "Found gpg-error-config at \"${GPG_ERROR_CONFIG}\", skipping install"
fi

GCRYPT_CONFIG=$(which libgcrypt-config)
if [[ -z ${GCRYPT_CONFIG} ]]; then
    echo "Downloading and unzipping gcrypt"
    wget https://www.gnupg.org/ftp/gcrypt/libgcrypt/libgcrypt-1.11.2.tar.bz2
    tar -xvf "libgcrypt-1.11.2.tar.bz2"
    rm "libgcrypt-1.11.2.tar.bz2"
    pushd "libgcrypt-1.11.2" > /dev/null
    ./configure --prefix=${INSTALL_DIR}/gcrypt-ins --with-libgpg-error-prefix=${INSTALL_DIR}/gpgerror-ins
    make -j`nproc` && make install
    popd > /dev/null
else
    echo "Found gcrypt-config at \"${GCRYPT_CONFIG}\", skipping install"
fi

echo "Downlaind and unzipping nettle"
wget https://ftp.gnu.org/gnu/nettle/nettle-3.10.2.tar.gz
tar -xvf "nettle-3.10.2.tar.gz"
rm "nettle-3.10.2.tar.gz"
pushd "nettle-3.10.2" > /dev/null
./configure --prefix=${INSTALL_DIR}/nettle-ins
make -j`nproc` && make install
popd > /dev/null

H5DUMP=$(which h5dump)
if [[ -z ${H5DUMP} ]]; then
    echo "Pulling and installing hdf5"
    git clone https://github.com/HDFGroup/hdf5.git
    pushd hdf5 > /dev/null
    mkdir out
    pushd out > /dev/null
    CC=${MPICC} CXX=${MPICXX} cmake .. \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}/hdf5-ins
    make -j`nproc` && make install
    popd > /dev/null
    popd > /dev/null
else
    echo "Found h5dump at \"${H5DUMP}\", skipping install"
fi

export GCRYPT_ROOT_DIR=${INSTALL_DIR}/gcrypt-ins
export GPG_ERROR_ROOT_DIR=${INSTALL_DIR}/gpgerror-ins

echo "Pulling and installing encryption wrapper"
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

            echo "WRAPPER_DIR: ${WRAPPER_DIR}"

            cmake .. \
                -Denc_wrapper_DIR=${WRAPPER_DIR}/cmake \
                -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
                -DCMAKE_INSTALL_PREFIX=${OUT_DIR} \
                -DCMAKE_BUILD_TYPE=Debug
                make -j`nproc` && make install
        popd
    popd
popd > /dev/null

popd > /dev/null
