#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S__
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
#ifndef __BG95_FTP_H__
#define __BG95_FTP_H__

#include "ff.h"
#include "bg95_ssl.h"
#include "at_osal.h"
#include "bg95_filesystem.h"
// #define BG95_EVENT_FTP_OPEN_OK             (1L << 0)
// #define BG95_EVENT_FTP_OPEN_FIAL             (1L << 1)
typedef size_t (*QL_FTP_CLIENT_WRITE_CB_EX)(void *ptr, size_t size, size_t nmemb, void *stream);
typedef size_t (*QL_FTP_CLIENT_READ_CB_EX)(void *ptr, size_t size, size_t nmemb, void *stream);
typedef struct {
    u8_t context_id;
    char username[10];
    char password[10];
    u8_t filetype;
    u8_t transmode;
    u8_t rsptimeout;
    char hostname[20]; // ?????200??
    int port; // ???,???21
    #ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__
    ql_SSL_Config ssl_config; //
    #endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__ */
    QL_FTP_CLIENT_WRITE_CB_EX write_cb; // FTP?????
    #ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_TFCARD__
    FIL file; 
    #endif 
    QL_FTP_CLIENT_READ_CB_EX read_cb; // FTP?????
} FTP_Config;

typedef struct {
    char filename[256];
    rt_uint32_t filesize;
    char date[30];  // ?????????,? "Sep 15  2022"
} FileInfo;

struct ftp_device
{
  
    struct at_client *client;                    /* AT Client object for AT device */
                           /* IP address */
    rt_event_t ftp_urc_event;
    rt_uint32_t filesize;                     
    void *user_data;

};


void PrintFileList(FileInfo *fileList, int fileCount);
int QL_ftp_cfg(FTP_Config *config);
int bg95_ftp_service_create(void);
int bg95_ftp_service_destroy(void);
int bg95_ftp_service_test(int argc, char *argv[]);
int QL_ftp_path_get(char *path_name, u8_t path_name_size);
int QL_ftp_list_get(const char *dirname, const char *local_name,FileInfo *fileList, u8_t maxFiles);
int QL_ftp_path_cfg(const char *path_name) ;
int QL_ftp_get(const char *filename, const char *target, int offset) ;
int ql_ftp_client_get_ex(char *remotefile, char *localfile, QL_FTP_CLIENT_WRITE_CB_EX write_cb) ;
int ql_ftp_client_put_ex(char *localfile, char *remotefile, QL_FTP_CLIENT_READ_CB_EX read_cb);

#endif /* __BG95_FTP_H__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */
