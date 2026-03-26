#include <stdio.h>
#include <hdf5.h>
#include <stdlib.h>

#include "../vol-encrypt/encrypt_vol_connector.h"
#include "enc_wrapper.h"

#define FILE_NAME "example.h5"
#define DATASET_COUNT 3
#define DATASET1_NAME "dataset1"
#define DIM0 16

int main() {
    hid_t file_id, dataset_id;
    herr_t status;
    hsize_t dims[1] = {DIM0};
    int data[DIM0];

    // key for accessing
    enc_config meta_config = {
        .alg = aes256,
        .lib = enc_lib_gcrypt
    };
    enc_load_config(meta_config);
    size_t key_size = enc_get_key_size();
    char* key = calloc(key_size, 1);
    struct encrypt_vol_key_property key_prop = {
        .key = key
    };
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_encrypt_vol_fapl(fapl, key, key_size);

    // Open the existing HDF5 file
    file_id = H5Fopen(FILE_NAME, H5F_ACC_RDONLY, fapl);
    if (file_id < 0) {
        printf("Failed to open file '%s'\n", FILE_NAME);
        return 1;
    }

    dataset_id = H5Dopen(file_id, DATASET1_NAME, H5P_DEFAULT);
    if (dataset_id < 0) {
        printf("Failed to open dataset '%s'\n", DATASET1_NAME);
    }

    status = H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
                     H5P_DEFAULT, data);

    printf("%s: ", DATASET1_NAME);
    for (int j = 0; j < dims[0]; j++) {
        printf("%d ", data[j]);
    }
    printf("\n");

    H5Dclose(dataset_id);
    H5Fclose(file_id);

    return 0;
}

