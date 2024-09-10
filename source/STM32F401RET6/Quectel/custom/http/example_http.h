#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S_EXAMPLE__
#ifndef __EXAMPLE_HTTP_H__
#define __EXAMPLE_HTTP_H__

#include "bg95_http.h"
typedef struct {
    ST_Http_Param_t param;
    ST_Http_Request_t request;
    char sd_card_path[64]; 
}http_test_config;

int example_http_test(void *argument);

#endif /* __EXAMPLE_HTTP_H__ */

#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S_EXAMPLE__ */