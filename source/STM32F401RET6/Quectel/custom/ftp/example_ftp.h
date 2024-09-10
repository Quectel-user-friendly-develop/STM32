#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S_EXAMPLE__
#ifndef __EXAMPLE_FTP_H__
#define __EXAMPLE_FTP_H__

#include "bg95_ftp.h"
FTP_Config Ql_get_ftp_Config();
typedef struct {
    u8_t context_id;
    char username[10];
    char password[10];
    u8_t filetype;
    u8_t transmode;
    u8_t rsptimeout;
    char request_url[32];
    int port;
    char directoryToSet[16];
    char local_name[64];
    char rem_name[64];
    #ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__
    ql_SSL_Config ssl_config; //
    #endif 
    int ftp_type;
}ftp_test_config;
int example_ftp_test(void *argument);
int ftp_test_get_List(ftp_test_config *config);
int ftp_test_uploader(ftp_test_config *config);
int ftp_test_get(ftp_test_config *config);
void ftp_test_close(void);
int ftp_test_open(ftp_test_config *config);
#endif /* __EXAMPLE_FTP_H__ */

#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S_EXAMPLE__ */