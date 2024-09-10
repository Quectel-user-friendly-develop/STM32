/****************************************************************************
*
* Copy right: 2020-, Copyrigths of Quectel Ltd.
****************************************************************************
* File name: dbg_log.c
* History: Rev1.0 2023-08-19
****************************************************************************/

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "debug_service.h"
#include "broadcast_service.h"
#include "ringbuffer.h"
#include "ff.h"
#include "at.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S__
#include "bg95_ftp.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_EXAMPLE_MAIN__
#include "user_main.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_EXAMPLE_MAIN__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S__
#include "bg95_http.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__
#include "bg95_net.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET__
#include "bg95_socket.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET__ */



#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SHELL__
static osSemaphoreId_t g_debug_input_sem_id = NULL;
static osThreadId_t g_serial_input_parse_thread_id = NULL;
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SHELL__*/

#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SAVE__
static osSemaphoreId_t g_debug_service_sem_id = NULL;
static osThreadId_t g_debug_service_thread_id = NULL;
static struct ringbuffer g_log_rb = {.buffer = NULL};
static uint8_t *g_log_rb_buf = RT_NULL;  
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SAVE__ */

#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_PRINT__
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_PRINT__ */

debug_level g_debug_level = LOG_VERBOSE;
static int32_t g_save_debug_flag = 0;
static int32_t debug_service_test(s32_t argc, char *argv[]);
static int32_t usage_help(s32_t argc, char *argv[]);

dbg_module g_debug_fun_array[] =
{
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__
	{"bg95_net",  	bg95_net_service_test},
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET__
	{"bg95_socket", bg95_socket_service_test},
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S__
	{"bg95_http", 	bg95_http_service_test},
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S__
	{"bg95_ftp",  	bg95_ftp_service_test},
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S__ */
	{"bcast",     	bcast_service_test},
	{"debug",     	debug_service_test},
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_EXAMPLE_MAIN__
	{"main",      	user_main_test},
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_EXAMPLE_MAIN__ */
	{"help",      	usage_help},
};

#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_PRINT__
void debug_print(const int level, const char *msg, const char *prefix, const char *suffix, const char *file, const char *func, const int line, const char *fmt,...)
{
    va_list arglst;
    static rt_mutex_t debug_slock = NULL;
	int ret = 0, total_size=0, size1=0, size2=0, size3=0;
	osThreadId_t thread_id = os_thread_get_thread_id();
	unsigned char DBG_BUFFER[DBG_BUFF_LEN]= {0};

	if (debug_slock == NULL)
	{
        debug_slock = rt_mutex_create("debug", RT_IPC_FLAG_FIFO);
        if (debug_slock == NULL)
        {
            LOG_E("No memory for debug allocation lock!");
            return;
        }
	}

	if (level >= g_debug_level)
	{
		rt_mutex_take(debug_slock, RT_WAITING_FOREVER);
		va_start(arglst,fmt);
		// vsnprintf(DBG_BUFFER,sizeof(DBG_BUFFER),fmt,arglst);
		// printf("%s[%-5s][%24s][%32s():%04d][%4d] %s%s", prefix, msg, file, func, line, os_thread_get_stack_space(os_thread_get_thread_id()), DBG_BUFFER, suffix);
		memset(DBG_BUFFER, 0, sizeof(DBG_BUFFER));
		size1 = snprintf(DBG_BUFFER+sizeof(total_size), sizeof(DBG_BUFFER), "%s[%-5s][%24s][%32s():%04d][%4d][%x] ", prefix, msg, file, func, line, os_thread_get_stack_space(thread_id), thread_id);
		//size1 = snprintf(DBG_BUFFER+sizeof(total_size), sizeof(DBG_BUFFER), "%s[%-5s][%24s][%32s():%04d][%4d/%4d][%x] ", prefix, msg, file, func, line, os_thread_get_stack_space(thread_id), os_thread_get_free_heap_size(), thread_id);
		//size1 = snprintf(DBG_BUFFER+sizeof(total_size), sizeof(DBG_BUFFER), "%s[%-5s][%24s][%32s():%04d][%4d][%4d/%4d][%x] ", prefix, msg, file, func, line, os_thread_get_stack_space(thread_id), os_thread_get_min_free_heap_size(), os_thread_get_free_heap_size(), thread_id);
		size2 = vsnprintf(DBG_BUFFER+size1+sizeof(total_size), sizeof(DBG_BUFFER)-size1, fmt, arglst);
		size3 = snprintf(DBG_BUFFER+size1+size2+sizeof(total_size), sizeof(DBG_BUFFER)-size1-size2, "%s", suffix);
		total_size = size1+size2+size3;
		memcpy(DBG_BUFFER, &total_size, sizeof(total_size));
		printf("%s", DBG_BUFFER+sizeof(total_size));
		#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SAVE__
		if (g_save_debug_flag && g_log_rb.buffer)
		{
			ringbuffer_putstr(&g_log_rb, DBG_BUFFER, total_size + sizeof(total_size));
			if (g_debug_service_sem_id)
				os_sem_release(g_debug_service_sem_id);
		}
		#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SAVE__ */
		va_end(arglst);
		rt_mutex_release(debug_slock);
	}
}
#endif  /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_PRINT__ */

#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SHELL__
static void debug_service_cmd_parse(char *cmd, s32_t *pargc, char *argv[])
{
	int32_t argc;
	int32_t s32StrMrk;
	char *pcCmdStr;
	int32_t s32Ch;

	//LOG_V("%s",__FUNCTION__);	

	if(cmd == NULL)
	{
		LOG_E("Cmd is null!");
		return;
	}

	pcCmdStr = cmd;

	/* skipping heading white spaces */
	while(s32Ch = *(unsigned char *)pcCmdStr, isspace(s32Ch))
	{
		++pcCmdStr;
	}

	argc = 0;
	while(s32Ch != '\0')
	{
		if(++argc >= CMD_ARGC_NUM)
		{
			LOG_W("cmd argc too many");
			--argc;
			break;
		}

		/* the leading token (cmd name) not supporting " or ' sign */
		if(argc && (s32Ch == '"' || s32Ch == '\''))
		{
			s32StrMrk = s32Ch;
			++pcCmdStr;
			*argv++ = pcCmdStr;
			while(s32Ch = *(unsigned char *)pcCmdStr, s32Ch && s32Ch != s32StrMrk)
			{
				++pcCmdStr;
			}

			if(s32Ch != '\0')
			{
				*pcCmdStr++ = '\0';
			}
			else
			{
				LOG_W("tailing >%c< expected", s32StrMrk);
				break;
			}
		}
		else
		{
			*argv++ = pcCmdStr;
			while(s32Ch && !isspace(s32Ch))
			{
				s32Ch = *(unsigned char *)++pcCmdStr;
			}

			if(s32Ch != '\0')
			{
				*pcCmdStr++ = '\0';
			}
		}

		while(s32Ch = *(unsigned char *)pcCmdStr, isspace(s32Ch))
		{
			++pcCmdStr;
		}
	}
	*argv = NULL;
	*pargc = argc;
}

static int debug_service_cmd_proc(const char *cmd)
{
	char *argv[CMD_ARGC_NUM];
	int32_t argc, ret = -1;;
	static int8_t enter_fun_index = -1;
	int32_t i = 0, debug_fun_array_size = sizeof(g_debug_fun_array)/sizeof(g_debug_fun_array[0]);

	//LOG_V("%s",__FUNCTION__);	

	if(cmd == NULL)
	{
		LOG_E ("Input is empty, please enter valid data");
		return -1;
	}

	debug_service_cmd_parse(cmd,&argc,argv);
	if (enter_fun_index != -1)
	{
		ret = g_debug_fun_array[enter_fun_index].fp(argc, argv);
		if (ret == -1)
		{
			enter_fun_index = -1;
			argc = 1;
			argv[0] = "help";
			usage_help(argc, argv);
		}
		return ;
	}

	for(i = 0; i < debug_fun_array_size; i++)
	{
		//LOG_V("%s -> %s", argv[0], g_debug_fun_array[i].module_name);	
		if(strcmp(argv[0], g_debug_fun_array[i].module_name) == 0)
		{
			enter_fun_index = i;
			argc = 1;
			argv[0] = "help";
			ret = g_debug_fun_array[i].fp(argc, argv);
			if (ret == -1)
				enter_fun_index = -1;
			break;
		}
	}

	if (i == debug_fun_array_size)
	{
		enter_fun_index = -1;
		argc = 1;
		argv[0] = "help";
		usage_help(argc, argv);
	}

	return 0;
}

void serial_input_parse_thread_wake_up()
{
	if (g_debug_input_sem_id)
		os_sem_release(g_debug_input_sem_id);

}
static void* serial_input_parse_thread_proc(void* pThreadParam)
{
	int32_t ret;
	uint8_t *pData = NULL;
	uint16_t Size;
	
	LOG_V("%s",__FUNCTION__);	

	while (1)
	{
		ret = os_sem_take(g_debug_input_sem_id, portMAX_DELAY);
		if (ret != RT_EOK)
		{
			LOG_E("os_sem_take msg failed!"); 
			return -1;
		}

		USER_GET_DEBUG_INPUT_DATA(&pData, &Size);
		pData[Size] = '\0';
		// taskENTER_CRITICAL(); // 禁用中断
		 LOG_V("\r\n");
		LOG_V("get = %s", pData);
		//   taskEXIT_CRITICAL(); // 重新启用中断
		debug_service_cmd_proc(pData);
		LOG_H("#");
		fflush(stdout);
	}
	LOG_V("%s over",__FUNCTION__);	
	os_thread_exit();
}

static int debug_input_service_create(void)
{
	int32_t ret;

	LOG_V("%s",__FUNCTION__);	

	static osThreadAttr_t serial_input_parse_thread_attr   = {.name = "Debug_S", .stack_size = 512*12, .priority = osPriorityNormal};

	g_debug_input_sem_id = os_sem_create(1, 0);
    if (g_debug_input_sem_id == NULL)
    {
        LOG_E("AT client initialize failed! g_debug_input_sem_id semaphore create failed!");
		return -1;
    }
	g_serial_input_parse_thread_id = os_thread_create(serial_input_parse_thread_proc, NULL, &serial_input_parse_thread_attr);
	if (NULL == g_serial_input_parse_thread_id)
	{
		LOG_E ("Broadcast g_serial_input_parse_thread_id could not start!");
		return -1;
	}
	LOG_I("%s over(%x)",__FUNCTION__, g_serial_input_parse_thread_id);	

	return 0;
}
static int debug_input_service_destroy(void)
{
	int ret;

	//1. Destroy net service
	if (NULL != g_serial_input_parse_thread_id)
    {
        ret = os_thread_destroy(g_serial_input_parse_thread_id);
        if (0 != ret)
        {
            LOG_E("Delete g_serial_input_parse_thread_id thread failed! %d", ret); 
            return -1;			
        }
    }

    //2. Delete msg/sem queue
	if (NULL != g_debug_input_sem_id)
    {
        ret = os_sem_delete(g_debug_input_sem_id);
        if (0 != ret)
        {
            LOG_E("Delete g_debug_input_sem_id msg failed! %d", ret); 
            return -1;			
        }
    }
	return 0;
}
#else
void serial_input_parse_thread_wake_up()
{

}
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SHELL__ */

#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SAVE__
static void* debug_service_thread_proc(void* pThreadParam)
{
	int32_t ret,size;
	FRESULT f_res;
	FIL SDFile;
	unsigned int fnum; 
	unsigned char LOG_BUFFER[DBG_BUFF_LEN]= {0};

	LOG_I("%s",__FUNCTION__);	

	//2. init fs
	f_res = f_open(&SDFile, "0:quectel.log", FA_CREATE_ALWAYS | FA_WRITE);
	if(f_res != FR_OK)
	{
		LOG_E("open file error : %d", f_res);
		return;
	}
	else
	{
		LOG_I("open file success");
	}

	//3. write to sd card
	while(g_save_debug_flag)
	{
		ret = os_sem_take(g_debug_service_sem_id, portMAX_DELAY);
		if (ret != RT_EOK)
		{
			LOG_E("os_sem_take msg failed!"); 
			return -1;
		}

		while (g_save_debug_flag&&ringbuffer_data_len(&g_log_rb))
		{
			memset(LOG_BUFFER, 0, DBG_BUFF_LEN);
			ringbuffer_getstr(&g_log_rb, LOG_BUFFER, sizeof(size));
			memcpy(&size, LOG_BUFFER, sizeof(size));
			if (size > DBG_BUFF_LEN)
			{
				LOG_E("size too big:data size = %d, buff size = %d", size, DBG_BUFF_LEN);
			}
			ringbuffer_getstr(&g_log_rb, LOG_BUFFER, size);
			//printf("A:%s", LOG_BUFFER);
			f_res = f_write(&SDFile, LOG_BUFFER, size, &fnum);
			if(f_res != FR_OK)
			{
				LOG_E("write file error : %d", f_res);
			}
		}
		f_sync(&SDFile);
	}

	//4. close file
	f_res = f_close(&SDFile);
	if(f_res != FR_OK)
	{
		LOG_E("close file error : %d", f_res);
		return;
	}

	LOG_I("%s over",__FUNCTION__);

	os_thread_exit();	
}

static int debug_save_service_create(int rb_size)
{
	static osThreadAttr_t debug_service_thread_thread_attr = {.name = "Log_S",   .stack_size = 512*7, .priority = osPriorityNormal};
	
	//1. Init ringbuffer
	g_log_rb_buf = malloc(rb_size);
	if (g_log_rb_buf == NULL )
	{
		LOG_E("ring buffer malloc error : %d, %d", rb_size, os_thread_get_free_heap_size());
		return -1;
	}
	ringbuffer_init(&g_log_rb, g_log_rb_buf, rb_size);

	//2. Init sem
	g_debug_service_sem_id = os_sem_create(1, 0);
    if (g_debug_service_sem_id == NULL)
    {
        LOG_E("AT client initialize failed! g_debug_service_sem_id semaphore create failed!");
		return -1;
    }

	//3. Create thread
	g_debug_service_thread_id = os_thread_create(debug_service_thread_proc, NULL, &debug_service_thread_thread_attr);
	if (NULL == g_debug_service_thread_id)
	{
		LOG_E ("g_debug_service_thread_id thread could not start!");
		return -1;
	}
	LOG_I("%s over(%x)",__FUNCTION__, g_debug_service_thread_id);	
}

static int debug_save_service_destroy(void)
{
	int ret;
	

	// //1. Destroy net service
    // if (NULL != g_debug_service_thread_id)
    // {
    //     ret = os_thread_destroy(g_debug_service_thread_id);
    //     if (0 != ret)
    //     {
    //         LOG_E("Delete g_debug_service_thread_id thread failed! %d", ret); 
    //         return -1;			
    //     }
	// 	g_debug_service_thread_id=NULL;
    // }

	//2. Delete msg/sem queue
	if (NULL != g_debug_service_sem_id)
    {
        ret = os_sem_delete(g_debug_service_sem_id);
        if (0 != ret)
        {
            LOG_E("Delete g_debug_service_sem_id msg failed! %d", ret); 
            return -1;			
        }
		g_debug_service_sem_id=NULL;
    }

	//3. Free ringbuffer
	if (g_log_rb_buf != NULL)
		free(g_log_rb_buf);
	g_log_rb_buf = RT_NULL;

	return 0;
}
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SAVE__ */

int debug_service_create(void)
{
	int32_t ret;

	LOG_V("%s",__FUNCTION__);	

	#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SHELL__
	debug_input_service_create();
	#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SHELL__ */

	return 0;
}

int debug_service_destroy(void)
{
	int32_t ret = 0;

    LOG_V("%s",__FUNCTION__);	
	#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SHELL__
	debug_input_service_destroy();
	#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SHELL__ */
	LOG_V("%s over",__FUNCTION__);	

    return 0;
}

