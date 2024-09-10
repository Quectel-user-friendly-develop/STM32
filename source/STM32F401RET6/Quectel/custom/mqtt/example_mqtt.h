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
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S_EXAMPLE__
#ifndef __EXAMPLE_MQTT_H__
#define __EXAMPLE_MQTT_H__
#include "bg95_mqtt.h"

typedef struct {
    u8_t mqtt_test_type; 
    u8_t Server_type; 
    BG95_MQTT_CONFIG_T conf_local_hq ;
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__
ql_SSL_Config ssl_config; //
#endif 


}mqtt_test_config;

int example_mqtt_test(void *argument);
int mqtt_config_conn(uint8_t mode,void *cfg_struct);

#endif /* __EXAMPLE_MQTT_H__ */

#endif  /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S_EXAMPLE__ */
