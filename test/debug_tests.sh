export HDF5_PLUGIN_PATH=$(realpath ../vol-encrypt/out)
export HDF5_VOL_CONNECTOR="encrypt_vol_connector"

rm example.h5

valgrind out/test-vol-encrypt-write
valgrind out/test-vol-encrypt-read
