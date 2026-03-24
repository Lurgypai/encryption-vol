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
#include "enc_gcrypt.h"
#include "enc_wrapper.h"
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

static struct encrypt_vol_property def_prop = {
    .alg = 0
};

static struct encrypt_vol_key_property def_key_prop = {
    .key = NULL
};

static herr_t init(hid_t vipl_id) {
    H5Pregister2(
        H5P_DATASET_CREATE,
        ENCRYPT_VOL_PROPERTY_NAME,
        sizeof(struct encrypt_vol_property),
        &def_prop,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    H5Pregister2(
        H5P_DATASET_ACCESS,
        ENCRYPT_VOL_KEY_PROPERTY_NAME,
        sizeof(struct encrypt_vol_key_property),
        &def_key_prop,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL);
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
} H5VLencrypt_file_t;

// create a file object that uses file as its posix handle
static H5VLencrypt_file_t* make_file() {
    H5VLencrypt_file_t* file_obj = malloc(sizeof(H5VLencrypt_file_t));
    file_obj->type = file;
    return file_obj;
}

// close file object
static void free_file(H5VLencrypt_file_t* file) {
    free(file);
}

// dataset object
typedef struct H5VLencrypt_dataset_t {
    H5VLencrypt_obj_type_t type;
    enc_object* obj;
} H5VLencrypt_dataset_t;

static H5VLencrypt_dataset_t* make_dataset(H5VLencrypt_file_t* file, const char* name, hid_t dcpl, hid_t dapl) {
    struct encrypt_vol_property encrypt_props;
    H5Pget(dcpl, ENCRYPT_VOL_PROPERTY_NAME, &encrypt_props);
    struct encrypt_vol_key_property encrypt_key_props;
    H5Pget(dapl, ENCRYPT_VOL_KEY_PROPERTY_NAME, &encrypt_key_props);

    enc_config cfg;
    // TODO add nettle support
    cfg.lib = enc_lib_gcrypt;
    if(encrypt_props.alg == 0) cfg.alg = aes256;
    else if (encrypt_props.alg == 1) cfg.alg = chacha20;

    H5VLencrypt_dataset_t* dset = malloc(sizeof(H5VLencrypt_dataset_t));
    dset->type = dataset;
    enc_store_add_object(&file->store, name, enc_object_layout_joined);
    dset->obj = ;
    return dset;
}

static void free_dataset(H5VLencrypt_dataset_t* dataset) {
    free(dataset);
}


static void *file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req) {
    H5VLencrypt_file_t* obj = make_file();

    // TODO add config to fcpl
    enc_config cfg;
    obj->store = enc_store_create(name, cfg);
    return obj;
}

static void *file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req) {
    H5VLencrypt_file_t* file_obj = make_file();
    // TODO add key to fapl
    char* key = NULL;
    file_obj->store = enc_store_open(name, key);
    return file_obj;
}

static herr_t file_close(void *file, hid_t dxpl_id, void **req) {
    H5VLencrypt_file_t* obj = (H5VLencrypt_file_t*)file;
    // TODO how do we get the key here
    char* key = NULL;
    enc_store_close(obj->store, key);
    free_file(obj);
    return 0;
}

static void *dataset_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req) {
    // printf("Creating dataset %s\n", name);
    // TODO handle any other acces method well at all
    H5VLencrypt_obj_type_t type = get_type(obj);
    if(type == file) {
        H5VLencrypt_dataset_t* dset = make_dataset((H5VLencrypt_file_t*)obj, dcpl_id, dapl_id);
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
        // find dataset metadata in file
        H5VLencrypt_file_t* file_obj = obj;
        file_dataset_spec_t* cur_dataset = file_obj->datasets;
        while(cur_dataset != NULL) {
            if(strcmp(cur_dataset->name, name) == 0) break;
            cur_dataset = cur_dataset->next;
        }
        // TODO some kind of error here
        if(cur_dataset == NULL) return NULL;

        // TODO INVALID SPACE
        struct encrypt_vol_key_property encrypt_key_props;
        H5Pget(dapl_id, ENCRYPT_VOL_KEY_PROPERTY_NAME, &encrypt_key_props);
        H5VLencrypt_dataset_t* dset = make_dataset(file_obj, cur_dataset->read_offset, name, 0,
                cur_dataset->alg,
                encrypt_key_props.key, encrypt_key_props.key_size);
        return dset;
    }
    else {
        // TODO handle group and crash for other cases
    }
    return NULL;
}

static herr_t dataset_read(size_t count, void *dset[],
        hid_t mem_type_id[], hid_t mem_space_id[], hid_t file_space_id[],
        hid_t plist_id, void *buf[], void **req) {
    for(size_t dset_idex = 0; dset_idex != count; ++dset_idex) {
        H5VLencrypt_dataset_t* dataset = (H5VLencrypt_dataset_t*)dset[dset_idex];
        // printf("Reading dataset %s\n", dataset->name);
        // find the dataset metadata
        H5VLencrypt_file_t* file_obj = dataset->file;
        file_dataset_spec_t* cur_dataset = file_obj->datasets;
        while(cur_dataset != NULL) {
            if(strcmp(cur_dataset->name, dataset->name) == 0) break;
            cur_dataset = cur_dataset->next;
        }
        // TODO some kind of error here
        if(cur_dataset == NULL) return -1;

        lseek(file_obj->file, cur_dataset->read_offset, SEEK_SET);
        size_t raw_size = cur_dataset->size;

        hid_t type_id  = mem_type_id[dset_idex];
        hid_t space_id = mem_space_id[dset_idex];
        if(space_id != 0) raw_size = H5Tget_size(type_id) * H5Sget_simple_extent_npoints(space_id);

        if(dataset->alg == -1) read(file_obj->file, buf[dset_idex], raw_size);
        else {
            enc_load_library(enc_get_gcrypt());
            enc_prepare(dataset->alg);
            enc_set_key(dataset->key, dataset->key_size);

            size_t nonce_size = enc_get_nonce_size();
            char* nonce = malloc(nonce_size);
            read(file_obj->file, nonce, nonce_size);
            enc_set_nonce(nonce, nonce_size);

            size_t out_size = raw_size - nonce_size;
            char* cipher_text = malloc(out_size);
            lseek(file_obj->file, cur_dataset->read_offset + nonce_size, SEEK_SET);
            read(file_obj->file, cipher_text, out_size);
            enc_decrypt(cipher_text, out_size, buf[dset_idex], out_size);

            free(nonce);
            free(cipher_text);
        }

    }
    return 0;
}

static herr_t dataset_write(size_t count, void *dset[],
        hid_t mem_type_id[], hid_t mem_space_id[], hid_t file_space_id[],
        hid_t plist_id, const void *buf[], void **req) {
    for(size_t dset_idex = 0; dset_idex != count; ++dset_idex) {
        H5VLencrypt_dataset_t* dataset = (H5VLencrypt_dataset_t*)dset[dset_idex];
        // printf("Writing dataset %s\n", dataset->name);
        hid_t type_id  = mem_type_id[dset_idex];
        hid_t space_id = file_space_id[dset_idex];
        if(space_id == 0) space_id = dataset->space_id;
        size_t raw_size = H5Tget_size(type_id) * H5Sget_select_npoints(space_id);
        // TODO seek to correct pos
        dataset->read_offset = dataset->file->write_pos;

        if(dataset->alg == -1) write(dataset->file->file, buf[dset_idex], raw_size);
        else {
            enc_load_library(enc_get_gcrypt());
            enc_prepare(dataset->alg);
            enc_set_key(dataset->key, dataset->key_size);

            char* nonce = enc_make_nonce();
            size_t nonce_size = enc_get_nonce_size();
            enc_set_nonce(nonce, nonce_size);

            char* out_buf = malloc(raw_size + nonce_size);
            memcpy(out_buf, nonce, nonce_size);
            enc_encrypt((void*)buf[dset_idex], raw_size, out_buf + nonce_size, raw_size);
            raw_size += nonce_size;
            write(dataset->file->file, out_buf, raw_size);
            free(out_buf);
            free(nonce);
        }
        dataset->file->write_pos += raw_size;

        file_dataset_spec_t* meta = make_dataset_spec(strdup(dataset->name), dataset->read_offset, raw_size, dataset->alg);
        
        // add to dataset metadata
        if(dataset->file->datasets == NULL) dataset->file->datasets = meta;
        else dataset->file->datasets_tail->next = meta;
        dataset->file->datasets_tail = meta;

        ++dataset->file->dataset_count;
    }
    return 0;
}

static herr_t dataset_close(void *dset, hid_t dxpl_id, void **req) {
    H5VLencrypt_dataset_t* dataset = dset;
    // printf("Closing dataset %s\n", dataset->name);
    free(dataset);
    return 0;
}

static herr_t opt_query(void *obj, H5VL_subclass_t subcls, int opt_type, uint64_t *flags) {
    return 0;
}

// questions to answer
//  how do we get the key to the close function?
//  how do we get region info to a dataset?
