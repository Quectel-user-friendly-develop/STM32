#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S__
/**
 * \file        bg95_mqtt.c
 * \brief       Handling mqtt transactions.
 * \author      Dusan Poluga
 * \copyright   All rights reserved - Quectel Wireless Solutions
 *
 * This file contains TEMPLATE_CONTENT_DESCRIPTION.
 */

// -------------------------------------------------------------------- INCLUDES

#include "bg95_mqtt.h"
#include "bg95_mqtt_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "main.h"
#include "bg95_ssl.h"
// ------------------------------------------------------------- INTERNAL MACROS

// -------------------------------------------------------------- INTERNAL TYPES

/**
 * @brief Structure representing an MQTT connection file descriptor and its associated configuration.
 *
 * This structure encapsulates an MQTT connection file descriptor along with its corresponding
 * configuration settings.
 */
static struct mqtt_device g_bg95_mqtt_device = {0};

typedef struct
{
	BG95_MQTT_CONN_FD fd;	 // File descriptor for the MQTT connection
	BG95_MQTT_CONFIG_T *cfg; // Pointer to the MQTT configuration settings

} __attribute__((aligned(4))) BG95_MQTT_FD;

// ------------------------------------------------------------------- CONSTANTS

static struct at_urc bg95_mqtt_urc_table[] = {
	{"+QMTRECV:", "\r\n", bg95_mqtt_urc_cb},
	{"CONNECT", "\r\n", bg95_mqtt_connect_cb},
	{"+QMTOPEN:", "\r\n", bg95_mqtt_qmtopen_cb},
	{"+QMTCONN:", "\r\n", bg95_mqtt_qmtconn_cb},
	{"+QMTSUB:", "\r\n", bg95_mqtt_qmtsub_cb},
	{"+QMTPUB:", "\r\n", bg95_mqtt_qmtpub_cb},
	{"+QMTSTAT:", "\r\n", bg95_mqtt_qmtstat_cb},
	{"+QMTDISC:", "\r\n", bg95_mqtt_qmtdisc_cb},
};

// ------------------------------------------------------------------- VARIABLES

/**
 * @brief Array of MQTT connection descriptors.
 *
 * This array stores the MQTT connection descriptors for the broker connections.
 * It has a size of BG95_MQTT_MAX_BROKER_CONNECTIONS and is initialized with default values.
 */
 BG95_MQTT_FD config_descriptor[BG95_MQTT_MAX_BROKER_CONNECTIONS] = {{-1}};

/**
 * @brief Mutex for BG95 MQTT operations.
 *
 * This mutex is used to ensure thread-safe access to BG95 MQTT operations.
 */
static rt_mutex_t bg95_mqtt_lock;

/**
 * @brief Flag indicating cold start for BG95 MQTT.
 *
 * This flag indicates whether BG95 MQTT has undergone a cold start or not.
 * It is initialized with a value of -1.
 */
static int8_t bg95_mqtt_cold_start = -1;

// ---------------------------------------------- PRIVATE FUNCTIONS DECLARATIONS

/**
 * @brief Sends an AT command related to BG95 MQTT.
 *
 * This function sends the specified AT command related to BG95 MQTT.
 *
 * @param at_cmd Pointer to the AT command string to be sent.
 * @return Returns an integer representing the status of the AT command execution.
 */
static int8_t bg95_mqtt_at_send(int8_t *at_cmd);

/**
 * @brief Sends an AT command along with data related to BG95 MQTT.
 *
 * This function sends the specified AT command along with additional data related to BG95 MQTT.
 *
 * @param at_cmd Pointer to the AT command string to be sent.
 * @param p_data Pointer to the additional data to be sent.
 * @return Returns an integer representing the status of the AT command execution.
 */
static int8_t bg95_mqtt_at_data_send(int8_t *at_cmd, int8_t *p_data);

/**
 * @brief Formats and sends an AT command related to BG95 MQTT.
 *
 * This function formats and sends the specified AT command related to BG95 MQTT.
 *
 * @param format Format string for the AT command.
 * @param ... Additional arguments to be formatted.
 * @return Returns an integer representing the status of the AT command execution.
 */
static int8_t bg95_mqtt_at_format_send(const char *format, ...);

/**
 * @brief Cleans up an MQTT connection file descriptor.
 *
 * This function cleans up an MQTT connection file descriptor by resetting it to default values.
 *
 * @param fd Pointer to the MQTT connection file descriptor to be cleaned.
 */
static void fd_clean(BG95_MQTT_FD *fd);

/**
 * @brief Gets a free slot for an MQTT connection.
 *
 * This function gets a free slot for an MQTT connection from the provided MQTT connection descriptor.
 *
 * @param fd Pointer to the MQTT connection descriptor.
 * @return Returns a file descriptor representing the free slot for the MQTT connection.
 */
static BG95_MQTT_CONN_FD get_free_slot(BG95_MQTT_FD *fd);

/**
 * @brief Gets the index of a matching connection file descriptor.
 *
 * This function gets the index of a matching connection file descriptor from the provided MQTT connection descriptor.
 *
 * @param fd MQTT connection file descriptor.
 * @return Returns a file descriptor representing the matching index.
 */
static BG95_MQTT_CONN_FD get_matching_index(BG95_MQTT_CONN_FD fd);
void clean_matching_index(uint8_t idx);
// --------------------------------------------- PUBLIC FUNCTIONS IMPLEMENTATION

struct mqtt_device *mqtt_device_get(void)
{
	return &g_bg95_mqtt_device;
}
int mqtt_device_register(struct at_client *client)
{
	// Declare a structure to hold event flag attributes.
	osEventFlagsAttr_t event_attr;
	memset(&event_attr, 0, sizeof(event_attr));
	// Set the name of the event attribute to "ftp_device".
	event_attr.name = "mqtt_device";

	// Create an event flag for the FTP URC (Unsolicited Result Code) with the specified attributes.
	// This is likely for handling asynchronous events or notifications for the FTP device.
	g_bg95_mqtt_device.mqtt_urc_event = OsalEventNCreate(&event_attr);

	// Check if the event creation was unsuccessful (possibly due to no memory).
	if (g_bg95_mqtt_device.mqtt_urc_event == RT_NULL)
	{
		// Log an error message indicating memory allocation failure.
		LOG_E("no memory for AT device socket event create.");

		// Return an error code indicating no memory available.
		return -RT_ENOMEM;
	}

	// Return a success code indicating the operation completed successfully.
	return RT_EOK;
}

// Function to unregister an FTP device.
// This function does g_bg95_mqtt_device.mqtt_urc_eventnot take any arguments.
int mqtt_device_unregister(void)
{
	// Check if the FTP URC (Unsolicited Result Code) event is initialized.
	if (g_bg95_mqtt_device.mqtt_urc_event)
	{
		// Delete or clean up the event associated with the FTP URC.
		// This is important to free up any resources used by the event.
		OsalEventDelete(g_bg95_mqtt_device.mqtt_urc_event);
	}

	// Return a success code indicating the operation completed successfully.
	return RT_EOK;
}

// Function to send an FTP-related event.
// It takes a pointer to an 'ftp_device' structure and an event code as arguments.
int bg95_mqtt_event_send(struct mqtt_device *device, uint32_t event)
{
	// Log the function call with a verbose log level.
	// It logs the name of the function and the event code being sent.
	LOG_V("%s, 0x%x", __FUNCTION__, event);

	// Send the specified event to the FTP URC (Unsolicited Result Code) event handle of the device.
	// 'OsalEventSend' is a function that likely sends or signals an event in the system.
	// The return value of 'OsalEventSend' is cast to an int and returned by this function.
	return (int)OsalEventSend(device->mqtt_urc_event, event);
}

// Function to receive an event for an FTP device.
// It takes a pointer to an 'ftp_device' structure, an event code, a timeout value, and an option flag as arguments.
static int bg95_mqtt_event_recv(struct mqtt_device *device, uint32_t event, uint32_t timeout, rt_uint8_t option)
{
	// Variable to store the received event.
	rt_uint32_t recved;

	// Log the function call with a verbose log level.
	// It logs the name of the function, event code, timeout value, and option flag.
	LOG_V("%s, event = 0x%x, timeout = %d, option = %d", __FUNCTION__, event, timeout, option);

	// Receive an event. The 'OsalEventRecv' function waits for a specific event to occur within the given timeout period.
	// The 'option' parameter is combined with the 'RT_EVENT_FLAG_CLEAR' flag to modify the behavior of the event wait.
	recved = OsalEventRecv(device->mqtt_urc_event, event, option, timeout);

	// Check if the event was not received within the timeout period.
	if (recved < 0)
	{
		// Return an error code indicating a timeout occurred.
		return -RT_ETIMEOUT;
	}

	// Log that the operation is completed.
	LOG_V("%s, event = 0x%x, timeout = %d, option = %d, over", __FUNCTION__, event, timeout, option);

	// Return the result of the event reception.
	return recved;
}

BG95_MQTT_ERROR_T bg95_mqtt_config(BG95_MQTT_CONN_FD *fd, uint8_t mode, BG95_MQTT_CONFIG_T *cfg_struct)
{
	BG95_MQTT_CONN_FD idx = 0;

	if (cfg_struct == 0)
	{
		return BG95_MQTT_NOK;
	}

	if (bg95_mqtt_cold_start < 0)
	{
		at_client_t client = at_client_get_first();
		bg95_mqtt_cold_start = 0;
		mqtt_device_register(client);
		fd_clean(&config_descriptor[0]);
		at_set_urc_table(bg95_mqtt_urc_table, sizeof(bg95_mqtt_urc_table) / sizeof(bg95_mqtt_urc_table[0]));
	}
	idx = get_free_slot(&config_descriptor[0]);

	if (idx == -1)
	{
		return BG95_MQTT_SLOTS_FULL; // All available slots are in use
	}
	config_descriptor[idx].fd = idx;
	if (mode == 0)
	{

		config_descriptor[idx].cfg = cfg_struct;
		LOG_I("config_server %s!", config_descriptor[idx].cfg->server);
		LOG_I("config_port %u!", config_descriptor[idx].cfg->port);
		LOG_I("config_port %s!", config_descriptor[idx].cfg->Client_ID);

		LOG_I("config_ProductKey %s!", config_descriptor[idx].cfg->ProductKey);
		LOG_I("config_DeviceName %s!", config_descriptor[idx].cfg->DeviceName);
		LOG_I("config_DeviceSecret %s!", config_descriptor[idx].cfg->DeviceSecret);
	}
	else
	{
		config_descriptor[idx].cfg = cfg_struct;
		LOG_I("config_server %s!", config_descriptor[idx].cfg->server);
		LOG_I("config_port %u!", config_descriptor[idx].cfg->port);
		LOG_I("config_Client_ID %s!", config_descriptor[idx].cfg->Client_ID);
		LOG_I("config_username %s!", config_descriptor[idx].cfg->username);
		LOG_I("config_ password %s!", config_descriptor[idx].cfg->password);
	}

	if (bg95_mqtt_at_format_send("AT+QMTCFG=\"recv/mode\",%d,0,1", config_descriptor[idx].fd))
	{
		LOG_E("Failed to set configuratin\r\n");
		*fd = -1;
		return BG95_MQTT_NOK;
	}

	*fd = config_descriptor[idx].fd;

	return BG95_MQTT_OK;
}

BG95_MQTT_ERROR_T bg95_mqtt_ssl_config(BG95_MQTT_CONN_FD fd, ql_SSL_Config *config)
{
	BG95_MQTT_CONFIG_T *cfg;
	BG95_MQTT_CONN_FD idx;
	if (fd < 0 || fd > 5)
	{
		LOG_E("Invalid value passed");
		return BG95_MQTT_NOK;
	}

	idx = get_matching_index(fd);

	if (idx == -1)
	{
		LOG_E("No descriptor initialized");
		return BG95_MQTT_NOK;
	}

	cfg = config_descriptor[idx].cfg;

	if (bg95_mqtt_at_format_send((char *)"AT+QMTCFG=\"SSL\",%d,1,%d", idx, config->sslctxid))
	{
		LOG_E("Command failed\r\n");
		return BG95_MQTT_NOK;
	}
	if (configure_ssl(config) != 0)
	{
		LOG_E("SSL configuration failed.\n");
		return -1;
	}

	return BG95_MQTT_OK;
}
BG95_MQTT_ERROR_T QL_mqtt_open(BG95_MQTT_CONN_FD fd, BG95_MQTT_CONFIG_T *cfg_struct)
{

	uint32_t event = 0;
	int err;
	BG95_MQTT_CONN_FD idx;
	int result = BG95_MQTT_OK;
	int event_result = 0;
	struct mqtt_device *device = RT_NULL;

	idx = get_matching_index(fd);

	if (idx == -1)
	{
		LOG_E("No descriptor initialized");
		return BG95_MQTT_NOK;
	}

	// Get the FTP device instance.
	device = mqtt_device_get();
	if (device == RT_NULL)
	{
		LOG_E("get device failed.");
		return -1;
	}

	// Check if the FTP configuration is valid.
	if (!cfg_struct)
	{
		LOG_E("FTP config is NULL.\n");
		return BG95_MQTT_NOK;
	}

	// Create an AT response object with a 3000ms timeout.
	at_response_t resp = at_create_resp(32, 0, rt_tick_from_millisecond(3000));
	if (!resp)
	{
		LOG_E("No memory for response object.\n");
		return -1;
	}

	// Define the expected FTP open events.
	event = BG95_EVENT_MQTT_OK | BG95_EVENT_MQTT_FIAL;

	// Wait for the specified FTP open events.
	bg95_mqtt_event_recv(device, event, 0, RT_EVENT_FLAG_OR);

	// Execute the AT command to open the FTP connection.
	at_exec_cmd(resp, "AT+QMTOPEN=%d,\"%s\",%u", (int)fd, cfg_struct->server, cfg_struct->port);

	int lineFound = 0;
	for (int i = 0; i < resp->line_counts; i++)
	{
		const char *line = at_resp_get_line(resp, i + 1);
		LOG_V("query_resp line [%d]: %s", i, line);

		// Check if there is a "+CME ERROR" line.
		if (strstr(line, "ERROR") != NULL)
		{

			at_delete_resp(resp);
			return -1;
		}
	}

	/* waiting OK or failed result */
	event_result = bg95_mqtt_event_recv(device,
										BG95_EVENT_MQTT_OK | BG95_EVENT_MQTT_FIAL, 40000, RT_EVENT_FLAG_OR);

	if (event_result < 0)
	{
		LOG_V("bg95_mqtt_event_recv time out");
		result = -RT_ERROR;
		goto __exit;
	}

	/* Check the result */
	if (event_result & BG95_EVENT_MQTT_FIAL)
	{
		clean_matching_index(idx);
	
		result = -RT_ERROR;
		goto __exit;
	}

__exit:
	at_delete_resp(resp);
	return result; // Operation succeeded
}
BG95_MQTT_ERROR_T QL_mqtt_conn(BG95_MQTT_CONN_FD fd, BG95_MQTT_CONFIG_T *cfg_struct)
{

	uint32_t event = 0;
	int err;
	int result = BG95_MQTT_OK;
	int event_result = 0;
	struct mqtt_device *device = RT_NULL;

	// Get the FTP device instance.
	device = mqtt_device_get();
	if (device == RT_NULL)
	{
		LOG_E("get device failed.");
		return -1;
	}

	// Check if the FTP configuration is valid.
	if (!cfg_struct)
	{
		LOG_E("FTP config is NULL.\n");
		return BG95_MQTT_NOK;
	}

	// Create an AT response object with a 3000ms timeout.
	at_response_t resp = at_create_resp(32, 0, rt_tick_from_millisecond(3000));
	if (!resp)
	{
		LOG_E("No memory for response object.\n");
		return -1;
	}

	// Define the expected FTP open events.
	event = BG95_EVENT_MQTT_OK | BG95_EVENT_MQTT_FIAL;

	// Wait for the specified FTP open events.
	bg95_mqtt_event_recv(device, event, 0, RT_EVENT_FLAG_OR);
	if ((cfg_struct->username[0] && cfg_struct->password[0]))
	{
		at_exec_cmd(resp, "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"", fd, cfg_struct->Client_ID, cfg_struct->username, cfg_struct->password);
	}
	else
	{
		at_exec_cmd(resp, "AT+QMTCONN=%d,\"%s\"", fd, cfg_struct->Client_ID);
	}

	int lineFound = 0;
	for (int i = 0; i < resp->line_counts; i++)
	{
		const char *line = at_resp_get_line(resp, i + 1);
		LOG_V("query_resp line [%d]: %s", i, line);

		// Check if there is a "+CME ERROR" line.
		if (strstr(line, "ERROR") != NULL)
		{

			at_delete_resp(resp);
			return -1;
		}
	}

	/* waiting OK or failed result */
	event_result = bg95_mqtt_event_recv(device,
										BG95_EVENT_MQTT_OK | BG95_EVENT_MQTT_FIAL, 40000, RT_EVENT_FLAG_OR);

	if (event_result < 0)
	{
		LOG_V("bg95_mqtt_event_recv time out");
		result = -RT_ERROR;
		goto __exit;
	}

	/* Check the result */
	if (event_result & BG95_EVENT_MQTT_FIAL)
	{
		// Handle the case where the FTP connection fails.
		result = -RT_ERROR;
		goto __exit;
	}

__exit:
	at_delete_resp(resp);
	return result; // Operation succeeded
}

BG95_MQTT_ERROR_T QL_mqtt_dis(BG95_MQTT_CONN_FD fd)
{

	uint32_t event = 0;
	int err;
	int result = 0;
	int event_result = 0;
	struct mqtt_device *device = RT_NULL;

	// Get the FTP device instance.
	device = mqtt_device_get();
	if (device == RT_NULL)
	{
		LOG_E("get device failed.");
		return -1;
	}

	// Create an AT response object with a 3000ms timeout.
	at_response_t resp = at_create_resp(32, 0, rt_tick_from_millisecond(3000));
	if (!resp)
	{
		LOG_E("No memory for response object.\n");
		return -1;
	}

	// Define the expected FTP open events.
	event = BG95_EVENT_MQTT_OK | BG95_EVENT_MQTT_FIAL;

	// Wait for the specified FTP open events.
	bg95_mqtt_event_recv(device, event, 0, RT_EVENT_FLAG_OR);

	at_exec_cmd(resp, "AT+QMTDISC=%d", fd);

	int lineFound = 0;
	for (int i = 0; i < resp->line_counts; i++)
	{
		const char *line = at_resp_get_line(resp, i + 1);
		LOG_V("query_resp line [%d]: %s", i, line);

		// Check if there is a "+CME ERROR" line.
		if (strstr(line, "ERROR") != NULL)
		{

			at_delete_resp(resp);
			return -1;
		}
	}

	/* waiting OK or failed result */
	event_result = bg95_mqtt_event_recv(device,
										BG95_EVENT_MQTT_OK | BG95_EVENT_MQTT_FIAL, 20000, RT_EVENT_FLAG_OR);

	if (event_result < 0)
	{
		LOG_V("bg95_mqtt_event_recv time out");
		result = -RT_ERROR;
		goto __exit;
	}

	/* Check the result */
	if (event_result & BG95_EVENT_MQTT_FIAL)
	{
		// Handle the case where the FTP connection fails.
		result = -RT_ERROR;
		goto __exit;
	}
	event_result = bg95_mqtt_event_recv(device, BG95_EVENT_MQTT_DIS_OK, 5000, RT_EVENT_FLAG_OR);
	if (event_result < 0)
	{
		LOG_V("bg95_mqtt_event_recv time out");
		result = BG95_MQTT_TCP_DIS_FAIL;
		goto __exit;
	}
	if (event_result & BG95_EVENT_MQTT_DIS_OK)
	{
		// Handle the case where the FTP connection fails.
		result = BG95_MQTT_TCP_DIS_OK;
		goto __exit;
	}

__exit:
	at_delete_resp(resp);
	return result; // Operation succeeded
}
BG95_MQTT_ERROR_T QL_mqtt_sub(BG95_MQTT_CONN_FD fd, const int8_t *topic)
{

	uint32_t event = 0;
	int err;
	int result = 0;
	int event_result = 0;
	struct mqtt_device *device = RT_NULL;

	// Get the FTP device instance.
	device = mqtt_device_get();
	if (device == RT_NULL)
	{
		LOG_E("get device failed.");
		return -1;
	}

	// Check if the FTP configuration is valid.
	if (!topic)
	{
		LOG_E("mqtt config is NULL.\n");
		return BG95_MQTT_NOK;
	}

	// Create an AT response object with a 3000ms timeout.
	at_response_t resp = at_create_resp(32, 0, rt_tick_from_millisecond(3000));
	if (!resp)
	{
		LOG_E("No memory for response object.\n");
		return -1;
	}

	// Define the expected FTP open events.
	event = BG95_EVENT_MQTT_OK | BG95_EVENT_MQTT_FIAL;

	// Wait for the specified FTP open events.
	bg95_mqtt_event_recv(device, event, 0, RT_EVENT_FLAG_OR);

	at_exec_cmd(resp, "AT+QMTSUB=%d,1,\"%s\",1", fd, topic);

	int lineFound = 0;
	for (int i = 0; i < resp->line_counts; i++)
	{
		const char *line = at_resp_get_line(resp, i + 1);
		LOG_V("query_resp line [%d]: %s", i, line);

		// Check if there is a "+CME ERROR" line.
		if (strstr(line, "ERROR") != NULL)
		{

			at_delete_resp(resp);
			return -1;
		}
	}

	/* waiting OK or failed result */
	event_result = bg95_mqtt_event_recv(device,
										BG95_EVENT_MQTT_OK | BG95_EVENT_MQTT_FIAL, 20000, RT_EVENT_FLAG_OR);

	if (event_result < 0)
	{
		LOG_V("bg95_mqtt_event_recv time out");
		result = -RT_ERROR;
		goto __exit;
	}

	/* Check the result */
	if (event_result & BG95_EVENT_MQTT_FIAL)
	{
		// Handle the case where the FTP connection fails.
		result = -RT_ERROR;
		goto __exit;
	}

__exit:
	at_delete_resp(resp);
	return result; // Operation succeeded
}

BG95_MQTT_ERROR_T QL_mqtt_pub(BG95_MQTT_CONN_FD fd, const int8_t *topic, const int8_t *publish_data)
{

	uint32_t event = 0;
	int err;
	int result = 0;
	int event_result = 0;
	struct mqtt_device *device = RT_NULL;

	// Get the FTP device instance.
	device = mqtt_device_get();
	if (device == RT_NULL)
	{
		LOG_E("get device failed.");
		return -1;
	}

	// Check if the FTP configuration is valid.
	if (!topic)
	{
		LOG_E("mqtt config is NULL.\n");
		return BG95_MQTT_NOK;
	}

	// Create an AT response object with a 3000ms timeout.
	at_response_t resp = at_create_resp(32, 0, rt_tick_from_millisecond(3000));
	if (!resp)
	{
		LOG_E("No memory for response object.\n");
		return -1;
	}

	// Define the expected FTP open events.
	event = BG95_EVENT_MQTT_OK | BG95_EVENT_MQTT_FIAL;

	// Wait for the specified FTP open events.
	bg95_mqtt_event_recv(device, event, 0, RT_EVENT_FLAG_OR);

	at_exec_cmd(resp, "AT+QMTPUBEX=%d,0,0,0,\"%s\",\"%s\"", fd, topic, publish_data);

	int lineFound = 0;
	for (int i = 0; i < resp->line_counts; i++)
	{
		const char *line = at_resp_get_line(resp, i + 1);
		LOG_V("query_resp line [%d]: %s", i, line);

		// Check if there is a "+CME ERROR" line.
		if (strstr(line, "ERROR") != NULL)
		{

			at_delete_resp(resp);
			return -1;
		}
	}

	/* waiting OK or failed result */
	event_result = bg95_mqtt_event_recv(device,
										BG95_EVENT_MQTT_OK | BG95_EVENT_MQTT_FIAL, 20000, RT_EVENT_FLAG_OR);

	if (event_result < 0)
	{
		LOG_V("bg95_mqtt_event_recv time out");
		result = -RT_ERROR;
		goto __exit;
	}

	/* Check the result */
	if (event_result & BG95_EVENT_MQTT_FIAL)
	{
		// Handle the case where the FTP connection fails.
		result = -RT_ERROR;
		goto __exit;
	}

__exit:
	at_delete_resp(resp);
	return result; // Operation succeeded
}
BG95_MQTT_ERROR_T bg95_mqtt_connect(BG95_MQTT_CONN_FD fd)
{
	BG95_MQTT_CONFIG_T *cfg;
	BG95_MQTT_CONN_FD idx;
	if (fd < 0 || fd > 5)
	{
		LOG_E("Invalid value passed");
		return BG95_MQTT_NOK;
	}

	idx = get_matching_index(fd);

	if (idx == -1)
	{
		LOG_E("No descriptor initialized");
		return BG95_MQTT_NOK;
	}

	cfg = config_descriptor[idx].cfg;
	if (cfg->server_type == 0)
	{
		if (bg95_mqtt_at_format_send("AT+QMTCFG=\"aliauth\",%d,\"%s\",\"%s\",\"%s\"", (int)idx, (char *)cfg->ProductKey, (unsigned int)cfg->DeviceName, (unsigned int)cfg->DeviceSecret))
		{
			LOG_E("+QMTOPEN command failed");
			return BG95_MQTT_NOK;
		}
	}
	if (QL_mqtt_open(idx, cfg) != 0)
	{
		LOG_E("+QMTOPEN command failed");
		return BG95_MQTT_NOK;
	}
	if (QL_mqtt_conn(idx, cfg) != 0)
	{
		LOG_E("+QMTOPEN command failed");
		return BG95_MQTT_NOK;
	}

	return BG95_MQTT_OK;
}

BG95_MQTT_ERROR_T bg95_mqtt_disconnect(BG95_MQTT_CONN_FD fd)
{
	BG95_MQTT_CONN_FD idx;
	BG95_MQTT_ERROR_T status = 0;
	if (fd < 0 || fd > 5)
	{
		LOG_E("Invalid value passed");
		return BG95_MQTT_NOK;
	}

	idx = get_matching_index(fd);

	if (idx == -1)
	{
		LOG_E("No descriptor initialized");
		return BG95_MQTT_NOK;
	}
	status = QL_mqtt_dis(idx);
	if (status == BG95_MQTT_TCP_DIS_OK)
	{
	clean_matching_index(idx);

		return BG95_MQTT_OK;
	}
	else if (status == BG95_MQTT_TCP_DIS_FAIL)
	{
		if (bg95_mqtt_at_format_send("AT+QMTCLOSE=%d", (int)idx) == 0)
		{
			clean_matching_index(idx);

			return BG95_MQTT_OK;
		}
	}

	LOG_E("mqtt_disconnect fail");
	return BG95_MQTT_NOK;
}

BG95_MQTT_ERROR_T bg95_mqtt_sub(BG95_MQTT_CONN_FD fd, const int8_t *topic)
{
	BG95_MQTT_CONN_FD idx;
	if (fd < 0 || fd > 5)
	{
		LOG_E("Invalid value passed");
		return BG95_MQTT_NOK;
	}
	if (topic == 0)
	{
		LOG_E("Invalid value passed");
		return BG95_MQTT_NOK;
	}

	idx = get_matching_index(fd);

	if (idx == -1)
	{
		LOG_E("No descriptor initialized");
		return BG95_MQTT_NOK;
	}

	if (QL_mqtt_sub(fd, topic) != 0)
	{
		LOG_E("+QMTSUB command failed");
		return BG95_MQTT_NOK;
	}

	return BG95_MQTT_OK;
}

BG95_MQTT_ERROR_T bg95_mqtt_pub(BG95_MQTT_CONN_FD fd, const int8_t *topic, const int8_t *publish_data)

{
	BG95_MQTT_CONN_FD idx;
	if (fd < 0 || fd > 5)
	{
		LOG_E("Invalid value passed");
		return BG95_MQTT_NOK;
	}
	if (topic == 0)
	{
		LOG_E("Invalid value passed");
		return BG95_MQTT_NOK;
	}

	idx = get_matching_index(fd);

	if (idx == -1)
	{
		LOG_E("No descriptor initialized");
		return BG95_MQTT_NOK;
	}
	if (QL_mqtt_pub(fd, topic, publish_data) != 0)
	{
		LOG_E("No descriptor initialized");
		return BG95_MQTT_NOK;
	}

	return BG95_MQTT_OK;
}

void bg95_mqtt_set_urc_cb(bg95_mqtt_callback_t cb)
{
	if (cb == 0)
	{
		return;
	}

	bg95_mqtt_handler_cb_set(cb);
}

// -------------------------------------------- PRIVATE FUNCTIONS IMPLEMENTATION

static int8_t bg95_mqtt_at_send(int8_t *at_cmd)
{
	at_client_t client = NULL;
	at_response_t resp;
	int8_t ret = 0;

	client = at_client_get_first();
	resp = at_create_resp(80, 0, rt_tick_from_millisecond(5000));
	at_exec_cmd(resp, (char *)at_cmd);
	LOG_D("AT: %s, %d, %d", resp->buf, resp->buf_size, resp->line_counts);
	if (client->resp_status == AT_RESP_OK)
	{
		LOG_W("%s ---------- OK", at_cmd);
		ret = 0;
	}
	else
	{
		LOG_E("%s ---------- Fail", at_cmd);
		ret = 1;
	}
	at_delete_resp(resp);

	return ret;
}

static int8_t bg95_mqtt_at_format_send(const char *format, ...)
{
	int8_t err = 0;
	int8_t buffer[512];
	va_list args;
	memset((char *)buffer, 0, 512);
	va_start(args, format);
	vsprintf((char *)buffer, format, args);
	err = bg95_mqtt_at_send(buffer);
	va_end(args);

	return err;
}

__attribute__((unused)) static int8_t bg95_mqtt_at_data_send(int8_t *at_cmd, int8_t *p_data)
{
	int8_t buf[326];
	at_client_t client = NULL;
	at_response_t resp;
	int8_t ret = 0;

	if (client)
		;
	client = at_client_get_first();
	resp = at_create_resp(80, 0, rt_tick_from_millisecond(1000));
	at_exec_cmd(resp, (char *)at_cmd);
	LOG_D("AT: %s, %d, %d", resp->buf, resp->buf_size, resp->line_counts);
	at_resp_parse_line_args(resp, 1, "%s", buf);
	if (strstr((char *)buf, (char *)">") != NULL)
	{
		LOG_W("Data mode enter");
		at_client_send((char *)p_data, strlen((char *)p_data));
		return ret;
	}

	at_delete_resp(resp);

	return ret;
}

static void fd_clean(BG95_MQTT_FD *fd)
{
	if (fd[0].fd == -1)
	{
		for (BG95_MQTT_CONN_FD i = 0; i < BG95_MQTT_MAX_BROKER_CONNECTIONS; i++)
		{
			BG95_MQTT_FD *fdTmp = &fd[i];
			fdTmp->fd = -1;
			// fdTmp->cfg = 0;
		}
	}
}

static BG95_MQTT_CONN_FD get_free_slot(BG95_MQTT_FD *fd)
{
	for (BG95_MQTT_CONN_FD i = 0; i < BG95_MQTT_MAX_BROKER_CONNECTIONS; i++)
	{
		BG95_MQTT_FD *fdTmp = &fd[i];
		if (fdTmp->fd == -1)
		{
			return i;
		}
	}
	return -1; // No free index found
}
	
static BG95_MQTT_CONN_FD get_matching_index(BG95_MQTT_CONN_FD fd)
{

	// if (config_descriptor[0].fd == -1)
	// {
	// 	return 0;
	// }
	for (BG95_MQTT_CONN_FD i = 0; i <= BG95_MQTT_MAX_BROKER_CONNECTIONS; i++)
	{
		if (config_descriptor[i].fd == fd)
		{
			return i;
		}
	}
	return -1; // No matching index found
}
 void clean_matching_index(uint8_t idx)
{

	config_descriptor[idx].cfg = 0;
		config_descriptor[idx].fd = -1;
		LOG_W("idx %d",idx);
}
// ----------------------------------------------------------------- END OF FILE
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */
