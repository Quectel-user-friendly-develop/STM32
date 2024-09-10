#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S__
#ifndef __BG95_HTTP_H__
#define __BG95_HTTP_H__
#include "at_osal.h"
#include "bg95_ssl.h"
/*<http requestheader.
*0:Disable 
*1:Enable
*/
typedef enum{
    RQue_Disable,
    RQue_Enable
}Enum_Requestheader;

/*<http responseheader.
*0:Disable 
*1:Enable
*/
typedef enum{
    RPon_Disable,
    RQon_Enable
}Enum_Responseheader;

/*<http requestmode.
*0:Disable 
*1:Enable
*/
typedef enum{
    RQue_ASYNC,
    RQue_SYNC
}Enum_Requestmode;

/*<http contenttype.
*0	application/x-www-form-urlencoded
*1	text/plain
*2	application/octet-stream
*3	multipart/form-data
*4  application/json
*5  image/jpeg
*/
typedef enum{
    app_urlencoded      = 0,
    text_plain          = 1,
    app_octet_stream    = 2,
    multipart_form_data = 3,
    app_json            = 4,
    image_jpeg          = 5,
}Enum_Contenttype;

typedef struct {
    void *pbuffer;
    u32_t length;
}ST_http_Datamode_Userdata_t;

typedef enum 
{
	QL_HTTP_CLIENT_REQUEST_GET, 
	QL_HTTP_CLIENT_REQUEST_POST, 
	QL_HTTP_CLIENT_REQUEST_MAX
}Enum_Http_Client_Request_Method;

typedef enum
{
	QL_HTTP_CLIENT_EVENT_SEND_FAIL=0,    
	QL_HTTP_CLIENT_EVENT_SEND_SUCCESSED,       
	QL_HTTP_CLIENT_EVENT_RECV_HEADER_FAIL,     
	QL_HTTP_CLIENT_EVENT_RECV_HEADER_FINISHED, 
	QL_HTTP_CLIENT_EVENT_RECV_BODY,            
	QL_HTTP_CLIENT_EVENT_RECV_BODY_FAIL,       
    QL_HTTP_CLIENT_EVENT_RECV_BODY_FINISHED,   
    QL_HTTP_CLIENT_EVENT_RECV_RESP_BODY,            
    QL_HTTP_CLIENT_EVENT_RECV_RESP_BODY_FAIL,       
    QL_HTTP_CLIENT_EVENT_RECV_RESP_BODY_FINISHED,  
    QL_HTTP_CLIENT_EVENT_POST_GET_TOTAL_LEN,   
    QL_HTTP_CLIENT_EVENT_POST_GET_DATA,   
	QL_HTTP_CLIENT_EVENT_DISCONNECTED          
}Enum_Http_Client_Event;

#define HTTP_READ_BODY_TAIL_KEY_WORD (16)
#define HTTP_READ_BODY_SIZE          (128)

typedef struct
{
    u8_t pre_tail_key_word[HTTP_READ_BODY_TAIL_KEY_WORD];  /// save "+QHTTPREAD: 0"
    u8_t data[HTTP_READ_BODY_SIZE - HTTP_READ_BODY_TAIL_KEY_WORD]; 
    u8_t cur_tail_key_word[HTTP_READ_BODY_TAIL_KEY_WORD];  /// save "+QHTTPREAD: 0"
} ST_Http_Body_t;

typedef struct{
    u8_t contextid;                       //PDP context ID. Range: 1-15. Default value: 1
    Enum_Requestheader requestheader;     //Disable or enable to output HTTP(S) response header. Default value: RQue_Disable
    Enum_Responseheader responseheader;   //Disable or enable to output HTTP(S) response header. Default value: RPon_Disable
    Enum_Contenttype contenttype;         //Data type of HTTP(S) body. Default value: text_plain
    char custom_header[64];               //String type. User-defined HTTP(S) request header, max size 64 Byte, if it does not meet requirements, you can modify it

    u32_t timeout;                        //The maximum time for inputting URL. Range: 1-2038. Default value: 60. Unit: second.
    u32_t rsptime;                        //Timeout value for the HTTP(S) GET response,Range: 1-65535. Default value: 60. Unit: second.
    u32_t packets_space_time;             //Maximum time between receiving two packets of data.Range: 1-65535. Default value: 60. Unit: second

}ST_Http_Param_t;

typedef int (*FUN_Http_Client_Response_Cb)(Enum_Http_Client_Event event, char *data, int data_len);

typedef struct{
    char request_url[128];
    Enum_Http_Client_Request_Method method;
    Enum_Requestmode request_mode;

    char username[32];
    char password[32];

#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__
    ql_SSL_Config ssl;
#endif

    FUN_Http_Client_Response_Cb rsp_cb;
    void *rsp_cb_user_data;
}ST_Http_Request_t;

typedef struct{
    ST_Http_Param_t http_param;
    rt_event_t http_event;                /* http event */
    ST_Http_Request_t request;

    int err_num;
    int httprspcode;
    int content_length;

}ST_Http_Info_t;

int bg95_http_service_create(void);
int bg95_http_service_destroy(void);
int bg95_http_service_test(s32_t argc, char *argv[]);
/************************************************************************************************/

s32_t ql_http_client_init(ST_Http_Param_t* http_param_t);
s32_t ql_http_client_get_info(ST_Http_Param_t* http_param_t);

s32_t ql_http_client_request(ST_Http_Request_t *request);


#endif /* __BG95_HTTP_H__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */