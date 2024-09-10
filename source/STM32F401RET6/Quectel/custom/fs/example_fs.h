#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FILESYSTEM_EXAMPLE__
#ifndef __EXAMPLE_FS_H__
#define __EXAMPLE_FS_H__

#include "bg95_filesystem.h"
typedef struct {
    int fs_type;//0 :list 
    char name_pattern[10];
    u8_t open_mode;
    u8_t file_handle;
    u8_t wirte_read_size; //
    char wirte_buffer[30];
}fs_test_config;
#endif /* __EXAMPLE_FTP_H__ */
#endif  /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FILESYSTEM_EXAMPLE__ */