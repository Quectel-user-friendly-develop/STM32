#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S__
#include <at.h>
#include "cmsis_os.h"
#include "bg95_ftp.h"
#include "bg95_net.h"
#include "at_osal.h"
#include "debug_service.h"
#include "broadcast_service.h"
#include "at_socket_device.h"
#include "user_main.h"
#include "ff.h"
#include "example_ftp.h"
#define BG95_EVENT_FTP_OK (1L << (8 + 4))
#define BG95_EVENT_FTP_FIAL (1L << (8 + 5))
#define BG95_EVENT_FTP_UP_OK (1L << (8 + 6))
#define BG95_EVENT_FTP_UP_FIAL (1L << (8 + 7))
static osMsgQueId_t g_bg95_ftp_msg_id = NULL;
static osThreadId_t g_bg95_ftp_thread_id = NULL;
static struct ftp_device g_bg95_ftp_device = {0};
extern FTP_Config user_ftp_config;
// extern FIL SDFile11;
#if 1

static void at_ftp_errcode_parse(int result) // FTP
{
    switch (result)
    {
    case 0:
        LOG_D("%d : Operation successful", result);
        break;
    case 601:
        LOG_E("%d : Unknown error", result);
        break;
    case 602:
        LOG_E("%d : FTP(S) server blocked", result);
        break;
    case 603:
        LOG_E("%d : FTP(S) server busy", result);
        break;
    case 604:
        LOG_E("%d : DNS parse failed", result);
        break;
    case 605:
        LOG_E("%d : Network error", result);
        break;
    case 606:
        LOG_E("%d : Control connection closed.", result);
        break;
    case 607:
        LOG_E("%d : Data connection closed", result);
        break;
    case 608:
        LOG_E("%d : Socket closed by peer", result);
        break;
    case 609:
        LOG_E("%d : Timeout error", result);
        break;
    case 610:
        LOG_E("%d : Invalid parameter", result);
        break;
    case 611:
        LOG_E("%d : Failed to open file", result);
        break;
    case 612:
        LOG_E("%d : File position invalid", result);
        break;
    case 613:
        LOG_E("%d : File error", result);
        break;
    case 614:
        LOG_E("%d : Service not available, closing control connection", result);
        break;
    case 615:
        LOG_E("%d : Open data connection failed", result);
        break;
    case 616:
        LOG_E("%d : Connection closed; transfer aborted", result);
        break;
    case 617:
        LOG_E("%d : Requested file action not taken", result);
        break;
    case 618:
        LOG_E("%d : Requested action aborted: local error in processing", result);
        break;
    case 619:
        LOG_E("%d : Requested action not taken: insufficient system storage", result);
        break;
    case 620:
        LOG_E("%d : Syntax error, command unrecognized", result);
        break;
    case 621:
        LOG_E("%d : Syntax error in parameters or arguments", result);
        break;
    case 622:
        LOG_E("%d : Command not implemented", result);
        break;
    case 623:
        LOG_E("%d : Bad sequence of commands", result);
        break;
    case 624:
        LOG_E("%d : Command parameter not implemented", result);
        break;
    case 625:
        LOG_E("%d : Not logged in", result);
        break;
    case 626:
        LOG_E("%d : Need account for storing files", result);
        break;
    case 627:
        LOG_E("%d : Requested action not taken", result);
        break;
    case 628:
        LOG_E("%d : Requested action aborted: page type unknown", result);
        break;
    case 629:
        LOG_E("%d : Requested file action aborted", result);
        break;
    case 630:
        LOG_E("%d : Requested file name invalid", result);
        break;
    case 631:
        LOG_E("%d : SSL authentication failed", result);
        break;
    default:
        LOG_E("%d : Unknown err code", result);
        break;
    }
}

static void at_ftp_protocol_errcode_parse(int result) // FTP_Protocol
{
    switch (result)
    {
    case 421:
        LOG_E("%d : Service not available, closing control connection", result);
        break;
    case 425:
        LOG_E("%d : Open data connection failed", result);
        break;
    case 426:
        LOG_E("%d : Connection closed; transfer aborted", result);
        break;
    case 450:
        LOG_E("%d : Requested file action not taken", result);
        break;
    case 451:
        LOG_E("%d : Requested action aborted: local error in processing", result);
        break;
    case 452:
        LOG_E("%d : Requested action not taken: insufficient system storage", result);
        break;
    case 500:
        LOG_E("%d : Syntax error, command unrecognized", result);
        break;
    case 501:
        LOG_E("%d : Syntax error in parameters or arguments", result);
        break;
    case 502:
        LOG_E("%d : Command not implemented", result);
        break;
    case 503:
        LOG_E("%d : Bad sequence of commands", result);
        break;
    case 504:
        LOG_E("%d : Command parameter not implemented", result);
        break;
    case 530:
        LOG_E("%d : Not logged in", result);
        break;
    case 532:
        LOG_E("%d : Need account for storing files", result);
        break;
    case 550:
        LOG_E("%d : Requested action not taken: file unavailable", result);
        break;
    case 551:
        LOG_E("%d : Requested action aborted: page type unknown", result);
        break;
    case 552:
        LOG_E("%d : Requested file action aborted: exceeded storage allocation", result);
        break;
    case 553:
        LOG_E("%d : Requested action not taken: file name not allowed", result);
        break;
    default:
        LOG_E("%d : Unknown err code", result);
        break;
    }
}
#endif /* BG95_USING_FTP */
struct ftp_device *ftp_device_get(void)
{
    return &g_bg95_ftp_device;
}
// Function to register an FTP device.
// Takes a pointer to an AT (ATtention) client structure as an argument.
int ftp_device_register(struct at_client *client)
{
    // Declare a structure to hold event flag attributes.
    osEventFlagsAttr_t event_attr;
    memset(&event_attr, 0, sizeof(event_attr));
    // Set the name of the event attribute to "ftp_device".
    event_attr.name = "ftp_device";

    // Create an event flag for the FTP URC (Unsolicited Result Code) with the specified attributes.
    // This is likely for handling asynchronous events or notifications for the FTP device.
    g_bg95_ftp_device.ftp_urc_event = OsalEventNCreate(&event_attr);

    // Check if the event creation was unsuccessful (possibly due to no memory).
    if (g_bg95_ftp_device.ftp_urc_event == RT_NULL)
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
// This function does not take any arguments.
int ftp_device_unregister(void)
{
    // Check if the FTP URC (Unsolicited Result Code) event is initialized.
    if (g_bg95_ftp_device.ftp_urc_event)
    {
        // Delete or clean up the event associated with the FTP URC.
        // This is important to free up any resources used by the event.
        OsalEventDelete(g_bg95_ftp_device.ftp_urc_event);
    }

    // Return a success code indicating the operation completed successfully.
    return RT_EOK;
}

// Function to send an FTP-related event.
// It takes a pointer to an 'ftp_device' structure and an event code as arguments.
static int bg95_ftp_event_send(struct ftp_device *device, uint32_t event)
{
    // Log the function call with a verbose log level.
    // It logs the name of the function and the event code being sent.
    LOG_V("%s, 0x%x", __FUNCTION__, event);

    // Send the specified event to the FTP URC (Unsolicited Result Code) event handle of the device.
    // 'OsalEventSend' is a function that likely sends or signals an event in the system.
    // The return value of 'OsalEventSend' is cast to an int and returned by this function.
    return (int)OsalEventSend(device->ftp_urc_event, event);
}

// Function to receive an event for an FTP device.
// It takes a pointer to an 'ftp_device' structure, an event code, a timeout value, and an option flag as arguments.
static int bg95_ftp_event_recv(struct ftp_device *device, uint32_t event, uint32_t timeout, rt_uint8_t option)
{
    // Variable to store the received event.
    rt_uint32_t recved;

    // Log the function call with a verbose log level.
    // It logs the name of the function, event code, timeout value, and option flag.
    LOG_V("%s, event = 0x%x, timeout = %d, option = %d", __FUNCTION__, event, timeout, option);

    // Receive an event. The 'OsalEventRecv' function waits for a specific event to occur within the given timeout period.
    // The 'option' parameter is combined with the 'RT_EVENT_FLAG_CLEAR' flag to modify the behavior of the event wait.
    recved = OsalEventRecv(device->ftp_urc_event, event, option, timeout);

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

// Function to handle the URC for opening an FTP connection.
// Takes an AT client structure, a string of data, and its size as arguments.
static void urc_ftp_open_func(struct at_client *client, const char *data, rt_size_t size)
{
    // Variables to hold error codes.
    int err = 0, protocol_err = 0;

    // Pointer for FTP device structure, initialized to null.
    struct ftp_device *device = RT_NULL;

    // Assert to ensure that the data pointer is not null and size is not zero.
    RT_ASSERT(data && size);

    // Log the function call with a verbose log level.
    LOG_V("%s, size = %d", __FUNCTION__, size);

    // Retrieve the current FTP device context.
    device = ftp_device_get();
    // Check if the FTP device was not successfully retrieved.
    if (device == RT_NULL)
    {
        // Log an error message indicating the failure to get the device.
        LOG_E("get device failed.");
        return;
    }

    // Parse the data string to extract error codes using sscanf.
    // It expects the data in a specific format, "+QFTPOPEN: <err>,<protocol_err>".
    sscanf(data, "+QFTPOPEN: %d,%d", &err, &protocol_err);

    // Check if there was no error (err == 0) in opening the FTP connection.
    if (err == 0)
    {
        // Send an event indicating FTP open was successful.
        bg95_ftp_event_send(device, BG95_EVENT_FTP_OK);
    }
    else
    {
        // Parse and handle the AT command error code and protocol error.
        at_ftp_errcode_parse(err);
        at_ftp_protocol_errcode_parse(protocol_err);

        // Send an event indicating FTP open failed.
        bg95_ftp_event_send(device, BG95_EVENT_FTP_FIAL);
    }
}

// Function to handle the URC for FTP change working directory (CWD) operation.
static void urc_ftp_cwd_func(struct at_client *client, const char *data, rt_size_t size)
{
    int err = 0, protocol_err = 0;
    struct ftp_device *device = RT_NULL;

    // Asserting the validity of data and its size.
    RT_ASSERT(data && size);

    // Logging the function call and the size of the data.
    LOG_V("%s, size = %d", __FUNCTION__, size);

    // Retrieving the current FTP device context.
    device = ftp_device_get();
    if (device == RT_NULL)
    {
        // Logging error if the FTP device is not retrieved.
        LOG_E("get device failed.");
        return;
    }

    // Parsing the data string for error codes.
    sscanf(data, "+QFTPCWD: %d,%d", &err, &protocol_err);

    // Checking for errors and sending appropriate events.
    if (err == 0)
    {
        // Sending an event for successful CWD operation.
        bg95_ftp_event_send(device, BG95_EVENT_FTP_OK);
    }
    else
    {
        // Parsing and handling the error codes, and sending an event for failed CWD operation.
        at_ftp_errcode_parse(err);
        at_ftp_protocol_errcode_parse(protocol_err);
        bg95_ftp_event_send(device, BG95_EVENT_FTP_FIAL);
    }
}
// Function to handle the URC for FTP print working directory (PWD) operation.
static void urc_ftp_size_func(struct at_client *client, const char *data, rt_size_t size)
{
    int err = 0, protocol_err = 0;
    struct ftp_device *device = RT_NULL;
    rt_size_t qftpfile_size = 0; // Local buffer to store the path

    // Asserting the validity of data and its size.
    RT_ASSERT(data && size);

    // Logging the function call and the size of the data.
    LOG_V("%s, size = %d", __FUNCTION__, size);

    // Retrieving the current FTP device context.
    device = ftp_device_get();
    if (device == RT_NULL)
    {
        // Logging error if the FTP device is not retrieved.
        LOG_E("get device failed.");
        return;
    }

    // Checking for errors in the response data and parsing the directory path if no error.
    if (sscanf(data, "+QFTPSIZE: %d", &err) > 0 && err == 0)
    {
        if (sscanf(data, "+QFTPSIZE: 0,%d", &qftpfile_size) == 1)
        {
            // Free existing user_data memory if allocated.
            device->filesize = qftpfile_size;
            // if (device->user_data) {
            //     free(device->user_data);
            //     device->user_data = NULL;
            // }

            // // Allocating memory for path and storing it in user_data.
            // device->user_data = malloc(sizeof(rt_size_t));
            // if (device->user_data) {
            //     *(rt_size_t *)device->user_data = qftpfile_size;
            //      LOG_E("Failed to allocate memory for user_data %d %d.",qftpfile_size,(rt_size_t *)device->user_data);
            // } else {
            //     // Handling memory allocation failure.
            //     LOG_E("Failed to allocate memory for user_data.");
            //     // Perform application-specific error handling here.
            // }
        }
    }

    // Sending appropriate events based on error status.
    if (err == 0)
    {
        bg95_ftp_event_send(device, BG95_EVENT_FTP_OK);
    }
    else
    {
        at_ftp_errcode_parse(err);
        at_ftp_protocol_errcode_parse(protocol_err);
        bg95_ftp_event_send(device, BG95_EVENT_FTP_FIAL);
    }
}

// Function to handle the URC for FTP print working directory (PWD) operation.
static void urc_ftp_pwd_func(struct at_client *client, const char *data, rt_size_t size)
{
    int err = 0, protocol_err = 0;
    struct ftp_device *device = RT_NULL;
    char path_buffer[255]; // Local buffer to store the path

    // Asserting the validity of data and its size.
    RT_ASSERT(data && size);

    // Logging the function call and the size of the data.
    LOG_V("%s, size = %d", __FUNCTION__, size);

    // Retrieving the current FTP device context.
    device = ftp_device_get();
    if (device == RT_NULL)
    {
        // Logging error if the FTP device is not retrieved.
        LOG_E("get device failed.");
        return;
    }

    // Checking for errors in the response data and parsing the directory path if no error.
    if (sscanf(data, "+QFTPPWD: %d", &err) > 0 && err == 0)
    {
        if (sscanf(data, "+QFTPPWD: 0,%255s", path_buffer) == 1)
        {
            // Free existing user_data memory if allocated.
            if (device->user_data)
            {
                free(device->user_data);
                device->user_data = NULL;
            }

            // Allocating memory for path and storing it in user_data.
            device->user_data = malloc(strlen(path_buffer) + 1);
            if (device->user_data)
            {
                strcpy((char *)device->user_data, path_buffer);
            }
            else
            {
                // Handling memory allocation failure.
                LOG_E("Failed to allocate memory for user_data.");
                // Perform application-specific error handling here.
            }
        }
    }

    // Sending appropriate events based on error status.
    if (err == 0)
    {
        bg95_ftp_event_send(device, BG95_EVENT_FTP_OK);
    }
    else
    {
        at_ftp_errcode_parse(err);
        at_ftp_protocol_errcode_parse(protocol_err);
        bg95_ftp_event_send(device, BG95_EVENT_FTP_FIAL);
    }
}

static void urc_ftp_list_func(struct at_client *client, const char *data, rt_size_t size)
{
    int err = 0, protocol_err = 0;
    struct ftp_device *device = RT_NULL;

    // Ensure that 'data' and 'size' are valid inputs.
    RT_ASSERT(data && size);

    // Log a verbose message indicating the function is called and the data size.
    LOG_V("%s, size = %d", __FUNCTION__, size);

    // Get the FTP device instance.
    device = ftp_device_get();
    if (device == RT_NULL)
    {
        // Log an error message if getting the device fails.
        LOG_E("get device failed.");
        return;
    }

    // Parse the FTP response data in the format "+QFTPLIST: %d,%d".
    sscanf(data, "+QFTPLIST: %d,%d", &err, &protocol_err);

    if (err == 0)
    {
        // If 'err' is 0, FTP listing was successful, so send a success event.
        bg95_ftp_event_send(device, BG95_EVENT_FTP_OK);
    }
    else
    {
        // If 'err' is not 0, handle FTP error codes and send a failure event.
        at_ftp_errcode_parse(err);                        // Parse FTP error code.
        at_ftp_protocol_errcode_parse(protocol_err);      // Parse FTP protocol error code.
        bg95_ftp_event_send(device, BG95_EVENT_FTP_FIAL); // Send a failure event.
    }
}

static const struct at_urc urc_table[] =
    {
        // Entry 1: Handle the "+QFTPOPEN" URC (Unsolicited Response Code).
        // It uses "\r\n" as the delimiter and calls the function urc_ftp_open_func when matched.
        {"+QFTPOPEN", "\r\n", urc_ftp_open_func},

        // Entry 2: Handle the "+QFTPCWD" URC.
        // It uses "\r\n" as the delimiter and calls the function urc_ftp_cwd_func when matched.
        {"+QFTPCWD", "\r\n", urc_ftp_cwd_func},

        // Entry 3: Handle the "+QFTPPWD" URC.
        // It uses "\r\n" as the delimiter and calls the function urc_ftp_pwd_func when matched.
        {"+QFTPPWD", "\r\n", urc_ftp_pwd_func},
        {"+QFTPSIZE", "\r\n", urc_ftp_size_func},
        // Entry 4: Handle the "+QFTPLIST" URC.
        // It uses "\r\n" as the delimiter and calls the function urc_ftp_list_func when matched.
        {"+QFTPLIST", "\r\n", urc_ftp_list_func},
};

/******************************************************************************
 * function : send broadcast msg
 * return: err:FAILURE  ok:SUCCESS
 ******************************************************************************/
s32_t ftp_send_my_msg(osMsgQueId_t my_msg_id, s32_t what, s32_t arg1, s32_t arg2, s32_t arg3)
{
    s32_t ret;
    msg_node msg;

    // Log a verbose message to provide information about the function's inputs.
    LOG_V("%s: my_msg_id = 0x%x, what = 0x%x, arg1 = 0x%x, arg2 = 0x%x, arg3 = 0x%x", __FUNCTION__, my_msg_id, what, arg1, arg2, arg3);

    // Initialize the 'msg' structure with the provided parameters.
    msg.what = what;
    msg.arg1 = arg1;
    msg.arg2 = arg2;
    msg.arg3 = arg3;

    // Send the message to the specified message queue 'my_msg_id'.
    ret = OsalMsgSend(my_msg_id, (void *)&msg, 0, 0);

    // Check if the message sending was successful or not.
    if (ret < 0)
    {
        // Log an error message if sending the message failed.
        LOG_E("Send msg to myself failed!%d", ret);
        return -1; // Return an error code to indicate the failure.
    }

    // Return the result of the message sending operation.
    return ret;
}

/******************************************************************************
 * function : send broadcast msg
 * return: err:FAILURE  ok:SUCCESS
 ******************************************************************************/
s32_t ftp_send_bcast_msg(s32_t what, s32_t arg1, s32_t arg2, s32_t arg3)
{
    msg_node msg;

    // Log a verbose message to provide information about the function's inputs.
    LOG_V("%s: what = 0x%x, arg1 = 0x%x, arg2 = 0x%x, arg3 = 0x%x", __FUNCTION__, what, arg1, arg2, arg3);

    // Initialize a 'msg' structure with the provided parameters.
    msg.what = what;
    msg.arg1 = arg1;
    msg.arg2 = arg2;
    msg.arg3 = arg3;

    // Call the 'ftp_send_my_msg' function to send the message to a specific message queue ('g_bg95_ftp_msg_id').
    // This function essentially forwards the message to 'ftp_send_my_msg'.
    return ftp_send_my_msg(g_bg95_ftp_msg_id, what, arg1, arg2, arg3);
}

static int check_at_response(at_response_t resp, int *err_code)
{
    const char *line;
    *err_code = 0; // Initialize the error code to 0 (no error)

    // Loop through each line in the AT response.
    for (int i = 0; i < resp->line_counts; i++)
    {
        line = at_resp_get_line(resp, i + 1);

        // Check if the line contains "OK." If found, return 0 (no error).
        if (strstr(line, "OK"))
        {
            return 0; // No error
        }

        // Check if the line contains "+CME ERROR:" (an error indicator).
        if (strstr(line, "+CME ERROR:") != NULL)
        {
            // Parse the error code from the line.
            sscanf(line, "+CME ERROR: %d", err_code);
            return -1; // Error found
        }
    }

    // If neither "OK" nor "+CME ERROR:" is found in any line, return -1 (error).
    return -1; // No "OK" or error found, return error
}

// Implementation of the FTP configuration function
int QL_ftp_cfg(FTP_Config *config)
{
    if (!config)
    {
        LOG_E("FTP config is NULL.\n");
        return -1;
    }

    // Define an array of AT command strings for FTP configuration.
    const char *at_commands[] = {
        "AT+QFTPCFG=\"contextid\",%d",
        "AT+QFTPCFG=\"account\",\"%s\",\"%s\"",
        "AT+QFTPCFG=\"filetype\",%d",
        "AT+QFTPCFG=\"transmode\",%d",
        "AT+QFTPCFG=\"rsptimeout\",%d",
    };

    // Iterate through the AT command array for configuration.
    for (int i = 0; i < sizeof(at_commands) / sizeof(at_commands[0]); i++)
    {
        // Create an AT response object with a 400ms timeout.
        at_response_t resp = at_create_resp(32, 0, rt_tick_from_millisecond(5000));
        if (!resp)
        {
            LOG_E("Failed to create response object.\n");
            return -1;
        }

        int err_code;
        const char *command = at_commands[i];

        // Execute the appropriate AT command based on the current iteration.
        switch (i)
        {
        case 0:
            at_exec_cmd(resp, command, config->context_id);
            break;
        case 1:
            at_exec_cmd(resp, command, config->username, config->password);
            break;
        case 2:
            at_exec_cmd(resp, command, config->filetype);
            break;
        case 3:
            at_exec_cmd(resp, command, config->transmode);
            break;
        case 4:
            at_exec_cmd(resp, command, config->rsptimeout);
            break;
        }
        osDelay(500);
        // Check if the AT command execution was successful.
        // if (check_at_response(resp, &err_code) != 0) {
        //     LOG_E("Failed to execute command: %s, Error: %d\n", command, err_code);
        //     at_ftp_errcode_parse(err_code); // Handle CME errors
        //     at_delete_resp(resp); // Delete the response object and return an error.
        //     return -1;
        // }

        at_delete_resp(resp); // Release the response object.
    }
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__
    if (config->ssl_config.sslenble)
    {
        // ql_file_del();
        at_response_t resp = at_create_resp(32, 0, rt_tick_from_millisecond(400));
        if (!resp)
        {
            LOG_E("Failed to create response object.\n");
            return -1;
        }

        // Send the "ssltype" command
        at_exec_cmd(resp, "AT+QFTPCFG=\"ssltype\",%d", config->ssl_config.ssltype);
        int err_code;
        if (check_at_response(resp, &err_code) != 0)
        {
            LOG_E("Failed to execute SSL type command, Error: %d\n", err_code);
            at_ftp_errcode_parse(err_code); // Handle CME errors
            at_delete_resp(resp);           // Delete the response object and return an error.
            return -1;
        }

        // Send the "sslctxid" command
        at_exec_cmd(resp, "AT+QFTPCFG=\"sslctxid\",%d", config->ssl_config.sslctxid);
        if (check_at_response(resp, &err_code) != 0)
        {
            LOG_E("Failed to execute SSL context ID command, Error: %d\n", err_code);
            at_ftp_errcode_parse(err_code); // Handle CME errors
            at_delete_resp(resp);           // Delete the response object and return an error.
            return -1;
        }
        at_delete_resp(resp); // Delete the response object and return an error.

        // Configure SSL settings if ssltenble is set
        if (configure_ssl(&config->ssl_config) != 0)
        {
            LOG_E("SSL configuration failed.\n");
            return -1;
        }
    }
    else
    {

        at_response_t resp = at_create_resp(32, 0, rt_tick_from_millisecond(400));
        if (!resp)
        {
            LOG_E("Failed to create response object.\n");
            return -1;
        }

        at_exec_cmd(resp, "AT+QFTPCFG=\"ssltype\",%d", 0);
        int err_code;
        if (check_at_response(resp, &err_code) != 0)
        {
            LOG_E("Failed to execute SSL type command, Error: %d\n", err_code);
            at_ftp_errcode_parse(err_code); // Handle CME errors
            at_delete_resp(resp);           // Delete the response object and return an error.
            return -1;
        }
        at_delete_resp(resp); // Delete the response object and return an error.
    }
#endif        /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SSL__ */
              //   at_delete_resp(resp); // Release the response object.
    return 0; // If all configurations were successfully set, return 0.
}

// Implementation of the function to open an FTP connection
int QL_ftp_open(FTP_Config *config)
{
    uint32_t event = 0;

    int err;
    int result = 0;
    int event_result = 0;
    struct ftp_device *device = RT_NULL;

    // Get the FTP device instance.
    device = ftp_device_get();
    if (device == RT_NULL)
    {
        LOG_E("get device failed.");
        return -1;
    }

    // Check if the FTP configuration is valid.
    if (!config)
    {
        LOG_E("FTP config is NULL.\n");
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
    event = BG95_EVENT_FTP_OK | BG95_EVENT_FTP_FIAL;

    // Wait for the specified FTP open events.
    bg95_ftp_event_recv(device, event, 0, RT_EVENT_FLAG_OR);

    // Execute the AT command to open the FTP connection.
    at_exec_cmd(resp, "AT+QFTPOPEN=\"%s\",%d", config->hostname, config->port);

    int lineFound = 0;
    for (int i = 0; i < resp->line_counts; i++)
    {
        const char *line = at_resp_get_line(resp, i + 1);
        LOG_V("query_resp line [%d]: %s", i, line);

        // Check if there is a "+CME ERROR" line.
        if (strstr(line, "+CME ERROR:") != NULL)
        {
            sscanf(line, "+CME ERROR: %d", &err);
            at_ftp_errcode_parse(err); // Handle CME errors
            at_delete_resp(resp);
            return -1;
        }
    }

    /* waiting OK or failed result */
    event_result = bg95_ftp_event_recv(device,
                                       BG95_EVENT_FTP_OK | BG95_EVENT_FTP_FIAL, 20000, RT_EVENT_FLAG_OR);

    if (event_result < 0)
    {
        // Handle the case where waiting for the connection result times out.
        result = -RT_ETIMEOUT;
        goto __exit;
    }

    /* Check the result */
    if (event_result & BG95_EVENT_FTP_FIAL)
    {
        // Handle the case where the FTP connection fails.
        result = -RT_ERROR;
        goto __exit;
    }

__exit:
    at_delete_resp(resp);
    return result; // Operation succeeded
}

// Function to set the current directory of the FTP server
int QL_ftp_path_cfg(const char *path_name)
{
    int err, protocol_err;
    int event_result = 0;
    int result = 0;
    uint32_t event = 0;

    // Check if the path name is valid.
    if (!path_name)
    {
        LOG_E("FTP QL_ftp_path_cfg is NULL.\n");
        return -1;
    }

    struct ftp_device *device = RT_NULL;
    device = ftp_device_get();

    // Create an AT response object with a 3000ms timeout and space for 4 lines.
    at_response_t resp = at_create_resp(64, 0, rt_tick_from_millisecond(5000));
    if (!resp)
    {
        LOG_E("No memory for response object.\n");
        return -1;
    }
    event = BG95_EVENT_FTP_OK | BG95_EVENT_FTP_FIAL;
    bg95_ftp_event_recv(device, event, 0, RT_EVENT_FLAG_OR);
    // Execute the AT command to change the current directory of the FTP server.
    at_exec_cmd(resp, "AT+QFTPCWD=\"%s\"", path_name);

    int lineFound = 0;
    for (int i = 0; i < resp->line_counts; i++)
    {
        const char *line = at_resp_get_line(resp, i + 1);
        LOG_V("query_resp line [%d]: %s", i, line);

        // Check if there is a "+CME ERROR" line.
        if (strstr(line, "+CME ERROR:") != NULL)
        {
            sscanf(line, "+CME ERROR: %d", &err);
            at_ftp_errcode_parse(err); // Handle CME errors
            at_delete_resp(resp);
            return -1;
        }
    }

    /* waiting OK or failed result */
    event_result = bg95_ftp_event_recv(device,
                                       BG95_EVENT_FTP_OK | BG95_EVENT_FTP_FIAL, 30 * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR);
    if (event_result < 0)
    {
        // Handle the case where waiting for the result times out.
        result = -RT_ETIMEOUT;
        goto __exit;
    }

    /* check result */
    if (event_result & BG95_EVENT_FTP_FIAL)
    {
        // Handle the case where the FTP command fails.
        result = -RT_ERROR;
        goto __exit;
    }

__exit:
    at_delete_resp(resp);
    return result; // Operation succeeded
}

// Function to get the current directory of the FTP server
int QL_ftp_path_get(char *path_name, u8_t path_name_size)
{
    int err = 0, protocol_err = 0;
    uint32_t event = 0;
    int result = 0;
    int event_result = 0;
    struct ftp_device *device = RT_NULL;
    device = ftp_device_get();

    // Initialize path_name as an empty string.
    path_name[0] = '\0';

    // Create an AT response object with a 3000ms timeout.
    at_response_t resp = at_create_resp(64, 0, rt_tick_from_millisecond(5000));
    if (!resp)
    {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    event = BG95_EVENT_FTP_OK | BG95_EVENT_FTP_FIAL;
    bg95_ftp_event_recv(device, event, 0, RT_EVENT_FLAG_OR);
    // Execute the AT command to retrieve the current directory of the FTP server.
    at_exec_cmd(resp, "AT+QFTPPWD");

    int lineFound = 0;
    for (int i = 0; i < resp->line_counts; i++)
    {
        const char *line = at_resp_get_line(resp, i + 1);
        LOG_V("query_resp line [%d]: %s", i, line);

        // Check if there is a "+CME ERROR" line.
        if (strstr(line, "+CME ERROR:") != NULL)
        {
            sscanf(line, "+CME ERROR: %d", &err);
            at_ftp_errcode_parse(err); // Handle CME errors
            at_delete_resp(resp);
            return -1;
        }
    }

    /* waiting OK or failed result */
    event_result = bg95_ftp_event_recv(device,
                                       BG95_EVENT_FTP_OK | BG95_EVENT_FTP_FIAL, 30 * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR);
    if (event_result < 0)
    {
        // Handle the case where waiting for the result times out.
        result = -RT_ETIMEOUT;
        goto __exit;
    }

    /* check result */
    if (event_result & BG95_EVENT_FTP_FIAL)
    {
        result = -RT_ERROR;
        goto __exit;
    }
    else
    {
        if (device && device->user_data)
        {
            char *path = (char *)device->user_data;

            // Use strncpy and ensure the string is correctly terminated.
            if (path_name_size > 0)
            {
                strncpy(path_name, path, path_name_size - 1); // Reserve one character for '\0'
                LOG_D("query_resp line path_name : %s", path_name);
                path_name[path_name_size - 1] = '\0'; // Ensure the string is correctly terminated.
            }

            free(device->user_data);
            device->user_data = NULL;
        }
    }

__exit:
    at_delete_resp(resp);
    return result; // Operation succeeded
}

// Function to get the current directory of the FTP server
rt_size_t QL_ftp_get_size(char *file_name)
{
    int err = 0, protocol_err = 0;
    uint32_t event = 0;
    int result = 0;
    int event_result = 0;
    struct ftp_device *device = RT_NULL;
    device = ftp_device_get();
    rt_size_t file_size = 0;

    // Create an AT response object with a 3000ms timeout.
    at_response_t resp = at_create_resp(64, 0, rt_tick_from_millisecond(5000));
    if (!resp)
    {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    event = BG95_EVENT_FTP_OK | BG95_EVENT_FTP_FIAL;
    bg95_ftp_event_recv(device, event, 0, RT_EVENT_FLAG_OR);
    // Execute the AT command to retrieve the current directory of the FTP server.
    at_exec_cmd(resp, "AT+QFTPSIZE=\"%s\"\r\n", file_name);

    int lineFound = 0;
    for (int i = 0; i < resp->line_counts; i++)
    {
        const char *line = at_resp_get_line(resp, i + 1);
        LOG_V("query_resp line [%d]: %s", i, line);

        // Check if there is a "+CME ERROR" line.
        if (strstr(line, "+CME ERROR:") != NULL)
        {
            sscanf(line, "+CME ERROR: %d", &err);
            at_ftp_errcode_parse(err); // Handle CME errors
            at_delete_resp(resp);
            return -1;
        }
    }

    /* waiting OK or failed result */
    event_result = bg95_ftp_event_recv(device,
                                       BG95_EVENT_FTP_OK | BG95_EVENT_FTP_FIAL, 30 * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR);
    if (event_result < 0)
    {
        // Handle the case where waiting for the result times out.
        result = -RT_ETIMEOUT;
        goto __exit;
    }

    /* check result */
    if (event_result & BG95_EVENT_FTP_FIAL)
    {
        result = -RT_ERROR;
        goto __exit;
    }
    else
    {
        if (device)
        {
            file_size = device->filesize;
            device->filesize = 0;
            at_delete_resp(resp);
            return file_size; // Operation succeeded
        }
    }

__exit:
    at_delete_resp(resp);
    return result; // Operation succeeded
}
// Function to list the contents of a directory on the FTP server
int QL_ftp_list_get(const char *dirname, const char *local_name, FileInfo *fileList, u8_t maxFiles)
{
    int err = 0, protocol_err = 0;
    uint32_t event = 0;
    int result = 0;
    int event_result = 0;
    struct ftp_device *device = RT_NULL;
    device = ftp_device_get();

    // Create an AT response object with a 8000ms timeout.
    at_response_t resp = at_create_resp(2048, 0, portMAX_DELAY);
    if (!resp)
    {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    u8_t fileCount = 0;
    u8_t isListing = 0;
    event = BG95_EVENT_FTP_OK | BG95_EVENT_FTP_FIAL;
    bg95_ftp_event_recv(device, event, 0, RT_EVENT_FLAG_OR);
    // Construct the AT command based on the value of local_name.
    if (strcmp(local_name, "COM:") == 0)
    {
        at_exec_cmd(resp, "AT+QFTPLIST=\"%s\"", dirname);
    }
    else
    {
        at_exec_cmd(resp, "AT+QFTPLIST=\"%s\",\"%s\"", dirname, local_name);
    }

    int lineFound = 0;
    for (int i = 0; i < resp->line_counts; i++)
    {
        const char *line = at_resp_get_line(resp, i + 1);
        //    LOG_V("query_resp line [%d]: %s", i, line);

        // Check if there is a "+CME ERROR" line.
        if (strstr(line, "+CME ERROR:") != NULL)
        {
            sscanf(line, "+CME ERROR: %d", &err);
            at_ftp_errcode_parse(err); // Handle CME errors
            at_delete_resp(resp);
            return -1;
        }

        if (strstr(line, "CONNECT") != NULL)
        {
            isListing = 1;
            continue;
        }

        if (strstr(line, "OK") != NULL)
        {
            break;
        }

        if (isListing && fileCount < maxFiles)
        {
            char month[20], day[20], year[20];
            long filesizeBytes; // Temporary variable to store byte size

            if (sscanf(line, "%*s %*d %*s %*s %ld %s %s %s %s",
                       &filesizeBytes,
                       month, day, year,
                       fileList[fileCount].filename) == 5)
            {
                // Convert bytes to kilobytes
                fileList[fileCount].filesize = filesizeBytes / 1024;
                snprintf(fileList[fileCount].date, sizeof(fileList[fileCount].date),
                         "%s %s %s", month, day, year);
                fileCount++;
            }
            else
            {
                LOG_E("Warning: Failed to parse line: %s\n", line);
            }
        }
    }

    if (!isListing)
    {
        LOG_E("Error: CONNECT not found in response.\n");
        at_delete_resp(resp);
        return -1;
    }

    if (fileCount == 0)
    {
        LOG_V("No files found in the directory.\n");
    }

    /* waiting OK or failed result */
    event_result = bg95_ftp_event_recv(device,
                                       BG95_EVENT_FTP_OK | BG95_EVENT_FTP_FIAL, 30 * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR);
    if (event_result < 0)
    {
        // Handle the case where waiting for the result times out.
        result = -RT_ETIMEOUT;
        goto __exit;
    }

    /* check result */
    if (event_result & BG95_EVENT_FTP_FIAL)
    {
        result = -RT_ERROR;
        goto __exit;
    }

__exit:
    at_delete_resp(resp);
    return fileCount; // Return the number of files listed
}

// Function to print a list of files with their details
void PrintFileList(FileInfo *fileList, int fileCount)
{
    for (int i = 0; i < fileCount; i++)
    {
        // Print the filename, size in kilobytes, and date for each file.
        LOG_V("Filename: %s       , KB: %ld        , Date: %s\n",
              fileList[i].filename,
              fileList[i].filesize,
              fileList[i].date);
    }
}

#define READ_DATA_LEN 1024 // ?????????

void bg95_ftp_client_downloader_func(const char *data, rt_size_t len)
{
    char read_data[READ_DATA_LEN] = {0}; // Buffer to store incoming data

    // rt_size_t buffered_data_len = 0;            // Length of data in the buffer
    rt_bool_t is_data_start = FALSE; // Flag to indicate the start of data

    rt_size_t total_data_written = 0; // Total data written
                                      // rt_size_t qftpget_size = 0;                 // Size received from "+QFTPGET:" response
    rt_size_t file_size = 0;          // Size received from "+QFTPGET:" response
    struct ftp_device *device = RT_NULL;

    // Get the FTP device
    device = ftp_device_get();
    if (device == RT_NULL)
    {
        LOG_E("get device failed.");
        return;
    }
    file_size = device->filesize;
    // Loop until the end of data is detected
    while (!is_data_start)
    {
        at_self_recv(read_data, 10, 500, 1);

        // Check if "CONNECT\r\n" is detected, indicating the start of data
        if (!is_data_start && strstr(read_data, "CONNECT\r\n"))
        {
            is_data_start = TRUE;
            break;
        }
        else
        {
            LOG_E("downloader fail ");
            break;
        }
    }
    if (is_data_start)
    {
        // 接收文件数据
        while (total_data_written < file_size)
        {
            rt_size_t read_data_len = at_self_recv(read_data, sizeof(read_data), 1000, 0);
            if (read_data_len > 0)
            {
                if (total_data_written + read_data_len > file_size)
                {
                    read_data_len = file_size - total_data_written; // 调整长度以匹配文件大小
                }
                user_ftp_config.write_cb(read_data, 1, read_data_len, &user_ftp_config.file);
                total_data_written += read_data_len;
            }
            else
            {
                LOG_E("down over ");
                break;
            }
        }
    }



    // Close the file
    f_close(&user_ftp_config.file);
    device->filesize = 0;


    // Compare the downloaded size with the value from the "+QFTPGET" response
    if (total_data_written == file_size)
    {
        LOG_I("Download successful. Total size: %d bytes.", total_data_written);
        bg95_ftp_event_send(device, BG95_EVENT_FTP_OK);
    }
    else
    {
        LOG_E("Download size mismatch. Expected: %d bytes, Received: %d bytes.", file_size, total_data_written);
        bg95_ftp_event_send(device, BG95_EVENT_FTP_FIAL);
    }
}

void bg95_ftp_client_uploader_func(const char *data, rt_size_t len)
{
    static rt_bool_t is_data_start = FALSE;       // Flag to indicate if data transmission has started
    char read_data[128] = {0};                    // Buffer to read data from the modem
    UINT bw = 0;                                  // Unused variable
    rt_size_t total_data_written = 0;             // Total bytes of data sent
    rt_size_t qftpget_size = 0;                   // Size of the data received from QFTPPUT
    int err = 0;                                  // Error code
    struct ftp_device *device = ftp_device_get(); // Get FTP device structure

    if (device == RT_NULL)
    {
        LOG_E("get device failed.");
        return -1; // Return error code
    }

    // Check if "CONNECT\r\n" is detected, indicating the start of data
    if (!is_data_start)
    {
        at_self_recv(read_data, READ_DATA_LEN, 2000, 1); // Receive data from the modem with a timeout of 2000ms
        if (strstr(read_data, "CONNECT\r\n"))
        {
            is_data_start = TRUE; // Set the flag to indicate data transmission has started
        }
    }

    // If data transmission has started, read and send the file data
    if (is_data_start)
    {
        unsigned char buffer[128]; // Buffer for reading file data
        size_t readBytes;          // Number of bytes read from the file

        while ((readBytes = user_ftp_config.read_cb(buffer, 1, sizeof(buffer), &user_ftp_config.file)) > 0)
        {
            at_client_send(buffer, readBytes); // Send data to the modem
            total_data_written += readBytes;   // Update the total bytes sent
        }

        // Send exit command and wait for "OK"
        osDelay(3000);            // Delay for 1500ms
        at_client_send("+++", 3); // Send "+++" to exit data mode

        // Wait for "+QFTPPUT" response
        while (1)
        {
            rt_size_t read_data_len = at_self_recv(read_data, 6, 2000, 1); // Receive data with a timeout of 2000ms

            char *qftpget_pos = strstr(read_data, "OK\r\n"); // Check for "OK\r\n" response
            if (qftpget_pos)
            {
                break; // Exit the loop when "OK\r\n" is found
            }
        }
        f_close(&user_ftp_config.file);
        // Wait for "+QFTPPUT" response
        while (1)
        {
            rt_size_t read_data_len = at_self_recv(read_data, READ_DATA_LEN, 2000, 1); // Receive data with a timeout of 2000ms

            char *qftpget_pos = strstr(read_data, "+QFTPPUT:"); // Check for "+QFTPPUT:" response
            if (qftpget_pos)
            {
                sscanf(qftpget_pos, "+QFTPPUT: %d,%d", &err, &qftpget_size); // Parse the error code and size
                break;                                                       // Exit the loop when the response is found
            }
            osDelay(2000); // Delay for 2000ms
        }

        // Check if the upload was successful and if the total data written matches the expected size
        if (err == 0 && total_data_written == qftpget_size)
        {
            LOG_I("Upload successful. Total size: %d bytes.", total_data_written);
            bg95_ftp_event_send(device, BG95_EVENT_FTP_UP_OK); // Send FTP upload success event
        }
        else
        {
            LOG_E("Upload failed or size mismatch.");
            bg95_ftp_event_send(device, BG95_EVENT_FTP_UP_FIAL); // Send FTP upload failure event
        }
    }
}

// ql_ftp_client_put_ex function, uploads a file using a callback function
int ql_ftp_client_put_ex(char *localfile, char *remotefile, QL_FTP_CLIENT_READ_CB_EX read_cb)
{

    uint32_t event = 0;
    int result = 0;
    int event_result = 0;
    int err = 0, err_code = 0;
    // Get the FTP device
    LOG_I("ql_ftp_client_put_ex.");
    struct ftp_device *device = ftp_device_get();
    if (device == RT_NULL)
    {
        LOG_E("get device failed.");
        return -1;
    }

    // Open the local file for reading
    FRESULT fr = f_open(&user_ftp_config.file, localfile, FA_READ);
    if (fr != FR_OK)
    {
        LOG_E("Failed to open file for reading.\n");
        return -1;
    }

    user_ftp_config.read_cb = read_cb;

    // Create an AT response object with a custom self-function handler (bg95_ftp_client_downloader_func)
    at_response_t resp = at_create_resp_by_selffunc(64, 0, rt_tick_from_millisecond(60000), bg95_ftp_client_uploader_func);
    if (!resp)
    {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Define the events to wait for
    event = BG95_EVENT_FTP_UP_OK | BG95_EVENT_FTP_UP_FIAL;
    bg95_ftp_event_recv(device, event, 0, RT_EVENT_FLAG_OR);
    at_exec_cmd(resp, "AT+QFTPPUT=\"%s\",\"%s\",0\r\n", remotefile, "COM:");

    // Wait for the upload result
    event_result = bg95_ftp_event_recv(device, BG95_EVENT_FTP_UP_OK | BG95_EVENT_FTP_UP_FIAL, 60 * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR);
    if (event_result < 0)
    {
        result = -RT_ETIMEOUT;
        goto __exit;
    }

    // Check the upload result
    if (event_result & BG95_EVENT_FTP_UP_FIAL)
    {
        result = -RT_ERROR;
        goto __exit;
    }

__exit:
    at_delete_resp(resp);
    return result;
}

int ql_ftp_client_get_ex(char *remotefile, char *localfile, QL_FTP_CLIENT_WRITE_CB_EX write_cb)
{
    struct ftp_device *device = RT_NULL;
    uint32_t event = 0;
    int result = 0;
    int event_result = 0;
    rt_size_t FILE_SIZE = 0;
    // Get the FTP device
    device = ftp_device_get();
    if (device == RT_NULL)
    {
        LOG_E("get device failed.");
        return -1;
    }
    FILE_SIZE = QL_ftp_get_size(remotefile);
    if (FILE_SIZE <= 0)
    {
        LOG_E("get FILE_SIZE failed.");
        return -1;
    }
    device->filesize = FILE_SIZE;
    // Get the first AT client
    at_client_t client = at_client_get_first();

    // Create an AT response object with a custom self-function handler (bg95_ftp_client_downloader_func)
    at_response_t resp = at_create_resp_by_selffunc(64, 1, rt_tick_from_millisecond(60000), bg95_ftp_client_downloader_func);
    if (!resp)
    {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Set the write callback function
    user_ftp_config.write_cb = write_cb;

    // Open the local file for writing
    FRESULT fr = f_open(&user_ftp_config.file, localfile, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK)
    {
        LOG_E("Failed to open file for writing.\n");
        return -1;
    }

    event = BG95_EVENT_FTP_OK | BG95_EVENT_FTP_FIAL;
    bg95_ftp_event_recv(device, event, 0, RT_EVENT_FLAG_OR);
    // Execute the FTP GET command
    at_exec_cmd(resp, "AT+QFTPGET=\"%s\",\"%s\",0\r\n", remotefile, "COM:");

    // Wait for the download result
    event_result = bg95_ftp_event_recv(device, BG95_EVENT_FTP_OK | BG95_EVENT_FTP_FIAL, 60 * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR);
    if (event_result < 0)
    {
        result = -RT_ETIMEOUT;
        goto __exit;
    }

    // Check the download result
    if (event_result & BG95_EVENT_FTP_FIAL)
    {
        result = -RT_ERROR;
        goto __exit;
    }

__exit:
    at_delete_resp(resp);
    return result;
}
int ql_ftp_close(void)
{
    // Creating a response object for the AT command
    at_response_t query_resp = NULL;

    // Creating a response object for the AT command
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(1000));

    // Check if memory allocation for the response object was successful
    if (query_resp == NULL)
    {
        LOG_E("Memory allocation for response object failed.\n");
        return -1;
    }

    // Sending the AT command to delete all files using QFDEL
    if (at_exec_cmd(query_resp, "AT+QFTPCLOSE") < 0)
    {
        LOG_E("AT+QFTPCLOSE command execution failed.\n");
        at_delete_resp(query_resp);
        return -1;
    }

    // Check if the expected format is not found in the response
    LOG_I("AT+QFTPCLOSE execution successful.\n");

    // Cleanup: Delete the response object
    at_delete_resp(query_resp);

    // Return 0 on success
    return 0;
}

static void bg95_ftp_service_proc(void *argument)
{
    int ret;
    msg_node msgs;
    // FTP_Config myConfig = Ql_get_ftp_Config();

    LOG_V("%s, stack space %d", __FUNCTION__, os_thread_get_stack_space(os_thread_get_thread_id()));

    // A service that provides network socket interface and keeps the services provided effective
    while (1)
    {
        ret = OsalMsgRcv(g_bg95_ftp_msg_id, (void *)&msgs, NULL, portMAX_DELAY);
        if (ret < 0)
        {
            LOG_E("Receive msg from broadcast thread error!");
            continue;
        }
        else
        {
            LOG_D("Receive broadcast msg is what=0x%x, arg1=0x%x, arg2=0x%x, arg3=0x%x", msgs.what, msgs.arg1, msgs.arg2, msgs.arg3);
            switch (msgs.what)
            {
            case MSG_WHAT_BG95_NET_DATACALL_SUCCESS:
                // Handle a message indicating successful data call establishment

                // if (QL_ftp_cfg(&myConfig) != 0) {
                //     LOG_D("QL_ftp_cfg error");
                // } else {
                //     ftp_send_bcast_msg(MSG_WHAT_BG95_FTP_INIT_SUCCESS, 0, 0, 0); // Send FTP initialization successful broadcast
                //     break;
                // }
                break;

            case MSG_WHAT_BG95_FTP_INIT_SUCCESS:
                // Handle a message indicating successful FTP initialization

                // if (QL_ftp_open(&myConfig) != 0) {
                //     LOG_D("QL_ftp_open error");
                // } else {
                //     LOG_D("QL_ftp_open ok");
                //     bcast_send_bcast_msg(MSG_WHAT_BG95_FTP_CONNENT_SUCCESS, 0, 0, 0); // Send FTP connection successful broadcast
                // }
                break;
            case MSG_WHAT_BG95_FTP_UP_SUCCESS:
                LOG_I("MSG_WHAT_BG95_FTP_UP_SUCCESS!");
                break;
            case MSG_WHAT_BG95_FTP_UP_FAIL:
                LOG_I("MSG_WHAT_BG95_FTP_UP_FAIL!");
                break;

            case MSG_WHAT_BG95_FTP_GET_SUCCESS:
                LOG_I("MSG_WHAT_BG95_FTP_GET_SUCCESS!");
                break;
            case MSG_WHAT_BG95_FTP_GET_FAIL:
                LOG_I("MSG_WHAT_BG95_FTP_GET_FAIL!");
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

int bg95_ftp_service_create(void)
{
    int ret;
    static osThreadAttr_t thread_attr = {.name = "Ftp_S", .stack_size = 512 * 6, .priority = osPriorityNormal};

    LOG_V("%s", __FUNCTION__);
    at_client_t client = at_client_get_first();
    // 1. Creat msg queue
    g_bg95_ftp_msg_id = OsalMsgQCreate(MAX_MSG_COUNT, sizeof(msg_node), NULL);
    if (NULL == g_bg95_ftp_msg_id)
    {
        LOG_E("Create bg95_socket_service_create msg failed!");
        return -1;
    }

    // 2. Register which broadcasts need to be processed
    // bcast_reg_receive_msg(MSG_WHAT_BG95_NET_DATACALL_SUCCESS,  g_bg95_ftp_msg_id);

    ftp_device_register(client);
    at_set_urc_table(urc_table, sizeof(urc_table) / sizeof(urc_table[0]));

    // 3. Create ftp service
    g_bg95_ftp_thread_id = os_thread_create(bg95_ftp_service_proc, NULL, &thread_attr);
    if (NULL == g_bg95_ftp_thread_id)
    {
        LOG_E("bg95_socket_service_create thread could not start!");
        return -1;
    }

    LOG_I("%s over", __FUNCTION__);

    return 0;
}

int bg95_ftp_service_destroy(void)
{
    int ret;
    osThreadId_t thread;
    char name[RT_NAME_MAX] = "Ftp_S";

    LOG_V("%s", __FUNCTION__);

    // 1. Deal with tcp problems

    // 2. UnRegister which broadcasts need to be processed

    bcast_unreg_receive_msg(MSG_WHAT_BG95_NET_DATACALL_SUCCESS, g_bg95_ftp_msg_id);

    // 3. Destroy net service
    if (NULL != g_bg95_ftp_thread_id)
    {
        ret = os_thread_destroy(g_bg95_ftp_thread_id);
        if (0 != ret)
        {
            LOG_E("Delete g_bg95_ftp_thread_id thread failed! %d", ret);
            return -1;
        }
    }

    // 4. Delete msg queue
    if (NULL != g_bg95_ftp_msg_id)
    {
        ret = OsalMsgQDelete(g_bg95_ftp_msg_id);
        if (0 != ret)
        {
            LOG_E("Delete g_bg95_ftp_msg_id msg failed! %d", ret);
            return -1;
        }
    }
    LOG_V("%s over", __FUNCTION__);

    return 0;
}

int bg95_ftp_service_test(s32_t argc, char *argv[])
{
    int i;

    // LOG_V("%s",__FUNCTION__);

    for (i = 0; i < argc; i++)
    {
        // LOG_V("%d = %s", i, argv[i]);
    }

    if (strcmp((const char *)argv[0], "help") == 0)
    {
        LOG_I("--------------------------------------------");
        LOG_I("| bg95_ftp:                                |");
        LOG_I("--------------------------------------------");
        LOG_I("| exit                                     |");
        LOG_I("| help                                     |");
        LOG_I("| create                                   |");
        LOG_I("| destroy                                  |");
        LOG_I("| send_msg 'what' 'arg1' 'arg2' 'arg3'     |");
        LOG_I("--------------------------------------------");
    }
    else if (strcmp((const char *)argv[0], "exit") == 0)
    {
        LOG_D("exit %s", __FUNCTION__);
        return -1;
    }
    else if (strcmp((const char *)argv[0], "create") == 0)
    {
        bg95_ftp_service_create();
    }
    else if (strcmp((const char *)argv[0], "destroy") == 0)
    {
        bg95_ftp_service_destroy();
    }
    else if (strcmp((const char *)argv[0], "send_msg") == 0)
    {
        LOG_I("send_msg 0x%x 0x%x 0x%x 0x%x", strtol(argv[1], NULL, 16), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
        bcast_send_my_msg(g_bg95_ftp_msg_id, strtol(argv[1], NULL, 16), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
    }
    else
    {
        LOG_E("Invalid parameter:%s", argv[0]);
    }

    return 0;
}
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FTP_S__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */
