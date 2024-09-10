/*------------------------------------------include-------------------------------------------------------*/
#include "broadcast_service.h"
/*------------------------------------------Variable-----------------------------------------------------*/
static bcast_reveice g_bcast_reveice[MAX_BROADCAST_RECEIVE] = { {0,0} };

static osMsgQueId_t g_bcast_msg_id  = NULL;
static osThreadId_t g_bcast_thread_id =NULL;
/*------------------------------------------Function-----------------------------------------------------*/
/*-------------------------------------------Code-------------------------------------------------------*/
/******************************************************************************
* function : register receive to broascast
* return: err:FAILURE  ok:SUCCESS 
******************************************************************************/
s32_t bcast_reg_receive_msg (s32_t what, osMsgQueId_t msgqid)
{
	s32_t s32i = 0;
	s32_t bcast_rev_cnt = 0;
	
	//LOG_V ("%s",__FUNCTION__);

	/* judge the msg has been regist or not */		
	for (s32i = 0; s32i < MAX_BROADCAST_RECEIVE; s32i++)
	{
		if (NULL != g_bcast_reveice[s32i].msgqid)
		{
			bcast_rev_cnt++;
			if ((msgqid == g_bcast_reveice[s32i].msgqid) && (what == g_bcast_reveice[s32i].what))
			{
				LOG_W ("msg has been regist msgqid = 0x%x", msgqid);
				return 0;
			}
		}
	}

	if (bcast_rev_cnt >= MAX_BROADCAST_RECEIVE)
	{
		LOG_E ("broadcast receive is full");
		return -1;
	}

	//LOG_V ("msg receiver:%lld, 0x%x, receiverq index:%d", what, msgqid, bcast_rev_cnt);

	for (s32i = 0; s32i < MAX_BROADCAST_RECEIVE; s32i++)
	{
		if (NULL == g_bcast_reveice[s32i].msgqid)
		{
			g_bcast_reveice[s32i].what = what;
			g_bcast_reveice[s32i].msgqid = msgqid;
			break;
		}
	}

	return 0;
}

/******************************************************************************
* function : unregister receive to broascast
* return: err:FAILURE  ok:SUCCESS 
******************************************************************************/
s32_t bcast_unreg_receive_msg (s32_t what, osMsgQueId_t msgqid)
{
	s32_t s32i = 0;
	s32_t bcast_rev_cnt = 0;
	
	LOG_V ("%s",__FUNCTION__);

	/* judge the msg has been regist or not */		
	for (s32i = 0; s32i < MAX_BROADCAST_RECEIVE; s32i++)
	{
		if ((msgqid == g_bcast_reveice[s32i].msgqid) && (what == g_bcast_reveice[s32i].what))
		{
			g_bcast_reveice[s32i].what = MSG_WHAT_INVALID;
			g_bcast_reveice[s32i].msgqid = NULL;
		}
		else if (msgqid != NULL)
		{
			bcast_rev_cnt++;
		}
	}

	LOG_V ("msg unreceiver:%lld, 0x%x, receiverq index:%d", what, msgqid, bcast_rev_cnt);

	return 0;
}
/******************************************************************************
* function : send msg to broadcast thread
* return: err:FAILURE  ok:SUCCESS 
******************************************************************************/
s32_t bcast_send_msg(msg_node *msg)
{
	s32_t ret;
    LOG_V("%s",__FUNCTION__);	

	ret = OsalMsgSend(g_bcast_msg_id, (void *)msg, 0, 0);
	if ( ret < 0 )
	{
		LOG_E("Send msg to broadcast service failed!");
		return -1; 
	}
		
	return 0;
}

/******************************************************************************
* function : send broadcast msg
* return: err:FAILURE  ok:SUCCESS 
******************************************************************************/
s32_t bcast_send_my_msg(osMsgQueId_t my_msg_id, s32_t what, s32_t arg1, s32_t arg2, s32_t arg3)
{
	s32_t ret;
	msg_node msg;

	LOG_V("%s: my_msg_id = 0x%x, what = 0x%x, arg1 = 0x%x, arg2 = 0x%x, arg3 = 0x%x", __FUNCTION__, my_msg_id, what, arg1, arg2, arg3);	

	msg.what = what;
	msg.arg1 = arg1;
	msg.arg2 = arg2;
	msg.arg3 = arg3;
	ret = OsalMsgSend(my_msg_id, (void *)&msg, 0, 0);
	if ( ret < 0 )
	{
		LOG_E("Send msg to myself failed! %d", ret);
		return -1; 
	}
	
	return ret;
}

/******************************************************************************
* function : send broadcast msg
* return: err:FAILURE  ok:SUCCESS 
******************************************************************************/
s32_t bcast_send_bcast_msg(s32_t what, s32_t arg1, s32_t arg2, s32_t arg3)
{
	msg_node msg;
	LOG_V("%s: what = 0x%x, arg1 = 0x%x, arg2 = 0x%x, arg3 = 0x%x", __FUNCTION__, what, arg1, arg2, arg3);	

	return bcast_send_my_msg(g_bcast_msg_id, what, arg1, arg2, arg3);	
}

/******************************************************************************
* function : broadcast thread
******************************************************************************/
static void* bcast_service_thread_proc(void* pThreadParam)
{
	s32_t ret;
	s32_t i;
	s8_t bfind;
	msg_node msgs;

	LOG_V("%s, stack space %d",__FUNCTION__, os_thread_get_stack_space(os_thread_get_thread_id()));	

	while (1)
	{
		ret = OsalMsgRcv(g_bcast_msg_id, (void *)&msgs, NULL, portMAX_DELAY);
		if (ret < 0)
		{
			LOG_E("receive msg from broadcast thread error!");
			continue;
		}
		
		//LOG_D("Receive broadcast msg is what=0x%x, arg1=0x%x, arg2=0x%x, arg3=0x%x", msgs.what, msgs.arg1, msgs.arg2, msgs.arg3);

		bfind = FALSE;
		for (i = 0; i < MAX_BROADCAST_RECEIVE; i++)
		{		
			//LOG_V ("Revceiver index:0x%x, receiver msg id: 0x%x, what:%d", i, g_bcast_reveice[i].msgqid, g_bcast_reveice[i].what);

			if (g_bcast_reveice[i].what == 0)
			{
				//LOG_V ("The broadcast receive search over!");
				break;
			}
			
			if (msgs.what == g_bcast_reveice[i].what)
			{
				//LOG_V ("Find the match what %d, revqid:0x%x, len:%d", msgs.what, g_bcast_reveice[i].msgqid, sizeof(msg_node));

				bfind = TRUE;
				
				ret = OsalMsgSend(g_bcast_reveice[i].msgqid, &msgs, 0, 0);
				if (0 != ret)
				{
					LOG_E ("Can not send msg to 0x%x", g_bcast_reveice[i].msgqid);
				}
			}				
		}

		if (!bfind)
		{
			LOG_V("This broadcast msg none receiver!");
		}
	}
	LOG_V("%s over",__FUNCTION__);	
	os_thread_exit();
}

/************************************************************************
 *function: bcast_service_create
 *brief: Broadcast service init
 *param: NULL
 *return: SUCCESS or FAILURE
**************************************************************************/
s32_t bcast_service_create(void)
{
	s32_t ret;
    static osThreadAttr_t thread_attr = {.name = "Bcast_S", .stack_size = 512*3, .priority = osPriorityNormal};

	LOG_V("%s",__FUNCTION__);	
	
	//create broadcast msg queue
	g_bcast_msg_id = OsalMsgQCreate(MAX_MSG_COUNT, sizeof(msg_node), NULL);
	if (NULL == g_bcast_msg_id)
	{
		LOG_E("Create broadcast msg failed!"); 
		return -1;			
	}

	g_bcast_thread_id = os_thread_create(bcast_service_thread_proc, NULL, &thread_attr);
	if (NULL == g_bcast_thread_id)
	{
		LOG_E ("Broadcast thread could not start!");
		return -1;
	}
	LOG_I("%s over(%x)",__FUNCTION__, g_bcast_thread_id);	

	return 0;
}

s32_t bcast_service_destroy(void)
{
	int ret = 0;

    LOG_V("%s",__FUNCTION__);	

	//1. Destroy net service
    if (NULL != g_bcast_thread_id)
    {
        ret = os_thread_destroy(g_bcast_thread_id);
        if (0 != ret)
        {
            LOG_E("Delete g_bcast_thread_id thread failed! %d", ret); 
            return -1;			
        }
    }

    //2. Delete msg queue
    if (NULL != g_bcast_thread_id)
    {
        ret = OsalMsgQDelete(g_bcast_thread_id);
        if (0 != ret)
        {
            LOG_E("Delete g_bcast_thread_id msg failed! %d", ret); 
            return -1;			
        }
    }
	LOG_V("%s over",__FUNCTION__);	
	
    return 0;
}

int bcast_service_test(s32_t argc, char *argv[])
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
		LOG_I("| bcast:                                   |");		
		LOG_I("--------------------------------------------");
        LOG_I("| exit                                     |");		
		LOG_I("| help                                     |");						
		LOG_I("| send 'what' 'arg1' 'arg2' 'arg3'         |");	
		LOG_I("--------------------------------------------");
	}
	else if (strcmp((const char *)argv[0], "exit")==0)
	{
        LOG_D("exit %s", __FUNCTION__);
        return -1;
	}
	else if (strcmp((const char *)argv[0], "send")==0)
	{
		LOG_I("send 0x%x 0x%x 0x%x 0x%x", strtol(argv[1], NULL, 16), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
		bcast_send_bcast_msg(strtol(argv[1], NULL, 16), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
	}
    else
    {
        LOG_E("Invalid parameter:%s", argv[0]);
    }
	LOG_V("%s over",__FUNCTION__);	

    return 0;
}

