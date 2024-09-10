/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-10-12     armink       first version
 */

/*
 * NOTE: DO NOT include this file on the header file.
 */

#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_EXAMPLE_MAIN__
#ifndef __MAIN_H__
#define __MAIN_H__

typedef enum {
    SOCKET_TCP,
    SOCKET_UDP,
    SOCKET_TCP_SERVER,
    SOCKET_UDP_SERVER,
} socket_test_type;

typedef struct {
    socket_test_type type;
    unsigned int max_connect_num;
    unsigned short sin_port;
    unsigned int loop_count;
    unsigned int loop_interval;  //In milliseconds
    char sin_addr[32];
    void *user_data;
}socket_test_config;


void user_main(void * argv);
int user_main_test(int argc, char *argv[]);



#endif /* __MAIN_H__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_EXAMPLE_MAIN__ */