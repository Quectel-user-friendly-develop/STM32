#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S__
#include "at.h"
#include "bg95_http.h"
#include "at_osal.h"
#include "broadcast_service.h"
#include "bg95_filesystem.h"

static osMsgQueId_t g_bg95_http_msg_id = NULL;
static osThreadId_t g_bg95_http_thread_id = NULL;

/* http event type */
#define BG95_HTTP_EVENT_AT_REQUEST_OK             (1L << (8 + 0))
#define BG95_HTTP_EVENT_AT_REQUEST_FAIL           (1L << (8 + 1))
#define BG95_HTTP_EVENT_AT_READ_OK                (1L << (8 + 2))
#define BG95_HTTP_EVENT_AT_READ_FAIL              (1L << (8 + 3))
#define BG95_HTTP_EVENT_REQUEST_OVER              (1L << (8 + 4))

static ST_Http_Info_t g_http_info = {0};

static int http_rspcode_parse(int result)
{
    int ret = -1;

    LOG_V("%s, %d",__FUNCTION__, result);	

    switch(result)
    {
        case 200 : LOG_D("%d : OK",                           result); ret = 0;  break;
        case 403 : LOG_E("%d : Forbidden",                    result); ret = -1; break;
        case 404 : LOG_E("%d : Not found",                    result); ret = -1; break;
        case 409 : LOG_E("%d : Conflict",                     result); ret = -1; break;
        case 411 : LOG_E("%d : Length required",              result); ret = -1; break;
        case 500 : LOG_E("%d : Internal server error",        result); ret = -1; break;
        default  : LOG_E("%d : Unknown err code",             result); ret = -1; break;
    }

    return ret;
}

static int http_errcode_parse(int result)
{
    int ret = 0;
    LOG_V("%s, %d",__FUNCTION__, result);	

    switch(result)
    {
        case 0   : LOG_D("%d : Operation successful",                    result); ret = 0;  break;
        case 701 : LOG_E("%d : HTTP(S) unknown error",                   result); ret = -1; break;
        case 702 : LOG_E("%d : HTTP(S) timeout",                         result); ret = -1; break;
        case 703 : LOG_E("%d : HTTP(S) busy",                            result); ret = -1; break;
        case 704 : LOG_E("%d : HTTP(S) UART busy",                       result); ret = -1; break;
        case 705 : LOG_E("%d : HTTP(S) no GET/POST/PUT requests",        result); ret = -1; break;
        case 706 : LOG_E("%d : HTTP(S) network busy",                    result); ret = -1; break;
        case 707 : LOG_E("%d : HTTP(S) network open failed",             result); ret = -1; break;
        case 708 : LOG_E("%d : HTTP(S) network no configuration",        result); ret = -1; break;
        case 709 : LOG_E("%d : HTTP(S) network deactivated",             result); ret = -1; break;
        case 710 : LOG_E("%d : HTTP(S) network error",                   result); ret = -1; break;
        case 711 : LOG_E("%d : HTTP(S) URL error",                       result); ret = -1; break;
        case 712 : LOG_E("%d : HTTP(S) empty URL",                       result); ret = -1; break;
        case 713 : LOG_E("%d : HTTP(S) IP address error",                result); ret = -1; break;
        case 714 : LOG_E("%d : HTTP(S) DNS error",                       result); ret = -1; break;
        case 715 : LOG_E("%d : HTTP(S) socket create error",             result); ret = -1; break;
        case 716 : LOG_E("%d : HTTP(S) socket connect error",            result); ret = -1; break;
        case 717 : LOG_E("%d : HTTP(S) socket read error",               result); ret = -1; break;
        case 718 : LOG_E("%d : HTTP(S) socket write error",              result); ret = -1; break;
        case 719 : LOG_D("%d : HTTP(S) socket closed",                   result); ret = -1; break;
        case 720 : LOG_E("%d : HTTP(S) data encode error",               result); ret = -1; break;
        case 721 : LOG_E("%d : HTTP(S) data decode error",               result); ret = -1; break;
        case 722 : LOG_E("%d : HTTP(S) read timeout",                    result); ret = -1; break;
        case 723 : LOG_E("%d : HTTP(S) response failed",                 result); ret = -1; break;
        case 724 : LOG_E("%d : Incoming call busy",                      result); ret = -1; break;
        case 725 : LOG_E("%d : Voice call busy",                         result); ret = -1; break;
        case 726 : LOG_E("%d : Input timeout",                           result); ret = -1; break;
        case 727 : LOG_E("%d : Wait data timeout",                       result); ret = -1; break;
        case 728 : LOG_E("%d : Wait HTTP(S) response timeout",           result); ret = -1; break;
        case 729 : LOG_E("%d : Memory allocation failed",                result); ret = -1; break;
        case 730 : LOG_E("%d : Invalid parameter",                       result); ret = -1; break;
        default  : LOG_E("%d : Unknown err code",                        result); ret = -1; break;
    }

    return ret;
}
static int bg95_http_event_send(ST_Http_Info_t *http_info, uint32_t event)
{
    LOG_V("%s, 0x%x",__FUNCTION__, event);	
    return (int) OsalEventSend(http_info->http_event, event);
}

static int bg95_http_event_recv(ST_Http_Info_t *http_info, uint32_t event, uint32_t timeout, rt_uint8_t option)
{
    rt_uint32_t recved;

    LOG_V("%s, event = 0x%x, timeout = %d, option = %d",__FUNCTION__, event, timeout, option);	

    recved = OsalEventRecv(http_info->http_event, event, option, timeout);
    if (recved < 0)
    {
        return -RT_ETIMEOUT;
    }

    return recved;
}

static void urc_qhttpget_func(struct at_client *client, const char *data, rt_size_t size)
{
    int httprspcode_ret, err_num_ret;

    LOG_V("%s, size = %d",__FUNCTION__, size);	

    RT_ASSERT(data && size);
    sscanf(data, "+QHTTPGET: %d,%d,%d", &err_num_ret, &httprspcode_ret, &g_http_info.content_length);
    g_http_info.httprspcode = http_rspcode_parse(httprspcode_ret);
    g_http_info.err_num     = http_errcode_parse(err_num_ret);
    if ((g_http_info.httprspcode == 0) && (g_http_info.err_num == 0))
        bg95_http_event_send(&g_http_info, BG95_HTTP_EVENT_AT_REQUEST_OK);
    else 
        bg95_http_event_send(&g_http_info, BG95_HTTP_EVENT_AT_REQUEST_FAIL);
}

static void urc_cmr_error_func(struct at_client *client, const char *data, rt_size_t size)
{
    int err_num_ret;

    LOG_V("%s, size = %d",__FUNCTION__, size);	

    RT_ASSERT(data && size);
    sscanf(data, "+CME ERROR:%d", &err_num_ret);
    g_http_info.err_num = http_errcode_parse(err_num_ret);
    bg95_http_event_send(&g_http_info, BG95_HTTP_EVENT_AT_REQUEST_FAIL);
}

static void urc_httpread_func(struct at_client *client, const char *data, rt_size_t size)
{
    int err_num_ret;

    LOG_V("%s, size = %d",__FUNCTION__, size);	

    RT_ASSERT(data && size);
    sscanf(data, "+QHTTPREAD:%d", &err_num_ret);
    g_http_info.err_num = http_errcode_parse(err_num_ret);
}

static void urc_httppost_func(struct at_client *client, const char *data, rt_size_t size)
{
    int httprspcode_ret, err_num_ret;

    LOG_V("%s, size = %d",__FUNCTION__, size);	

    RT_ASSERT(data && size);
    sscanf(data, "+QHTTPPOST: %d,%d,%d", &err_num_ret, &httprspcode_ret, &g_http_info.content_length);
    g_http_info.httprspcode = http_rspcode_parse(httprspcode_ret);
    g_http_info.err_num     = http_errcode_parse(err_num_ret);
    if ((g_http_info.httprspcode == 0) && (g_http_info.err_num == 0))
        bg95_http_event_send(&g_http_info, BG95_HTTP_EVENT_AT_REQUEST_OK);
    else 
        bg95_http_event_send(&g_http_info, BG95_HTTP_EVENT_AT_REQUEST_FAIL);
}

static const struct at_urc urc_table[] =
{
 //   {"+CME ERROR:",    "\r\n",                 urc_cmr_error_func},
    {"+QHTTPGET:",     "\r\n",                 urc_qhttpget_func},
    {"+QHTTPREAD:",    "\r\n",                 urc_httpread_func},
    {"+QHTTPPOST:",    "\r\n",                 urc_httppost_func},
};

#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__
static int configure_http_ssl(ql_SSL_Config *config) 
{
    if (config->sslenble) 
    {
        // SSL is enabled, configure SSL settings using ql_sslcfg_set function

        at_response_t query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(400));
        if (!query_resp) {
            LOG_E("Failed to create response object.\n");
            return -1;
        }
        // Send the "sslctxid" command
        if (at_exec_cmd(query_resp, "AT+QHTTPCFG=\"sslctxid\",%d\r\n",config->sslctxid) < 0) 
        {
            LOG_E("AT+QHTTPCFG=\"sslctxid\" failed");
            at_delete_resp(query_resp);
            return -1;
        }
        at_delete_resp(query_resp); // Delete the response object and return an error.

        // Configure SSL settings if ssltenble is set
        if (configure_ssl(config) != 0) {
            LOG_E("SSL configuration failed.\n");
            return -1;
        }

    }
    return 0; // Return success
}
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__ */

static s32_t ql_http_param_init(ST_Http_Param_t* http_param_t)
{
    at_response_t query_resp = NULL;

    RT_ASSERT(http_param_t);
    LOG_V("%s",__FUNCTION__);	

    memcpy(&g_http_info.http_param, http_param_t, sizeof(ST_Http_Param_t));
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(300));
    if (query_resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    parameter_range_check(http_param_t->contextid, 1, 16, 1);
    if (at_exec_cmd(query_resp, "AT+QHTTPCFG=\"contextid\",%d\r\n",http_param_t->contextid) < 0) 
    {
        LOG_E("AT+QHTTPCFG=\"contextid\" failed");
        at_delete_resp(query_resp);
        return -1;
    }

    parameter_range_check(http_param_t->requestheader, RQue_Disable, RQue_Enable, RQue_Disable);
    at_resp_set_info(query_resp, 32, 0, rt_tick_from_millisecond(300));
    if (at_exec_cmd(query_resp, "AT+QHTTPCFG=\"requestheader\",%d\r\n",http_param_t->requestheader) < 0) 
    {
        LOG_E("AT+QHTTPCFG=\"requestheader\" failed");
        at_delete_resp(query_resp);
        return -1;
    }

    parameter_range_check(http_param_t->responseheader, RPon_Disable, RQon_Enable, RPon_Disable);
    at_resp_set_info(query_resp, 32, 0, rt_tick_from_millisecond(300));
    if (at_exec_cmd(query_resp, "AT+QHTTPCFG=\"responseheader\",%d\r\n",http_param_t->responseheader) < 0) 
    {
        LOG_E("AT+QHTTPCFG=\"responseheader\" failed");
        at_delete_resp(query_resp);
        return -1;
    }

    parameter_range_check(http_param_t->contenttype, app_urlencoded, image_jpeg, text_plain);
    at_resp_set_info(query_resp, 32, 0, rt_tick_from_millisecond(300));
    if (at_exec_cmd(query_resp, "AT+QHTTPCFG=\"contenttype\",%d\r\n",http_param_t->contenttype) < 0) 
    {
        LOG_E("AT+QHTTPCFG=\"contenttype\" failed");
        at_delete_resp(query_resp);
        return -1;
    }

    if (strlen(http_param_t->custom_header))
    {
        at_resp_set_info(query_resp, sizeof(http_param_t->custom_header) + strlen("AT+QHTTPCFG=\"custom_header\","), 0, rt_tick_from_millisecond(300));
        if (at_exec_cmd(query_resp, "AT+QHTTPCFG=\"custom_header\",%s\r\n",http_param_t->custom_header) < 0) 
        {
            LOG_E("AT+QHTTPCFG=\"custom_header\" %s failed", http_param_t->custom_header);
            at_delete_resp(query_resp);
            return -1;
        }
    }


    at_delete_resp(query_resp);

    parameter_range_check(http_param_t->timeout, 1, 65535, 60);
    parameter_range_check(http_param_t->rsptime, 1, 65535, 60);
    parameter_range_check(http_param_t->packets_space_time, 1, 65535, 60);

    return 0;
}

static int request_data_process_func(const char *data, rt_size_t len, const char *head_key_word, Enum_Http_Client_Event data_incoming_event, Enum_Http_Client_Event data_ok_event, Enum_Http_Client_Event data_fail_event)
{
    ST_Http_Body_t body;
    uint32_t read_data_len = 0, need_read_data_len = 0, total_read_data_len = 0, need_read_count = ALIGN_UP(g_http_info.content_length, HTTP_READ_BODY_SIZE)/HTTP_READ_BODY_SIZE, i;
    int result = 0;

    if (g_http_info.request.rsp_cb)
    {
        at_client_recv(body.data, strlen(head_key_word), 2*RT_TICK_PER_SECOND);
        if (memcmp(body.data, head_key_word, strlen(head_key_word)) != 0)
        {
            LOG_E("GET CONNECT FAILED %s",body.data);
            result = -1;
            goto __req_data_proc_exit;
        }
        LOG_V("need_read_count = %d, g_http_info.content_length = %d", need_read_count, g_http_info.content_length);

        for (i=0; i<need_read_count; i++)
        {
            if (g_http_info.content_length - total_read_data_len > HTTP_READ_BODY_SIZE)
                need_read_data_len = HTTP_READ_BODY_SIZE;
            else
                need_read_data_len = g_http_info.content_length - total_read_data_len;
                
            read_data_len = at_client_recv(body.data, need_read_data_len, 2000);
            total_read_data_len += read_data_len;
            // LOG_V("need_read_count = %d, i = %d, need_read_data_len = %d, read_data_len = %d, total_read_data_len = %d", need_read_count, i, need_read_data_len, read_data_len, total_read_data_len);

            if (read_data_len == need_read_data_len)
            {
                if (i == 0)  //First packet
                {
                    if (need_read_data_len == HTTP_READ_BODY_SIZE) //One pack is full, maybe need more packets
                    {
                        g_http_info.request.rsp_cb(data_incoming_event, body.data, HTTP_READ_BODY_SIZE-HTTP_READ_BODY_TAIL_KEY_WORD);
                        memcpy(body.pre_tail_key_word, &body.cur_tail_key_word, HTTP_READ_BODY_TAIL_KEY_WORD);
                    }
                    else  //One pack is enough (need_read_data_len < HTTP_READ_BODY_SIZE)
                    {
                        g_http_info.request.rsp_cb(data_incoming_event, body.data, need_read_data_len);
                    }
                }
                else if (i == (need_read_count - 1)) //Last pack 
                {
                    g_http_info.request.rsp_cb(data_incoming_event, body.pre_tail_key_word, need_read_data_len + HTTP_READ_BODY_TAIL_KEY_WORD);
                }
                else //Middle packs
                {
                    g_http_info.request.rsp_cb(data_incoming_event, body.pre_tail_key_word, need_read_data_len);
                    memcpy(body.pre_tail_key_word, &body.cur_tail_key_word, HTTP_READ_BODY_TAIL_KEY_WORD);
                }
            }
            else
            {
                if (strstr(body.pre_tail_key_word, "+QHTTPREAD:") != NULL)
                {
                    LOG_E("Http read timeout");
                }
                else if (strstr(body.pre_tail_key_word, "+CME ERROR:") != NULL)
                {
                    LOG_E("Http read err");
                }
                else
                {
                    LOG_E("Http read other err");
                }
                result = -1;
                goto __req_data_proc_exit;
            }
        }

__req_data_proc_exit:
        if (result == 0)
        {                   
            g_http_info.request.rsp_cb(data_ok_event, NULL, 0);
        }
        else 
        {
            LOG_E("Received data error");
            g_http_info.request.rsp_cb(data_fail_event, NULL, 0);
        }
    }

    return result;
}

static void bg95_http_client_request_get_func(const char *data, rt_size_t len)
{
    int result = 0;

    result = request_data_process_func(data, len, "CONNECT\r\n", QL_HTTP_CLIENT_EVENT_RECV_BODY, QL_HTTP_CLIENT_EVENT_RECV_BODY_FINISHED, QL_HTTP_CLIENT_EVENT_RECV_BODY_FAIL);
    if (result == 0)
        bg95_http_event_send(&g_http_info, BG95_HTTP_EVENT_AT_READ_OK);
    else 
        bg95_http_event_send(&g_http_info, BG95_HTTP_EVENT_AT_READ_FAIL);
}

static void bg95_http_client_request_post_func(const char *data, rt_size_t len)
{
    int result = 0;

    result = request_data_process_func(data, len, "CONNECT\r\n", QL_HTTP_CLIENT_EVENT_RECV_RESP_BODY, QL_HTTP_CLIENT_EVENT_RECV_RESP_BODY_FINISHED, QL_HTTP_CLIENT_EVENT_RECV_RESP_BODY_FAIL);
    if (result == 0)
        bg95_http_event_send(&g_http_info, BG95_HTTP_EVENT_AT_READ_OK);
    else 
        bg95_http_event_send(&g_http_info, BG95_HTTP_EVENT_AT_READ_FAIL);
}