static int32_t usage_help(s32_t argc, char *argv[])
{
	if (strcmp((const char *)argv[0], "help")==0)
	{
		LOG_I("--------------------------------------------");
		LOG_I("| Usage:(%s)                |", __QUECTEL_USER_FRIENDLY_PROJECT_VERSION);
		int32_t i = 0, debug_fun_array_size = sizeof(g_debug_fun_array)/sizeof(g_debug_fun_array[0]);

		for(i = 0; i < debug_fun_array_size; i++)
		{
			LOG_I("| %-40s |", g_debug_fun_array[i].module_name);
		}
		LOG_I("--------------------------------------------");
	}

	return -1;
}

static int32_t debug_service_test(s32_t argc, char *argv[])
{
    int32_t i, save_debug_flag;

    //LOG_V("%s",__FUNCTION__);	

    for (i=0; i<argc; i++)
    {
        //LOG_V("%d = %s", i, argv[i]);
    }

	if (strcmp((const char *)argv[0], "help")==0)
	{
		LOG_I("--------------------------------------------");
		LOG_I("| debug:                                   |");		
		LOG_I("--------------------------------------------");
        LOG_I("| exit                                     |");		
		LOG_I("| help                                     |");						
		LOG_I("| save (2:show, on:1, off:0)               |");	
		LOG_I("| at (ATI)                                 |");	
		LOG_I("| level (v:0, D:1, I:2, W:3, E:4)          |");	
		LOG_I("| test                                     |");	
		LOG_I("--------------------------------------------");
	}
	else if (strcmp((const char *)argv[0], "exit")==0)
	{
        LOG_D("exit %s", __FUNCTION__);
        return -1;
	}
	else if (strcmp((const char *)argv[0], "test")==0)
	{
		LOG_V("Welcome to use quectel module");
		LOG_D("Welcome to use quectel module");
		LOG_I("Welcome to use quectel module");
		LOG_W("Welcome to use quectel module");
		LOG_E("Welcome to use quectel module");
	}
	else if (strcmp((const char *)argv[0], "level")==0)
	{
		g_debug_level = atoi(argv[1]);
	}
	else if (strcmp((const char *)argv[0], "save")==0)
	{
		#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_SAVE__
		if ((argc > 1))
		{
			if (((atoi(argv[1]) != 0) && (atoi(argv[1]) != 1)) || (atoi(argv[1]) == g_save_debug_flag))
			{
				LOG_I("Current log saving status is %s, free heap size is %d", (g_save_debug_flag == 1 ? "on" : "off"), os_thread_get_free_heap_size());
			}
			else if (atoi(argv[1]) == 1)
			{
				if (argc != 3)
				{
					LOG_I("Cur free heap space(%d), default use 2048 byte for input buffer size", os_thread_get_free_heap_size());
					g_save_debug_flag = 1;
					debug_save_service_create(2048);
				}
				else
				{
					LOG_I("Now start saving the log to sd card, input buffer size is %d, free heap size is %d", atoi(argv[2]), os_thread_get_free_heap_size());
					g_save_debug_flag = 1;
					debug_save_service_create(atoi(argv[2]));
				}
			}
			else 
			{
				LOG_I("Now stop saving the log");
				g_save_debug_flag = 0;
					if (g_debug_service_sem_id)
				os_sem_release(g_debug_service_sem_id);
				 osDelay(1000);
				debug_save_service_destroy();
			}
		}
		#else
        LOG_W("This function is not supported");
		#endif
	}
	else if (strcmp((const char *)argv[0], "at")==0)
	{
		at_response_t resp;
		LOG_V("%s", argv[1]);
		resp = at_create_resp(1024, 0, rt_tick_from_millisecond(3000));
		at_exec_cmd(resp, argv[1]);
    	at_delete_resp(resp);
	}
    else
    {
        LOG_E("Invalid parameter:%s", argv[0]);
    }
	LOG_V("%s over",__FUNCTION__);	

    return 0;
}
