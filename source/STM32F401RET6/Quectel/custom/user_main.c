#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_EXAMPLE_MAIN__
#include "at_osal.h"
#include "debug_service.h"
#include "broadcast_service.h"
#include "user_main.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__
#include "bg95_net.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_PSM__
#include "bg95_psm.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_PSM__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FILESYSTEM_EXAMPLE__
#include "example_fs.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FILESYSTEM_EXAMPLE__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S_EXAMPLE__
#include "example_ftp.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S_EXAMPLE__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S_EXAMPLE__
#include "example_http.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S_EXAMPLE__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S_EXAMPLE__

#include "example_mqtt.h"
static int8_t test_mqtt_cold_start = -1;
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S_EXAMPLE__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK_EXAMPLE__
#include "example_network.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK_EXAMPLE__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_PSM_EXAMPLE__
#include "example_psm.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_PSM_EXAMPLE__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_TCP_SERVER_EXAMPLE__
#include "example_tcp_server.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_TCP_SERVER_EXAMPLE__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_TCP_CLIENT_EXAMPLE__
#include "example_tcp.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_TCP_CLIENT_EXAMPLE__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_UDP_SERVER_EXAMPLE__
#include "example_udp_server.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_UDP_SERVER_EXAMPLE__ */
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_UDP_CLIENT_EXAMPLE__
#include "example_udp.h"
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_UDP_CLIENT_EXAMPLE__ */

static osMsgQueId_t g_main_msg_id = NULL;

#define USER_FRIENDLY_PROJECT_POWER_PIN GPIO_PIN_0
#define USER_FRIENDLY_PROJECT_POWER_PORT GPIOC
#define USER_FRIENDLY_PROJECT_RESET_PIN GPIO_PIN_3
#define USER_FRIENDLY_PROJECT_RESET_PORT GPIOC
#define USER_FRIENDLY_PROJECT_UART_PORT "Uart2"

#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__
Module_Config user_module_config = {

    .workType = Fastly_mode,
    .apn = "ctnet",
    .mcc_mnc = "46011",
    .rat_mode = rat_set_auto,
    .lte_mode = lte_eMTC_NBlOT,
    .NB_scan_level = 0,

};
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__ */

void moudule_hardware_init(void)
{
    LOG_D("Now restart the module");
    HAL_GPIO_WritePin(USER_FRIENDLY_PROJECT_RESET_PORT, USER_FRIENDLY_PROJECT_RESET_PIN, GPIO_PIN_SET);
    osDelay(500);
    HAL_GPIO_WritePin(USER_FRIENDLY_PROJECT_RESET_PORT, USER_FRIENDLY_PROJECT_RESET_PIN, GPIO_PIN_RESET);
    osDelay(500);
    HAL_GPIO_WritePin(USER_FRIENDLY_PROJECT_POWER_PORT, USER_FRIENDLY_PROJECT_POWER_PIN, GPIO_PIN_SET);
    osDelay(500);
    HAL_GPIO_WritePin(USER_FRIENDLY_PROJECT_POWER_PORT, USER_FRIENDLY_PROJECT_POWER_PIN, GPIO_PIN_RESET);
    osDelay(2000);
    LOG_D("Now restart the module over");
}
/********************************************************TCP/UDP Test start***********************************************************************************************/
static void socket_service_proc(void *argument)
{
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__
    int ret;
    socket_test_config *config = (socket_test_config *)argument;
    ip_addr_t cur_addr = QL_bg95_net_get_ip();

    LOG_V("%s Start(heap size = %d)\r\n\r\n", __FUNCTION__, os_thread_get_free_heap_size());
    if (config->type == SOCKET_TCP)
    {
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_TCP_CLIENT_EXAMPLE__
        example_tcp_client_test(config->sin_port, config->sin_addr, config->loop_count, config->loop_interval);
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_TCP_CLIENT_EXAMPLE__ */
    }
    else if (config->type == SOCKET_UDP)
    {
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_UDP_CLIENT_EXAMPLE__
        example_udp_client_test(config->sin_port, config->sin_addr, config->loop_count, config->loop_interval);
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_UDP_CLIENT_EXAMPLE__ */
    }
    else if (config->type == SOCKET_TCP_SERVER)
    {
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_TCP_SERVER_EXAMPLE__
        strcpy(config->sin_addr, inet_ntoa(cur_addr.addr));
        example_tcp_server_test(config->sin_port, config->sin_addr, config->max_connect_num, config->loop_count, config->loop_interval);
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_TCP_SERVER_EXAMPLE__ */
    }
    else if (config->type == SOCKET_UDP_SERVER)
    {
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_UDP_SERVER_EXAMPLE__
        strcpy(config->sin_addr, inet_ntoa(cur_addr.addr));
        example_udp_server_test(config->sin_port, config->sin_addr, config->loop_count, config->loop_interval);
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_UDP_SERVER_EXAMPLE__ */
    }
    else
    {
        LOG_E("Invalid parameter %d", config->type);
    }
    LOG_V("%s over(heap size = %d)", __FUNCTION__, os_thread_get_free_heap_size());
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__ */
    os_thread_exit();
}

static int user_socket_test(socket_test_config *config)
{
    osThreadId_t thread_id = NULL;
    osThreadAttr_t thread_attr = {.stack_size = 256 * 20, .priority = osPriorityNormal};

    LOG_V("%s, heap size = %d", __FUNCTION__, os_thread_get_free_heap_size());
    // Create net service
    thread_id = os_thread_create(socket_service_proc, (void *)config, &thread_attr);
    if (NULL == thread_id)
    {
        LOG_E("thread_id thread could not start!");
        return -1;
    }
    LOG_I("%s over(%x, %d, heap size = %d)", __FUNCTION__, thread_id, config->type, os_thread_get_free_heap_size());
}
/********************************************************TCP/UDP Test end***********************************************************************************************/
/********************************************************HTTP/HTTPS Test start***********************************************************************************************/
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S_EXAMPLE__
static int user_http_test(http_test_config *config)
{
    osThreadId_t thread_id = NULL;
    osThreadAttr_t thread_attr = {.name = "Http_T", .stack_size = 256 * 40, .priority = osPriorityNormal};

    LOG_V("%s", __FUNCTION__);
    // Create http service
    thread_id = os_thread_create(example_http_test, (void *)config, &thread_attr);
    if (NULL == thread_id)
    {
        LOG_E("thread_id thread could not start!");
        return -1;
    }
    LOG_I("%s over(%x)", __FUNCTION__, thread_id);
}
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S_EXAMPLE__ */

static osMsgQueId_t g_bg95_mqtt_msg_id = NULL;
static int user_mqtt_test(mqtt_test_config *config)
{
    osThreadId_t thread_id = NULL;
    osThreadAttr_t thread_attr = {.name = "Mqtt_T", .stack_size = 256 * 10, .priority = osPriorityNormal};

    // 1. Creat msg queue
    g_bg95_mqtt_msg_id = OsalMsgQCreate(MAX_MSG_COUNT, sizeof(msg_node), NULL);
    if (NULL == g_bg95_mqtt_msg_id)
    {
        LOG_E("Create bg95_MQTT_service_create msg failed!");
        return -1;
    }
    LOG_V("%s", __FUNCTION__);
    // Create http service
    thread_id = os_thread_create(example_mqtt_test, (void *)config, &thread_attr);
    if (NULL == thread_id)
    {
        LOG_E("thread_id thread could not start!");
        return -1;
    }
    LOG_I("%s over(%x)", __FUNCTION__, thread_id);
}
/********************************************************HTTP/HTTPS Test end***********************************************************************************************/
/********************************************************FTP/FTPS Test start***********************************************************************************************/

/********************************************************FTP/FTPS Test end***********************************************************************************************/
void user_main(void *argv)
{
    int ret = -1;
    msg_node msgs;

    LOG_I("Welcome to the Quectel test program stack space");

    // 1.creat msg queue
    g_main_msg_id = OsalMsgQCreate(MAX_MSG_COUNT, sizeof(msg_node), NULL);
    if (NULL == g_main_msg_id)
    {
        LOG_E("Create system_init msg failed!");
        return -1;
    }
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_TFCARD__
    SD_INIT();
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_TFCARD__ */

    // 2.start debug service
    ret = debug_service_create();

    // 3. module hardware init
    moudule_hardware_init();
    at_client_init(USER_FRIENDLY_PROJECT_UART_PORT, 128);

    // 4. Initialize broadcast service
    ret = bcast_service_create();

// 5.start tcp server
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET__
    ret = bg95_socket_service_create();
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET__ */

// 6. Start net server
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__
    ret = QL_bg95_net_create(&user_module_config);
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__ */

// 7.start http server
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S__
    ret = bg95_http_service_create();
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S__ */

// 8. Start FTP server
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S_EXAMPLE__
    ret = bg95_ftp_service_create();
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S_EXAMPLE__ */

    // 9. register which broadcasts need to be processed
    bcast_reg_receive_msg(MSG_WHAT_BG95_MODULE_FAILURE, g_main_msg_id);
    bcast_reg_receive_msg(MSG_WHAT_MAIN_STATE, g_main_msg_id);
    bcast_reg_receive_msg(MSG_WHAT_BG95_SOCKET_INIT_SUCCESS, g_main_msg_id);
    bcast_reg_receive_msg(MSG_WHAT_BG95_FTP_CONNENT_SUCCESS, g_main_msg_id);
    bcast_reg_receive_msg(MSG_WHAT_BG95_NET_DATACALL_SUCCESS, g_main_msg_id);
    while (1)
    {
        ret = OsalMsgRcv(g_main_msg_id, (void *)&msgs, NULL, portMAX_DELAY);
        if (ret < 0)
        {
            LOG_E("Receive msg from broadcast thread error!");
            continue;
        }
        else
        {
            LOG_D("Receive broadcast msg is what=%d, arg1=%d, arg2=%d, arg3=%d", msgs.what, msgs.arg1, msgs.arg2, msgs.arg3);
            switch (msgs.what)
            {
            case MSG_WHAT_MAIN_STATE:
                LOG_D("Do something");
                break;

            case MSG_WHAT_BG95_NET_DATACALL_SUCCESS:
                LOG_I("Do your own business");
                break;

            case MSG_WHAT_BG95_SOCKET_INIT_SUCCESS:
                break;

            case MSG_WHAT_BG95_FTP_CONNENT_SUCCESS:
                LOG_D("MSG_WHAT_BG95_FTP_CONNENT_SUCCESS");
                break;

            case MSG_WHAT_BG95_MODULE_FAILURE:
                LOG_D("Do something");
                break;
            default:
                LOG_D("Unrecognized message");
                break;
            }
        }
    }
    LOG_V("%s over", __FUNCTION__);
    os_thread_exit();
}

/// @brief
/// @param argc
/// @param argv
/// @return
int user_main_test(s32_t argc, char *argv[])
{
    int32_t i, save_debug_flag;
    static socket_test_config config;
    // LOG_V("%s",__FUNCTION__);

    for (i = 0; i < argc; i++)
    {
        // LOG_V("%d = %s", i, argv[i]);
    }
    if (strcmp((const char *)argv[0], "help") == 0 && argv[1] == NULL)
    {
        LOG_I("---------------------------------------------------------------------------------------------------");
        LOG_I("| main:                                                                                            |");
        LOG_I("---------------------------------------------------------------------------------------------------|");
        LOG_I("| help                                                                                             |");
        LOG_I("| mqtt                                                                                             |");
        LOG_I("| ftp                                                                                              |");
        LOG_I("| http                                                                                             |");
        LOG_I("| file                                                                                             |");
        LOG_I("| socket                                                                                           |");
        LOG_I("| example:http help                                                                                |");
    }
    else if ((strcmp((const char *)argv[0], "file") == 0) && (strcmp((const char *)argv[1], "help") == 0))
    {
        LOG_I("  0.   query files");
        LOG_I("     example: file 0 \"*\"");
        LOG_I("  1.   del files");
        LOG_I("     example: file 1 \"123.txt\"");
        LOG_I("  2.   query free");
        LOG_I("     example: file 2 \"UFS\"");
        LOG_I("  3.   open files");
        LOG_I("     example: file 3 \"text\" 0");
        LOG_I("     example: file 3 filename  mode");
        LOG_I("     mode    Integer type. The open mode of the file");
        LOG_I("     0 If the file does not exist, it is created. If the file exists, it is opened directly. In any case, the file can be read and written.");
        LOG_I("     1 If the file does not exist, it is created. If the file exists, it is overwritten. In any case, the file can be read and written.");
        LOG_I("     2 If the file exists, it is opened directly and is read only. If the file does not exist,an error is returned.");
        LOG_I("     3 If the file does not exist, it is created. If the file exists, write data to the file. In any case, the file can be read and written.");
        LOG_I("  4.   wirte files");
        LOG_I("     example: file 4 1 5 12345");
        LOG_I("     example: file 4 filehandle length DATA");
        LOG_I("     <filehandle> Integer type. The handle of the file to be operated.");
        LOG_I("     <length>     Integer type. The length of the file to be written.");
        LOG_I("     <DATA>       WIRTE DATA");
        LOG_I("  5.   close files");
        LOG_I("     example: file 5 1");
        LOG_I("     example: file 5 filehandle ");
        LOG_I("  6.   read files");
        LOG_I("     example: file 6 1 5");
        LOG_I("     example: file 6 filehandle length");
    }
    else if ((strcmp((const char *)argv[0], "mqtt") == 0) && (strcmp((const char *)argv[1], "help") == 0))
    {
        LOG_I("  1.   open mqtt ");
        LOG_I("              mqtt test_type Server_type 0:ALP 1:other  Client_ID server port ProductKey/username DeviceName/password DeviceSecret sslenble ciphersuite seclevel sslversion");
        LOG_I("     example: mqtt 1 0 Test a1vvrmkn43t.iot-as-mqtt.cn-shanghai.aliyuncs.com 1883 a1vvrmkn43t NiFtKoHMcu6j0VIXtC6e 3115a9a768482d98a28d7390e7b9376b 0");
        LOG_I("  2.  sub topic");
        LOG_I("              mqtt test_type  mqtt_fd topic_name ");
        LOG_I("     example: mqtt 2 0 /a1vvrmkn43t/NiFtKoHMcu6j0VIXtC6e/user/tre1");
        LOG_I("  3.   public topic");
        LOG_I("              mqtt test_type  mqtt_fd topic_name mssagec");
        LOG_I("     example: mqtt 3 1 /a1vvrmkn43t/p1U1UtVAPjZhkOEZnlUt/user/get 111");
        LOG_I(" 4.   dis mqtt");
        LOG_I("              mqtt test_type  mqtt_fd ");
        LOG_I("     example: mqtt 4 1 ");
        LOG_I("     sslenble      : Whether ssl is enabled");
        LOG_I("                     0: Disable SSL");
        LOG_I("                     1: Enable SSL");
        LOG_I("            ciphersuite   : Numeric type in HEX format. SSL cipher suites");
        LOG_I("                            0x0035:TLS_RSA_WITH_AES_256_CBC_SHA");
        LOG_I("                            0x002F:TLS_RSA_WITH_AES_128_CBC_SHA");
        LOG_I("                            0x0005:TLS_RSA_WITH_RC4_128_SHA");
        LOG_I("                            0x0004:TLS_RSA_WITH_RC4_128_MD5");
        LOG_I("                            0x000A:TLS_RSA_WITH_3DES_EDE_CBC_SHA");
        LOG_I("                            0x003D:TLS_RSA_WITH_AES_256_CBC_SHA256");
        LOG_I("                            0xC002:TLS_ECDH_ECDSA_WITH_RC4_128_SHA");
        LOG_I("                            0xC003:TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA");
        LOG_I("                            0xC004:TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA");
        LOG_I("                            0xC005:TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA");
        LOG_I("                            0xC007:TLS_ECDHE_ECDSA_WITH_RC4_128_SHA");
        LOG_I("                            0xC008:TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA");
        LOG_I("                            0xC009:TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA");
        LOG_I("                            0xC00A:TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA");
        LOG_I("                            0xC011:TLS_ECDHE_RSA_WITH_RC4_128_SHA");
        LOG_I("                            0xC012:TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA");
        LOG_I("                            0xC013:TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA");
        LOG_I("                            0xC014:TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA");
        LOG_I("                            0xC00C:TLS_ECDH_RSA_WITH_RC4_128_SHA");
        LOG_I("                            0xC00D:TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA");
        LOG_I("                            0xC00E:TLS_ECDH_RSA_WITH_AES_128_CBC_SHA");
        LOG_I("                            0xC00F:TLS_ECDH_RSA_WITH_AES_256_CBC_SHA");
        LOG_I("                            0xC023:TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256");
        LOG_I("                            0xC024:TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384");
        LOG_I("                            0xC025:TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256");
        LOG_I("                            0xC026:TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384");
        LOG_I("                            0xC027:TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256");
        LOG_I("                            0xC028:TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384");
        LOG_I("                            0xC029:TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256");
        LOG_I("                            0xC02A:TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384");
        LOG_I("                            0xC02B:TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256");
        LOG_I("                            0xC02F:TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256");
        LOG_I("                            0xC0A8:TLS_PSK_WITH_AES_128_CCM_8");
        LOG_I("                            0x00AE:TLS_PSK_WITH_AES_128_CBC_SHA256");
        LOG_I("                            0xC0AE:TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8");
        LOG_I("                            0xFFFF:ALL");
        LOG_I("            seclevel      : Authentication mode");
        LOG_I("                            0: No authentication");
        LOG_I("                            1: Perform server authentication");
        LOG_I("                            2: Perform server and client authentication if requested by the remote server");
        LOG_I("            sslversion    : SSL Version");
        LOG_I("                            0: SSL3.0");
        LOG_I("                            1: TLS1.0");
        LOG_I("                            2: TLS1.1");
        LOG_I("                            3: TLS1.2");
        LOG_I("                            4: ALL");
    }
    else if ((strcmp((const char *)argv[0], "ftp") == 0) && (strcmp((const char *)argv[1], "help") == 0))
    {
        LOG_I("ftp contextid username password filetype transmode rsptimeout hostname port ftp_type directoryToSet local_name rem_name  sslenble ssltype sslctxid ciphersuite seclevel sslversion");
        LOG_I("     contextid     : PDP context ID");
        LOG_I("     username      : Username for logging in to the Ftp(S) server");
        LOG_I("     password      : Password for logging in to the Ftp(S) server");
        LOG_I("     file_type     : The type of transferred data.");
        LOG_I("     0: Binary 1: ASCII");
        LOG_I("     transmode     : Whether the FTP(S) server or client listens on a port for data connection.");
        LOG_I("     0: Active mode, the module will listen on a port for data connection");
        LOG_I("     1: Passive mode, the FTP(S) server will listen on a port for data connection");
        LOG_I("     rsptimeout       : Range: 20-180. Default value: 90. Unit: second.");
        LOG_I("     hostname       : FTP(S) server URL");
        LOG_I("     port           : FTP(S) server port");
        LOG_I("     ftp_type       : FTP fun mode");
        LOG_I("     1: file     list ");
        LOG_I("     2: file     get");
        LOG_I("     3: file     uploader,");
        LOG_I("     directoryToSet       : The directory of the server");
        LOG_I("     local_name  : Data path in SD card");
        LOG_I("     rem_name  : The file name of the server");
        LOG_I("     sslenble      : Whether ssl is enabled");
        LOG_I("                     0: Disable SSL");
        LOG_I("                     1: Enable SSL");
        LOG_I("            sslctype      : Module used as FTP client or FTPS client");
        LOG_I("                            0 FTP clients");
        LOG_I("                            1 FTPS implicit encryption");
        LOG_I("                            2 FTPS explicit encryption");
        LOG_I("            sslctxid      : SSL context ID used for FTPS(S)");
        LOG_I("            ciphersuite   : Numeric type in HEX format. SSL cipher suites");
        LOG_I("                            0x0035:TLS_RSA_WITH_AES_256_CBC_SHA");
        LOG_I("                            0x002F:TLS_RSA_WITH_AES_128_CBC_SHA");
        LOG_I("                            0x0005:TLS_RSA_WITH_RC4_128_SHA");
        LOG_I("                            0x0004:TLS_RSA_WITH_RC4_128_MD5");
        LOG_I("                            0x000A:TLS_RSA_WITH_3DES_EDE_CBC_SHA");
        LOG_I("                            0x003D:TLS_RSA_WITH_AES_256_CBC_SHA256");
        LOG_I("                            0xC002:TLS_ECDH_ECDSA_WITH_RC4_128_SHA");
        LOG_I("                            0xC003:TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA");
        LOG_I("                            0xC004:TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA");
        LOG_I("                            0xC005:TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA");
        LOG_I("                            0xC007:TLS_ECDHE_ECDSA_WITH_RC4_128_SHA");
        LOG_I("                            0xC008:TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA");
        LOG_I("                            0xC009:TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA");
        LOG_I("                            0xC00A:TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA");
        LOG_I("                            0xC011:TLS_ECDHE_RSA_WITH_RC4_128_SHA");
        LOG_I("                            0xC012:TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA");
        LOG_I("                            0xC013:TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA");
        LOG_I("                            0xC014:TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA");
        LOG_I("                            0xC00C:TLS_ECDH_RSA_WITH_RC4_128_SHA");
        LOG_I("                            0xC00D:TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA");
        LOG_I("                            0xC00E:TLS_ECDH_RSA_WITH_AES_128_CBC_SHA");
        LOG_I("                            0xC00F:TLS_ECDH_RSA_WITH_AES_256_CBC_SHA");
        LOG_I("                            0xC023:TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256");
        LOG_I("                            0xC024:TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384");
        LOG_I("                            0xC025:TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256");
        LOG_I("                            0xC026:TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384");
        LOG_I("                            0xC027:TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256");
        LOG_I("                            0xC028:TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384");
        LOG_I("                            0xC029:TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256");
        LOG_I("                            0xC02A:TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384");
        LOG_I("                            0xC02B:TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256");
        LOG_I("                            0xC02F:TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256");
        LOG_I("                            0xC0A8:TLS_PSK_WITH_AES_128_CCM_8");
        LOG_I("                            0x00AE:TLS_PSK_WITH_AES_128_CBC_SHA256");
        LOG_I("                            0xC0AE:TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8");
        LOG_I("                            0xFFFF:ALL");
        LOG_I("            seclevel      : Authentication mode");
        LOG_I("                            0: No authentication");
        LOG_I("                            1: Perform server authentication");
        LOG_I("                            2: Perform server and client authentication if requested by the remote server");
        LOG_I("            sslversion    : SSL Version");
        LOG_I("                            0: SSL3.0");
        LOG_I("                            1: TLS1.0");
        LOG_I("                            2: TLS1.1");
        LOG_I("                            3: TLS1.2");
        LOG_I("                            4: ALL");
    }
    else if ((strcmp((const char *)argv[0], "http") == 0) && (strcmp((const char *)argv[1], "help") == 0))
    {
        LOG_I("| http contextid requestheader responseheader contenttype custom_header timeout rsptime         \\ |");
        LOG_I("|    packets_space_time request_url method request_mode username password sd_card_path sslenble \\ |");
        LOG_I("|    sslctxid ciphersuite seclevel sslversion                                                      |");
        LOG_I("|      contextid     : PDP context ID                                                              |");
        LOG_I("|                      Range: 1-16                                                                 |");
        LOG_I("|      requestheader : Disable or enable customization of HTTP(S) request header                   |");
        LOG_I("|                      0: Disable                                                                  |");
        LOG_I("|                      1: Enable                                                                   |");
        LOG_I("|      responseheader: Disable or enable the outputting of HTTP(S) response header                 |");
        LOG_I("|                      0: Disable                                                                  |");
        LOG_I("|                      1: Enable                                                                   |");
        LOG_I("|      contenttype   : Data type of HTTP(S) body                                                   |");
        LOG_I("|                      0: application/x-www-form-urlencoded                                        |");
        LOG_I("|                      1: text/plain                                                               |");
        LOG_I("|                      2: application/octet-stream                                                 |");
        LOG_I("|                      3: multipart/form-data                                                      |");
        LOG_I("|                      4: application/json                                                         |");
        LOG_I("|                      5: image/jpeg                                                               |");
        LOG_I("|      custom_header : User-defined HTTP(S) request header                                         |");
        LOG_I("|      timeout       : The maximum time for inputting URL. Range: 1-2038. Unit: second             |");
        LOG_I("|      rsptime       : Timeout value for the HTTP(S) GET response,Range: 1-65535. Unit: second     |");
        LOG_I("| packets_space_time : Maximum time between receive two packets of data.Range: 1-65535.Unit: second|");
        LOG_I("|      request_url   : HTTP(S) server URL                                                          |");
        LOG_I("|      method        : Request type                                                                |");
        LOG_I("|                      0: Get                                                                      |");
        LOG_I("|                      1: Post                                                                     |");
        LOG_I("|      request_mode  : Request mode                                                                |");
        LOG_I("|                      0: Async                                                                    |");
        LOG_I("|                      1: Sync                                                                     |");
        LOG_I("|      username      : Username for logging in to the HTTP(S) server                               |");
        LOG_I("|      password      : Password for logging in to the HTTP(S) server                               |");
        LOG_I("|      sd_card_path  : Data path in SD card                                                        |");
        LOG_I("|      sslenble      : Whether ssl is enabled                                                      |");
        LOG_I("|                      0: Disable SSL                                                              |");
        LOG_I("|                      1: Enable SSL                                                               |");
        LOG_I("|             sslctxid      : SSL context ID used for HTTP(S)                                      |");
        LOG_I("|                             Range: 0-5                                                           |");
        LOG_I("|             ciphersuite   : Numeric type in HEX format. SSL cipher suites                        |");
        LOG_I("|                             0x0035:TLS_RSA_WITH_AES_256_CBC_SHA                                  |");
        LOG_I("|                             0x002F:TLS_RSA_WITH_AES_128_CBC_SHA                                  |");
        LOG_I("|                             0x0005:TLS_RSA_WITH_RC4_128_SHA                                      |");
        LOG_I("|                             0x0004:TLS_RSA_WITH_RC4_128_MD5                                      |");
        LOG_I("|                             0x000A:TLS_RSA_WITH_3DES_EDE_CBC_SHA                                 |");
        LOG_I("|                             0x003D:TLS_RSA_WITH_AES_256_CBC_SHA256                               |");
        LOG_I("|                             0xC002:TLS_ECDH_ECDSA_WITH_RC4_128_SHA                               |");
        LOG_I("|                             0xC003:TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA                          |");
        LOG_I("|                             0xC004:TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA                           |");
        LOG_I("|                             0xC005:TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA                           |");
        LOG_I("|                             0xC007:TLS_ECDHE_ECDSA_WITH_RC4_128_SHA                              |");
        LOG_I("|                             0xC008:TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA                         |");
        LOG_I("|                             0xC009:TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA                          |");
        LOG_I("|                             0xC00A:TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA                          |");
        LOG_I("|                             0xC011:TLS_ECDHE_RSA_WITH_RC4_128_SHA                                |");
        LOG_I("|                             0xC012:TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA                           |");
        LOG_I("|                             0xC013:TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA                            |");
        LOG_I("|                             0xC014:TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA                            |");
        LOG_I("|                             0xC00C:TLS_ECDH_RSA_WITH_RC4_128_SHA                                 |");
        LOG_I("|                             0xC00D:TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA                            |");
        LOG_I("|                             0xC00E:TLS_ECDH_RSA_WITH_AES_128_CBC_SHA                             |");
        LOG_I("|                             0xC00F:TLS_ECDH_RSA_WITH_AES_256_CBC_SHA                             |");
        LOG_I("|                             0xC023:TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256                       |");
        LOG_I("|                             0xC024:TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384                       |");
        LOG_I("|                             0xC025:TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256                        |");
        LOG_I("|                             0xC026:TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384                        |");
        LOG_I("|                             0xC027:TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256                         |");
        LOG_I("|                             0xC028:TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384                         |");
        LOG_I("|                             0xC029:TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256                          |");
        LOG_I("|                             0xC02A:TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384                          |");
        LOG_I("|                             0xC02B:TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256                       |");
        LOG_I("|                             0xC02F:TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256                         |");
        LOG_I("|                             0xC0A8:TLS_PSK_WITH_AES_128_CCM_8                                    |");
        LOG_I("|                             0x00AE:TLS_PSK_WITH_AES_128_CBC_SHA256                               |");
        LOG_I("|                             0xC0AE:TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8                            |");
        LOG_I("|             seclevel      : Authentication mode                                                  |");
        LOG_I("|                             0: No authentication                                                 |");
        LOG_I("|                             1: Perform server authentication                                     |");
        LOG_I("|                             2: Perform server and client authentication                          |");
        LOG_I("|             sslversion    : SSL Version                                                          |");
        LOG_I("|                             0: SSL3.0                                                            |");
        LOG_I("|                             1: TLS1.0                                                            |");
        LOG_I("|                             2: TLS1.1                                                            |");
        LOG_I("|                             3: TLS1.2                                                            |");
        LOG_I("|                             4: ALL                                                               |");

        LOG_I("----------------------------------------------------------------------------------------------------");
    }
    else if ((strcmp((const char *)argv[0], "psm") == 0) && (strcmp((const char *)argv[1], "help") == 0))
    {
        LOG_I("| psm                                                                                              |");
        LOG_I("| \tpsm enable/disable                                                                             |");
        LOG_I("| \tpsm settings - TAU/active time (ex setting 00000100 00001111))                                 |");
        LOG_I("|                             0: Requested Periodic TAU                                            |");
        LOG_I("|                             1: Requested Active Time                                             |");
        LOG_I("| \tpsm threshold - sets the minimum threshold value to enter PSM(ex threshold 100))               |");
        LOG_I("| \tpsm modem Optimization - sets the Modem Optimization (ex modem 2 2 120 5 120 3))               |");
        LOG_I("|                             0: PSM opt mask                                                      |");
        LOG_I("|                             1: PSM max oos full scans                                            |");
        LOG_I("|                             2: PSM duration due to oos                                           |");
        LOG_I("|                             3: PSM randomization window                                          |");
        LOG_I("|                             4: PSM max oos time                                                  |");
        LOG_I("|                             5: PSM early wakeup time                                             |");
        LOG_I("| \tpsm stat - show all psm setting                                                                |");
    }
    else if ((strcmp((const char *)argv[0], "socket") == 0) && (strcmp((const char *)argv[1], "help") == 0))
    {
        LOG_I("| socket socket_type ip port count interval_ms max_connect_num                                     |");
        LOG_I("|      socket_type   : socket type                                                                 |");
        LOG_I("|                      0: TCP                                                                      |");
        LOG_I("|                      1: UDP                                                                      |");
        LOG_I("|                      2: TCP SERVER                                                               |");
        LOG_I("|                      3: UDP SERVER                                                               |");
        LOG_I("|      ip            : ip address                                                                  |");
        LOG_I("|      port          : port                                                                        |");
        LOG_I("|      count         : Number of server cyclic forwarding times                                    |");
        LOG_I("|      interval_ms   : Interval for sending data                                                   |");
        LOG_I("|    max_connect_num : Max number connect request(only tcp server need set)                        |");
        LOG_I("|                                                                                                  |");
    }
    else if (strcmp((const char *)argv[0], "exit") == 0)
    {
        LOG_D("exit %s", __FUNCTION__);
        return -1;
    }
    else if (strcmp((const char *)argv[0], "mqtt") == 0)
    {
#if defined(__QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S__) && defined(__QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_MQTT_S_EXAMPLE__)
        //  example_mqtt_test(NULL);
        static mqtt_test_config mqtt_config;
        mqtt_config.mqtt_test_type = atoi(argv[1]);

        if (mqtt_config.mqtt_test_type == 1)
        {
            mqtt_config.conf_local_hq.server_type = atoi(argv[2]);
            u8_t fd;
            if (mqtt_config.Server_type == 0)
            {

                strcpy(mqtt_config.conf_local_hq.Client_ID, argv[3]);
                strcpy(mqtt_config.conf_local_hq.server, argv[4]);
                mqtt_config.conf_local_hq.port = atoi(argv[5]);

                strcpy(mqtt_config.conf_local_hq.ProductKey, argv[6]);
                strcpy(mqtt_config.conf_local_hq.DeviceName, argv[7]);
                strcpy(mqtt_config.conf_local_hq.DeviceSecret, argv[8]);
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__
                mqtt_config.ssl_config.sslenble = atoi(argv[9]);
                if (mqtt_config.ssl_config.sslenble)
                {

                    mqtt_config.ssl_config.ciphersuite = strtol(argv[10], NULL, 16);
                    mqtt_config.ssl_config.seclevel = atoi(argv[11]);
                    mqtt_config.ssl_config.sslversion = atoi(argv[12]);

                    LOG_I("            ciphersuite   : 0x%x", mqtt_config.ssl_config.ciphersuite);
                    LOG_I("            seclevel      : %d", mqtt_config.ssl_config.seclevel);
                    LOG_I("            sslversion    : %d", mqtt_config.ssl_config.sslversion);
                }

#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__ */
            }
            else
            {

                strcpy(mqtt_config.conf_local_hq.Client_ID, argv[3]);
                strcpy(mqtt_config.conf_local_hq.server, argv[4]);
                mqtt_config.conf_local_hq.port = atoi(argv[5]);

                strcpy(mqtt_config.conf_local_hq.username, argv[6]);
                strcpy(mqtt_config.conf_local_hq.password, argv[7]);

#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__
                mqtt_config.ssl_config.sslenble = atoi(argv[9]);
                if (mqtt_config.ssl_config.sslenble)
                {

                    mqtt_config.ssl_config.ciphersuite = strtol(argv[10], NULL, 16);
                    mqtt_config.ssl_config.seclevel = atoi(argv[11]);
                    mqtt_config.ssl_config.sslversion = atoi(argv[12]);

                    LOG_I("            ciphersuite   : 0x%x", mqtt_config.ssl_config.ciphersuite);
                    LOG_I("            seclevel      : %d", mqtt_config.ssl_config.seclevel);
                    LOG_I("            sslversion    : %d", mqtt_config.ssl_config.sslversion);
                }

#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__ */
            }
            LOG_W("server_type %d", mqtt_config.conf_local_hq.server_type);
            fd = mqtt_config_conn(mqtt_config.conf_local_hq.server_type, &mqtt_config);
            LOG_W("mqtt_config_conn %d", fd);
        }
        else if (mqtt_config.mqtt_test_type == 2)
        {
            u8_t fd;
            int8_t subbuf[50];
            strcpy(subbuf, argv[3]);
            fd = atoi(argv[2]);
            if (bg95_mqtt_sub(fd, (int8_t *)subbuf) != BG95_MQTT_OK)
            {
                LOG_D("Problem subscribing to topic");
            }
            else
            LOG_D("bg95_mqtt_sub success");
        }

        else if (mqtt_config.mqtt_test_type == 3)
        {
            u8_t fd;
            int8_t TOPIC[50];
            int8_t SNDBUF[50];
            strcpy(TOPIC, argv[3]);
            strcpy(SNDBUF, argv[4]);
            fd = atoi(argv[2]);
            if (bg95_mqtt_pub(fd, (int8_t *)TOPIC, SNDBUF) != BG95_MQTT_OK)
            {
                LOG_D("Problem subscribing to topic");
            }
            else
             LOG_D("bg95_mqtt_pub success");
        }
        else if (mqtt_config.mqtt_test_type == 4)
        {
            u8_t fd;
            fd = atoi(argv[2]);
            if (bg95_mqtt_disconnect(fd) != BG95_MQTT_OK)
            {
                LOG_D("bg95_mqtt_disconnect fail");
            }
            else
            {
                LOG_D("bg95_mqtt_disconnect success");
            }
        }

#else
        LOG_W("This function is not supported");
#endif
    }
    else if (strcmp((const char *)argv[0], "file") == 0)
    {
#if defined(__QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FILESYSTEM__) && defined(__QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FILESYSTEM_EXAMPLE__)
        static fs_test_config FS_config;
        FS_config.fs_type = atoi(argv[1]);
        if (FS_config.fs_type == 0)
        {
            strcpy(FS_config.name_pattern, argv[2]);
        }
        if (FS_config.fs_type == 1)
        {
            strcpy(FS_config.name_pattern, argv[2]);
        }
        if (FS_config.fs_type == 2)
        {
            strcpy(FS_config.name_pattern, argv[2]);
        }
        else if (FS_config.fs_type == 3)
        {
            strcpy(FS_config.name_pattern, argv[2]);
            FS_config.open_mode = atoi(argv[3]);
        }
        else if (FS_config.fs_type == 4)
        {

            FS_config.file_handle = atoi(argv[2]);
            FS_config.wirte_read_size = atoi(argv[3]);
            strcpy(FS_config.wirte_buffer, argv[4]);
        }
        else if (FS_config.fs_type == 5)
        {

            FS_config.file_handle = atoi(argv[2]);
        }
        else if (FS_config.fs_type == 6)
        {

            FS_config.file_handle = atoi(argv[2]);
            FS_config.wirte_read_size = atoi(argv[3]);
        }
        user_fs_test(&FS_config);
#else
        LOG_W("This function is not supported");
#endif
    }
    else if (strcmp((const char *)argv[0], "socket") == 0)
    {
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET__
        static socket_test_config config;

        config.type = atoi(argv[1]);
        strcpy(config.sin_addr, argv[2]);
        config.sin_port = atoi(argv[3]);
        config.loop_count = atoi(argv[4]);
        config.loop_interval = atoi(argv[5]);
        if (argc == 7)
            config.max_connect_num = atoi(argv[6]);
        LOG_I("type = %d, ip = %s, port = %d, loop_count = %d, loop_interval = %d, max_connect_num = %d", config.type, config.sin_addr, config.sin_port, config.loop_count, config.loop_interval, config.max_connect_num);
        user_socket_test(&config);
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET__ */
    }
    else if (strcmp((const char *)argv[0], "http") == 0)
    {
#if defined(__QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S__) && defined(__QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_HTTP_S_EXAMPLE__)
        static http_test_config http_config;
        http_config.param.contextid = atoi(argv[1]);
        http_config.param.requestheader = atoi(argv[2]);
        http_config.param.responseheader = atoi(argv[3]);
        http_config.param.contenttype = atoi(argv[4]);
        if (strlen(argv[5]) < 5)
            strcpy(http_config.param.custom_header, "");
        else
            strcpy(http_config.param.custom_header, argv[5]);

        http_config.param.timeout = atoi(argv[6]);
        http_config.param.rsptime = atoi(argv[7]);
        http_config.param.packets_space_time = atoi(argv[8]);

        strcpy(http_config.request.request_url, argv[9]);
        http_config.request.method = atoi(argv[10]);
        http_config.request.request_mode = atoi(argv[11]);
        strcpy(http_config.request.username, argv[12]);
        strcpy(http_config.request.password, argv[13]);
        strcpy(http_config.sd_card_path, argv[14]);

        LOG_I("     contextid     : %d", http_config.param.contextid);
        LOG_I("     requestheader : %d", http_config.param.requestheader);
        LOG_I("     responseheader: %d", http_config.param.responseheader);
        LOG_I("     contenttype   : %d", http_config.param.contenttype);
        LOG_I("     custom_header : %s", http_config.param.custom_header);
        LOG_I("     timeout       : %d", http_config.param.timeout);
        LOG_I("     rsptime       : %d", http_config.param.rsptime);
        LOG_I(" packets_space_time: %d", http_config.param.packets_space_time);
        LOG_I("     request_url   : %s", http_config.request.request_url);
        LOG_I("     method        : %d", http_config.request.method);
        LOG_I("     request_mode  : %d", http_config.request.request_mode);
        LOG_I("     username      : %s", http_config.request.username);
        LOG_I("     password      : %s", http_config.request.password);
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__
        LOG_I("     sslenble      : %d", http_config.request.ssl.sslenble);
        http_config.request.ssl.sslenble = atoi(argv[15]);
        if (http_config.request.ssl.sslenble == 1)
        {
            http_config.request.ssl.sslctxid = atoi(argv[16]);
            http_config.request.ssl.ciphersuite = strtol(argv[17], NULL, 16);
            http_config.request.ssl.seclevel = atoi(argv[18]);
            http_config.request.ssl.sslversion = atoi(argv[19]);
            LOG_I("            sslctxid      : %d", http_config.request.ssl.sslctxid);
            LOG_I("            ciphersuite   : 0x%x", http_config.request.ssl.ciphersuite);
            LOG_I("            seclevel      : %d", http_config.request.ssl.seclevel);
            LOG_I("            sslversion    : %d", http_config.request.ssl.sslversion);
        }
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__ */
        user_http_test(&http_config);
#else
        LOG_W("This function is not supported");
#endif
    }
    else if (strcmp((const char *)argv[0], "ftp") == 0)
    {
#if defined(__QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S__) && defined(__QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S_EXAMPLE__)
        static ftp_test_config FTP_config;
        FTP_config.context_id = atoi(argv[1]);
        strcpy(FTP_config.username, argv[2]);
        strcpy(FTP_config.password, argv[3]);
        FTP_config.filetype = atoi(argv[4]);
        FTP_config.transmode = atoi(argv[5]);
        FTP_config.rsptimeout = atoi(argv[6]);
        strcpy(FTP_config.request_url, argv[7]);
        FTP_config.port = atoi(argv[8]);
        FTP_config.ftp_type = atoi(argv[9]);
        //        if(FTP_config.ftp_type ==0)
        //       {
        strcpy(FTP_config.directoryToSet, argv[10]);
        strcpy(FTP_config.local_name, argv[11]);
        strcpy(FTP_config.rem_name, argv[12]);
        //       }
        LOG_I("     contextid         : %d", FTP_config.context_id);
        LOG_I("     username          : %s", FTP_config.username);
        LOG_I("     password          : %s", FTP_config.password);
        LOG_I("     filetype          : %d", FTP_config.filetype);
        LOG_I("     transmode         : %d", FTP_config.transmode);
        LOG_I("     rsptimeout        : %d", FTP_config.rsptimeout);
        LOG_I("     port              : %d", FTP_config.port);
        LOG_I("     ftp_type          : %d", FTP_config.ftp_type);
        LOG_I("     request_url       : %s", FTP_config.request_url);
        //   if(FTP_config.ftp_type == 0) {
        LOG_I("     directoryToSet   : %s", FTP_config.directoryToSet);
        LOG_I("     local_name       : %s", FTP_config.local_name);
        LOG_I("     rem_name         : %s", FTP_config.rem_name);
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__
        FTP_config.ssl_config.sslenble = atoi(argv[13]);
        if (FTP_config.ssl_config.sslenble)
        {
            FTP_config.ssl_config.ssltype = atoi(argv[14]);
            FTP_config.ssl_config.sslctxid = atoi(argv[15]);
            FTP_config.ssl_config.ciphersuite = strtol(argv[16], NULL, 16);
            FTP_config.ssl_config.seclevel = atoi(argv[17]);
            FTP_config.ssl_config.sslversion = atoi(argv[18]);
        }
        LOG_I("            ssltype      : %d", FTP_config.ssl_config.ssltype);
        LOG_I("            sslctxid      : %d", FTP_config.ssl_config.sslctxid);
        LOG_I("            ciphersuite   : 0x%x", FTP_config.ssl_config.ciphersuite);
        LOG_I("            seclevel      : %d", FTP_config.ssl_config.seclevel);
        LOG_I("            sslversion    : %d", FTP_config.ssl_config.sslversion);
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__ */
        user_ftp_test(&FTP_config);
#else
        LOG_W("This function is not supported");
#endif
    }
    else if (strcmp((const char *)argv[0], "psm") == 0)
    {
#if defined(__QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_PSM__) && defined(__QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_PSM_EXAMPLE__)
        static psm_setting psm_setting_test;
        static psm_threshold_setting psm_threshold_setting_test;
        static psm_ext_cfg psm_ext_cfg_test;

        if (strcmp((const char *)argv[1], "enable") == 0)
        {
            LOG_I("PSM mode enabled");
            psm_setting_test.Mode = true;
            QL_psm_settings_write(&psm_setting_test);
        }
        else if (strcmp((const char *)argv[1], "disable") == 0)
        {
            LOG_I("PSM mode disabled");
            psm_setting_test.Mode = false;
            QL_psm_settings_write(&psm_setting_test);
        }
        else if (strcmp((const char *)argv[1], "setting") == 0)
        {
            psm_setting_test.Mode = true;
            psm_setting_test.Requested_Periodic_TAU = atoi(argv[2]);
            psm_setting_test.Requested_Active_Time = atoi(argv[3]);

            LOG_I("PSM mode enabled");
            LOG_I("PSM requested periodic TAU: %08d", psm_setting_test.Requested_Periodic_TAU);
            LOG_I("PSM requested active time: %08d", psm_setting_test.Requested_Active_Time);

            QL_psm_settings_write(&psm_setting_test);
        }
        else if (strcmp((const char *)argv[1], "threshold") == 0)
        {
            psm_threshold_setting_test.threshold = atoi(argv[2]);
            LOG_I("PSM threshold: %d", psm_threshold_setting_test.threshold);
            QL_psm_threshold_settings_write(&psm_threshold_setting_test);
        }
        else if (strcmp((const char *)argv[1], "modem") == 0)
        {
            psm_ext_cfg_test.PSM_opt_mask = atoi(argv[2]);
            psm_ext_cfg_test.max_oos_full_scans = atoi(argv[3]);
            psm_ext_cfg_test.PSM_duration_due_to_oos = atoi(argv[4]);
            psm_ext_cfg_test.PSM_randomization_window = atoi(argv[5]);
            psm_ext_cfg_test.max_oos_time = atoi(argv[6]);
            psm_ext_cfg_test.early_wakeup_time = atoi(argv[7]);

            LOG_I("PSM opt mask: %d", psm_ext_cfg_test.PSM_opt_mask);
            LOG_I("PSM max oos full scans: %d", psm_ext_cfg_test.max_oos_full_scans);
            LOG_I("PSM duration due to oos: %d", psm_ext_cfg_test.PSM_duration_due_to_oos);
            LOG_I("PSM randomization window: %d", psm_ext_cfg_test.PSM_randomization_window);
            LOG_I("PSM max oos time: %d", psm_ext_cfg_test.max_oos_time);
            LOG_I("PSM early wakeup time: %d", psm_ext_cfg_test.early_wakeup_time);

            QL_psm_ext_cfg_write(&psm_ext_cfg_test);
        }
        else if (strcmp((const char *)argv[1], "stat") == 0)
            QL_psm_stat();
#else
        LOG_W("This function is not supported");
#endif
    }
    else
    {
        LOG_E("Invalid parameter:%s", argv[0]);
    }
    // LOG_V("%s over",__FUNCTION__);

    return 0;
}
#else
void user_main(void *argv)
{
}
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_EXAMPLE_MAIN__ */
