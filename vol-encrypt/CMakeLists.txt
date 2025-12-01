cmake_minimum_required(VERSION 3.10)

project(vol-encrypt)

add_library(vol-encrypt SHARED encrypt_vol_connector.c)

find_package(HDF5)
target_link_libraries(vol-encrypt PUBLIC HDF5::HDF5)
