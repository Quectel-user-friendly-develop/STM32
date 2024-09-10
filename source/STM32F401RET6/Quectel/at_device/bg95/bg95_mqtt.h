#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S__
/**
 * \file        bg95_mqtt.h
 * \brief       Handling mqtt transactions.
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

#ifndef _BG95_MQTT_H_
#define _BG95_MQTT_H_

#define TEMPLATE_CAP_VERSION   ("1.0.0")

// -------------------------------------------------------------------- INCLUDES

#include <stdint.h>
#include <at.h>
#include "at_osal.h"
#include "debug_service.h"
#include "broadcast_service.h"
#include "bg95_ssl.h"
// -------------------------------------------------------- MODULE CONFIGURATION

/**
 * @brief Structure for MQTT configuration parameters.
 *
 * This structure holds the configuration parameters required for setting up an MQTT connection.
 */
typedef struct {
    int8_t      server[50];                     // Broker server address or hostname
     uint8_t    server_type;
    uint16_t    port;                            // Broker port number
    int8_t      Client_ID[25];                  // Name of the device
    int8_t      username[25];                    // Username for authentication (if applicable)
    int8_t      password[25];                    // Password for authentication (if applicable)
         int8_t      ProductKey[25];                  // Name of the device
    int8_t      DeviceName[25];                    // Username for authentication (if applicable)
    int8_t      DeviceSecret[50];                    // Password for authentication (if applicable)
    // int8_t      cacert_root_pem_cert_path[100];  // Path to the root CA certificate in PEM format on the module's file system
    // int8_t      client_cert_pem_cert_path[100];  // Path to the client certificate in PEM format on the module's file system
    // int8_t      client_key_pem_cert_path[100];   // Path to the client private key in PEM format on the module's file system
} __attribute__((aligned(4))) BG95_MQTT_CONFIG_T;

struct mqtt_device
{
  
    struct at_client *client;                    /* AT Client object for AT device */
                         
    rt_event_t mqtt_urc_event;

};

/**
 * @brief Type definition for MQTT callback function pointer.
 *
 * This typedef defines a pointer to a callback function that handles MQTT events or notifications.
 * The callback function takes three parameters: topic, message payload, and payload length.
 *
 * @param topic Pointer to the MQTT topic string.
 * @param message Pointer to the MQTT message payload.
 * @param length Length of the MQTT message payload.
 */
typedef void (*bg95_mqtt_callback_t)(int8_t *topic, int8_t *message, uint16_t length);


// --------------------------------------------------------------- PUBLIC MACROS

/**
 * @brief Maximum number of broker connections supported by BG95 MQTT module.
 *
 * This macro defines the maximum number of broker connections that can be established
 * simultaneously by the BG95 MQTT module. Adjust this value according to the system's
 * requirements and resource constraints.
 */
#define BG95_MQTT_MAX_BROKER_CONNECTIONS 5


// ---------------------------------------------------------------- PUBLIC TYPES

/**
 * @brief Type definition for MQTT connection file descriptor.
 *
 * Represents a file descriptor for an MQTT connection.
 */
typedef int8_t BG95_MQTT_CONN_FD;

/**
 * @brief Enumeration representing MQTT error codes.
 */
typedef enum {
    BG95_MQTT_NOK = -1,      // Indicates an error failure.
    BG95_MQTT_OK = 0,        // Indicates success.
    BG95_MQTT_TCP_DIS_OK ,        // Indicates success.
    BG95_MQTT_TCP_DIS_FAIL ,        // Indicates success.
    BG95_MQTT_SLOTS_FULL     // Indicates that all descriptors from 0 to 5 are populated.
} BG95_MQTT_ERROR_T;

// ------------------------------------------------- PUBLIC FUNCTIONS PROTOTYPES


/**
 * @brief Configures the MQTT connection with the provided configuration structure.
 *
 * @param fd Pointer to the MQTT connection file descriptor.
 * @param cfg_struct Pointer to the MQTT configuration structure.
 * @return BG95_MQTT_ERROR_T indicating the success or failure of the operation.
 */
BG95_MQTT_ERROR_T bg95_mqtt_config(BG95_MQTT_CONN_FD *fd, uint8_t mode,BG95_MQTT_CONFIG_T *cfg_struct);

/**
 * @brief Configures SSL for the MQTT connection.
 *
 * @param fd MQTT connection file descriptor.
 * @return BG95_MQTT_ERROR_T indicating the success or failure of the operation.
 */
BG95_MQTT_ERROR_T bg95_mqtt_ssl_config(BG95_MQTT_CONN_FD fd,ql_SSL_Config *config);

/**
 * @brief Connects to the MQTT broker using the provided connection file descriptor.
 *
 * @param fd MQTT connection file descriptor.
 * @return BG95_MQTT_ERROR_T indicating the success or failure of the operation.
 */
BG95_MQTT_ERROR_T bg95_mqtt_connect(BG95_MQTT_CONN_FD fd);

/**
 * @brief Disconnects from the MQTT broker using the provided connection file descriptor.
 *
 * @param fd MQTT connection file descriptor.
 * @return BG95_MQTT_ERROR_T indicating the success or failure of the operation.
 */
BG95_MQTT_ERROR_T bg95_mqtt_disconnect(BG95_MQTT_CONN_FD fd);

/**
 * @brief Subscribes to a topic on the MQTT broker using the provided connection file descriptor.
 *
 * @param fd MQTT connection file descriptor.
 * @param topic Pointer to the topic string.
 * @return BG95_MQTT_ERROR_T indicating the success or failure of the operation.
 */
BG95_MQTT_ERROR_T bg95_mqtt_sub(BG95_MQTT_CONN_FD fd, const int8_t *topic);

/**
 * @brief Publishes data to a topic on the MQTT broker using the provided connection file descriptor.
 *
 * @param fd MQTT connection file descriptor.
 * @param topic Pointer to the topic string.
 * @param publish_data Pointer to the data to be published.
 * @return BG95_MQTT_ERROR_T indicating the success or failure of the operation.
 */
BG95_MQTT_ERROR_T bg95_mqtt_pub(BG95_MQTT_CONN_FD fd, const int8_t *topic, const int8_t *publish_data);

/**
 * @brief Sets a callback function to handle MQTT URC (Unsolicited Result Code) events.
 *
 * @param cb Pointer to the callback function.
 */
void bg95_mqtt_set_urc_cb(bg95_mqtt_callback_t cb);
int bg95_mqtt_event_send(struct mqtt_device *device, uint32_t event);

#endif // _BG95_MQTT_H_

// ----------------------------------------------------------------- END OF FILE
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */
