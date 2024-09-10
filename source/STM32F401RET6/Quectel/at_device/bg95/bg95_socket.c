#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET__
#include <at.h>
#include "cmsis_os.h"
#include "bg95_socket.h"
#include "bg95_net.h"
#include "at_osal.h"
#include "debug_service.h"
#include "at_socket.h"
#include "broadcast_service.h"
#include "at_socket_device.h"

static osMsgQueId_t g_bg95_socket_msg_id = NULL;
static osThreadId_t g_bg95_socket_thread_id =NULL;


#define BG95_MODULE_SEND_MAX_SIZE       1460

/* set real event by current socket and current state */
#define SET_EVENT(socket, event)       (((socket + 1) << 16) | (event))

/* AT socket event type */
#define BG95_EVENT_CONN_OK             (1L << 0)
#define BG95_EVENT_SEND_OK             (1L << 1)
#define BG95_EVENT_RECV_OK             (1L << 2)
#define BG95_EVNET_CLOSE_OK            (1L << 3)
#define BG95_EVENT_CONN_FAIL           (1L << 4)
#define BG95_EVENT_SEND_FAIL           (1L << 5)
#define BG95_EVENT_DOMAIN_OK           (1L << 6)
#define BG95_EVENT_INCOMING_OK         (1L << 7)

static at_evt_cb_t at_evt_cb_set[] = {
        [AT_SOCKET_EVT_RECV] = NULL,
        [AT_SOCKET_EVT_CLOSED] = NULL,
};

static void at_tcp_ip_errcode_parse(int result)//TCP/IP_QIGETERROR
{
    LOG_V("%s, %d",__FUNCTION__, result);	

    switch(result)
    {
    case 0   : LOG_D("%d : Operation successful",         result); break;
    case 550 : LOG_E("%d : Unknown error",                result); break;
    case 551 : LOG_E("%d : Operation blocked",            result); break;
    case 552 : LOG_E("%d : Invalid parameters",           result); break;
    case 553 : LOG_E("%d : Memory not enough",            result); break;
    case 554 : LOG_E("%d : Create socket failed",         result); break;
    case 555 : LOG_E("%d : Operation not supported",      result); break;
    case 556 : LOG_E("%d : Socket bind failed",           result); break;
    case 557 : LOG_E("%d : Socket listen failed",         result); break;
    case 558 : LOG_E("%d : Socket write failed",          result); break;
    case 559 : LOG_E("%d : Socket read failed",           result); break;
    case 560 : LOG_E("%d : Socket accept failed",         result); break;
    case 561 : LOG_E("%d : Open PDP context failed",      result); break;
    case 562 : LOG_E("%d : Close PDP context failed",     result); break;
    case 563 : LOG_W("%d : Socket identity has been used", result); break;
    case 564 : LOG_E("%d : DNS busy",                     result); break;
    case 565 : LOG_E("%d : DNS parse failed",             result); break;
    case 566 : LOG_E("%d : Socket connect failed",        result); break;
    // case 567 : LOG_W("%d : Socket has been closed",       result); break;
    case 567 : break;
    case 568 : LOG_E("%d : Operation busy",               result); break;
    case 569 : LOG_E("%d : Operation timeout",            result); break;
    case 570 : LOG_E("%d : PDP context broken down",      result); break;
    case 571 : LOG_E("%d : Cancel send",                  result); break;
    case 572 : LOG_E("%d : Operation not allowed",        result); break;
    case 573 : LOG_E("%d : APN not configured",           result); break;
    case 574 : LOG_E("%d : Port busy",                    result); break;
    default  : LOG_E("%d : Unknown err code",             result); break;
    }
}

static int bg95_socket_event_send(struct at_device *device, uint32_t event)
{
    LOG_V("%s, 0x%x",__FUNCTION__, event);	
    return (int) OsalEventSend(device->socket_event, event);
}

static int bg95_socket_event_recv(struct at_device *device, uint32_t event, uint32_t timeout, rt_uint8_t option)
{
    rt_uint32_t recved;

    LOG_V("%s, event = 0x%x, timeout = %d, option = %d",__FUNCTION__, event, timeout, option);	

    recved = OsalEventRecv(device->socket_event, event, option, timeout);
    if (recved < 0)
    {
        return -RT_ETIMEOUT;
    }

    return recved;
}

/**
 * close socket by AT commands.
 *
 * @param current socket
 *
 * @return  0: close socket success
 *         -1: send AT commands error
 *         -2: wait socket event timeout
 *         -5: no memory
 */
static int bg95_socket_at_closesocket(struct at_socket *socket)
{
    int result = RT_EOK;
    at_response_t resp = RT_NULL;
    int device_socket = (int) socket->user_data;
    struct at_device *device = (struct at_device *) socket->device;

    LOG_V("%s",__FUNCTION__);	

    resp = at_create_resp(64, 0, 5 * RT_TICK_PER_SECOND);
    if (resp == RT_NULL)
    {
        LOG_E("no memory for resp create.");
        return -RT_ENOMEM;
    }

    /* default connection timeout is 10 seconds, but it set to 1 seconds is convenient to use.*/
    result = at_obj_exec_cmd(device->client, resp, "AT+QICLOSE=%d,1", device_socket);

    if (resp)
    {
        at_delete_resp(resp);
    }

    return result;
}

/**
 * create TCP/UDP client or server connect by AT commands.
 *
 * @param socket current socket
 * @param ip server or client IP address
 * @param port server or client port
 * @param type connect socket type(tcp, udp)
 * @param is_client connection is client
 *
 * @return   0: connect success
 *          -1: connect failed, send commands error or type error
 *          -2: wait socket event timeout
 *          -5: no memory
 */
static int bg95_socket_at_connect(struct at_socket *socket, char *ip, int32_t port, enum at_socket_type type, rt_bool_t is_client)
{
    uint32_t event = 0;
    rt_bool_t retryed = RT_FALSE;
    at_response_t resp = RT_NULL;
    int result = 0, event_result = 0;
    int device_socket = (int) socket->user_data;
    struct at_device *device = (struct at_device *) socket->device;

    LOG_V("%s, ip = %s, port = %d, type = %d, is_client = %d", __FUNCTION__, ip, port, type, is_client);	

    RT_ASSERT(port >= 0);

    resp = at_create_resp(128, 0, 5 * RT_TICK_PER_SECOND);
    if (resp == RT_NULL)
    {
        LOG_E("no memory for resp create.");
        return -RT_ENOMEM;
    }

__retry:
    /* clear socket connect event */
    event = SET_EVENT(device_socket, BG95_EVENT_CONN_OK | BG95_EVENT_CONN_FAIL);
    bg95_socket_event_recv(device, event, 0, RT_EVENT_FLAG_OR);

    if (is_client)
    {
        switch (type)
        {
        case AT_SOCKET_TCP:
            /* send AT commands(AT+QIOPEN=<contextID>,<socket>,"<TCP/UDP>","<IP_address>/<domain_name>", */
            /* <remote_port>,<local_port>,<access_mode>) to connect TCP server */
            /* contextID   = 1 : use same contextID as AT+QICSGP & AT+QIACT */
            /* local_port  = 0 : local port assigned automatically */
            /* access_mode = 1 : Direct push mode */
            if (at_obj_exec_cmd(device->client, resp,
                                "AT+QIOPEN=1,%d,\"TCP\",\"%s\",%d,0,1", device_socket, ip, port) < 0)
            {
                result = -RT_ERROR;
                goto __exit;
            }
            break;

        case AT_SOCKET_UDP:
            if (at_obj_exec_cmd(device->client, resp,
                                "AT+QIOPEN=1,%d,\"UDP\",\"%s\",%d,0,1", device_socket, ip, port) < 0)
            {
                result = -RT_ERROR;
                goto __exit;
            }
            break;

        default:
            LOG_E("not supported connect type : %d.", type);
            return -RT_ERROR;
        }
    }
    else /* TCP/UDP Server*/
    {
        switch (type)
        {
        case AT_SOCKET_TCP:
            /* send AT commands(AT+QIOPEN=<contextID>,<socket>,"<TCP/UDP>","<IP_address>/<domain_name>", */
            /* <remote_port>,<local_port>,<access_mode>) to connect TCP server */
            /* contextID   = 1 : use same contextID as AT+QICSGP & AT+QIACT */
            /* local_port  = 0 : local port assigned automatically */
            /* access_mode = 1 : Direct push mode */
            if (at_obj_exec_cmd(device->client, resp,
                                "AT+QIOPEN=1,%d,\"TCP LISTENER\",\"127.0.0.1\",0,%d,1", device_socket, port) < 0)
            {
                result = -RT_ERROR;
                goto __exit;
            }
            break;

        case AT_SOCKET_UDP:
            if (at_obj_exec_cmd(device->client, resp,
                                "AT+QIOPEN=1,%d,\"UDP SERVICE\",\"127.0.0.1\",0,%d,1", device_socket, port) < 0)
            {
                result = -RT_ERROR;
                goto __exit;
            }
            break;

        default:
            LOG_E("not supported connect type : %d.", type);
            return -RT_ERROR;
        }

    }
    /* waiting result event from AT URC, the device default connection timeout is 75 seconds, but it set to 10 seconds is convenient to use.*/
    if (bg95_socket_event_recv(device, SET_EVENT(device_socket, 0), 10 * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR) < 0)
    {
        LOG_E("device socket(%d) wait connect result timeout.", device_socket);
        result = -RT_ETIMEOUT;
        goto __exit;
    }

    /* waiting OK or failed result */
    event_result = bg95_socket_event_recv(device,
        BG95_EVENT_CONN_OK | BG95_EVENT_CONN_FAIL, 1 * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR);
    if (event_result < 0)
    {
        LOG_E("device socket(%d) wait connect OK|FAIL timeout.", device_socket);
        result = -RT_ETIMEOUT;
        goto __exit;
    }

    /* check result */
    if (event_result & BG95_EVENT_CONN_FAIL)
    {
        if (retryed == RT_FALSE)
        {
            LOG_D("device socket(%d) connect failed, the socket was not be closed and now will connect retey.",
                    device_socket);
            /* default connection timeout is 10 seconds, but it set to 1 seconds is convenient to use.*/
            if (bg95_socket_at_closesocket(socket) < 0)
            {
                result = -RT_ERROR;
                goto __exit;
            }
            retryed = RT_TRUE;
            goto __retry;
        }
        LOG_E("device socket(%d) connect failed.", device_socket);
        result = -RT_ERROR;
        goto __exit;
    }

__exit:
    if (resp)
    {
        at_delete_resp(resp);
    }
    LOG_V("%s over", __FUNCTION__);	
    
    return result;
}

// static int bg95_socket_at_socket(struct at_device *device, enum at_socket_type type)
// {

//     LOG_V("%s at_socket_type = %d",__FUNCTION__, type);	
// }

static int at_get_send_size(struct at_socket *socket, size_t *size, size_t *acked, size_t *nacked)
{
    int result = 0;
    at_response_t resp = RT_NULL;
    int device_socket = (int) socket->user_data;
    struct at_device *device = (struct at_device *) socket->device;

    LOG_V("%s",__FUNCTION__);	

    resp = at_create_resp(128, 0, 5 * RT_TICK_PER_SECOND);
    if (resp == RT_NULL)
    {
        LOG_E("no memory for resp create.");
        result = -RT_ENOMEM;
        goto __exit;
    }

    if (at_obj_exec_cmd(device->client, resp, "AT+QISEND=%d,0", device_socket) < 0)
    {
        result = -RT_ERROR;
        goto __exit;
    }

    if (at_resp_parse_line_args_by_kw(resp, "+QISEND:", "+QISEND: %d,%d,%d", size, acked, nacked) <= 0)
    {
        result = -RT_ERROR;
        goto __exit;
    }

__exit:
    if (resp)
    {
        at_delete_resp(resp);
    }
    LOG_V("%s over", __FUNCTION__);	

    return result;
}

static int at_wait_send_finish(struct at_socket *socket, size_t settings_size)
{
    /* get the timeout by the input data size */
    rt_tick_t timeout = rt_tick_from_millisecond(settings_size);
    rt_tick_t last_time = rt_tick_get();
    size_t size = 0, acked = 0, nacked = 0xFFFF;

    LOG_V("%s, settings_size = %d",__FUNCTION__, settings_size);	

    while (rt_tick_get() - last_time <= timeout)
    {
        at_get_send_size(socket, &size, &acked, &nacked);
        if (nacked == 0)
        {
            return RT_EOK;
        }
        rt_thread_mdelay(50);
    }

    return -RT_ETIMEOUT;
}

/**
 * send data to server or client by AT commands.
 *
 * @param socket current socket
 * @param buff send buffer
 * @param bfsz send buffer size
 * @param type connect socket type(tcp, udp)
 *
 * @return >=0: the size of send success
 *          -1: send AT commands error or send data error
 *          -2: waited socket event timeout
 *          -5: no memory
 */
static int bg95_socket_at_send(struct at_socket *socket, const char *buff, size_t bfsz, char *ip, int32_t port, enum at_socket_type type, rt_bool_t is_client)
{
    uint32_t event = 0;
    int result = 0, event_result = 0;
    size_t cur_pkt_size = 0, sent_size = 0;
    at_response_t resp = RT_NULL;
    int device_socket = (int) socket->user_data;
    struct at_device *device = (struct at_device *) socket->device;
    struct at_device_data *bg95 = (struct at_device_data *) device->user_data;
    static rt_mutex_t lock = RT_NULL;//device->client->lock;

    LOG_V("%s, buff = %s, bfsz = %d, type = %d", __FUNCTION__, buff, bfsz, type);	

    RT_ASSERT(buff);

    resp = at_create_resp(128, 2, 5 * RT_TICK_PER_SECOND);
    if (resp == RT_NULL)
    {
        LOG_E("no memory for resp create.");
        return -RT_ENOMEM;
    }

    if (lock == RT_NULL)
    {
        /* create TCP send lock */
        lock = rt_mutex_create("tcplock", RT_IPC_FLAG_FIFO);
        if (lock == RT_NULL)
        {
            LOG_E("No memory for socket allocation lock!");
            return RT_NULL;
        }
    }
    rt_mutex_take(lock, RT_WAITING_FOREVER);//FreeRtos fifo is not supported

    /* set current socket for send URC event */
    bg95->user_data = (void *) device_socket;

    /* clear socket send event */
    event = SET_EVENT(device_socket, BG95_EVENT_SEND_OK | BG95_EVENT_SEND_FAIL);
    bg95_socket_event_recv(device, event, 0, RT_EVENT_FLAG_OR);

    /* set AT client end sign to deal with '>' sign.*/
    at_obj_set_end_sign(device->client, '>');

    while (sent_size < bfsz)
    {
        if (bfsz - sent_size < BG95_MODULE_SEND_MAX_SIZE)
        {
            cur_pkt_size = bfsz - sent_size;
        }
        else
        {
            cur_pkt_size = BG95_MODULE_SEND_MAX_SIZE;
        }

        /* send the "AT+QISEND" commands to AT server than receive the '>' response on the first line. */
        if ((type == AT_SOCKET_UDP) && !is_client)
        {
            RT_ASSERT(ip);
            if (at_obj_exec_cmd(device->client, resp, "AT+QISEND=%d,%d,%s,%d", device_socket, cur_pkt_size, ip, port) < 0)
            {
                result = -RT_ERROR;
                goto __exit;
            }
        }
        else
        {
            if (at_obj_exec_cmd(device->client, resp, "AT+QISEND=%d,%d", device_socket, cur_pkt_size) < 0)
            {
                result = -RT_ERROR;
                goto __exit;
            }
        }

        /* send the real data to server or client */
        result = (int) at_client_send(buff + sent_size, cur_pkt_size);
        if (result == 0)
        {
            result = -RT_ERROR;
            goto __exit;
        }

        /* waiting result event from AT URC */
        if (bg95_socket_event_recv(device, SET_EVENT(device_socket, 0), 10 * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR) < 0)
        {
            result = -RT_ETIMEOUT;
            goto __exit;
        }
        /* waiting OK or failed result */
        event_result = bg95_socket_event_recv(device,
            BG95_EVENT_SEND_OK | BG95_EVENT_SEND_FAIL, 1 * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR);
        if (event_result < 0)
        {
            LOG_E("device socket(%d) wait sned OK|FAIL timeout.", device_socket);
            result = -RT_ETIMEOUT;
            goto __exit;
        }
        /* check result */
        if (event_result & BG95_EVENT_SEND_FAIL)
        {
            LOG_E("device socket(%d) send failed.", device_socket);
            result = -RT_ERROR;
            goto __exit;
        }

        if (type == AT_SOCKET_TCP)
        {
            // at_wait_send_finish(socket, cur_pkt_size);
            rt_thread_mdelay(10);
        }

        sent_size += cur_pkt_size;
    }

__exit:
    /* reset the end sign for data conflict */
    at_obj_set_end_sign(device->client, 0);

    rt_mutex_release(lock); 

    if (resp)
    {
        at_delete_resp(resp);
    }

    LOG_V("%s over", __FUNCTION__);	
    
    return result > 0 ? sent_size : result;
}

