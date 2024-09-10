#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S_EXAMPLE__
#include "cmsis_os2.h"
#include "at_osal.h"
#include "debug_service.h"
#include "broadcast_service.h"
#include "debug_service.h"
#include "bg95_http.h"
#include "bg95_ssl.h"
#include "example_http.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_TFCARD__
#include "ff.h"
#endif 

static char g_file_path[64];

static int http_client_response_cb(Enum_Http_Client_Event event, char *data, int data_len)
{
    int i, ret;
    static int fd = -1;
    #ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_TFCARD__
	static FIL SDFile;
    #endif
	unsigned int fnum, file_size = 0; 
    static int num = 0;
    char path[64];

    switch (event)
    {
        case QL_HTTP_CLIENT_EVENT_SEND_FAIL:            
            LOG_I("QL_HTTP_CLIENT_EVENT_SEND_FAIL %d", data_len);
            break;

        case QL_HTTP_CLIENT_EVENT_SEND_SUCCESSED:       
            LOG_I("QL_HTTP_CLIENT_EVENT_SEND_SUCCESSED %d", data_len);
            break;

        case QL_HTTP_CLIENT_EVENT_RECV_HEADER_FAIL:     
            LOG_I("QL_HTTP_CLIENT_EVENT_RECV_HEADER_FAIL %d", data_len);
            break;

        case QL_HTTP_CLIENT_EVENT_RECV_HEADER_FINISHED: 
            LOG_I("QL_HTTP_CLIENT_EVENT_RECV_HEADER_FINISHED %d", data_len);
            break;

        case QL_HTTP_CLIENT_EVENT_RECV_BODY:           
            // LOG_I("QL_HTTP_CLIENT_EVENT_RECV_BODY %d", data_len);
            // LOG_I("%s", data);

            // for (i=0; i<data_len; i++)
            // {
            //     LOG_I("%3d, %3d = 0x%x, %c", i, num++, data[i], data[i]);
            // }
            #ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_TFCARD__
            if (fd == -1)
            {
                sprintf(path, "0:%s", g_file_path);
                LOG_V("patch = %s", path);
                fd = f_open(&SDFile, path, FA_CREATE_ALWAYS | FA_WRITE);
                if(fd != FR_OK)
                {
                    fd = -1;
                    LOG_E("open file error : %d", fd);
                    return;
                }
                num = 0;
            }
            if (fd != -1)
            {
                num+=data_len;
                f_write(&SDFile, data, data_len, &fnum);
            }
            #endif
            break;

        case QL_HTTP_CLIENT_EVENT_RECV_BODY_FINISHED:  
            LOG_I("QL_HTTP_CLIENT_EVENT_RECV_BODY_FINISHED %d, total get %d byte", data_len, num);
            #ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_TFCARD__
            f_close(&SDFile);
            #endif
            fd = -1;
            break;

        case QL_HTTP_CLIENT_EVENT_DISCONNECTED:         
            LOG_I("QL_HTTP_CLIENT_EVENT_DISCONNECTED %d", data_len);
            break;
        
        case QL_HTTP_CLIENT_EVENT_RECV_RESP_BODY:
            LOG_I("QL_HTTP_CLIENT_EVENT_RECV_RESP_BODY %d", data_len);
            LOG_I("%s", data);
            break; 

        case QL_HTTP_CLIENT_EVENT_RECV_RESP_BODY_FAIL: 
            LOG_I("QL_HTTP_CLIENT_EVENT_RECV_RESP_BODY_FAIL %d", data_len);
            break; 

        case QL_HTTP_CLIENT_EVENT_RECV_RESP_BODY_FINISHED:
            LOG_I("QL_HTTP_CLIENT_EVENT_RECV_RESP_BODY_FINISHED %d", data_len);
            break;  
        
        case QL_HTTP_CLIENT_EVENT_POST_GET_TOTAL_LEN:
            LOG_I("QL_HTTP_CLIENT_EVENT_GET_POST_TOTAL_LEN");
            #ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_TFCARD__
            //read data from sd card
            sprintf(path, "0:%s", g_file_path);
            fd = f_open(&SDFile, path, FA_OPEN_EXISTING | FA_READ);
            if(fd != FR_OK)
            {
                LOG_E("open file error : %d", fd);
                return 0;
            }
            num = 0;
            file_size = f_size(&SDFile);
            LOG_I("path = %s, file size = %d", path, file_size);
            #endif 
            return file_size;

        case QL_HTTP_CLIENT_EVENT_POST_GET_DATA:
            // LOG_I("QL_HTTP_CLIENT_EVENT_POST_GET_DATA %d", data_len);
            #ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_TFCARD__
            ret = f_read(&SDFile, data, data_len, &fnum);
            if ((ret != FR_OK) || (fnum == 0))
            {
                f_close(&SDFile);
                fd = -1;
                LOG_I("Close file, total post %d byte", num);
                return 0;  
            }
            #endif 
            // LOG_V("read %s file %d, %d, %d", path, fnum, data_len, ret);
            num+=fnum;
            return fnum;

        default:
            break;
    }

    return 0;
}

int example_http_test(void *argument)
{
    http_test_config *config = (http_test_config *)argument;
    ST_Http_Request_t request;
    ST_Http_Param_t http_param_t = 
    {
        .contextid          = config->param.contextid,                    
        .requestheader      = config->param.requestheader,     
        .responseheader     = config->param.responseheader,   
        .contenttype        = config->param.contenttype,        
        .custom_header      = "",             

        .timeout            = config->param.timeout,                      
        .rsptime            = config->param.rsptime,                        
        .packets_space_time = config->param.packets_space_time,    
    };
    //strcpy(http_param_t.custom_header, config->param.custom_header);
    ql_http_client_init(&http_param_t);

    strcpy(request.request_url, config->request.request_url);
    request.method          = config->request.method;
    request.request_mode    = config->request.request_mode;
    strcpy(request.username, config->request.username);
    strcpy(request.password, config->request.password);
    request.rsp_cb          = http_client_response_cb;

    #ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__
    request.ssl.sslenble    = config->request.ssl.sslenble;
    request.ssl.sslctxid    = config->request.ssl.sslctxid;
    request.ssl.ciphersuite = config->request.ssl.ciphersuite;
    request.ssl.seclevel    = config->request.ssl.seclevel;
    request.ssl.sslversion  = config->request.ssl.sslversion;  
    #endif
    
    strcpy(g_file_path, config->sd_card_path);
    ql_http_client_request(&request);

    LOG_V("%s over",__FUNCTION__);	
    os_thread_exit();
    return 0;
}

#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S_EXAMPLE__ */
