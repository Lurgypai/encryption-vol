cmake_minimum_required(VERSION 3.10)

project(test-vol-encrypt)

add_subdirectory(encryption_wrapper)

find_package(HDF5 REQUIRED)

add_executable(test-vol-encrypt-write test-write.c)
add_executable(test-vol-encrypt-read test-read.c)

target_link_libraries(test-vol-encrypt-write HDF5::HDF5 enc_wrapper)
target_link_libraries(test-vol-encrypt-read HDF5::HDF5 enc_wrapper)