static s32_t bg95_http_client_request(ST_Http_Request_t *request)
{
    int i, result = -1;
    uint32_t event = 0, event_result, total_need_send_data_len = 0, read_size = 0, total_read_len = 0;
    at_response_t query_resp = NULL;
    uint8_t temp[128];

    LOG_V("%s",__FUNCTION__);	

    RT_ASSERT(request);
    RT_ASSERT(request->request_url);
    RT_ASSERT(request->rsp_cb);
    LOG_D("%s: request_url=%s, method=%d, request_mode = %d, username = %s, password = %s",__FUNCTION__, request->request_url, request->method, request->request_mode, request->username, request->password);	

    //1. Set SSL
    #ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__
    if (configure_http_ssl(&request->ssl) != 0)
    {
        LOG_E("SSL configuration failed.\n");
        goto __exit;
    }
    #endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__ */

    //2. Set URL
    query_resp = at_create_resp(32, 2, rt_tick_from_millisecond(g_http_info.http_param.timeout*RT_TICK_PER_SECOND));
    if (query_resp == NULL) {
        LOG_E("No memory for response object");
        return result;
    }

    at_exec_cmd(query_resp, "AT+QHTTPURL=%d,%d\r\n", strlen(request->request_url), g_http_info.http_param.timeout);
    if ((at_resp_get_line_by_kw(query_resp, "CONNECT") == NULL))
    {
        LOG_E("AT+QHTTPURL=%d,%d failed CONNECT", strlen(request->request_url), g_http_info.http_param.timeout);
        goto __exit;
    }

    at_resp_set_info(query_resp, 32, 0, rt_tick_from_millisecond(600));
    if (at_exec_cmd(query_resp, "%s", request->request_url) < 0)
    {
        LOG_E("AT+QHTTPURL=%d,%d,%s timeout", strlen(request->request_url), g_http_info.http_param.timeout, request->request_url);
        goto __exit;
    }

    //3. GET/POST
    if (request->method == QL_HTTP_CLIENT_REQUEST_GET)
    {
        /* clear http event */
        event = BG95_HTTP_EVENT_AT_REQUEST_OK|BG95_HTTP_EVENT_AT_REQUEST_FAIL;
        bg95_http_event_recv(&g_http_info, event, 0, RT_EVENT_FLAG_OR);
        
        if (g_http_info.http_param.requestheader == RQue_Disable)
        {
            at_resp_set_info(query_resp, 32, 0, rt_tick_from_millisecond(300));
            if (at_exec_cmd(query_resp, "AT+QHTTPGET=%d\r\n", g_http_info.http_param.rsptime) < 0) 
            {
                LOG_E("AT+QHTTPGET=%d,timeout", g_http_info.http_param.rsptime);
                goto __exit;
            }
        }
        else if (g_http_info.http_param.requestheader == RQue_Enable)
        {
            at_resp_set_info(query_resp, 32, 2, rt_tick_from_millisecond(g_http_info.http_param.rsptime*RT_TICK_PER_SECOND));
            at_exec_cmd(query_resp, "AT+QHTTPGET=%d,%d,%d\r\n", g_http_info.http_param.rsptime, strlen(g_http_info.http_param.custom_header), g_http_info.http_param.timeout);
            if (at_resp_get_line_by_kw(query_resp, "CONNECT") == NULL)
            {
                LOG_E("AT+QHTTPGET=%d,timeout", g_http_info.http_param.rsptime);
                goto __exit;
            }

            at_resp_set_info(query_resp, 32, 0, rt_tick_from_millisecond(300));
            if (at_exec_cmd(query_resp, "%s", g_http_info.http_param.custom_header) < 0) 
            {
                LOG_E("AT+QHTTPGET=%d,%d,%s timeout", strlen(g_http_info.http_param.custom_header), g_http_info.http_param.rsptime, g_http_info.http_param.custom_header);
                goto __exit;
            }
        }

        /* waiting OK or failed result */
        LOG_D("wait +QHTTPGET:");
        event_result = bg95_http_event_recv(&g_http_info, event, g_http_info.http_param.rsptime * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR);
        if (event_result < 0)
        {
            LOG_E("http wait QHTTPGET OK|FAIL timeout.");
            goto __exit;
        }
        LOG_D("wait +QHTTPGET: over");

        if (event_result & BG95_HTTP_EVENT_AT_REQUEST_FAIL)
        {
            result = -1;
            goto  __exit;
        }
    }
    else if (request->method == QL_HTTP_CLIENT_REQUEST_POST)
    {
        total_need_send_data_len = g_http_info.request.rsp_cb(QL_HTTP_CLIENT_EVENT_POST_GET_TOTAL_LEN, NULL, 0);
        if (total_need_send_data_len <= 0)
        {
            LOG_W("The length of the data to be sent is 0");
            result = -1;
            goto  __exit;
        }

        /* clear http event */
        event = BG95_HTTP_EVENT_AT_REQUEST_OK|BG95_HTTP_EVENT_AT_REQUEST_FAIL;
        bg95_http_event_recv(&g_http_info, event, 0, RT_EVENT_FLAG_OR);

        at_resp_set_info(query_resp, 32, 2, rt_tick_from_millisecond(g_http_info.http_param.rsptime*RT_TICK_PER_SECOND));
        if (g_http_info.http_param.requestheader == RQue_Disable)
            at_exec_cmd(query_resp, "AT+QHTTPPOST=%d,%d,%d\r\n", total_need_send_data_len, g_http_info.http_param.timeout, g_http_info.http_param.rsptime);
        else if (g_http_info.http_param.requestheader == RQue_Enable)
            at_exec_cmd(query_resp, "AT+QHTTPPOST=%d,%d,%d\r\n", total_need_send_data_len+strlen(g_http_info.http_param.custom_header), g_http_info.http_param.timeout, g_http_info.http_param.rsptime);
        if (at_resp_get_line_by_kw(query_resp, "CONNECT") == NULL)
        {
            LOG_E("AT+QHTTPPOST=%d,timeout", g_http_info.http_param.rsptime);
            result = -1;
            goto __exit;
        }

        g_http_info.err_num = 0;
        g_http_info.httprspcode = 0;
        g_http_info.content_length = 0;
        while (((read_size = g_http_info.request.rsp_cb(QL_HTTP_CLIENT_EVENT_POST_GET_DATA, temp, sizeof(temp))) > 0) && (g_http_info.err_num == 0) && (g_http_info.httprspcode == 0))
        {
            at_client_send(temp, read_size);
            total_read_len += read_size;
        }
        LOG_V("http send post data over %d byte(err_num = %d, httprspcode = %d)", total_read_len, g_http_info.err_num, g_http_info.httprspcode);

        if ((g_http_info.err_num != 0) || (g_http_info.httprspcode != 0))
        {
            LOG_E("Http data is sent failed, now exit(err_num = %d, httprspcode = %d)", g_http_info.err_num, g_http_info.httprspcode);
            goto __exit;
        }

        if (total_read_len != total_need_send_data_len)
            LOG_W("The length of the obtained data is not enough, please note(need:%d, now:%d)", total_need_send_data_len, total_read_len);

        if (g_http_info.http_param.requestheader == RQue_Enable)
            result = (int) at_client_send(g_http_info.http_param.custom_header, strlen(g_http_info.http_param.custom_header));

        /* waiting OK or failed result */
        LOG_V("waiting OK or failed result");
        event_result = bg95_http_event_recv(&g_http_info, event, g_http_info.http_param.rsptime * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR);
        if (event_result < 0)
        {
            LOG_E("http wait QHTTPGET OK|FAIL timeout.");
            goto __exit;
        }
        LOG_V("waiting OK or failed result over 0x%x", event_result);
        if (event_result & BG95_HTTP_EVENT_AT_REQUEST_FAIL)
        {
            result = -1;
            goto  __exit;
        }
        result = 0;
    }
    else
    {
        LOG_E("Invalid parameter %d", request->method);
    }
    LOG_V("err_num = %d, httprspcode = %d, content_length = %d", g_http_info.err_num, g_http_info.httprspcode, g_http_info.content_length);

    if ((g_http_info.err_num == 0) && (g_http_info.httprspcode == 0) && (g_http_info.content_length > 0))
    {
        at_delete_resp(query_resp);
        if (request->method == QL_HTTP_CLIENT_REQUEST_GET)
            query_resp = at_create_resp_by_selffunc(32, 1, rt_tick_from_millisecond(g_http_info.http_param.rsptime*RT_TICK_PER_SECOND), bg95_http_client_request_get_func);
        if (request->method == QL_HTTP_CLIENT_REQUEST_POST)
            query_resp = at_create_resp_by_selffunc(32, 1, rt_tick_from_millisecond(g_http_info.http_param.rsptime*RT_TICK_PER_SECOND), bg95_http_client_request_post_func);
        if (query_resp == NULL) {
            LOG_E("No memory for response object");
            result = -1;
            goto __exit;
        }
        /* clear http event */
        event = BG95_HTTP_EVENT_AT_READ_OK|BG95_HTTP_EVENT_AT_READ_FAIL;
        bg95_http_event_recv(&g_http_info, event, 0, RT_EVENT_FLAG_OR);

        at_exec_cmd(query_resp, "AT+QHTTPREAD=%d\r\n", g_http_info.http_param.rsptime*RT_TICK_PER_SECOND);
        at_delete_resp(query_resp);
        query_resp = NULL;
        LOG_V("AT+QHTTPPOST over");

        /* waiting OK or failed result */
        LOG_V("wait read ok");
        event_result = bg95_http_event_recv(&g_http_info, event, portMAX_DELAY, RT_EVENT_FLAG_OR);
        if (event_result < 0)
        {
            LOG_E("http wait QHTTPREAD OK|FAIL timeout.");
            result = -1;
            goto __exit;
        }
        LOG_V("wait read ok over");

        if (event_result & BG95_HTTP_EVENT_AT_READ_FAIL)
        {
            result = -1;
            goto  __exit;
        }
        result = 0;
    }
    else
    {
        LOG_E("Http data is get failed, now exit(err_num = %d, httprspcode = %d, content_length = %d)", g_http_info.err_num, g_http_info.httprspcode, g_http_info.content_length);
    }

__exit:
    if (query_resp)
        at_delete_resp(query_resp);

    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(2000));
    if (query_resp == NULL) {
        LOG_E("No memory for response object");
        return result;
    }
    if (at_exec_cmd(query_resp, "AT+QHTTPSTOP") < 0) 
    {
        LOG_E("AT+QHTTPSTOP timeout");
        result = -1;
    }
    at_delete_resp(query_resp);
        
    if (result == 0)
        bcast_send_my_msg(g_bg95_http_msg_id, MSG_WHAT_BG95_HTTP_REQUEST_SUCCESS, (s32_t)request, 0, 0);  
    else
        bcast_send_my_msg(g_bg95_http_msg_id, MSG_WHAT_BG95_HTTP_REQUEST_FAILURE, (s32_t)request, 0, 0);  

    return result;
}

s32_t ql_http_client_get_info(ST_Http_Param_t* http_param_t)
{
    int ret = 0;
    at_response_t query_resp = NULL;

    LOG_V("%s",__FUNCTION__);	

    RT_ASSERT(http_param_t);

    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(300));
    if (query_resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    if ((at_exec_cmd(query_resp, "AT+QHTTPCFG=\"contextid\"\r\n") < 0) || (at_resp_parse_line_args(query_resp, 1, "+QHTTPCFG: \"contextid\",%d", &http_param_t->contextid) < 0))
    {
        LOG_E("AT+QHTTPCFG=\"contextid\" failed");
        at_delete_resp(query_resp);
        return -1;
    }

    at_resp_set_info(query_resp, 32, 0, rt_tick_from_millisecond(300));
    if ((at_exec_cmd(query_resp, "AT+QHTTPCFG=\"requestheader\"\r\n") < 0) || (at_resp_parse_line_args(query_resp, 1, "+QHTTPCFG: \"requestheader\",%d", &http_param_t->requestheader) < 0))
    {
        LOG_E("AT+QHTTPCFG=\"requestheader\" failed");
        at_delete_resp(query_resp);
        return -1;
    }

    at_resp_set_info(query_resp, 32, 0, rt_tick_from_millisecond(300));
    if ((at_exec_cmd(query_resp, "AT+QHTTPCFG=\"responseheader\"\r\n") < 0) || (at_resp_parse_line_args(query_resp, 1, "+QHTTPCFG: \"responseheader\",%d", &http_param_t->responseheader) < 0))
    {
        LOG_E("AT+QHTTPCFG=\"responseheader\" failed");
        at_delete_resp(query_resp);
        return -1;
    }

    at_resp_set_info(query_resp, 32, 0, rt_tick_from_millisecond(300));
    if ((at_exec_cmd(query_resp, "AT+QHTTPCFG=\"contenttype\"\r\n") < 0) || (at_resp_parse_line_args(query_resp, 1, "+QHTTPCFG: \"contenttype\",%d", &http_param_t->contenttype) < 0))
    {
        LOG_E("AT+QHTTPCFG=\"contenttype\" failed");
        at_delete_resp(query_resp);
        return -1;
    }

    at_resp_set_info(query_resp, sizeof(http_param_t->custom_header) + strlen("AT+QHTTPCFG=\"custom_header\","), 0, rt_tick_from_millisecond(300));
    if ((at_exec_cmd(query_resp, "AT+QHTTPCFG=\"custom_header\"\r\n") < 0) || (at_resp_parse_line_args(query_resp, 1, "+QHTTPCFG: \"custom_header\",%s", &http_param_t->custom_header) < 0))
    {
        LOG_E("AT+QHTTPCFG=\"custom_header\" failed");
        at_delete_resp(query_resp);
        return -1;
    }
    at_delete_resp(query_resp);

    http_param_t->timeout   = g_http_info.http_param.timeout;
    http_param_t->rsptime   = g_http_info.http_param.rsptime;
    http_param_t->packets_space_time = g_http_info.http_param.packets_space_time;

    return 0;
}

s32_t ql_http_client_request(ST_Http_Request_t *request)
{
    static u8_t event_flags = 0; //It supports up to 8 requests
    uint32_t event = BG95_HTTP_EVENT_REQUEST_OVER;
    s32_t ret = 0;

    LOG_V("%s, request = 0x%x, request_mode = 0x%x",__FUNCTION__, (u32_t)request, request->request_mode);	

    RT_ASSERT(request);

    memcpy(&g_http_info.request, request, sizeof(ST_Http_Request_t));

    if (request->request_mode == RQue_SYNC)
    {
        LOG_D("ql_http_client_request SYNC %x", OsalEventFlagsGet(g_http_info.http_event));
        bcast_send_my_msg(g_bg95_http_msg_id, MSG_WHAT_BG95_HTTP_REQUEST, 0, 0, 0); 
        ret = bg95_http_event_recv(&g_http_info, event, portMAX_DELAY, RT_EVENT_FLAG_OR);
        if (ret < 0)
        {
            LOG_E("ql_http_client_request timeout event = 0x%x, %d, 0x%x", event, g_http_info.http_param.rsptime, OsalEventFlagsGet(g_http_info.http_event));
            return -1;
        }
        LOG_D("ql_http_client_request SYNC over 0x%x", OsalEventFlagsGet(g_http_info.http_event));
    }
    else
    {
        bcast_send_my_msg(g_bg95_http_msg_id, MSG_WHAT_BG95_HTTP_REQUEST, 0, 0, 0); 
    }

    return 0;
}

s32_t ql_http_client_init(ST_Http_Param_t* http_param_t)
{
    LOG_V("%s",__FUNCTION__);	
    ql_http_param_init(http_param_t);
}

/*****************************************************************************************/
static void bg95_http_service_proc(void *argument)
{
    int ret;
    msg_node msgs;

	LOG_V("%s, stack space %d",__FUNCTION__, os_thread_get_stack_space(os_thread_get_thread_id()));	

    /* A service that provides network socket interface, and keep the services provided effective */
    while(1)
    {
        ret = OsalMsgRcv(g_bg95_http_msg_id, (void *)&msgs, NULL, portMAX_DELAY);
		if (ret < 0)
		{
			LOG_E("Receive msg from broadcast thread error!");
			continue;
		}
        else
        {
            LOG_V("Receive broadcast msg is what=0x%x, arg1=0x%x, arg2=0x%x, arg3=0x%x", msgs.what, msgs.arg1, msgs.arg2, msgs.arg3);
            switch (msgs.what)
            {
                case MSG_WHAT_BG95_HTTP_INIT_SUCCESS:
                    LOG_D("Do something MSG_WHAT_BG95_HTTP_INIT_SUCCESS");
                    break;

                case MSG_WHAT_BG95_HTTP_REQUEST:
                    LOG_D("Do something MSG_WHAT_BG95_HTTP_REQUEST");
                    ret = bg95_http_client_request((ST_Http_Request_t *)&g_http_info.request);
                    bg95_http_event_send(&g_http_info, BG95_HTTP_EVENT_REQUEST_OVER);
                    break;

                case MSG_WHAT_BG95_HTTP_REQUEST_SUCCESS:
                    LOG_D("Do something MSG_WHAT_BG95_HTTP_REQUEST_SUCCESS");
                    break;

                case MSG_WHAT_BG95_HTTP_REQUEST_FAILURE:
                    LOG_D("Do something MSG_WHAT_BG95_HTTP_REQUEST_FAILURE");
                    break;

                default:
                    LOG_D("Unrecognized message");
                    break;
            }
        }
    }
    LOG_V("%s over",__FUNCTION__);	
    os_thread_exit();
}