/**
 * domain resolve by AT commands.
 *
 * @param name domain name
 * @param ip parsed IP address, it's length must be 16
 *
 * @return  0: domain resolve success
 *         -1: send AT commands error or response error
 *         -2: wait socket event timeout
 *         -5: no memory
 */
static int bg95_socket_at_domain_resolve(const char *name, char ip[16])
{
#define RESOLVE_RETRY                  3

    int i, result;
    at_response_t resp = RT_NULL;
    struct at_device *device = RT_NULL;

    LOG_V("%s, name = %s", __FUNCTION__, name);	

    RT_ASSERT(name);
    RT_ASSERT(ip);

    device = at_device_get();
    if (device == RT_NULL)
    {
        LOG_E("get first init device failed.");
        return -RT_ERROR;
    }

    /* the maximum response time is 60 seconds, but it set to 10 seconds is convenient to use. */
    resp = at_create_resp(128, 0, 10 * RT_TICK_PER_SECOND);
    if (!resp)
    {
        LOG_E("no memory for resp create.");
        return -RT_ENOMEM;
    }

    /* clear BG95_EVENT_DOMAIN_OK */
    bg95_socket_event_recv(device, BG95_EVENT_DOMAIN_OK, 0, RT_EVENT_FLAG_OR);

    result = at_obj_exec_cmd(device->client, resp, "AT+QIDNSGIP=1,\"%s\"", name);
    if (result < 0)
    {
        goto __exit;
    }

    if (result == RT_EOK)
    {
        for(i = 0; i < RESOLVE_RETRY; i++)
        {
            /* waiting result event from AT URC, the device default connection timeout is 60 seconds.*/
            if (bg95_socket_event_recv(device, BG95_EVENT_DOMAIN_OK, 10 * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR) < 0)
            {
                continue;
            }
            else
            {
                struct at_device_data *bg95 = (struct at_device_data *) device->user_data;
                char *recv_ip = (char *) bg95->socket_data;

                if (rt_strlen(recv_ip) < 8)
                {
                    rt_thread_mdelay(100);
                    /* resolve failed, maybe receive an URC CRLF */
                    result = -RT_ERROR;
                    continue;
                }
                else
                {
                    rt_strncpy(ip, recv_ip, 15);
                    ip[15] = '\0';
                    result = RT_EOK;
                    break;
                }
            }
        }

        /* response timeout */
        if (i == RESOLVE_RETRY)
        {
            result = -RT_ENOMEM;
        }
    }

 __exit:
    if (resp)
    {
        at_delete_resp(resp);
    }

    return result;

}

/**
 * set AT socket event notice callback
 *
 * @param event notice event
 * @param cb notice callback
 */
static void bg95_socket_at_set_event_cb(at_socket_evt_t event, at_evt_cb_t cb)
{
    LOG_V("%s, event = %d",__FUNCTION__, event);	

    if (event < sizeof(at_evt_cb_set) / sizeof(at_evt_cb_set[1]))
    {
        at_evt_cb_set[event] = cb;
    }
}

static int bg95_get_socket_num(struct at_device *device, int device_socket)
{
    int i;
    struct at_socket *socket = RT_NULL;

    LOG_V("%s",__FUNCTION__);	

    RT_ASSERT(device);

    for (i=0; i<device->socket_num; i++)
    {
        if ((int)device->sockets[i].user_data == device_socket)
            break;
    }

    RT_ASSERT(i!=device->socket_num);

    return i;
}

static void urc_connect_func(struct at_client *client, const char *data, rt_size_t size)
{
    int device_socket = 0, result = 0;
    struct at_device *device = RT_NULL;

    RT_ASSERT(data && size);

    LOG_V("%s, size = %d",__FUNCTION__, size);	

    device = at_device_get();
    if (device == RT_NULL)
    {
        LOG_E("get device failed.");
        return;
    }

    sscanf(data, "+QIOPEN: %d,%d", &device_socket , &result);

    if (result == 0)
    {
        bg95_socket_event_send(device, SET_EVENT(device_socket, BG95_EVENT_CONN_OK));
    }
    else
    {
        at_tcp_ip_errcode_parse(result);
        bg95_socket_event_send(device, SET_EVENT(device_socket, BG95_EVENT_CONN_FAIL));
    }
}

static void urc_send_func(struct at_client *client, const char *data, rt_size_t size)
{
    int device_socket = 0;
    struct at_device *device = RT_NULL;
    struct at_device_data *bg95 = RT_NULL;

    LOG_V("%s, size = %d",__FUNCTION__, size);	

    RT_ASSERT(data && size);

    device = at_device_get();
    if (device == RT_NULL)
    {
        LOG_E("get device failed.");
        return;
    }
    bg95 = (struct at_device_data *) device->user_data;
    device_socket = (int) bg95->user_data;

    if (rt_strstr(data, "SEND OK"))
    {
        bg95_socket_event_send(device, SET_EVENT(device_socket, BG95_EVENT_SEND_OK));
    }
    else if (rt_strstr(data, "SEND FAIL"))
    {
        bg95_socket_event_send(device, SET_EVENT(device_socket, BG95_EVENT_SEND_FAIL));
    }
}

static void urc_close_func(struct at_client *client, const char *data, rt_size_t size)
{
    int device_socket = 0;
    struct at_socket *socket = RT_NULL;
    struct at_device *device = RT_NULL;

    LOG_V("%s, size = %d",__FUNCTION__, size);	

    RT_ASSERT(data && size);

    device = at_device_get();
    if (device == RT_NULL)
    {
        LOG_E("get device failed.");
        return;
    }

    sscanf(data, "+QIURC: \"closed\",%d", &device_socket);
    /* get at socket object by device socket descriptor */
    socket = &(device->sockets[bg95_get_socket_num(device, device_socket)]);
    /* notice the socket is disconnect by remote */
    if (at_evt_cb_set[AT_SOCKET_EVT_CLOSED])
    {
        at_evt_cb_set[AT_SOCKET_EVT_CLOSED](socket, AT_SOCKET_EVT_CLOSED, NULL, 0, 0);
    }
}

static void urc_recv_func(struct at_client *client, const char *data, rt_size_t size)
{
    int device_socket = 0, i = 0, j = 0;
    rt_int32_t timeout;
    rt_size_t bfsz = 0, temp_size = 0;
    char *recv_buf = RT_NULL, temp[8] = {0};
    struct at_socket *socket = RT_NULL;
    struct at_device *device = RT_NULL;
    char recv_ip[16] = {0}, *p = RT_NULL;
    at_socket_addr at_socket_info;  

    LOG_V("%s, size = %d",__FUNCTION__, size);	

    RT_ASSERT(data && size);

    device = at_device_get();
    if (device == RT_NULL)
    {
        LOG_E("get device failed.");
        return;
    }

    for (i = 0; i < size; i++)
    {
        if (*(data + i) == '.')
            j++;
    }

    if (j == 3)
    {
        /* get the current socket and receive buffer size by receive data */
        sscanf(data, "+QIURC: \"recv\",%d,%d,\"%[^\"]\"", &device_socket, (int *) &bfsz, recv_ip);
        recv_ip[15] = '\0';
        p = strrchr(data, ',');
        p++;
        at_socket_info.addr.port = atoi(p);
        at_socket_info.addr.sin_addr = inet_addr(recv_ip); //ip
        LOG_V("incoming:%d, %d, %s, %d", device_socket, bfsz, recv_ip, at_socket_info.addr.port);
    }
    else
    {
        /* get the current socket and receive buffer size by receive data */
        sscanf(data, "+QIURC: \"recv\",%d,%d", &device_socket, (int *) &bfsz);
    }

    /* set receive timeout by receive buffer length, not less than 10 ms */
    timeout = bfsz > 10 ? bfsz : 10;

    if (device_socket < 0 || bfsz == 0)
    {
        return;
    }

    recv_buf = (char *) rt_calloc(1, bfsz);

    if (recv_buf == RT_NULL)
    {
        LOG_E("no memory for URC receive buffer(%d).", bfsz);
        /* read and clean the coming data */
        while (temp_size < bfsz)
        {
            if (bfsz - temp_size > sizeof(temp))
            {
                at_client_obj_recv(client, temp, sizeof(temp), timeout);
            }
            else
            {
                at_client_obj_recv(client, temp, bfsz - temp_size, timeout);
            }
            temp_size += sizeof(temp);
        }
        return;
    }

    /* sync receive data */
    if (at_client_obj_recv(client, recv_buf, bfsz, timeout) != bfsz)
    {
        LOG_E("device receive size(%d) data failed.", bfsz);
        rt_free(recv_buf);
        return;
    }
    
    /* get at socket object by device socket descriptor */
    socket = &(device->sockets[bg95_get_socket_num(device, device_socket)]);
    /* notice the receive buffer and buffer size */
    if (at_evt_cb_set[AT_SOCKET_EVT_RECV])
    {
        at_evt_cb_set[AT_SOCKET_EVT_RECV](socket, AT_SOCKET_EVT_RECV, recv_buf, bfsz, at_socket_info.data);
    }
}

