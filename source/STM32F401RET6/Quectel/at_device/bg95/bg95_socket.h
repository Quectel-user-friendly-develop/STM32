#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET__
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
#ifndef __BG95_SOCKET_H__
#define __BG95_SOCKET_H__


int bg95_socket_service_create(void);
int bg95_socket_service_destroy(void);
int bg95_socket_service_test(int argc, char *argv[]);


#endif /* __BG95_SOCKET_H__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */