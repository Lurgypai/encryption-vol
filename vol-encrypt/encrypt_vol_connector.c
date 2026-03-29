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

/* Purpose:     A simple virtual object layer (VOL) connector with almost no
 *              functionality that can serve as a encrypt for creating other
 *              connectors.
 */

/* This connector's header */
#include "encrypt_vol_connector.h"
#include "enc_store.h"

#include <hdf5.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

static herr_t init(hid_t vipl_id);

// file
static void *file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req);
static void *file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req);
static herr_t file_close(void *file, hid_t dxpl_id, void **req);

// dataset
static void *dataset_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req);
static void *dataset_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t dapl_id, hid_t dxpl_id, void **req);
static herr_t dataset_read(size_t count, void *dset[],
        hid_t mem_type_id[], hid_t mem_space_id[], hid_t file_space_id[],
        hid_t plist_id, void *buf[], void **req);
static herr_t dataset_write(size_t count, void *dset[],
        hid_t mem_type_id[], hid_t mem_space_id[], hid_t file_space_id[],
        hid_t plist_id, const void *buf[], void **req);
static herr_t dataset_close(void *dset, hid_t dxpl_id, void **req);

static herr_t opt_query(void *obj, H5VL_subclass_t subcls, int opt_type, uint64_t *flags);


/* The VOL class struct */
static const H5VL_class_t encrypt_class_g = {
    3,                                              /* VOL class struct version */
    ENCRYPT_VOL_CONNECTOR_VALUE,                   /* value                    */
    ENCRYPT_VOL_CONNECTOR_NAME,                    /* name                     */
    1,                                              /* version                  */
    0,                                              /* capability flags         */
    init,                                           /* initialize               */
    NULL,                                           /* terminate                */
    {   /* info_cls */
        (size_t)0,                                  /* size    */
        NULL,                                       /* copy    */
        NULL,                                       /* compare */
        NULL,                                       /* free    */
        NULL,                                       /* to_str  */
        NULL,                                       /* from_str */
    },
    {   /* wrap_cls */
        NULL,                                       /* get_object   */
        NULL,                                       /* get_wrap_ctx */
        NULL,                                       /* wrap_object  */
        NULL,                                       /* unwrap_object */
        NULL,                                       /* free_wrap_ctx */
    },
    {   /* attribute_cls */
        NULL,                                       /* create       */
        NULL,                                       /* open         */
        NULL,                                       /* read         */
        NULL,                                       /* write        */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        NULL                                        /* close        */
    },
    {   /* dataset_cls */
        dataset_create,                                       /* create       */
        dataset_open,                                       /* open         */
        dataset_read,                                       /* read         */
        dataset_write,                                       /* write        */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        dataset_close                                        /* close        */
    },
    {   /* datatype_cls */
        NULL,                                       /* commit       */
        NULL,                                       /* open         */
        NULL,                                       /* get_size     */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        NULL                                        /* close        */
    },
    {   /* file_cls */
        file_create,                                       /* create       */
        file_open,                                       /* open         */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        file_close                                        /* close        */
    },
    {   /* group_cls */
        NULL,                                       /* create       */
        NULL,                                       /* open         */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        NULL                                        /* close        */
    },
    {   /* link_cls */
        NULL,                                       /* create       */
        NULL,                                       /* copy         */
        NULL,                                       /* move         */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL                                        /* optional     */
    },
    {   /* object_cls */
        NULL,                                       /* open         */
        NULL,                                       /* copy         */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL                                        /* optional     */
    },
    {   /* introscpect_cls */
        NULL,                                       /* get_conn_cls  */
        NULL,                                       /* get_cap_flags */
        opt_query                                        /* opt_query     */
    },
    {   /* request_cls */
        NULL,                                       /* wait         */
        NULL,                                       /* notify       */
        NULL,                                       /* cancel       */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        NULL                                        /* free         */
    },
    {   /* blob_cls */
        NULL,                                       /* put          */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL                                        /* optional     */
    },
    {   /* token_cls */
        NULL,                                       /* cmp          */
        NULL,                                       /* to_str       */
        NULL                                        /* from_str     */
    },
    NULL                                            /* optional     */
};

static struct encrypt_vol_file_config_property def_file_prop = {
};

static struct encrypt_vol_grains_property def_prop = {
    .grains = NULL,
    .grain_cnt = 0
};

static struct encrypt_vol_key_property def_key_prop = {
    .key = NULL,
    .key_size = 0
};

static herr_t init(hid_t vipl_id) {
    return 0;
}

herr_t H5Pset_encrypt_vol_fcpl(hid_t fcpl, enc_config cfg) {
    struct encrypt_vol_file_config_property config_prop = {
        .cfg = cfg
    };
    if(H5Pexist(fcpl, ENCRYPT_VOL_FILE_CONFIG_PROPERTY_NAME) <= 0) {
        return H5Pinsert2(fcpl, ENCRYPT_VOL_FILE_CONFIG_PROPERTY_NAME, sizeof(struct encrypt_vol_file_config_property), &config_prop,
                NULL, NULL, NULL, NULL, NULL, NULL);
    }
    else return H5Pset(fcpl, ENCRYPT_VOL_FILE_CONFIG_PROPERTY_NAME, &config_prop);
}
herr_t H5Pget_encrypt_vol_fcpl(hid_t fcpl, enc_config* cfg) {
    if(H5Pexist(fcpl, ENCRYPT_VOL_FILE_CONFIG_PROPERTY_NAME) <= 0) {
        *cfg = def_file_prop.cfg;
        return 0;
    }
    struct encrypt_vol_file_config_property config_prop;
    H5Pget(fcpl, ENCRYPT_VOL_FILE_CONFIG_PROPERTY_NAME, &config_prop);
    *cfg = config_prop.cfg;
    return 0;
}

herr_t H5Pset_encrypt_vol_fapl(hid_t fapl, char* key, size_t key_size) {
    struct encrypt_vol_key_property key_prop = {
        .key = key,
        .key_size = key_size
    };
    if(H5Pexist(fapl, ENCRYPT_VOL_KEY_PROPERTY_NAME) <= 0) {
        return H5Pinsert2(fapl, ENCRYPT_VOL_KEY_PROPERTY_NAME, sizeof(struct encrypt_vol_key_property), &key_prop,
                NULL, NULL, NULL, NULL, NULL, NULL);
    }
    else return H5Pset(fapl, ENCRYPT_VOL_KEY_PROPERTY_NAME, &key_prop);
}
herr_t H5Pget_encrypt_vol_fapl(hid_t fapl, char** key, size_t* key_size) {
    if(H5Pexist(fapl, ENCRYPT_VOL_KEY_PROPERTY_NAME) <= 0) {
        *key = NULL;
        *key_size = 0;
        return 0;
    }
    struct encrypt_vol_key_property key_prop;
    H5Pget(fapl, ENCRYPT_VOL_KEY_PROPERTY_NAME, &key_prop);
    *key = key_prop.key;
    *key_size = key_prop.key_size;
    return 0;
}

herr_t H5Pset_encrypt_vol_dcpl(hid_t dcpl, enc_grain_meta* grains, size_t grain_cnt) {
    struct encrypt_vol_grains_property grains_prop = {
        .grains = grains,
        .grain_cnt = grain_cnt
    };
    if(H5Pexist(dcpl, ENCRYPT_VOL_GRAINS_PROPERTY_NAME) <= 0) {
        return H5Pinsert2(dcpl, ENCRYPT_VOL_GRAINS_PROPERTY_NAME, sizeof(struct encrypt_vol_grains_property), &grains_prop,
                NULL, NULL, NULL, NULL, NULL, NULL);
    }
    else return H5Pset(dcpl, ENCRYPT_VOL_GRAINS_PROPERTY_NAME, &grains_prop);
}
herr_t H5Pget_encrypt_vol_dcpl(hid_t dcpl, enc_grain_meta** grains, size_t* grain_cnt) {
    if(H5Pexist(dcpl, ENCRYPT_VOL_GRAINS_PROPERTY_NAME) <= 0) {
        *grains = NULL;
        *grain_cnt = 0;
        return 0;
    }
    struct encrypt_vol_grains_property grains_prop;
    H5Pget(dcpl, ENCRYPT_VOL_GRAINS_PROPERTY_NAME, &grains_prop);
    *grains = grains_prop.grains;
    *grain_cnt = grains_prop.grain_cnt;
    return 0;
}

/* These two functions are necessary to load this plugin using
 * the HDF5 library.
 */

H5PL_type_t H5PLget_plugin_type(void) {return H5PL_TYPE_VOL;}
const void *H5PLget_plugin_info(void) {return &encrypt_class_g;}

// the types of the objects passed through callbacks
// at the front of each object
typedef enum H5VLencrypt_obj_type_t {
    file,
    group,
    dataset
} H5VLencrypt_obj_type_t;

