#ifndef __DEBUG_SERVICE_H__
#define __DEBUG_SERVICE_H__
#include "QuectelConfig.h"

#define DBG_BUFF_LEN		    (1024)
#define CMD_ARGC_NUM	     	(25)
#define NAME_MAX_LEN		    (16)

#define USE_DEBUG_ASSERT

#ifdef  USE_DEBUG_ASSERT
#define dbg_assert_param(expr) ((expr) ? (void)0U : dbg_assert_failed((uint8_t *)__FILE__, __LINE__))
#else
#define assert_param(expr) ((void)0U)
#endif /* USE_DEBUG_ASSERT */

typedef enum 
{
  LOG_VERBOSE, 
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARN,
  LOG_ERR,
  LOG_MAX
} debug_level;

#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_PRINT__
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#define LOG_V( ...) debug_print(LOG_VERBOSE, "VER",   "\033[0;37m", "\033[0m\r\n", __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_D( ...) debug_print(LOG_DEBUG,   "DEBUG", "\033[0;37m", "\033[0m\r\n", __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_I( ...) debug_print(LOG_INFO,    "INFO",  "\033[0;34m", "\033[0m\r\n", __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)	
#define LOG_H( ...) debug_print(LOG_INFO,    "INFO",  "\033[0;34m", "\033[0m",     __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)	
#define LOG_W( ...) debug_print(LOG_WARN,    "WARN",  "\033[0;33m", "\033[0m\r\n", __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_E( ...) debug_print(LOG_ERR,     "ERR",   "\033[0;31m", "\033[0m\r\n", __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)	
#else
#define LOG_V( ...) 
#define LOG_D( ...) 
#define LOG_I( ...) 
#define LOG_H( ...) 
#define LOG_W( ...) 
#define LOG_E( ...) 	
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_DEBUG_PRINT__ */

typedef struct 
{
	char module_name[NAME_MAX_LEN];
	int (*fp)(int argc, char *argv[]);
}dbg_module, *dbg_module_t;
extern debug_level g_debug_level;

int debug_service_create(void);
int debug_service_destroy(void);
void serial_input_parse_thread_wake_up();


#endif /* __DEBUG_SERVICE_H__ */