static void urc_pdpdeact_func(struct at_client *client, const char *data, rt_size_t size)
{
    int connectID = 0;

    LOG_V("%s, size = %d",__FUNCTION__, size);	

    RT_ASSERT(data && size);

    sscanf(data, "+QIURC: \"pdpdeact\",%d", &connectID);

    LOG_E("context (%d) is deactivated.", connectID);
}

static void urc_incoming_func(struct at_client *client, const char *data, rt_size_t size)
{
    int i = 0, j = 0;
    char recv_ip[16] = {0},*p =NULL; 
    int result, ip_count, dns_ttl;
    struct at_device *device = RT_NULL;
    struct at_device_data *bg95 = RT_NULL;
    struct at_socket *socket = RT_NULL;
    struct at_incoming_info *incoming_info = NULL;

    LOG_V("%s, size = %d",__FUNCTION__, size);	

    RT_ASSERT(data && size);

    device = at_device_get();
    if (device == RT_NULL)
    {
        LOG_E("get device failed.");
        return;
    }
    bg95 = (struct at_device_data *) device->user_data;

    for (i = 0; i < size; i++)
    {
        if (*(data + i) == '.')
            j++;
    }

    if (j == 3)
    {
        incoming_info = (char *) rt_calloc(1, sizeof(struct at_incoming_info));
        if (incoming_info == RT_NULL)
        {
            LOG_E("no memory for incoming URC receive buffer(%d).", sizeof(struct at_incoming_info));
            return;
        }                                       
        //sscanf(data, "+QIURC: \"incoming\",%d,%d,\"%[^\"]\",%d", &incoming_info->socket, &incoming_info->device_socket, incoming_info->ip, &incoming_info->port);
        sscanf(data, "+QIURC: \"incoming\",%d,%d,\"%[^\"]\"", &incoming_info->socket, &incoming_info->device_socket, recv_ip);
        recv_ip[15] = '\0';
        p = strrchr(data, ',');
        p++;
        incoming_info->socket_addr.addr.port = atoi(p);
        incoming_info->socket_addr.addr.sin_addr = inet_addr(recv_ip);

        if (incoming_info->socket < 0 || incoming_info->device_socket < 0)
        {
            return;
        }

        /* get at socket object by device socket descriptor */
        socket = &(device->sockets[bg95_get_socket_num(device, incoming_info->device_socket)]);
        LOG_V("socket = %d, device_socket = %d, ip = %s, port = %d", incoming_info->socket, incoming_info->device_socket, recv_ip, incoming_info->socket_addr.addr.port);	

        /* notice the receive buffer and buffer size */
        if (at_evt_cb_set[AT_SOCKET_EVT_RECV])
        {
            at_evt_cb_set[AT_SOCKET_EVT_RECV](socket, AT_SOCKET_EVT_RECV, incoming_info, sizeof(struct at_incoming_info), 0);
        }
    }
}

static void urc_dnsqip_func(struct at_client *client, const char *data, rt_size_t size)
{
    int i = 0, j = 0;
    char recv_ip[16] = {0};
    int result, ip_count, dns_ttl;
    struct at_device *device = RT_NULL;
    struct at_device_data *bg95 = RT_NULL;

    LOG_V("%s, size = %d",__FUNCTION__, size);	

    RT_ASSERT(data && size);

    device = at_device_get();
    if (device == RT_NULL)
    {
        LOG_E("get device failed.");
        return;
    }
    bg95 = (struct at_device_data *) device->user_data;

    for (i = 0; i < size; i++)
    {
        if (*(data + i) == '.')
            j++;
    }
    /* There would be several dns result, we just pickup one */
    if (j == 3)
    {
        sscanf(data, "+QIURC: \"dnsgip\",\"%[^\"]", recv_ip);
        recv_ip[15] = '\0';

        /* set bg95 information socket data */
        if (bg95->socket_data == RT_NULL)
        {
            bg95->socket_data = rt_calloc(1, sizeof(recv_ip));
            if (bg95->socket_data == RT_NULL)
            {
                return;
            }
        }
        rt_memcpy(bg95->socket_data, recv_ip, sizeof(recv_ip));


        bg95_socket_event_send(device, BG95_EVENT_DOMAIN_OK);
    }
    else
    {
        sscanf(data, "+QIURC: \"dnsgip\",%d,%d,%d", &result, &ip_count, &dns_ttl);
        if (result)
        {
            at_tcp_ip_errcode_parse(result);
        }
    }
}

static void urc_func(struct at_client *client, const char *data, rt_size_t size)
{
    RT_ASSERT(data);

    LOG_I("URC data : %.*s", size, data);
}

static void urc_qiurc_func(struct at_client *client, const char *data, rt_size_t size)
{
    RT_ASSERT(data && size);

    switch(*(data + 9))
    {
        case 'c' : urc_close_func(client, data, size);    break;//+QIURC: "closed"
        case 'r' : urc_recv_func(client, data, size);     break;//+QIURC: "recv"
        case 'p' : urc_pdpdeact_func(client, data, size); break;//+QIURC: "pdpdeact"
        case 'd' : urc_dnsqip_func(client, data, size);   break;//+QIURC: "dnsgip"
        case 'i' : urc_incoming_func(client, data, size); break;//+QIURC: "incoming"
        default  : urc_func(client, data, size);          break;
    }
}

static const struct at_urc urc_table[] =
{
    {"SEND OK",     "\r\n",                 urc_send_func},
    {"SEND FAIL",   "\r\n",                 urc_send_func},
    {"+QIOPEN:",    "\r\n",                 urc_connect_func},
    {"+QIURC:",     "\r\n",                 urc_qiurc_func},
};

/*****************************************************************************************/

static void bg95_socket_service_proc(void *argument)
{
    int ret;
    msg_node msgs;

	LOG_V("%s, stack space %d",__FUNCTION__, os_thread_get_stack_space(os_thread_get_thread_id()));	

    /* A service that provides network socket interface, and keep the services provided effective */
    while(1)
    {
        ret = OsalMsgRcv(g_bg95_socket_msg_id, (void *)&msgs, NULL, portMAX_DELAY);
		if (ret < 0)
		{
			LOG_E("Receive msg from broadcast thread error!");
			continue;
		}
        else
        {
            LOG_D("Receive broadcast msg is what=0x%x, arg1=0x%x, arg2=0x%x, arg3=0x%x", msgs.what, msgs.arg1, msgs.arg2, msgs.arg3);
            switch (msgs.what)
            {
                case MSG_WHAT_BG95_SOCKET_CONNECT_SUCCESS:
                    LOG_D("Do something MSG_WHAT_BG95_SOCKET_CONNECT_SUCCESS");
                    break;

                case MSG_WHAT_BG95_SOCKET_CONNECT_FAILURE:
                    LOG_D("Do something MSG_WHAT_BG95_SOCKET_CONNECT_FAILURE");
                    break;

                case MSG_WHAT_BG95_SOCKET_SEND_DATA_SUCCESS:
                    LOG_D("Do something MSG_WHAT_BG95_SOCKET_SEND_DATA_SUCCESS");
                    break;

                case MSG_WHAT_BG95_SOCKET_SEND_DATA_FAILURE:
                    LOG_D("Do something MSG_WHAT_BG95_SOCKET_SEND_DATA_FAILURE");
                    break;

                case MSG_WHAT_BG95_SOCKET_RECV_DATA_SUCCESS:
                    LOG_D("Do something MSG_WHAT_BG95_SOCKET_RECV_DATA_SUCCESS");
                    break;

                case MSG_WHAT_BG95_SOCKET_RECV_DATA_FAILURE:
                    LOG_D("Do something MSG_WHAT_BG95_SOCKET_RECV_DATA_FAILURE");
                    break;
                
                case MSG_WHAT_BG95_NET_DATACALL_SUCCESS:
                    {
                        struct at_socket_ops bg95_at_socket_ops;
                        at_client_t client = at_client_get_first();
                        ip_addr_t module_addr;
                        #ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__
                        module_addr = QL_bg95_net_get_ip();
                        #endif
 
                        memset(&bg95_at_socket_ops, 0, sizeof(struct at_socket_ops));

                        bg95_at_socket_ops.at_connect        = bg95_socket_at_connect;
                        bg95_at_socket_ops.at_closesocket    = bg95_socket_at_closesocket;
                        bg95_at_socket_ops.at_send           = bg95_socket_at_send;
                        bg95_at_socket_ops.at_domain_resolve = bg95_socket_at_domain_resolve;
                        bg95_at_socket_ops.at_set_event_cb   = bg95_socket_at_set_event_cb;
                        // bg95_at_socket_ops.at_socket         = bg95_socket_at_socket;
                        at_device_socket_register(12, &bg95_at_socket_ops, client, &module_addr);
                        LOG_D("at_device_socket_register success");
                        bcast_send_bcast_msg(MSG_WHAT_BG95_SOCKET_INIT_SUCCESS, 0, 0, 0);  //Send tcp init successful broadcast
                    }
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

int bg95_socket_service_create(void)
{
    int ret;
    static osThreadAttr_t thread_attr = {.name = "Tcp_S", .stack_size = 512*10, .priority = osPriorityNormal};

    LOG_V("%s",__FUNCTION__);	

    //1. Creat msg queue
   	g_bg95_socket_msg_id = OsalMsgQCreate(MAX_MSG_COUNT, sizeof(msg_node), NULL);
	if (NULL == g_bg95_socket_msg_id)
	{
		LOG_E("Create bg95_socket_service_create msg failed!"); 
		return -1;			
	}

    //2. Register which broadcasts need to be processed
    bcast_reg_receive_msg(MSG_WHAT_BG95_SOCKET_CONNECT_SUCCESS,   g_bg95_socket_msg_id);
    bcast_reg_receive_msg(MSG_WHAT_BG95_SOCKET_CONNECT_FAILURE,   g_bg95_socket_msg_id);
    bcast_reg_receive_msg(MSG_WHAT_BG95_SOCKET_SEND_DATA_SUCCESS, g_bg95_socket_msg_id);
    bcast_reg_receive_msg(MSG_WHAT_BG95_SOCKET_SEND_DATA_FAILURE, g_bg95_socket_msg_id);
    bcast_reg_receive_msg(MSG_WHAT_BG95_SOCKET_RECV_DATA_SUCCESS, g_bg95_socket_msg_id);
    bcast_reg_receive_msg(MSG_WHAT_BG95_SOCKET_RECV_DATA_FAILURE, g_bg95_socket_msg_id);
    bcast_reg_receive_msg(MSG_WHAT_BG95_NET_DATACALL_SUCCESS,  g_bg95_socket_msg_id);

    //3. Deal with tcp problems
    /* register URC data execution function  */
    at_set_urc_table(urc_table, sizeof(urc_table) / sizeof(urc_table[0]));

    //4. Create net service
    g_bg95_socket_thread_id = os_thread_create(bg95_socket_service_proc, NULL, &thread_attr);
	if (NULL == g_bg95_socket_thread_id)
	{
		LOG_E ("bg95_socket_service_create thread could not start!");
		return -1;
	}

    LOG_I("%s over(%x)",__FUNCTION__, g_bg95_socket_thread_id);	

    return 0;
}

int bg95_socket_service_destroy(void)
{
    int ret;
    osThreadId_t thread;
    char name[RT_NAME_MAX] = "Tcp_S"; 

    LOG_V("%s",__FUNCTION__);	

    //1. Deal with tcp problems

    //2. UnRegister which broadcasts need to be processed
    bcast_unreg_receive_msg(MSG_WHAT_BG95_SOCKET_CONNECT_SUCCESS,   g_bg95_socket_msg_id);
    bcast_unreg_receive_msg(MSG_WHAT_BG95_SOCKET_CONNECT_FAILURE,   g_bg95_socket_msg_id);
    bcast_unreg_receive_msg(MSG_WHAT_BG95_SOCKET_SEND_DATA_SUCCESS, g_bg95_socket_msg_id);
    bcast_unreg_receive_msg(MSG_WHAT_BG95_SOCKET_SEND_DATA_FAILURE, g_bg95_socket_msg_id);
    bcast_unreg_receive_msg(MSG_WHAT_BG95_SOCKET_RECV_DATA_SUCCESS, g_bg95_socket_msg_id);
    bcast_unreg_receive_msg(MSG_WHAT_BG95_SOCKET_RECV_DATA_FAILURE, g_bg95_socket_msg_id);
    bcast_unreg_receive_msg(MSG_WHAT_BG95_NET_DATACALL_SUCCESS,  g_bg95_socket_msg_id);

    //3. Destroy net service
    if (NULL != g_bg95_socket_thread_id)
    {
        ret = os_thread_destroy(g_bg95_socket_thread_id);
        if (0 != ret)
        {
            LOG_E("Delete g_bg95_socket_thread_id thread failed! %d", ret); 
            return -1;			
        }
    }

    //4. Delete msg queue
    if (NULL != g_bg95_socket_msg_id)
    {
        ret = OsalMsgQDelete(g_bg95_socket_msg_id);
        if (0 != ret)
        {
            LOG_E("Delete g_bg95_socket_msg_id msg failed! %d", ret); 
            return -1;			
        }
    }
    LOG_V("%s over",__FUNCTION__);	

    return 0;
}

int bg95_socket_service_test(s32_t argc, char *argv[])
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
        LOG_I("| bg95_socket:                             |");		
        LOG_I("--------------------------------------------");
        LOG_I("| exit                                     |");		
		LOG_I("| help                                     |");						
		LOG_I("| create                                   |");		
        LOG_I("| destroy                                  |");	
        LOG_I("| send_msg 'what' 'arg1' 'arg2' 'arg3'     |");		
        LOG_I("--------------------------------------------");
	}
    else if (strcmp((const char *)argv[0], "exit")==0)
	{
        LOG_D("exit %s", __FUNCTION__);
        return -1;
	}
	else if (strcmp((const char *)argv[0], "create")==0)
	{
        bg95_socket_service_create();
	}
	else if (strcmp((const char *)argv[0], "destroy")==0)
	{
        bg95_socket_service_destroy();
	}
    else if (strcmp((const char *)argv[0], "send_msg")==0)
	{
		LOG_I("send_msg 0x%x 0x%x 0x%x 0x%x", strtol(argv[1], NULL, 16), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
        bcast_send_my_msg(g_bg95_socket_msg_id, strtol(argv[1], NULL, 16), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
	}
    else
    {
        LOG_E("Invalid parameter:%s", argv[0]);
    }

    return 0;
}
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */