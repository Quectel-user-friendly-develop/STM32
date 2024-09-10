#ifndef __BROADCAST_SERVICE_H__
#define __BROADCAST_SERVICE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

/*------------------------------------------include-------------------------------------------------------*/
#include "at_osal.h"
/*------------------------------------------Macro--------------------------------------------------------*/
#define MAX_MSG_COUNT    	  (16)
#define MAX_BROADCAST_RECEIVE (32)

#define MSG_WHAT_INVALID                         0x0000


typedef enum
{
	MSG_WHAT_MAIN_STATE                   = 0x1000,
	
	MSG_WHAT_BG95_MODULE_INIT             = 0x2000,
	MSG_WHAT_BG95_MODULE_FAILURE,
	MSG_WHAT_BG95_SIM_START,
	MSG_WHAT_BG95_SIM_READY_FAILURE,
	MSG_WHAT_BG95_NW_START,
	MSG_WHAT_BG95_NET_NET_REG_SUCCESS,
	MSG_WHAT_BG95_NET_NET_REG_FAILURE,
	MSG_WHAT_BG95_DATACALL_START,
	MSG_WHAT_BG95_NET_DATACALL_SUCCESS,
	MSG_WHAT_BG95_NET_DATACALL_FAILURE,
	MSG_WHAT_BG95_NET_KEEPALIVE,
	
	MSG_WHAT_BG95_SOCKET_INIT_SUCCESS        = 0x3000,
	MSG_WHAT_BG95_SOCKET_CONNECT_SUCCESS,
	MSG_WHAT_BG95_SOCKET_CONNECT_FAILURE,
	MSG_WHAT_BG95_SOCKET_SEND_DATA_SUCCESS,
	MSG_WHAT_BG95_SOCKET_SEND_DATA_FAILURE,
	MSG_WHAT_BG95_SOCKET_RECV_DATA_SUCCESS,
	MSG_WHAT_BG95_SOCKET_RECV_DATA_FAILURE,

	MSG_WHAT_BG95_FTP_INIT_SUCCESS = 0x4000,
	MSG_WHAT_BG95_FTP_CONNENT_SUCCESS,
	MSG_WHAT_BG95_FTP_UP_SUCCESS,
	MSG_WHAT_BG95_FTP_UP_FAIL,
	MSG_WHAT_BG95_FTP_GET_SUCCESS,
	MSG_WHAT_BG95_FTP_GET_FAIL,
	MSG_WHAT_BG95_FTP_CLOSE_SUCCESS,
	
	MSG_WHAT_BG95_HTTP_INIT_SUCCESS        = 0x5000,
	MSG_WHAT_BG95_HTTP_REQUEST,
	MSG_WHAT_BG95_HTTP_REQUEST_SUCCESS,
	MSG_WHAT_BG95_HTTP_REQUEST_FAILURE,
} MSG_WHAT_STATE;

/*------------------------------------------typedef-----------------------------------------------------*/
typedef struct
{
	s32_t what;
	s32_t arg1;
	s32_t arg2;
	s32_t arg3;
} msg_node, *msg_node_t;

typedef struct 
{
	s32_t what;
	osMsgQueId_t msgqid; 	/* Msg queue id */
} bcast_reveice, *bcast_reveice_t;

/*------------------------------------------Func------------------------------------ --------------------*/
/*******************************************************
    function api
*******************************************************/
s32_t bcast_service_create(void);
s32_t bcast_service_destroy(void);

s32_t bcast_reg_receive_msg(s32_t what, osMsgQueId_t msgqid);
s32_t bcast_unreg_receive_msg (s32_t what, osMsgQueId_t msgqid);

s32_t bcast_send_msg(msg_node_t msg);
s32_t bcast_send_bcast_msg(s32_t what, s32_t arg1, s32_t arg2, s32_t arg3);
s32_t bcast_send_my_msg(osMsgQueId_t my_msg_id, s32_t what, s32_t arg1, s32_t arg2, s32_t arg3);

int bcast_service_test(s32_t argc, char *argv[]);
/*------------------------------------------end--------------------------------------------------------*/
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __BROADCAST_SERVICE_H__ */
