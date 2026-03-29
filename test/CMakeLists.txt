cmake_minimum_required(VERSION 3.10)

project(test-vol-encrypt)

find_package(HDF5 REQUIRED)
find_package(enc_wrapper REQUIRED)
find_package(enc_io REQUIRED)

set(ENCRYPT_VOL_CONNECTOR_SRC "../vol-encrypt/encrypt_vol_connector.c")

add_executable(test-vol-encrypt-write test-write.c ${ENCRYPT_VOL_CONNECTOR_SRC})
add_executable(test-vol-encrypt-read test-read.c ${ENCRYPT_VOL_CONNECTOR_SRC})

add_executable(benchmark benchmark.cpp ${ENCRYPT_VOL_CONNECTOR_SRC})

target_link_libraries(test-vol-encrypt-write HDF5::HDF5 enc_wrapper::enc_io)
target_link_libraries(test-vol-encrypt-read HDF5::HDF5 enc_wrapper::enc_io)

target_link_libraries(benchmark HDF5::HDF5 enc_wrapper::enc_io)
