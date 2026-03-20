cmake_minimum_required(VERSION 3.10)

project(vol-encrypt)

find_package(HDF5)
find_package(enc_wrapper)
find_package(enc_io)

add_library(encrypt_vol_connector SHARED encrypt_vol_connector.c)

target_link_libraries(encrypt_vol_connector HDF5::HDF5 enc_wrapper::enc_io)