int bg95_http_service_create(void)
{
    int ret;
    osEventFlagsAttr_t event_attr;
    static osThreadAttr_t thread_attr = {.name = "Http_S", .stack_size = 512*7, .priority = osPriorityNormal};

    LOG_V("%s",__FUNCTION__);	

    //1. Creat msg queue
   	g_bg95_http_msg_id = OsalMsgQCreate(MAX_MSG_COUNT, sizeof(msg_node), NULL);
	if (NULL == g_bg95_http_msg_id)
	{
		LOG_E("Create bg95_http_service_create msg failed!"); 
		return -1;			
	}

    //2. Register which broadcasts need to be processed
    bcast_reg_receive_msg(MSG_WHAT_BG95_HTTP_REQUEST_SUCCESS,   g_bg95_http_msg_id);
    bcast_reg_receive_msg(MSG_WHAT_BG95_HTTP_REQUEST_FAILURE,   g_bg95_http_msg_id);

    //3. Deal with http problems
    memset(&event_attr, 0, sizeof(event_attr));
    event_attr.name = "http_E";
    g_http_info.http_event = OsalEventNCreate(&event_attr);
    if (g_http_info.http_event == RT_NULL)
    {
        LOG_E("no memory for http event create.");
        return -1;
    }

    /* register URC data execution function  */
    at_set_urc_table(urc_table, sizeof(urc_table) / sizeof(urc_table[0]));

    //4. Create net service
    g_bg95_http_thread_id = os_thread_create(bg95_http_service_proc, NULL, &thread_attr);
	if (NULL == g_bg95_http_thread_id)
	{
		LOG_E ("g_bg95_http_thread_id thread could not start!");
		return -1;
	}

    LOG_I("%s over(%x)",__FUNCTION__, g_bg95_http_thread_id);	

    return 0;
}

int bg95_http_service_destroy(void)
{
    int ret;
    osThreadId_t thread;
    char name[RT_NAME_MAX] = "Http_S"; 

    LOG_V("%s",__FUNCTION__);	

    //1. Deal with tcp problems
    if (g_http_info.http_event)
        OsalEventDelete(g_http_info.http_event);

    //2. UnRegister which broadcasts need to be processed
    bcast_unreg_receive_msg(MSG_WHAT_BG95_HTTP_INIT_SUCCESS,      g_bg95_http_msg_id);
    bcast_unreg_receive_msg(MSG_WHAT_BG95_HTTP_REQUEST,           g_bg95_http_msg_id);
    bcast_unreg_receive_msg(MSG_WHAT_BG95_HTTP_REQUEST_SUCCESS,   g_bg95_http_msg_id);
    bcast_unreg_receive_msg(MSG_WHAT_BG95_HTTP_REQUEST_FAILURE,   g_bg95_http_msg_id);

    //3. Destroy net service
    if (NULL != g_bg95_http_thread_id)
    {
        ret = os_thread_destroy(g_bg95_http_thread_id);
        if (0 != ret)
        {
            LOG_E("Delete g_bg95_http_thread_id thread failed! %d", ret); 
            return -1;			
        }
    }

    //4. Delete msg queue
    if (NULL != g_bg95_http_msg_id)
    {
        ret = OsalMsgQDelete(g_bg95_http_msg_id);
        if (0 != ret)
        {
            LOG_E("Delete g_bg95_http_msg_id msg failed! %d", ret); 
            return -1;			
        }
    }
    LOG_V("%s over",__FUNCTION__);	

    return 0;
}

int bg95_http_service_test(s32_t argc, char *argv[])
{
    int i;

    //LOG_V("%s",__FUNCTION__);	

    for (i=0; i<argc; i++)
    {
        //LOG_V("%d = %s", i, argv[i]);
    }

	if (strcmp((const char *)argv[0], "help")==0)
	{
        LOG_I("--------------------------------------------");
        LOG_I("| bg95_http:                               |");		
        LOG_I("--------------------------------------------");
        LOG_I("| exit                                     |");		
		LOG_I("| help                                     |");	
        LOG_I("| -----------------------------------------|");
	}
    else if (strcmp((const char *)argv[0], "exit")==0)
	{
        LOG_D("exit %s", __FUNCTION__);
        return -1;
	}
    else
    {
        LOG_E("Invalid parameter:%s", argv[0]);
    }

    return 0;
}
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */