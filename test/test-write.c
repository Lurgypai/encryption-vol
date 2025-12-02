#include <stdio.h>
#include <hdf5.h>
#include <stdlib.h>

#include "../vol-encrypt/encrypt_vol_connector.h"
#include "../dependencies/encryption_wrapper/enc_wrapper.h"
#include "../dependencies/encryption_wrapper/enc_algorithm.h"
#include "../dependencies/encryption_wrapper/gcrypt_impl/enc_gcrypt.h"

#define FILE_NAME "example.h5"
#define DATASET1_NAME "dataset1"
#define DATASET2_NAME "dataset2"
#define DATASET3_NAME "dataset3"
#define DIM0 8

int main() {
    hid_t file_id, dataset_id, dataspace_id;
    hsize_t dims[1] = {DIM0};
    herr_t status;

    // Sample data for datasets
    int data1[DIM0] = {0, 1, 2, 3, 4, 5, 6, 7};
    int data2[DIM0] = {8, 9, 10, 11, 12, 13, 14, 15};
    int data3[DIM0] = {16, 17, 18, 19, 20, 21, 22, 23};

    enc_load_library(enc_get_gcrypt());
    enc_prepare(aes256);
    size_t key_size = enc_get_key_size();
    // blank key for testing
    char* key = calloc(key_size, 1);

    // Create a new file using default properties
    file_id = H5Fcreate(FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    // Create dataspace
    dataspace_id = H5Screate_simple(1, dims, NULL);

    // NO ENCRYPTION =================
    hid_t dcpl_id, dapl_id;
    struct encrypt_vol_property enc_prop = {
        .alg = -1
    };
    struct encrypt_vol_key_property enc_key_prop = {
        .key = key,
        .key_size = key_size
    };
    dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset(dcpl_id, ENCRYPT_VOL_PROPERTY_NAME, &enc_prop);
    dapl_id = H5Pcreate(H5P_DATASET_ACCESS);
    H5Pset(dapl_id, ENCRYPT_VOL_KEY_PROPERTY_NAME, &enc_key_prop);
    dataset_id = H5Dcreate(file_id, DATASET1_NAME, H5T_NATIVE_INT, dataspace_id,
                           H5P_DEFAULT, dcpl_id, dapl_id);
    status = H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
                      H5P_DEFAULT, data1);
    H5Dclose(dataset_id);
    
    // AES256 =================
    enc_prop.alg = aes256;
    dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset(dcpl_id, ENCRYPT_VOL_PROPERTY_NAME, &enc_prop);
    dataset_id = H5Dcreate(file_id, DATASET2_NAME, H5T_NATIVE_INT, dataspace_id,
                           H5P_DEFAULT, dcpl_id, dapl_id);
    status = H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
                      H5P_DEFAULT, data2);
    H5Dclose(dataset_id);

    // chacha20 =================
    enc_prop.alg = chacha20;
    dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset(dcpl_id, ENCRYPT_VOL_PROPERTY_NAME, &enc_prop);
    dataset_id = H5Dcreate(file_id, DATASET3_NAME, H5T_NATIVE_INT, dataspace_id,
                           H5P_DEFAULT, dcpl_id, dapl_id);
    status = H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
                      H5P_DEFAULT, data3);
    H5Dclose(dataset_id);

    // Close dataspace and file
    H5Sclose(dataspace_id);
    H5Fclose(file_id);

    printf("HDF5 file '%s' with 3 datasets created successfully.\n", FILE_NAME);

    return 0;
}

