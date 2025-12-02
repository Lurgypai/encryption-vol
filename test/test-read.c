#include <stdio.h>
#include <hdf5.h>
#include <stdlib.h>

#include "../vol-encrypt/encrypt_vol_connector.h"
#include "../dependencies/encryption_wrapper/enc_wrapper.h"
#include "../dependencies/encryption_wrapper/enc_algorithm.h"
#include "../dependencies/encryption_wrapper/gcrypt_impl/enc_gcrypt.h"

#define FILE_NAME "example.h5"
#define DATASET_COUNT 3
#define DATASET_NAMES {"dataset1", "dataset2", "dataset3"}
#define DIM0 8

int main() {
    hid_t file_id, dataset_id;
    herr_t status;
    hsize_t dims[1] = {DIM0};
    int data[DIM0];

    const char *dataset_names[DATASET_COUNT] = DATASET_NAMES;

    // Open the existing HDF5 file
    file_id = H5Fopen(FILE_NAME, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file_id < 0) {
        printf("Failed to open file '%s'\n", FILE_NAME);
        return 1;
    }

    enc_load_library(enc_get_gcrypt());
    enc_prepare(aes256);
    size_t key_size = enc_get_key_size();
    // blank key for testing
    char* key = calloc(1, key_size);

    for (int i = 0; i < DATASET_COUNT; i++) {
        hid_t dapl_id = H5Pcreate(H5P_DATASET_ACCESS);
        struct encrypt_vol_key_property enc_key_prop = {
            .key = key,
            .key_size = key_size
        };
        H5Pset(dapl_id, ENCRYPT_VOL_KEY_PROPERTY_NAME, &enc_key_prop);

        dataset_id = H5Dopen(file_id, dataset_names[i], dapl_id);
        if (dataset_id < 0) {
            printf("Failed to open dataset '%s'\n", dataset_names[i]);
            continue;
        }

        
        status = H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
                         H5P_DEFAULT, data);

        printf("%s: ", dataset_names[i]);
        for (int j = 0; j < dims[0]; j++) {
            printf("%d ", data[j]);
        }
        printf("\n");

        H5Dclose(dataset_id);
    }

    // Close file
    H5Fclose(file_id);

    return 0;
}

