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

#include <hdf5.h>
#include <H5PLextern.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

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

/* The VOL class struct */
static const H5VL_class_t encrypt_class_g = {
    2,                                              /* VOL class struct version */
    ENCRYPT_VOL_CONNECTOR_VALUE,                   /* value                    */
    ENCRYPT_VOL_CONNECTOR_NAME,                    /* name                     */
    1,                                              /* version                  */
    0,                                              /* capability flags         */
    NULL,                                           /* initialize               */
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
        NULL,                                       /* create       */
        NULL,                                       /* open         */
        NULL,                                       /* read         */
        NULL,                                       /* write        */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        NULL                                        /* close        */
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
        NULL,                                       /* create       */
        NULL,                                       /* open         */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        NULL                                        /* close        */
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
        NULL                                        /* opt_query     */
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

// dataset metadata stored in file
typedef struct file_dataset_spec_t {
    char * name;
    // where in the file to read from
    size_t read_offset;
    struct file_dataset_spec_t* next;
} file_dataset_spec_t;

// allocate new dataset spec, takes ownership of name
file_dataset_spec_t* make_dataset_spec(char* name, size_t read_offset) {
    file_dataset_spec_t* spec = malloc(sizeof(file_dataset_spec_t));
    spec->name = name;
    spec->read_offset = read_offset;
    return spec;
}

void free_dataset_spec(file_dataset_spec_t* spec) {
    size_t str_len = strlen(spec->name) + 1;
    free(spec->name);
    free(spec);
}

// file object
typedef struct H5VLencrypt_file_t {
    H5VLencrypt_obj_type_t type;
    int file;
    size_t write_pos;
    size_t dataset_count;
    file_dataset_spec_t* datasets;
    file_dataset_spec_t* datasets_tail;
} H5VLencrypt_file_t;

// create a file object that uses file as its posix handle
static H5VLencrypt_file_t* make_file(int file_) {
    H5VLencrypt_file_t* file_obj = malloc(sizeof(H5VLencrypt_file_t));
    file_obj->file = file_;
    file_obj->write_pos = 0;
    file_obj->dataset_count = 0;
    file_obj->datasets = NULL;
    file_obj->datasets_tail = NULL;
    return file_obj;
}

// close file object
static void free_file(H5VLencrypt_file_t* file) {
    file_dataset_spec_t* cur_dataset = file->datasets;
    while(cur_dataset != NULL) {
        file_dataset_spec_t* next = cur_dataset->next;
        free_dataset_spec(cur_dataset);
        cur_dataset = next;
    }
    free(file);
}

// dataset object
typedef struct H5VLencrypt_dataset_t {
    H5VLencrypt_obj_type_t type;
    H5VLencrypt_file_t* file;
    // where in the file this dataset is read from
    size_t read_offset;
    // TODO this probably will need to involve a malloc and a copy but for now pretend its fine
    const char* name;
} H5VLencrypt_dataset_t;


static void *file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req) {
    printf("DEBUG: Creating file named %s\n", name);
    // TODO handle flags correctly
    int file = open(name, O_RDWR | O_CREAT, 0644);
    H5VLencrypt_file_t* obj = make_file(file);
    return obj;
}

static void *file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req) {
    printf("DEBUG: Opening file named %s\n", name);
    // TODO handle flags correctly
    int file = open(name, O_RDWR, 0644);
    H5VLencrypt_file_t* file_obj = make_file(file);
    lseek(file, -1 * sizeof(size_t), SEEK_END);
    size_t dataset_count;
    read(file, &dataset_count, sizeof(size_t));

    off_t cur_pos = sizeof(size_t);
    for(int dataset_num = 0; dataset_num != dataset_count; ++dataset_num) {
        // read name
        // get size
        size_t name_size;
        lseek(file, -cur_pos - sizeof(size_t), SEEK_END);
        read(file, &name_size, sizeof(size_t));
        // read name
        char* name = malloc(name_size + 1);
        name[name_size] = '\0';
        lseek(file, -cur_pos - sizeof(size_t) - name_size, SEEK_END);
        read(file, name, name_size);
        // read offset
        size_t read_offset;
        lseek(file, -cur_pos - sizeof(size_t) - name_size - sizeof(size_t), SEEK_END);
        read(file, &read_offset, sizeof(size_t));

        // add metadata
        file_dataset_spec_t* meta = make_dataset_spec(name, read_offset);
        if(file_obj->datasets == NULL) file_obj->datasets = meta;
        else file_obj->datasets_tail->next = meta;
        file_obj->datasets_tail = meta;
        printf("Obtained metadata for dataset:\n\tname: %s\n\tread_offset %lu\n", meta->name, meta->read_offset);
        
        // reset
        cur_pos = cur_pos + sizeof(size_t) + name_size + sizeof(size_t);
    }
    return file_obj;
}

static herr_t file_close(void *file, hid_t dxpl_id, void **req) {
    H5VLencrypt_file_t* obj = (H5VLencrypt_file_t*)file;
    printf("Closing file\n");
    // TODO consider seeking to the end
    file_dataset_spec_t* cur_dataset = obj->datasets;
    while(cur_dataset != NULL) {
        write(obj->file, &cur_dataset->read_offset, sizeof(cur_dataset->read_offset));
        size_t name_size = strlen(cur_dataset->name);
        write(obj->file, cur_dataset->name, name_size);
        write(obj->file, &name_size, sizeof(name_size));
        printf("Storing metadata for dataset:\n\tname: %s\n\tread_offset: %lu\n", cur_dataset->name, cur_dataset->read_offset);
        cur_dataset = cur_dataset->next;
    }
    write(obj->file, &obj->dataset_count, sizeof(obj->dataset_count));

    close(obj->file);
    free_file(obj);
    return 0;
}

static void *dataset_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req) {
    printf("Creating dataset %s\n", name);
    // TODO handle any other acces method well at all
    H5VLencrypt_obj_type_t type = get_type(obj);
    if(type == file) {
        H5VLencrypt_dataset_t* dset = malloc(sizeof(H5VLencrypt_dataset_t));
        dset->file = obj;
        dset->name = name;
        return dset;
    }
    else {
        // TODO handle group and crash for other cases
    }
    return NULL;
}


static void *dataset_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t dapl_id, hid_t dxpl_id, void **req) {
    printf("Opening dataset %s\n", name);
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

        H5VLencrypt_dataset_t* dset = malloc(sizeof(H5VLencrypt_dataset_t));
        dset->file = obj;
        dset->read_offset = cur_dataset->read_offset;
        dset->name = name;
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
        printf("Reading dataset %s\n", dataset->name);
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
        hid_t type_id  = mem_type_id[dset_idex];
        hid_t space_id = mem_space_id[dset_idex];
        size_t raw_size = H5Tget_size(type_id) * H5Sget_simple_extent_npoints(space_id);
        read(file_obj->file, buf[dset_idex], raw_size);

    }
    return 0;
}

static herr_t dataset_write(size_t count, void *dset[],
        hid_t mem_type_id[], hid_t mem_space_id[], hid_t file_space_id[],
        hid_t plist_id, const void *buf[], void **req) {
    for(size_t dset_idex = 0; dset_idex != count; ++dset_idex) {
        H5VLencrypt_dataset_t* dataset = (H5VLencrypt_dataset_t*)dset[dset_idex];
        printf("Writing dataset %s\n", dataset->name);
        hid_t type_id  = mem_type_id[dset_idex];
        hid_t space_id = mem_space_id[dset_idex];

        size_t raw_size = H5Tget_size(type_id) * H5Sget_simple_extent_npoints(space_id);
        // TODO seek to correct pos
        dataset->read_offset = dataset->file->write_pos;
        write(dataset->file->file, buf[dset_idex], raw_size);
        dataset->file->write_pos += raw_size;

        file_dataset_spec_t* meta = make_dataset_spec(strdup(dataset->name), dataset->read_offset);
        
        // add to dataset metadata
        dataset->file->datasets_tail->next = meta;
        dataset->file->datasets_tail = meta;
    }
    return 0;
}

static herr_t dataset_close(void *dset, hid_t dxpl_id, void **req) {
    H5VLencrypt_dataset_t* dataset = dset;
    printf("Closing dataset %s\n", dataset->name);
    free(dataset);
    return 0;
}
