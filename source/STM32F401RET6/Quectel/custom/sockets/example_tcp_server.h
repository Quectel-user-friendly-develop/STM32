#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_TCP_SERVER_EXAMPLE__
#ifndef __EXAMPLE_TCP_SERVER_H__
#define __EXAMPLE_TCP_SERVER_H__

typedef struct {
    unsigned int loop_count;
    unsigned int loop_interval;  //In milliseconds
    int sock_fd;
}socket_tcp_server_config;

int example_tcp_server_test(short sin_port, char *sin_addr, int max_connect_num, int loop_count, int loop_interval);

#endif /* __EXAMPLE_TCP_SERVER_H__ */
#endif  /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_TCP_SERVER_EXAMPLE__ */