/*
 * File      : at_socket_device.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2018, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-05-08     chenyong     first version
 */
#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET__
#ifndef __AT_SOCKET_DEVICE_H__
#define __AT_SOCKET_DEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <at.h>
#include <at_socket.h>

#define AT_DEVICE_SW_VERSION           "2.0.4"
#define AT_DEVICE_SW_VERSION_NUM       0x20004

struct at_device_data
{

    void *socket_data;
    void *user_data;
};

struct at_device
{
    uint32_t socket_num;                         /* The maximum number of sockets need use */
    const struct at_socket_ops *socket_ops;      /* AT device socket operations */
    struct at_client *client;                    /* AT Client object for AT device */
    ip_addr_t ip_addr;                           /* IP address */
    rt_event_t socket_event;                     /* AT device socket event */
    struct at_socket *sockets;                   /* AT device sockets list */
    /* user-specific data */
    void *user_data;
};
/* Get AT device object */
struct at_device *at_device_get();

int at_device_socket_register(uint32_t socket_num, struct at_socket_ops *socket_ops, struct at_client *client, ip_addr_t *ip_addr);
int at_device_socket_unregister(void);
#ifdef __cplusplus
}
#endif

#endif /* __AT_SOCKET_DEVICE_H__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET__ */