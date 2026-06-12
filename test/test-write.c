#include <stdio.h>
#include <hdf5.h>
#include <stdlib.h>
#include <mpi.h>

#include "../vol-encrypt/encrypt_vol_connector.h"
#include "enc_wrapper.h"

#define FILE_NAME "example.h5"
#define DATASET1_NAME "dataset1"
#define DIM0 32

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    hid_t file_id, dataset_id, dataspace_id;
    hsize_t dims[1] = {DIM0};
    herr_t status;

    // Sample data for datasets
    int data1[DIM0] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

    enc_config meta_config = {
        .alg = aes256,
        .lib = enc_lib_gcrypt
    };
    enc_load_config(meta_config);
    size_t key_size = enc_get_key_size();
    char* key = calloc(key_size, 1);

    // file meta config
    hid_t fcpl = H5Pcreate(H5P_FILE_CREATE);
    H5Pset_encrypt_vol_fcpl(fcpl, meta_config);

    // object region metadata
    enc_grain_meta grains[2] = {};
    grains[0].cfg.alg = aes256;
    grains[0].cfg.lib = enc_lib_gcrypt;
    grains[0].size = (DIM0/2) * sizeof(int);
    grains[1].cfg.alg = chacha20;
    grains[1].cfg.lib = enc_lib_gcrypt;
    grains[1].size = (DIM0/2) * sizeof(int);
    hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_encrypt_vol_dcpl(dcpl, grains, 2);

    // key for accessing
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_encrypt_vol_fapl(fapl, key, key_size);

    // Create a new file using default properties
    file_id = H5Fcreate(FILE_NAME, H5F_ACC_TRUNC, fcpl, fapl);

    // Create dataspace
    dataspace_id = H5Screate_simple(1, dims, NULL);

    // create and write dataset
    dataset_id = H5Dcreate(file_id, DATASET1_NAME, H5T_NATIVE_INT, dataspace_id,
                           H5P_DEFAULT, dcpl, H5P_DEFAULT);
    status = H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data1);

    // close all
    H5Dclose(dataset_id);
    H5Sclose(dataspace_id);
    H5Fclose(file_id);

    printf("File %s with 1 dataset containing 2 regions created successfully.\n", FILE_NAME);
    MPI_Finalize();
    return 0;
}

