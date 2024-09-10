#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FILESYSTEM__

/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-10-12     armink       first version
 */

/*
 * NOTE: DO NOT include this file on the header file.
 */
#ifndef __BG95_FILESYSTEM_H__
#define __BG95_FILESYSTEM_H__
#include "ff.h"
#include "at_osal.h"
typedef struct {
    char filename[256];
    rt_uint32_t filesize;
} File_Moudle_Info;

struct file_device
{
    struct at_client *client;                    /* AT Client object for AT device */
    rt_event_t file_event;
    #ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_TFCARD__
    FIL file;                      /* AT device socket event */
    #endif
};
int ql_module_list_get(const char *dirname, File_Moudle_Info *fileList, u8_t maxFiles, u8_t mode);
int ql_file_put_ex(char *localfile, char *remotefile,u32_t up_size );
int ql_file_del(const char *dirname);
int QL_fs_free(char *localfile, rt_size_t *free_size, rt_size_t *total_size);
int QL_fs_open(char *localfile,u8_t mode);
int QL_fs_write(u8_t file_handle,u8_t wirte_size, char *wirte_buffer);
int QL_fs_close(u8_t file_handle);
int QL_fs_read(u8_t file_handle, u8_t read_size, char *out_data);
#endif /* __BG95_FTP_H__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FILESYSTEM__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */
