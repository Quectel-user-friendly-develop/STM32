#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S_EXAMPLE__
#include "main.h"
#include "cmsis_os.h"
#include "bg95_socket.h"
#include "bg95_net.h"
#include "ec20_socket.h"
#include "ec20_net.h"
#include "at_osal.h"
#include "at.h"
#include "debug_service.h"
#include "at_socket.h"
#include "broadcast_service.h"

#include "example_mqtt.h"

void test_topic_cb(int8_t *topic, int8_t *string_data, uint16_t string_len)
{
    LOG_D("[MQTT MSG]: Topic:%s, Data: %s, Data Len: %d\r\n", topic, string_data, string_len);
}



int mqtt_config_conn(uint8_t mode,void *cfg_struct)
{
    BG95_MQTT_CONN_FD local_conn;
    mqtt_test_config *config = (mqtt_test_config *)cfg_struct;
    LOG_D("Entering MQTT Demo");
    if (bg95_mqtt_config(&local_conn,mode, &config->conf_local_hq) != BG95_MQTT_OK)
    {
        LOG_D("Error initializing config");

    }
  
    bg95_mqtt_set_urc_cb(test_topic_cb);
    LOG_D("Configuring  MQTT");
 
    config->ssl_config.sslctxid=local_conn;
    if(config->ssl_config.sslenble)
    {
      bg95_mqtt_ssl_config(local_conn,&config->ssl_config);
    }
    
    if (bg95_mqtt_connect(local_conn) != BG95_MQTT_OK)
    {
        LOG_D("Problem with creating connection");
        return 0xff;
    }
    return local_conn;
}


#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S_EXAMPLE__ */
