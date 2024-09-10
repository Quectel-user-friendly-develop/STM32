#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S__
/**
 * \file        bg95_mqtt_handler.h
 * \brief       Handling mqtt callbacks.
 * \author      Dusan Poluga
 * \copyright   All rights reserved - Quectel Wireless Solutions
 *
 * This file contains TEMPLATE_CONTENT_DESCRIPTION.
 *
 * Version changes:
 *
 * - v1.0.0 - Dusan Poluga
 * + Module created.
 */

#ifndef _BG95_MQTT_HANDLER_H_
#define _BG95_MQTT_HANDLER_H_

#define TEMPLATE_CAP_VERSION   ("1.0.0")

// -------------------------------------------------------------------- INCLUDES

#include <stdint.h>
#include <at.h>
#include "at_osal.h"
#include "debug_service.h"
#include "broadcast_service.h"

// -------------------------------------------------------- MODULE CONFIGURATION
#define BG95_EVENT_MQTT_OK (1L << (8 + 4))
#define BG95_EVENT_MQTT_FIAL (1L << (8 + 5))
#define BG95_EVENT_MQTT_DIS_OK (1L << (8 + 6))
// --------------------------------------------------------------- PUBLIC MACROS



// ---------------------------------------------------------------- PUBLIC TYPES

typedef void (*bg95_mqtt_handler_callback_t)(int8_t*, int8_t*, uint16_t);


// ------------------------------------------------- PUBLIC FUNCTIONS PROTOTYPES

void bg95_mqtt_connect_cb(struct at_client *client ,const char *data, rt_size_t size);
void bg95_mqtt_qmtopen_cb(struct at_client *client ,const char *data, rt_size_t size);
void bg95_mqtt_qmtconn_cb(struct at_client *client ,const char *data, rt_size_t size);
void bg95_mqtt_qmtsub_cb(struct at_client *client ,const char *data, rt_size_t size);
void bg95_mqtt_qmtpub_cb(struct at_client *client ,const char *data, rt_size_t size);
void bg95_mqtt_qmtdisc_cb(struct at_client *client ,const char *data, rt_size_t size);
void bg95_mqtt_qmtstat_cb(struct at_client *client, const char *data, rt_size_t size);
void bg95_mqtt_urc_cb(struct at_client *client ,const char *data, rt_size_t size);
void bg95_mqtt_handler_cb_set(bg95_mqtt_handler_callback_t cb);


#endif // _BG95_MQTT_HANDLER_H_

// ----------------------------------------------------------------- END OF FILE
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */
