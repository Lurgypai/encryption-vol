export HDF5_PLUGIN_PATH=$(realpath ../vol-encrypt/out)
export HDF5_VOL_CONNECTOR="encrypt_vol_connector"

rm example.h5

out/test-vol-encrypt-write
gdb --args out/test-vol-encrypt-read
