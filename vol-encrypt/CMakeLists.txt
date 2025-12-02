cmake_minimum_required(VERSION 3.10)

project(vol-encrypt)

add_library(encrypt_vol_connector SHARED encrypt_vol_connector.c)

set(ENC_WRAPPER_INCLUDE "/home/isaiah/Documents/Research/OSU/SecurityProject/code/encryption-vol/dependencies/encryption_wrapper")
set(ENC_WRAPPER_LIB "/home/isaiah/Documents/Research/OSU/SecurityProject/code/encryption-vol/dependencies/encryption_wrapper/out/libenc_wrapper.a")

find_package(HDF5)

target_include_directories(encrypt_vol_connector PRIVATE ${ENC_WRAPPER_INCLUDE})
target_link_libraries(encrypt_vol_connector HDF5::HDF5 ${ENC_WRAPPER_LIB})