// generic object type for just reading the type at the front
typedef struct H5VLencrypt_obj_t {
    H5VLencrypt_obj_type_t type;
} H5VLencrypt_obj_t;

// helper function to get an objects type
static H5VLencrypt_obj_type_t get_type(void* obj) {
    return ((H5VLencrypt_obj_t*)obj)->type;
}

// file object
typedef struct H5VLencrypt_file_t {
    H5VLencrypt_obj_type_t type;
    enc_store store;
    // move to safe memory? fix so we don't flush file metadata on close?
    char* key;
} H5VLencrypt_file_t;

// create a file object that uses file as its posix handle
static H5VLencrypt_file_t* make_file(const char* name, hid_t fcpl, hid_t fapl) {
    enc_config cfg;
    H5Pget_encrypt_vol_fcpl(fcpl, &cfg);

    char* key;
    size_t key_size;
    H5Pget_encrypt_vol_fapl(fapl, &key, &key_size);

    H5VLencrypt_file_t* file_obj = malloc(sizeof(H5VLencrypt_file_t));
    file_obj->type = file;
    file_obj->store = enc_store_create(name, cfg);
    file_obj->key = malloc(key_size);
    memcpy(file_obj->key, key, key_size);
    return file_obj;
}

// close file object
static void free_file(H5VLencrypt_file_t* file) {
    enc_store_close(file->store, file->key);
    free(file->key);
    free(file);
}

// dataset object
typedef struct H5VLencrypt_dataset_t {
    H5VLencrypt_obj_type_t type;
    char* name;
    H5VLencrypt_file_t* file;
} H5VLencrypt_dataset_t;

static H5VLencrypt_dataset_t* make_dataset(H5VLencrypt_file_t* file, const char* name, hid_t dcpl) {
    H5VLencrypt_dataset_t* dset = malloc(sizeof(H5VLencrypt_dataset_t));
    dset->type = dataset;
    enc_store_add_object(&file->store, name, enc_object_layout_joined);
    dset->name = strdup(name);
    dset->file = file;

    enc_grain_meta* grains;
    size_t grain_cnt;
    H5Pget_encrypt_vol_dcpl(dcpl, &grains, &grain_cnt);

    // add regions from dcpl
    enc_object* obj = enc_store_get_object(file->store, name);
    for(int grain_idx = 0; grain_idx != grain_cnt; ++ grain_idx) {
        enc_object_add_grain(obj, grains[grain_idx]);
    }

    return dset;
}

static void free_dataset(H5VLencrypt_dataset_t* dataset) {
    free(dataset->name);
    free(dataset);
}


static void *file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req) {
    // TODO add config to fcpl
    H5VLencrypt_file_t* obj = make_file(name, fcpl_id, fapl_id);
    return obj;
}

static void *file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req) {
    H5VLencrypt_file_t* file_obj = malloc(sizeof(H5VLencrypt_file_t));

    char* key;
    size_t key_size;
    H5Pget_encrypt_vol_fapl(fapl_id, &key, &key_size);

    file_obj->type = file;
    file_obj->store = enc_store_open(name, key);
    file_obj->key = malloc(key_size);
    memcpy(file_obj->key, key, key_size);
    return file_obj;
}

static herr_t file_close(void *file, hid_t dxpl_id, void **req) {
    H5VLencrypt_file_t* obj = (H5VLencrypt_file_t*)file;
    // TODO how do we get the key here
    free_file(obj);
    return 0;
}

static void *dataset_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req) {
    // printf("Creating dataset %s\n", name);
    // TODO handle any other acces method well at all
    H5VLencrypt_obj_type_t type = get_type(obj);
    if(type == file) {
        H5VLencrypt_dataset_t* dset = make_dataset((H5VLencrypt_file_t*)obj, name, dcpl_id);
        return dset;
    }
    else {
        // TODO handle group and crash for other cases
    }
    return NULL;
}


static void *dataset_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t dapl_id, hid_t dxpl_id, void **req) {
    // printf("Opening dataset %s\n", name);
    // TODO handle any other acces method well at all
    H5VLencrypt_obj_type_t type = get_type(obj);
    if(type == file) {
        H5VLencrypt_file_t* file_obj = obj;

        // read the object region data
        H5VLencrypt_dataset_t* dset = malloc(sizeof(H5VLencrypt_dataset_t));
        dset->type = dataset;
        dset->name = strdup(name);
        dset->file = file_obj;

        enc_store_grains_read(file_obj->store, name, file_obj->key);
        return dset;
    }
    else {
        // TODO handle group and crash for other cases
    }
    return NULL;
}

static void get_offset_size(const enc_object* obj, hid_t type_id, hid_t mem_sid, hid_t file_sid, size_t* offset_out, size_t* size_out) {
    if(mem_sid == H5S_ALL && file_sid == H5S_ALL) {
        *offset_out = 0;
        *size_out = 0;
        for(int i = 0; i != obj->grain_cnt; ++i) {
            *size_out += obj->grains[i].size;
        }
        return;
    }
    // calculate size of selection
    // element size
    size_t dtype_size = H5Tget_size(type_id);
    // elements in selection
    size_t nelmts = 0;
    if(mem_sid == H5S_ALL) nelmts = H5Sget_simple_extent_npoints(file_sid);
    else nelmts = H5Sget_select_npoints(mem_sid);
    // size
    *size_out = nelmts * dtype_size;

    // calculate offset
    *offset_out = 0;
    H5S_sel_type sel_type = H5Sget_select_type(file_sid);
    if(sel_type != H5S_SEL_ALL && sel_type != H5S_SEL_NONE) {
        // get the number of dims and bounding box
        int ndims = H5Sget_simple_extent_ndims(file_sid);
        hsize_t bb_start[ndims], bb_end[ndims];
        H5Sget_select_bounds(file_sid, bb_start, bb_end);

        // get the dimensions of the file in unit size
        hsize_t file_dims[ndims];
        H5Sget_simple_extent_dims(file_sid, file_dims, NULL);

        hsize_t flat_offset = 0, stride = 1;
        for (int i = ndims - 1; i >= 0; i--) {
            flat_offset += bb_start[i] * stride;
            stride *= file_dims[i];
        }
        *offset_out = flat_offset * dtype_size;
    }
}

static herr_t dataset_read(size_t count, void *dset[],
        hid_t mem_type_id[], hid_t mem_space_id[], hid_t file_space_id[],
        hid_t plist_id, void *buf[], void **req) {
    for(size_t dset_idex = 0; dset_idex != count; ++dset_idex) {
        hid_t type_id  = mem_type_id[dset_idex];
        hid_t mem_sid = mem_space_id[dset_idex];
        hid_t file_sid = file_space_id[dset_idex];
        size_t size = 0, offset = 0; 

        H5VLencrypt_dataset_t* dataset = (H5VLencrypt_dataset_t*)dset[dset_idex];
        enc_object* obj = enc_store_get_object(dataset->file->store, dataset->name);
        get_offset_size(obj, type_id, mem_sid, file_sid, &offset, &size);
        H5VLencrypt_file_t* file_obj = dataset->file;
        enc_store_read(file_obj->store, dataset->name, offset, size, buf[dset_idex], file_obj->key);
    }
    return 0;
}

static herr_t dataset_write(size_t count, void *dset[],
        hid_t mem_type_id[], hid_t mem_space_id[], hid_t file_space_id[],
        hid_t plist_id, const void *buf[], void **req) {
    for(size_t dset_idex = 0; dset_idex != count; ++dset_idex) {
        hid_t type_id  = mem_type_id[dset_idex];
        hid_t mem_sid = mem_space_id[dset_idex];
        hid_t file_sid = file_space_id[dset_idex];
        size_t size = 0, offset = 0; 

        H5VLencrypt_dataset_t* dataset = (H5VLencrypt_dataset_t*)dset[dset_idex];
        enc_object* obj = enc_store_get_object(dataset->file->store, dataset->name);
        get_offset_size(obj, type_id, mem_sid, file_sid, &offset, &size);
        H5VLencrypt_file_t* file_obj = dataset->file;
        enc_store_write(file_obj->store, dataset->name, offset, size, buf[dset_idex], file_obj->key);

        enc_store_grains_write(file_obj->store, dataset->name, file_obj->key);
    }
    return 0;
}

static herr_t dataset_close(void *dset, hid_t dxpl_id, void **req) {
    H5VLencrypt_dataset_t* dataset = dset;
    // printf("Closing dataset %s\n", dataset->name);
    free_dataset(dataset);
    return 0;
}

static herr_t opt_query(void *obj, H5VL_subclass_t subcls, int opt_type, uint64_t *flags) {
    return 0;
}

// questions to answer
//  how do we get the key to the close function?
//  how do we get region info to a dataset?
//      dcpl should have a list of regions to add to the dataset
//      dxpl has key
//
// notes
//  passing regions through dcpl, ignores dataspaces
//  store key because we can't pass anything to the file_close
