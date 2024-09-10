#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S__
/**
 * \file        bg95_mqtt_handler.c
 * \brief       Handling mqtt callbacks.
 * \author      Dusan Poluga
 * \copyright   All rights reserved - Quectel Wireless Solutions
 *
 * This file contains TEMPLATE_CONTENT_DESCRIPTION.
 */

// -------------------------------------------------------------------- INCLUDES

#include "bg95_mqtt_handler.h"
#include "bg95_mqtt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "main.h"

// ------------------------------------------------------------- INTERNAL MACROS

// -------------------------------------------------------------- INTERNAL TYPES

// ------------------------------------------------------------------- CONSTANTS

// ------------------------------------------------------------------- VARIABLES

bg95_mqtt_handler_callback_t cb_run;

// ---------------------------------------------- PRIVATE FUNCTIONS DECLARATIONS

// --------------------------------------------- PUBLIC FUNCTIONS IMPLEMENTATION

void bg95_mqtt_handler_cb_set(bg95_mqtt_handler_callback_t cb)
{
	cb_run = cb;
}

void bg95_mqtt_connect_cb(struct at_client *client, const char *data, rt_size_t size)
{
	LOG_D("DATA[%d]: %s\r\n", size, data);
}

void bg95_mqtt_qmtopen_cb(struct at_client *client, const char *data, rt_size_t size)
{
	LOG_D("DATA[%d]: %s\r\n", size, data);
	// Variables to hold error codes.
	int protocol_err = 0;

	// Pointer for FTP device structure, initialized to null.
	struct mqtt_device *device = RT_NULL;

	// Assert to ensure that the data pointer is not null and size is not zero.
	RT_ASSERT(data && size);

	// Log the function call with a verbose log level.
	LOG_V("%s, size = %d", __FUNCTION__, size);

	// Retrieve the current FTP device context.
	device = mqtt_device_get();
	// Check if the FTP device was not successfully retrieved.
	if (device == RT_NULL)
	{
		// Log an error message indicating the failure to get the device.
		LOG_E("get device failed.");
		return;
	}

	// Parse the data string to extract error codes using sscanf.
	// It expects the data in a specific format, "+QFTPOPEN: <err>,<protocol_err>".
	sscanf(data, "+QMTOPEN: %*d,%d", &protocol_err);

	// Check if there was no error (err == 0) in opening the FTP connection.
	if (protocol_err == 0)
	{

		bg95_mqtt_event_send(device, BG95_EVENT_MQTT_OK);
	}
	else
	{
		LOG_E("BG95_EVENT_MQTT_FIAL");
		// Send an event indicating FTP open failed.
		bg95_mqtt_event_send(device, BG95_EVENT_MQTT_FIAL);
	}
}

void bg95_mqtt_qmtconn_cb(struct at_client *client, const char *data, rt_size_t size)
{
	LOG_D("DATA[%d]: %s\r\n", size, data);
	int protocol_err = 0;

	// Pointer for FTP device structure, initialized to null.
	struct mqtt_device *device = RT_NULL;

	// Assert to ensure that the data pointer is not null and size is not zero.
	RT_ASSERT(data && size);

	// Log the function call with a verbose log level.
	LOG_V("%s, size = %d", __FUNCTION__, size);

	// Retrieve the current FTP device context.
	device = mqtt_device_get();
	// Check if the FTP device was not successfully retrieved.
	if (device == RT_NULL)
	{
		// Log an error message indicating the failure to get the device.
		LOG_E("get device failed.");
		return;
	}

	// Parse the data string to extract error codes using sscanf.
	// It expects the data in a specific format, "+QFTPOPEN: <err>,<protocol_err>".
	sscanf(data, "+QMTCONN: %*d,%d", &protocol_err);

	// Check if there was no error (err == 0) in opening the FTP connection.
	if (protocol_err == 0)
	{
		// Send an event indicating FTP open was successful.
		bg95_mqtt_event_send(device, BG95_EVENT_MQTT_OK);
	}
	else
	{
		LOG_E("BG95_EVENT_MQTT_FIAL.");
		// Send an event indicating FTP open failed.
		bg95_mqtt_event_send(device, BG95_EVENT_MQTT_FIAL);
	}
}

void bg95_mqtt_qmtsub_cb(struct at_client *client, const char *data, rt_size_t size)
{
	LOG_D("DATA[%d]: %s\r\n", size, data);
	int result = 0;

	// Pointer for FTP device structure, initialized to null.
	struct mqtt_device *device = RT_NULL;

	// Assert to ensure that the data pointer is not null and size is not zero.
	RT_ASSERT(data && size);

	// Log the function call with a verbose log level.
	LOG_V("%s, size = %d", __FUNCTION__, size);

	// Retrieve the current FTP device context.
	device = mqtt_device_get();
	// Check if the FTP device was not successfully retrieved.
	if (device == RT_NULL)
	{
		// Log an error message indicating the failure to get the device.
		LOG_E("get device failed.");
		return;
	}

	// Parse the data string to extract error codes using sscanf.
	// It expects the data in a specific format, "+QFTPOPEN: <err>,<protocol_err>".
	sscanf(data, "+QMTSUB: %*d,%*d,%d", &result);

	// Check if there was no error (err == 0) in opening the FTP connection.
	if (result == 0)
	{
		// Send an event indicating FTP open was successful.
		bg95_mqtt_event_send(device, BG95_EVENT_MQTT_OK);
	}
	else
	{
		LOG_E("BG95_EVENT_MQTT_FIAL.");
		// Send an event indicating FTP open failed.
		bg95_mqtt_event_send(device, BG95_EVENT_MQTT_FIAL);
	}
}
void bg95_mqtt_qmtpub_cb(struct at_client *client, const char *data, rt_size_t size)
{
	LOG_D("DATA[%d]: %s\r\n", size, data);
	int result = 0;

	// Pointer for FTP device structure, initialized to null.
	struct mqtt_device *device = RT_NULL;

	// Assert to ensure that the data pointer is not null and size is not zero.
	RT_ASSERT(data && size);

	// Log the function call with a verbose log level.
	LOG_V("%s, size = %d", __FUNCTION__, size);

	// Retrieve the current FTP device context.
	device = mqtt_device_get();
	// Check if the FTP device was not successfully retrieved.
	if (device == RT_NULL)
	{
		// Log an error message indicating the failure to get the device.
		LOG_E("get device failed.");
		return;
	}

	// Parse the data string to extract error codes using sscanf.
	// It expects the data in a specific format, "+QFTPOPEN: <err>,<protocol_err>".
	sscanf(data, "+QMTPUB: %*d,%*d,%d", &result);

	// Check if there was no error (err == 0) in opening the FTP connection.
	if (result == 0)
	{
		// Send an event indicating FTP open was successful.
		bg95_mqtt_event_send(device, BG95_EVENT_MQTT_OK);
	}
	else
	{
		LOG_E("BG95_EVENT_MQTT_FIAL.");
		// Send an event indicating FTP open failed.
		bg95_mqtt_event_send(device, BG95_EVENT_MQTT_FIAL);
	}
}

void bg95_mqtt_qmtstat_cb(struct at_client *client, const char *data, rt_size_t size)
{
	LOG_D("DATA[%d]: %s\r\n", size, data);
	int result = 0;
     int client_idx = 0;
	// Pointer for FTP device structure, initialized to null.
	struct mqtt_device *device = RT_NULL;

	// Assert to ensure that the data pointer is not null and size is not zero.
	RT_ASSERT(data && size);

	// Log the function call with a verbose log level.
	LOG_V("%s, size = %d", __FUNCTION__, size);

	// Retrieve the current FTP device context.
	device = mqtt_device_get();
	// Check if the FTP device was not successfully retrieved.
	if (device == RT_NULL)
	{
		// Log an error message indicating the failure to get the device.
		LOG_E("get device failed.");
		return;
	}

	// Parse the data string to extract error codes using sscanf.
	// It expects the data in a specific format, "+QFTPOPEN: <err>,<protocol_err>".
	sscanf(data, "+QMTSTAT: %d,%d", &client_idx,&result);
	if (result == 5)
	{
		// Send an event indicating FTP open was successful.
		bg95_mqtt_event_send(device, BG95_EVENT_MQTT_DIS_OK);
	}
	if(result==1)
	{
		clean_matching_index(client_idx);
	
	}
}

void bg95_mqtt_qmtdisc_cb(struct at_client *client, const char *data, rt_size_t size)
{
	LOG_D("DATA[%d]: %s\r\n", size, data);
	int result = 0;

	// Pointer for FTP device structure, initialized to null.
	struct mqtt_device *device = RT_NULL;

	// Assert to ensure that the data pointer is not null and size is not zero.
	RT_ASSERT(data && size);

	// Log the function call with a verbose log level.
	LOG_V("%s, size = %d", __FUNCTION__, size);

	// Retrieve the current FTP device context.
	device = mqtt_device_get();
	// Check if the FTP device was not successfully retrieved.
	if (device == RT_NULL)
	{
		// Log an error message indicating the failure to get the device.
		LOG_E("get device failed.");
		return;
	}

	// Parse the data string to extract error codes using sscanf.
	// It expects the data in a specific format, "+QFTPOPEN: <err>,<protocol_err>".
	sscanf(data, "+QMTDISC: %*d,%d", &result);

	// Check if there was no error (err == 0) in opening the FTP connection.
	if (result == 0)
	{
		// Send an event indicating FTP open was successful.
		bg95_mqtt_event_send(device, BG95_EVENT_MQTT_OK);
	}
	else
	{
		LOG_E("BG95_EVENT_MQTT_FIAL.");
		// Send an event indicating FTP open failed.
		bg95_mqtt_event_send(device, BG95_EVENT_MQTT_FIAL);
	}
}

void bg95_mqtt_urc_cb(struct at_client *client, const char *data, rt_size_t size)
{
	char tmp[4];
	int idx;
	int num_messages;
	char *topic;
	int msg_len;
	char *s_data;

	char *data_begin = strstr(data, ": ") + 2;
	char *data_end = strstr(data_begin, ",");
	memset(tmp, 0, 4);
	strncpy(tmp, data_begin, data_end - data_begin);
	idx = atoi(tmp);

	data_begin = data_end + 1;
	data_end = strstr(data_begin, ",");
	memset(tmp, 0, 4);
	strncpy(tmp, data_begin, data_end - data_begin);
	num_messages = atoi(tmp);

	data_begin = data_end + 1 + 1;
	data_end = strstr(data_begin, "\"");
	*data_end = 0;
	topic = data_begin;
	data_end += 2;

	data_begin = data_end;
	data_end = strstr(data_begin, ",");
	memset(tmp, 0, 4);
	strncpy(tmp, data_begin, data_end - data_begin);
	msg_len = atoi(tmp);

	data_begin = data_end + 2;
	data_end = strstr(data_begin, "\"");
	*data_end = 0;
	s_data = data_begin;

	if (cb_run)
	{
		cb_run((int8_t *)topic, (int8_t *)s_data, msg_len);
	}
}

// ----------------------------------------------------------------- END OF FILE
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */