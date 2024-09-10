#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FILESYSTEM__

#include <at.h>
#include "cmsis_os.h"
#include "bg95_filesystem.h"
#include "bg95_net.h"
#include "at_osal.h"
#include "debug_service.h"
#include "broadcast_service.h"
#include "at_socket_device.h"
#include "user_main.h"

#define BG95_EVENT_FILE_UP_OK (1L << (8 + 8))
#define BG95_EVENT_FILE_UP_FIAL (1L << (8 + 9))
#if 1

static void at_file_errcode_parse(int result)
{
    switch (result)
    {
    case 400:
        LOG_E("%d : Invalid input value", result);
        break;
    case 401:
        LOG_E("%d : Larger than the size of the file", result);
        break;
    case 402:
        LOG_E("%d : Read zero byte", result);
        break;
    case 403:
        LOG_E("%d : Drive full", result);
        break;
    case 405:
        LOG_E("%d : File not found", result);
        break;
    case 406:
        LOG_E("%d : Invalid file name", result);
        break;
    case 407:
        LOG_E("%d : File already existed", result);
        break;
    case 409:
        LOG_E("%d : Fail to write the file", result);
        break;
    case 410:
        LOG_E("%d : Fail to open the file", result);
        break;
    case 411:
        LOG_E("%d : Fail to read the file", result);
        break;
    case 413:
        LOG_E("%d : Reach the max number of file allowed to be opened", result);
        break;
    case 414:
        LOG_E("%d : The file read-only", result);
        break;
    case 416:
        LOG_E("%d : Invalid file descriptor", result);
        break;
    case 417:
        LOG_E("%d : Fail to list the file", result);
        break;
    case 418:
        LOG_E("%d : Fail to delete the file", result);
        break;
    case 419:
        LOG_E("%d : Fail to get disk info", result);
        break;
    case 420:
        LOG_E("%d : No space", result);
        break;
    case 421:
        LOG_E("%d : Time out", result);
        break;
    case 423:
        LOG_E("%d : File too large", result);
        break;
    case 425:
        LOG_E("%d : Invalid parameter", result);
        break;
    case 426:
        LOG_E("%d : File already opened", result);
        break;
    default:
        LOG_E("%d : Unknown err code", result);
        break;
    }
}

#endif /* BG95_USING_FTP */
static struct file_device g_bg95_file_device = {0};
struct file_device *file_device_get(void)
{
    return &g_bg95_file_device;
}

// Function to register a file device.
// Takes a pointer to an AT (Attention) client structure as an argument.
int file_device_register(struct at_client *client)
{
    // Declare a structure to hold event flag attributes.
    osEventFlagsAttr_t event_attr;

    // Initialize the event attributes structure with zeros.
    memset(&event_attr, 0, sizeof(event_attr));

    // Set the name of the event attribute to "file_device".
    event_attr.name = "file_device";

    // Create an event flag for the file device's URC (Unsolicited Result Code) with the specified attributes.
    // This is likely for handling asynchronous events or notifications related to the file device.
    g_bg95_file_device.file_event = OsalEventNCreate(&event_attr);

    // Check if the event creation was unsuccessful (possibly due to insufficient memory).
    if (g_bg95_file_device.file_event == RT_NULL)
    {
        // Log an error message indicating a memory allocation failure.
        LOG_E("No memory available for AT device socket event creation.");

        // Return an error code indicating that there is not enough memory available.
        return -RT_ENOMEM;
    }

    // Return a success code indicating that the operation completed successfully.
    return RT_EOK;
}

// Function to unregister a file device.
// This function does not take any arguments.
int file_device_unregister(void)
{
    // Check if the file device's URC (Unsolicited Result Code) event is initialized.
    if (g_bg95_file_device.file_event)
    {
        // Delete or clean up the event associated with the file device's URC.
        // This step is important to free up any resources used by the event.
        OsalEventDelete(g_bg95_file_device.file_event);
    }

    // Return a success code indicating that the operation completed successfully.
    return RT_EOK;
}


// It takes a pointer to a 'file_device' structure and an event code as arguments.
static int bg95_file_event_send(struct file_device *device, uint32_t event)
{
    // Log the function call with a verbose log level.
    // It logs the name of the function and the event code being sent.
    LOG_V("%s, 0x%x", __FUNCTION__, event);

    // Send the specified event to the file device's URC (Unsolicited Result Code) event handle.
    // 'OsalEventSend' is a function that likely sends or signals an event within the system.
    // The return value of 'OsalEventSend' is cast to an int and returned by this function.
    return (int)OsalEventSend(device->file_event, event);
}


// This function handles the reception of events for a BG95 file device.
// It takes a pointer to a 'file_device' structure, an event code, a timeout value, and an option flag as arguments.
static int bg95_file_event_recv(struct file_device *device, uint32_t event, uint32_t timeout, rt_uint8_t option)
{
    // Variable to store the received event.
    rt_uint32_t recved;

    // Log the function call with a verbose log level.
    // It logs the name of the function, event code, timeout value, and option flag.
    LOG_V("%s, event = 0x%x, timeout = %d, option = %d", __FUNCTION__, event, timeout, option);

    // Receive an event. The 'OsalEventRecv' function waits for a specific event to occur within the given timeout period.
    // The 'option' parameter is combined with the 'RT_EVENT_FLAG_CLEAR' flag to modify the behavior of the event wait.
    recved = OsalEventRecv(device->file_event, event, option, timeout);

    // Check if the event was not received within the timeout period.
    if (recved < 0)
    {
        // Return an error code indicating a timeout occurred.
        return -RT_ETIMEOUT;
    }

    // Log that the operation is completed.
    LOG_V("%s, event = 0x%x, timeout = %d, option = %d, operation completed", __FUNCTION__, event, timeout, option);

    // Return the result of the event reception.
    return recved;
}

// This function is responsible for deleting files using the QFDEL AT command.
// It returns 0 on success and -1 on failure.

int ql_file_del(const char *dirname)
{
    // Creating a response object for the AT command
    at_response_t query_resp = NULL;

    // Creating a response object for the AT command
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(500));

    // Check if memory allocation for the response object was successful
    if (query_resp == NULL)
    {
        LOG_E("Memory allocation for response object failed.\n");
        return -1;
    }

    // Sending the AT command to delete all files using QFDEL
    if (at_exec_cmd(query_resp, "AT+QFDEL=\"%s\"", dirname) < 0)
    {
        LOG_E("AT+QFDEL command execution failed.\n");
        at_delete_resp(query_resp);
        return -1;
    }

    // Check if the expected format is not found in the response
    LOG_E("AT+QFDEL execution successful.\n");

    // Cleanup: Delete the response object
    at_delete_resp(query_resp);

    // Return 0 on success
    return 0;
}

// Function to list the contents of a directory on the FTP server or compare files.
// Parameters:
// - dirname: The name of the directory to list or the filename to compare.
// - fileList: An array to store File_Module_Info structures representing the directory contents.
// - maxFiles: The maximum number of files to retrieve or compare.
// - mode: The operation mode. 0 for listing files, 1 for comparing files with the provided directory.

int ql_module_list_get(const char *dirname, File_Moudle_Info *fileList, u8_t maxFiles, u8_t mode)
{
    int err = 0;

    // Create an AT response object with an 8000ms timeout.
    at_response_t resp = at_create_resp(512, 0, rt_tick_from_millisecond(10000));
    if (!resp)
    {
        LOG_E("Memory allocation for response object failed.\n");
        return -1;
    }

    u8_t fileCount = 0;

    // Construct the AT command to list the directory contents.
    at_exec_cmd(resp, "AT+QFLST=\"%s\"", dirname);

    int lineFound = 0;
    for (int i = 0; i < resp->line_counts; i++)
    {
        const char *line = at_resp_get_line(resp, i + 1);
        LOG_V("query_resp line [%d]: %s", i, line);

        // Check if there is a "+CME ERROR" line.
        if (strstr(line, "+CME ERROR:") != NULL)
        {
            sscanf(line, "+CME ERROR: %d", &err);
            at_file_errcode_parse(err); // Handle CME errors
            at_delete_resp(resp);
            return -1;
        }

        if (strstr(line, "+QFLST:") != NULL)
        {
            char filename[256]; // Temporary variable to store filename
            u32_t filesize;     // Temporary variable to store filesize

            // Parse the response format "+QFLST: "filename",filesize"
            if (sscanf(line, "+QFLST: \"%[^\"]\",%u", filename, &filesize) == 2)
            {
                if (fileCount < maxFiles)
                {
                    // Copy the parsed filename and filesize to the FileInfo structure
                    strncpy(fileList[fileCount].filename, filename, sizeof(fileList[fileCount].filename));
                    fileList[fileCount].filename[sizeof(fileList[fileCount].filename) - 1] = '\0'; // Ensure null-terminated
                    fileList[fileCount].filesize = filesize;
                    fileCount++;
                }
                else
                {
                    LOG_E("Max files reached, ignoring additional entries.\n");
                    break;
                }
            }
            else
            {
                LOG_E("Warning: Failed to parse line: %s\n", line);
            }
        }
    }

    if (fileCount == 0)
    {
        LOG_V("No files found in the directory.\n");
    }

    at_delete_resp(resp); // Release the response object.

    if (mode == 1)
    {
        // Compare filenames with the provided directory
        for (int i = 0; i < fileCount; i++)
        {
            if (strcmp(dirname, fileList[i].filename) == 0)
            {
                LOG_V("Match found, return OK.\n");
                return 0; // Match found, return OK
            }
        }
        return -1; // No match found, return an error
    }

    return fileCount; // Return the number of files found.
}

// Function to upload data to the BG95 modem using QFUPL AT command
#define READ_DATA_LEN 128 // Length of data to be read in each iteration

void bg95_file_uploader_func(const char *data, rt_size_t len)
{
    rt_bool_t is_data_start = RT_FALSE;      // Flag to indicate if data transmission has started
    char read_data[128] = {0};                      // Buffer to read data from the modem
    UINT bw = 0;                                    // Unused variable
    rt_size_t total_data_written = 0;               // Total bytes of data sent
    rt_size_t qftpget_size = 0;                     // Size of the data received from QFUPL
    rt_size_t FILE_CRC = 0;                         // Error code
    struct file_device *device = file_device_get(); // Get the FTP device structure

    if (device == RT_NULL)
    {
        LOG_E("Failed to get the device.");
        return; // Return without error code
    }

    // Check if "CONNECT\r\n" is detected, indicating the start of data
    if (!is_data_start)
    {
        
        at_self_recv(read_data, READ_DATA_LEN, 2000,1); // Receive data from the modem with a timeout of 2000ms
     //   LOG_E("read_data %s",read_data);
        if (strstr(read_data, "CONNECT\r\n"))
        {
            is_data_start = RT_TRUE; // Set the flag to indicate data transmission has started
        }
    }

    // If data transmission has started, read and send the file data
    if (is_data_start)
    {
        unsigned char buffer[128]; // Buffer for reading file data
        UINT bytesRead;
        FRESULT res;

        while (1)
        {
            res = f_read(&g_bg95_file_device.file, buffer, sizeof(buffer), &bytesRead);
            if (res != FR_OK || bytesRead == 0)
            {
                break; // Exit the loop when file data reading is completed or an error occurs
            }

            // Send data to the modem
            total_data_written += bytesRead; // Update the total bytes sent
            at_client_send(buffer, bytesRead);
            printf("Sending data to the modem...\n");
        }

        // Send exit command and wait for "OK"
        osDelay(1500); // Delay for 1500ms

        // Wait for the QFUPL response
        while (1)
        {
            rt_size_t read_data_len = at_self_recv(read_data, READ_DATA_LEN, 2000,1); // Receive data with a timeout of 2000ms
           //  LOG_E("read_data %s",read_data);
            char *qftpget_pos = strstr(read_data, "+QFUPL:"); // Check for "+QFUPL:" response
            if (qftpget_pos)
            {
                sscanf(qftpget_pos, "+QFUPL: %d,%d", &qftpget_size, &FILE_CRC); // Parse the error code and size
                break;                                                          // Exit the loop when the response is found
            }
            osDelay(2000); // Delay for 2000ms
        }

        // Wait for the "OK\r\n" response
        while (1)
        {
            rt_size_t read_data_len = at_self_recv(read_data, 6, 2000,1); // Receive data with a timeout of 2000ms
 //LOG_E("read_data %s",read_data);
            char *ok_pos = strstr(read_data, "OK\r\n"); // Check for "OK\r\n" response
            if (ok_pos)
            {
                break; // Exit the loop when "OK\r\n" is found
            }
        }

        // Check if the upload was successful and if the total data written matches the expected size
        if (qftpget_size != 0 && total_data_written == qftpget_size)
        {
            LOG_I("Upload successful. Total size: %d bytes.", total_data_written);
            bg95_file_event_send(device, BG95_EVENT_FILE_UP_OK); // Send upload success event
        }
        else
        {
            LOG_E("Upload failed or size mismatch.");
            bg95_file_event_send(device, BG95_EVENT_FILE_UP_FIAL); // Send upload failure event
        }
    }
    else
    {
		LOG_E("Upload failed .");
		bg95_file_event_send(device, BG95_EVENT_FILE_UP_FIAL); // Send upload failure event
    }
}
// Function to upload a file to the BG95 modem using QFUPL AT command
// Parameters:
// - localfile: Path to the local file to be uploaded.
// - remotefile: Name of the file on the remote server.
// - up_size: Size of the file to upload.
int ql_file_put_ex(char *localfile, char *remotefile, u32_t up_size)
{
    int result = 0;
    at_client_t client = at_client_get_first();
    file_device_register(client);
    uint32_t event = 0;
    int event_result = 0;
    struct file_device *device = RT_NULL;

    // Get the FTP device instance.
    device = file_device_get();
    if (device == RT_NULL)
    {
        LOG_E("Failed to get the device.");
        return -1;
    }

    // Open the file to be read.
    FRESULT res = f_open(&g_bg95_file_device.file, localfile, FA_READ);
    if (res != FR_OK)
    {
        printf("Error opening file for reading.\n");
        f_mount(NULL, "", 1); // Unmount the file system
        return -1;
    }

    // Create an AT response object with a custom self-function handler (bg95_file_uploader_func)
    at_response_t resp = at_create_resp_by_selffunc(64, 0, rt_tick_from_millisecond(60000), bg95_file_uploader_func);
    if (!resp)
    {
        LOG_E("Memory allocation for response object failed.\n");
        return -1;
    }
    // Define the events to wait for
     event = BG95_EVENT_FILE_UP_OK | BG95_EVENT_FILE_UP_FIAL;
     bg95_file_event_recv(device, event, 0, RT_EVENT_FLAG_OR);

    // Send the AT command "AT+QFUPL" to initiate the upload session
    at_exec_cmd(resp, "AT+QFUPL=\"%s\",%d,20000", remotefile, up_size);



    // Wait for the upload result
    event_result = bg95_file_event_recv(device, BG95_EVENT_FILE_UP_OK | BG95_EVENT_FILE_UP_FIAL, 60 * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR);
    if (event_result < 0)
    {
        result = -RT_ETIMEOUT;
        goto __exit;
    }

    // Check the upload result
    if (event_result & BG95_EVENT_FILE_UP_FIAL)
    {
        result = -RT_ERROR;
        goto __exit;
    }

__exit:
    // Close the file
    f_close(&g_bg95_file_device.file);
    file_device_unregister();
    at_delete_resp(resp);
    return result;
}
int QL_fs_free(char *localfile, rt_size_t *free_size, rt_size_t *total_size)
{
    at_response_t query_resp = NULL;

    // 閿熸枻鎷烽敓锟??閿熸枻鎷烽敓鏂ゆ嫹?閿熸枻鎷烽敓鏂ゆ嫹AT?閿熸枻鎷烽敓鏂ゆ嫹?閿熸枻鎷???閿熸枻鎷烽敓鏂ゆ嫹|???閿熸枻鎷?
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(500));
    if (query_resp == NULL)
    {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    if (at_exec_cmd(query_resp, "AT+QFLDS=\"%s\"", localfile) < 0)
    {
        LOG_E("AT+QFLDS query failed.\n");
        at_delete_resp(query_resp);
        return -1;
    }

    for (u8_t i = 0; i < query_resp->line_counts; i++)
    {
        const char *line = at_resp_get_line(query_resp, i + 1);
        LOG_D("query_resp line [%d]: %s", i, line);

        rt_size_t free, total;
        if (sscanf(line, "+QFLDS: %d,%d", &free, &total) == 2)
        {
            LOG_I("QFLDS: free_size=%d, total_size=%d\n", free, total);
            if (free_size != NULL)
                *free_size = free;
            if (total_size != NULL)
                *total_size = total;
            at_delete_resp(query_resp);
            return 0;
        }
    }

    LOG_E("Network scan mode query failed.\n");
    at_delete_resp(query_resp);
    return -1;
}
int QL_fs_open(char *localfile, u8_t mode)
{
    at_response_t query_resp = NULL;

    // 閿熸枻鎷烽敓锟??閿熸枻鎷烽敓鏂ゆ嫹?閿熸枻鎷烽敓鏂ゆ嫹AT?閿熸枻鎷烽敓鏂ゆ嫹?閿熸枻鎷???閿熸枻鎷烽敓鏂ゆ嫹|???閿熸枻鎷?
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(500));
    if (query_resp == NULL)
    {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    if (at_exec_cmd(query_resp, "AT+QFOPEN=\"%s\",%d", localfile, mode) < 0)
    {
        LOG_E("AT+QFOPEN failed.\n");
        at_delete_resp(query_resp);
        return -1;
    }

    for (u8_t i = 0; i < query_resp->line_counts; i++)
    {
        const char *line = at_resp_get_line(query_resp, i + 1);
        LOG_D("query_resp line [%d]: %s", i, line);

        u8_t file_handle;
        if (sscanf(line, "+QFOPEN: %d", &file_handle) == 1)
        {
            LOG_I("file_handle %d\n", file_handle);
            at_delete_resp(query_resp);
            return file_handle;
        }
    }

    LOG_E("QFOPEN failed.\n");
    at_delete_resp(query_resp);
    return -1;
}
int QL_fs_close(u8_t file_handle)
{
    at_response_t query_resp = NULL;

    // 閿熸枻鎷烽敓锟??閿熸枻鎷烽敓鏂ゆ嫹?閿熸枻鎷烽敓鏂ゆ嫹AT?閿熸枻鎷烽敓鏂ゆ嫹?閿熸枻鎷???閿熸枻鎷烽敓鏂ゆ嫹|???閿熸枻鎷?
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(500));
    if (query_resp == NULL)
    {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    if (at_exec_cmd(query_resp, "AT+QFCLOSE=%d", file_handle) < 0)
    {
        LOG_E("AT+QFOPEN failed.\n");
        at_delete_resp(query_resp);
        return -1;
    }

    LOG_E("QFOPEN ok.\n");
    at_delete_resp(query_resp);
    return 0;
}

void bg95_file_wirte_func(const char *data, rt_size_t len)
{
    static rt_bool_t is_data_start = RT_FALSE; // Flag to indicate if data transmission has started
    char read_data[128] = {0};                 // Buffer to read data from the modem

    u8_t FILE_written = 0; // Total bytes of data sent

    u8_t FILE_CRC = 0;                              // Error code
    struct file_device *device = file_device_get(); // Get the FTP device structure

    if (device == RT_NULL)
    {
        LOG_E("Failed to get the device.");
        return; // Return without error code
    }

    // Check if "CONNECT\r\n" is detected, indicating the start of data
    if (!is_data_start)
    {
        at_self_recv(read_data, READ_DATA_LEN, 2000,1); // Receive data from the modem with a timeout of 2000ms
        if (strstr(read_data, "CONNECT\r\n"))
        {
            is_data_start = RT_TRUE; // Set the flag to indicate data transmission has started
        }
    }

    // If data transmission has started, read and send the file data
    if (is_data_start)
    {
        bg95_file_event_send(device, BG95_EVENT_FILE_UP_OK); // Send upload success event
    }
    else
        bg95_file_event_send(device, BG95_EVENT_FILE_UP_FIAL); // Send upload success event

    // Wait for the QFUPL response
    while (1)
    {
        rt_size_t read_data_len = at_self_recv(read_data, READ_DATA_LEN, 2000,1); // Receive data with a timeout of 2000ms

        char *qftpget_pos = strstr(read_data, "+QFWRITE:"); // Check for "+QFUPL:" response
        if (qftpget_pos)
        {
            sscanf(qftpget_pos, "+QFWRITE: %d,%d", &FILE_written, &FILE_CRC); // Parse the error code and size
            break;                                                            // Exit the loop when the response is found
        }
        osDelay(2000); // Delay for 2000ms
    }

    // Wait for the "OK\r\n" response
    while (1)
    {
        rt_size_t read_data_len = at_self_recv(read_data, 6, 2000,1); // Receive data with a timeout of 2000ms

        char *ok_pos = strstr(read_data, "OK\r\n"); // Check for "OK\r\n" response
        if (ok_pos)
        {
            break; // Exit the loop when "OK\r\n" is found
        }
    }
    if (FILE_written == FILE_CRC)
    {
        bg95_file_event_send(device, BG95_EVENT_FILE_UP_OK); // Send upload success event
    }
    else
    {
        bg95_file_event_send(device, BG95_EVENT_FILE_UP_FIAL); // Send upload success event
    }
}
// Function to list the contents of a directory on the FTP server
int QL_fs_write(u8_t file_handle, u8_t wirte_size, char *wirte_buffer)
{
    int result = 0;
    at_client_t client = at_client_get_first();
    file_device_register(client);
    uint32_t event = 0;
    int event_result = 0;
    struct file_device *device = RT_NULL;

    // Get the FTP device instance.
    device = file_device_get();
    if (device == RT_NULL)
    {
        LOG_E("Failed to get the device.");
        return -1;
    }
    // Define the events to wait for
    event = BG95_EVENT_FILE_UP_OK | BG95_EVENT_FILE_UP_FIAL;
    bg95_file_event_recv(device, event, 0, RT_EVENT_FLAG_OR);
    // Create an AT response object with a custom self-function handler (bg95_file_uploader_func)
    at_response_t resp = at_create_resp_by_selffunc(64, 0, rt_tick_from_millisecond(60000), bg95_file_wirte_func);
    if (!resp)
    {
        LOG_E("Memory allocation for response object failed.\n");
        return -1;
    }
    u8_t isListing = 0;
    at_exec_cmd(resp, "AT+QFWRITE=%d,%d", file_handle, wirte_size);
    // Wait for the upload result
    event_result = bg95_file_event_recv(device, BG95_EVENT_FILE_UP_OK | BG95_EVENT_FILE_UP_FIAL, 20 * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR);
    if (event_result < 0)
    {
        result = -RT_ETIMEOUT;
        goto __exit;
    }
    // Check the upload result
    if (event_result & BG95_EVENT_FILE_UP_FIAL)
    {
        result = -RT_ERROR;
        goto __exit;
    }
    at_client_send(wirte_buffer, wirte_size); // Send data to the modem
    event_result = bg95_file_event_recv(device, BG95_EVENT_FILE_UP_OK | BG95_EVENT_FILE_UP_FIAL, 20 * RT_TICK_PER_SECOND, RT_EVENT_FLAG_OR);
    if (event_result < 0)
    {
        result = -RT_ETIMEOUT;
        goto __exit;
    }
    // Check the upload result
    if (event_result & BG95_EVENT_FILE_UP_FIAL)
    {
        result = -RT_ERROR;
        goto __exit;
    }

__exit:

    file_device_unregister();
    at_delete_resp(resp);
    return result;

} // Function to list the contents of a directory on the FTP server

int QL_fs_read(u8_t file_handle, u8_t read_size, char *out_data)
{
    at_response_t query_resp = NULL;
    u8_t read_len = 0;
    int data_copied = 0; // 閿熸枻鎷??閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹??閿熸枻鎷??閿熸枻鎷??閿熸枻鎷???-?閿熸枻鎷???閿熸枻鎷??閿熸枻鎷穣?Y

    // 閿熸枻鎷烽敓锟??閿熸枻鎷稟T?閿熸枻鎷烽敓鏂ゆ嫹?閿熸枻鎷???閿熸枻鎷烽敓鏂ゆ嫹|???閿熸枻鎷?
    query_resp = at_create_resp(256, 0, rt_tick_from_millisecond(500));
    if (query_resp == NULL)
    {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    at_exec_cmd(query_resp, "AT+QFREAD=%d,%d", file_handle, read_size);

    for (u8_t i = 0; i < query_resp->line_counts; i++)
    {
        const char *line = at_resp_get_line(query_resp, i + 1);

        if (!data_copied && sscanf(line, "CONNECT %d", &read_len) == 1)
        {
            // 閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹??閿熸枻鎷??DD閿熸枻鎷???閿熸枻鎷?
            if (i + 1 < query_resp->line_counts)
            {
                const char *data_line = at_resp_get_line(query_resp, i + 2);
                // ???閿熸枻鎷??????閿熸枻鎷?3閿熸枻鎷??閿熸枻鎷烽敓鏂ゆ嫹?閿熸枻鎷穣?Y
                strncpy(out_data, data_line, read_len);
                out_data[read_len] = '\0'; // 閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹閿燂拷?閿熸枻鎷??閿熸枻鎷???閿熸枻鎷烽敓鏂ゆ嫹?
                data_copied = 1;           // 閿熸枻鎷烽敓鏂ゆ嫹??閿熸枻鎷穣?Y閿熸枻鎷??閿熸枻鎷???閿熸枻鎷???
            }
        }
    }

    if (!data_copied)
    {
        LOG_E("Failed to read data.\n");
        at_delete_resp(query_resp);
        return -1;
    }

    // ??閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹?閿熸枻鎷?
    at_delete_resp(query_resp);
    return read_len; // 閿熸枻鎷烽敓鏂ゆ嫹???閿熸枻鎷烽敓鏂ゆ嫹?閿熸枻鎷??閿熸枻鎷穣?Y3閿熸枻鎷??閿熸枻鎷?
}

#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FILESYSTEM__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */
