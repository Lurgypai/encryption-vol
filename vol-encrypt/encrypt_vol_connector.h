/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef _encrypt_vol_connector_H
#define _encrypt_vol_connector_H

#include <string.h>

#include <hdf5.h>

#include "enc_grain.h"

/* The value must be between 256 and 65535 (inclusive) */
#define ENCRYPT_VOL_CONNECTOR_VALUE    ((H5VL_class_value_t)12202)
#define ENCRYPT_VOL_CONNECTOR_NAME     "encrypt_vol_connector"

#define ENCRYPT_VOL_GRAINS_PROPERTY_NAME      "encrypt_vol_grains_property"
#define ENCRYPT_VOL_KEY_PROPERTY_NAME      "encrypt_vol_key_property"
#define ENCRYPT_VOL_FILE_CONFIG_PROPERTY_NAME      "encrypt_vol_file_config_property"

struct encrypt_vol_file_config_property {
    enc_config cfg;
};

// used for creating the dataset (dcpl)
struct encrypt_vol_grains_property {
    enc_grain_meta* grains;
    size_t grain_cnt;
};

// used for transfering data (fapl)
struct encrypt_vol_key_property {
    char* key;
    size_t key_size;
};
herr_t H5Pset_encrypt_vol_fcpl(hid_t fcpl, enc_config cfg);
herr_t H5Pget_encrypt_vol_fcpl(hid_t fcpl, enc_config* cfg);

herr_t H5Pset_encrypt_vol_fapl(hid_t fapl, char* key, size_t key_size);
herr_t H5Pget_encrypt_vol_fapl(hid_t fapl, char** key, size_t* key_size);

herr_t H5Pset_encrypt_vol_dcpl(hid_t dcpl, enc_grain_meta* grains, size_t grain_cnt);
herr_t H5Pget_encrypt_vol_dcpl(hid_t dcpl, enc_grain_meta** grains, size_t* grain_cnt);


#endif /* _encrypt_vol_connector_H */

