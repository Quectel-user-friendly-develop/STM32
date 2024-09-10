#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__
#include <at.h>
#include "at_osal.h"
#include "debug_service.h"
#include "broadcast_service.h"
#include "bg95_net.h"
#include "user_main.h"

#define static_device
static osThreadId_t g_bg95_net_thread_id =NULL;
static int QL_air_mode_set(u8_t cfun_mode);
static ip_addr_t g_module_addr = {0};
static osMsgQueId_t g_bg95_net_msg_id = NULL;
static osTimeId_t   g_bg95_net_timer_id = NULL;
/******************************************************************************
* Function: net_send_msg
* Description: This function is used to send a message to the broadcast thread.
*              It uses 'OsalMsgSend' for sending the message.
* Parameters:
*   msg_node *msg - Pointer to the message node that needs to be sent.
* Return: 
*   s32_t - Returns a status code.
*          SUCCESS (0) if the message is sent successfully.
*          FAILURE (-1) if there is an error in sending the message.
******************************************************************************/
s32_t net_send_msg(msg_node *msg)
{
    s32_t ret;

    // Sending the message using OsalMsgSend
    ret = OsalMsgSend(g_bg95_net_msg_id, (void *)msg, 0, 0);

    // Check if the message sending failed
    if (ret < 0)
    {
        // Log the error if the message sending fails
        LOG_E("Send msg to broadcast service failed!\n");
        return -1; // Return FAILURE
    }

    return 0; // Return SUCCESS
}

/******************************************************************************
* Function: net_send_bcast_msg
* Description: This function is used to send a broadcast message. It prepares
*              a message node with specified arguments and then calls 
*              'net_send_msg' to send this message.
* Parameters:
*   s32_t what - An identifier for the message type or action.
*   s32_t arg1 - The first argument associated with the message.
*   s32_t arg2 - The second argument associated with the message.
*   s32_t arg3 - The third argument associated with the message.
* Return: 
*   s32_t - Returns a status code.
*          SUCCESS (0) if the broadcast message is sent successfully.
*          FAILURE (-1) if there is an error in sending the broadcast message.
******************************************************************************/
s32_t net_send_bcast_msg(s32_t what, s32_t arg1, s32_t arg2, s32_t arg3)
{
    msg_node msg;

    // Setting up the message with provided arguments
    msg.what = what;
    msg.arg1 = arg1;
    msg.arg2 = arg2;
    msg.arg3 = arg3;

    // Sending the message using net_send_msg function
    return net_send_msg(&msg);
}

/******************************************************************************
* Function: QL_sim_state_query
* Description: This function queries the state of the SIM card. It sends the
*              AT command 'AT+CPIN?' up to three times to check the SIM status.
*              If the SIM is ready, it returns success; otherwise, it logs the 
*              error and returns failure after three attempts.
* Return: 
*   int - Returns a status code.
*         SUCCESS (0) if the SIM is ready.
*         FAILURE (-1) if the SIM is not ready or there is an error in query.
******************************************************************************/
static int QL_sim_state_query(void) {
    at_response_t resp;
    const char *line;
    rt_bool_t found = FALSE;

    // Creating a response object for AT command
    resp = at_create_resp(128, 0, rt_tick_from_millisecond(200));
    if (!resp) {
        LOG_E("No memory for AT response.\n");
        return -1;
    }

    // Trying up to 3 times to check SIM state
    for (u8_t attempt = 0; attempt < 3; attempt++) {
        at_exec_cmd(resp, "AT+CPIN?");

        // Scanning each line of the AT command response
        for (int i = 0; i < resp->line_counts; i++) {
            line = at_resp_get_line(resp, i + 1);

            // Checking if the line contains the SIM state information
            if (strstr(line, "+CPIN:")) {
                found = TRUE;
                if (strstr(line, "READY")) {
                    LOG_I("SIM1 Test OK");
                    at_delete_resp(resp);
                    return 0; // SIM is ready
                } else {
                    LOG_E("SIM1 Test Fail, Try %d", 3 - 1 - attempt);
                    break; // Break out of the inner loop if SIM not ready
                }
            }
        }

        // Logging response lines if the keyword was not found
        if (!found) {
            for (int i = 0; i < resp->line_counts; i++) {
                LOG_E("resp line [%d]: %s", i, at_resp_get_line(resp, i + 1));
            }
        }

        osDelay(2000); // Delay before the next attempt
    }

    at_delete_resp(resp);
    return -1; // SIM is not ready or error occurred
}

/******************************************************************************
* Function: QL_start_datacall
* Description: This function starts a data call by sending the AT command
*              'AT+QIACT=1'. It waits for the response and checks for 'OK' or 
*              'ERROR' in the response. The function returns success if 'OK' is 
*              found, and different error codes if 'OK' is not received or if 
*              an 'ERROR' is found in the response.
* Return: 
*   int - Returns a status code.
*         SUCCESS (0) if the command returns 'OK'.
*         FAILURE (-1) if the command does not return 'OK'.
*         FAILURE (-2) if the command returns 'ERROR'.
******************************************************************************/
static int QL_start_datacall(void) {
    at_response_t resp;
    const char *line;
    rt_bool_t foundQIACT1 = FALSE;

    // Initial delay before sending the command
    osDelay(2000);

    // Creating a response object for AT command
    resp = at_create_resp(128, 0, rt_tick_from_millisecond(4000));
    at_exec_cmd(resp, "AT+QIACT=1");

    // Scanning each line of the AT command response
    for (int i = 0; i < resp->line_counts; i++) {
        line = at_resp_get_line(resp, i + 1);

        // Checking for 'OK' in the response
        if (strstr(line, "OK")) {
            foundQIACT1 = TRUE;
            break; // 'OK' found, break out of the loop
        }
        else if (strstr(line, "ERROR")) {
            // If 'ERROR' is found in the response
            at_delete_resp(resp);
            return -2; // Return error code -2
        }
        LOG_D("resp line [%d]: %s", i, line); // Log each line of the response
    }

    // If 'OK' was not found in the response
    if (!foundQIACT1) {
        LOG_E("`AT+QIACT=1` command did not return OK.");
        at_delete_resp(resp);
        return -1; // Return error code -1
    }

    at_delete_resp(resp); // Clean up the response object
    return 0; // Return success
}

/******************************************************************************
* Function: QL_check_datacall_state
* Description: This function checks the state of a data call by sending the AT 
*              command 'AT+QIACT?'. It scans the response for '+QIACT:' to 
*              determine the data call state and tries to parse the IP address 
*              from the response. It returns different status codes based on 
*              the findings in the response.
* Return: 
*   int - Returns a status code.
*         SUCCESS (0) if the data call is active and IP address is parsed.
*         FAILURE (-1) if failed to parse the IP address.
*         FAILURE (-2) if '+QIACT:' is not found in the response.
*         OTHER (1) if 'OK' is found in the response without '+QIACT:'.
******************************************************************************/
static int QL_check_datacall_state(void) {
    at_response_t resp;
    const char *line;
    rt_bool_t foundQIACTQuery = FALSE;
    #define IP_ADDR_SIZE_MAX    16
    char ipaddr[IP_ADDR_SIZE_MAX] = {0};

    // Creating a response object for AT command
    resp = at_create_resp(512, 0, rt_tick_from_millisecond(4000));
    at_exec_cmd(resp, "AT+QIACT?");

    // Scanning each line of the AT command response
    for (int i = 0; i < resp->line_counts; i++) {
        line = at_resp_get_line(resp, i + 1);
        
        // Checking for '+QIACT:' in the response
        if (strstr(line, "+QIACT:")) {
            foundQIACTQuery = TRUE;

            // Try to parse the IP address from the line
            if (sscanf(line, "+QIACT: %*d,%*d,%*d,\"%15[^\"]\"", ipaddr) == 1) {
                LOG_I("device IP address: %s", ipaddr);
                break; // Found and parsed the data, break out of the loop
            } else {
                LOG_E("Failed to parse IP address from +QIACT? response.");
                at_delete_resp(resp);
                return -1; // Failed to parse IP address
            }
        }

        // Check if the response line contains 'OK'
        if (strstr(line, "OK")) {
            at_delete_resp(resp);
            return 1; // 'OK' found but '+QIACT:' not found
        }

        LOG_D("resp line [%d]: %s", i, line); // Log each line of the response
    }

    // If '+QIACT:' was not found in the response
    if (!foundQIACTQuery) {
        LOG_E("Keyword +QIACT: not found in any line.");
        at_delete_resp(resp);
        return -2; // '+QIACT:' not found
    }

    // Set network interface address information (if needed)
    inet_aton(ipaddr, &g_module_addr);
    at_delete_resp(resp);

    return 0; // Data call active and IP address parsed
}




/******************************************************************************
* Function: QL_net_state_2gcs_get
* Description: This function queries the 2G network registration status by 
*              sending the AT command 'AT+CREG?'. It parses the response to 
*              extract the network registration status and returns the status 
*              code. If the response does not contain the expected format or 
*              if there is an error in executing the command, the function 
*              returns an error.
* Return: 
*   int - Returns the network registration status code on success.
*         FAILURE (-1) if there is an error in executing the command or 
*         if the response does not contain the expected format.
******************************************************************************/
static int QL_net_state_2gcs_get(void) {
    u8_t mode, status;
    at_response_t query_resp = NULL;

    // Creating a response object for AT command
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(500));
    if (query_resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to query network registration status
    if (at_exec_cmd(query_resp, "AT+CREG?") < 0) {
        LOG_E("AT+CREG? query failed.\n");
        at_delete_resp(query_resp);
        return -1;
    }

    // Parsing the response to extract network registration status
    for (int i = 0; i < query_resp->line_counts; i++) {
        const char *line = at_resp_get_line(query_resp, i + 1);
        LOG_D("query_resp line [%d]: %s", i, line);

        // Checking if the response line contains the expected format
        if (sscanf(line, "+CREG: %d,%d", &mode, &status) == 2) {
            LOG_I("CREG scan mode: %d.\n", status);
            at_delete_resp(query_resp);
            return status; // Return the network registration status
        }
    }

    // If the expected format is not found in the response
    LOG_E("CREG scan mode query failed.\n");
    at_delete_resp(query_resp);
    return -1; // Return error if query fails
}
/******************************************************************************
* Function: QL_net_state_2gps_get
* Description: This function queries the 2G PS (Packet Switched) network 
*              registration status by sending the AT command 'AT+CGREG?'. It 
*              parses the response to extract the network registration status 
*              and returns the status code. If the response does not contain 
*              the expected format or if there is an error in executing the 
*              command, the function returns an error.
* Return: 
*   int - Returns the network registration status code on success.
*         FAILURE (-1) if there is an error in executing the command or 
*         if the response does not contain the expected format.
******************************************************************************/
static int QL_net_state_2gps_get(void) {
    u8_t mode, status;
    at_response_t query_resp = NULL;

    // Creating a response object for AT command
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(500));
    if (query_resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to query PS network registration status
    if (at_exec_cmd(query_resp, "AT+CGREG?") < 0) {
        LOG_E("AT+CGREG? query failed.\n");
        at_delete_resp(query_resp);
        return -1;
    }

    // Parsing the response to extract PS network registration status
    for (int i = 0; i < query_resp->line_counts; i++) {
        const char *line = at_resp_get_line(query_resp, i + 1);
        LOG_D("query_resp line [%d]: %s", i, line);

        // Checking if the response line contains the expected format
        if (sscanf(line, "+CGREG: %d,%d", &mode, &status) == 2) {
            LOG_I("CGREG scan mode: %d.\n", status);
            at_delete_resp(query_resp);
            return status; // Return the PS network registration status
        }
    }

    // If the expected format is not found in the response
    LOG_E("CGREG scan mode query failed.\n");
    at_delete_resp(query_resp);
    return -1; // Return error if query fails
}

/******************************************************************************
* Function: QL_net_state_lte_get
* Description: This function queries the LTE network registration status by 
*              sending the AT command 'AT+CEREG?'. It parses the response to 
*              extract the network registration status and returns the status 
*              code. If the response does not contain the expected format or 
*              if there is an error in executing the command, the function 
*              returns an error.
* Return: 
*   int - Returns the network registration status code on success.
*         FAILURE (-1) if there is an error in executing the command or 
*         if the response does not contain the expected format.
******************************************************************************/
static int QL_net_state_lte_get(void) {
    u8_t mode, status;
    at_response_t query_resp = NULL;

    // Creating a response object for AT command
    query_resp = at_create_resp(64, 0, rt_tick_from_millisecond(500));
    if (query_resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to query LTE network registration status
    if (at_exec_cmd(query_resp, "AT+CEREG?") < 0) {
        LOG_E("AT+CEREG? query failed.\n");
        at_delete_resp(query_resp);
        return -1;
    }

    // Parsing the response to extract LTE network registration status
    for (int i = 0; i < query_resp->line_counts; i++) {
        const char *line = at_resp_get_line(query_resp, i + 1);
        LOG_D("query_resp line [%d]: %s", i, line);

        // Checking if the response line contains the expected format
        if (sscanf(line, "+CEREG: %d,%d", &mode, &status) == 2) {
            LOG_W("CEREG scan mode: %d.\n", status);
            at_delete_resp(query_resp);
            return status; // Return the LTE network registration status
        }
    }

    // If the expected format is not found in the response
    LOG_E("CEREG scan mode query failed.\n");
    at_delete_resp(query_resp);
    return -1; // Return error if query fails
}
/**
 * Function: QL_apn_get
 * Description: This function queries the APN (Access Point Name) settings for a 
 *              specified context ID. It sends the AT command 'AT+CGDCONT?' and 
 *              parses the response to find the APN for the given context ID.
 * Parameters:
 *   u8_t context_id - The context ID for which the APN is to be queried.
 * Return:
 *   char* - Returns a pointer to a string containing the APN on success.
 *           Returns NULL if the query fails or if no matching context ID is found.
 */
static char* QL_apn_get(u8_t context_id) {
    at_response_t resp;
    char *apn = NULL;
    rt_bool_t found = FALSE;

    // Creating a response object for AT command
    resp = at_create_resp(64, 0, rt_tick_from_millisecond(500));
    if (resp == NULL) {
        LOG_E("No memory for response object.\n");
        return NULL;
    }

    // Sending the AT command to query APN settings
    if (at_exec_cmd(resp, "AT+CGDCONT?") < 0) {
        LOG_E("AT+CGDCONT query failed.\n");
        at_delete_resp(resp);
        return NULL;
    }

    // Allocating memory for APN string
    apn = malloc(32 * sizeof(char));
    if (apn == NULL) {
        LOG_E("Failed to allocate memory for APN.\n");
        at_delete_resp(resp);
        return NULL;
    }

    // Scanning each line of the AT command response
    for (int i = 0; i < resp->line_counts; i++) {
        const char *line = at_resp_get_line(resp, i + 1);
        u8_t pdpid = 0;

        // Parsing the response to extract APN
        if (sscanf(line, "+CGDCONT: %d,%*[^,],\"%[^\"]\"", &pdpid, apn) == 2) {
            if (pdpid == context_id) {
                found = TRUE;
                LOG_I("APN query success. APN: %s\n", apn);
                break; // Matching context ID found, break out of the loop
            }
        }
    }

    // Handling case where no matching context ID is found
    if (!found) {
        LOG_W("No matching context ID found in APN query.\n");
        free(apn);
        apn = NULL;
    }

    at_delete_resp(resp);
    return apn; // Return APN string or NULL if not found
}


/******************************************************************************
* Function: QL_apn_set
* Description: This function sets the APN for a given context ID. It sends an 
*              AT command 'AT+CGDCONT' with the context ID and APN information. 
*              The function checks the mode to determine if APN settings are 
*              required and sets the APN accordingly.
* Parameters:
*   u8_t mode - The mode indicating if APN settings are required.
*   u8_t context_id - The context ID for which the APN is to be set.
*   Module_Config *operator_info - Pointer to the structure containing APN and 
*                                  other operator information.
* Return: 
*   int - Returns SUCCESS (0) on successful setting of APN.
*         FAILURE (-1) if there is an error in the process or if the APN 
*         settings are not required.
******************************************************************************/
static int QL_apn_set(u8_t mode, u8_t context_id, Module_Config *operator_info) {
    at_response_t resp;
    const char *line;
    rt_bool_t isCommandSuccess = FALSE;

    // Check if operator_info is NULL
    if (!operator_info) {
        LOG_E("operator_info is NULL.\n");
        return -1;
    }

    // Check if mode indicates that APN settings are not required
    if (mode == 0) {
        LOG_I("APN settings are not required\n");
        return 0;
    } else if (mode == 1) {
        // Creating a response object for AT command
        resp = at_create_resp(128, 0, rt_tick_from_millisecond(400));

        // Sending the AT command to set the APN
        at_exec_cmd(resp, "AT+CGDCONT=%d,\"IP\",\"%s\"", context_id, operator_info->apn);

        // Scanning each line of the AT command response
        for (int i = 0; i < resp->line_counts; i++) {
            line = at_resp_get_line(resp, i + 1);
            LOG_D("resp line [%d]: %s", i, line);

            // Checking if the response contains 'OK'
            if (strstr(line, "OK")) {
                isCommandSuccess = TRUE;
                break; // 'OK' found, break out of the loop
            }
        }

        // Handling case where the AT command did not succeed
        if (!isCommandSuccess) {
            LOG_E("Set operator info failed.\n");
            at_delete_resp(resp);
            return -1; // Return error if setting APN failed
        } else {
            LOG_I("APN set successfully.\n");
        }

        at_delete_resp(resp); // Clean up the response object
    }

    return 0; // Return success
}

/******************************************************************************
* Function: QL_cell_info_get
* Description: This function retrieves the current cell information by sending 
*              the AT command 'AT+QENG="servingcell"'. It parses the response 
*              to extract information such as MCC, MNC, PCI, EARFCN, and more, 
*              based on the Radio Access Technology (RAT) type.
* Parameters:
*   struct QL_CellInfo *cell_info - Pointer to a structure where the cell 
*                                   information will be stored.
* Return: 
*   int - Returns SUCCESS (0) on successful retrieval and parsing of cell 
*         information.
*         FAILURE (-1) if there is an error in the process or if the required 
*         keyword is not found in the response.
******************************************************************************/
static int QL_cell_info_get(struct QL_CellInfo *cell_info)
{
    at_response_t resp;
    char rat[10];  // Variable to store RAT type
    const char *line;
    rt_bool_t found = FALSE;

    // Creating a response object for AT command
    resp = at_create_resp(512, 0, rt_tick_from_millisecond(2000));
    if (resp == NULL)
    {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to get serving cell information
    at_exec_cmd(resp, "AT+QENG=\"servingcell\"");

    // Scanning each line of the AT command response
    for (int i = 0; i < resp->line_counts; i++) {
        line = at_resp_get_line(resp, i + 1);

        // Checking if the line contains the desired keyword
        if (strstr(line, "+QENG: \"servingcell\"")) {
            found = TRUE;

            // First, determine the RAT type
            sscanf(line, "+QENG: \"servingcell\",\"%[^\"]\"", rat);

            // Parse the response based on RAT type
            if (strcmp(rat, "GSM") == 0) {
                // Parse for GSM mode
                // (Add your GSM mode parsing logic here)
            } else {
                // Parse for LTE mode
                if (sscanf(line, "+QENG: \"servingcell\",\"%*[^\"]\",\"%*[^\"]\",\"%*[^\"]\",%d,%d,%*x,%d,%d,%d,%*d,%*d,%*x,%d,%*d,%*d,%d",
                           &cell_info->mcc, &cell_info->mnc, &cell_info->pci, &cell_info->earfcn, &cell_info->band, &cell_info->rsrp, &cell_info->sinr) == 7) {
                    // Parsing successful
                } else {
                    LOG_E("Parse error.\n");
                    at_delete_resp(resp);
                    return -1; // Parse error
                }
            }

            break; // Exit the loop once data is found and parsed
        }
    }

    // Handling case where the keyword is not found in any line
    if (!found) {
        LOG_E("Keyword not found in any line.\n");
        at_delete_resp(resp);
        return -1;
    }

    at_delete_resp(resp);
    return 0; // Return success
}


#include <stdio.h>
#include <string.h>

/******************************************************************************
* Function: QL_neig_info_get
* Description: This function retrieves information about neighbouring cells by 
*              sending the AT command 'AT+QENG="neighbourcell"'. It parses the 
*              response to extract information about each neighbour cell, such 
*              as EARFCN, PCI, RSRP, and SINR. The function stores this 
*              information in an array of QL_NeighbourCellInfo structures.
* Parameters:
*   QL_NeighbourCellInfo neigh_info[] - Array to store neighbour cell information.
*   int max_neighs - The maximum number of neighbour cells to parse.
* Return: 
*   int - Returns the number of neighbour cells parsed on success.
*         FAILURE (-1) if there is an error in the process or if no neighbour 
*         cells are found in the response.
******************************************************************************/
static int QL_neig_info_get(QL_NeighbourCellInfo neigh_info[], int max_neighs) {
    at_response_t resp;
    resp = at_create_resp(2048, 0, rt_tick_from_millisecond(2000));

    // Check if response object creation was successful
    if (resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to get neighbour cell information
    at_exec_cmd(resp, "AT+QENG=\"neighbourcell\"");

    int parsed_neighs = 0;
    const char *line;

    // Scanning each line of the AT command response
    for (int i = 0; i < resp->line_counts && parsed_neighs < max_neighs; i++) {
        line = at_resp_get_line(resp, i + 1);

        // Checking if the line contains information about intra-cell neighbours
        if (strstr(line, "+QENG: \"neighbourcell intra\"")) {
            // Parsing the neighbour cell information
            if (sscanf(line, "+QENG: \"neighbourcell intra\",\"eMTC\",%d,%d,%*d,%d,%*d,%d",
                       &neigh_info[parsed_neighs].EARFCN, &neigh_info[parsed_neighs].PCI, &neigh_info[parsed_neighs].RSRP, &neigh_info[parsed_neighs].SINR) != 4) {
                LOG_E("Parse error at line %d.\n", i + 1);
                at_delete_resp(resp);
                return -1; // Parse error
            }
            parsed_neighs++;
        }
    }

    // Handling case where no neighbour cells are found
    if (parsed_neighs == 0) {
        LOG_E("No neighbour cells found in response.\n");
        at_delete_resp(resp);
        return -1;
    }

    at_delete_resp(resp);
    return parsed_neighs; // Return the number of neighbour cells parsed
}


/******************************************************************************
* Function: QL_operator_set
* Description: This function sets the mobile operator based on the given mode. 
*              If mode is 0, it sets the operator selection to automatic. If 
*              mode is 1, it sets a specific operator using the provided MCC 
*              and MNC codes from operator_info. The function also verifies 
*              the operator setting by querying the current operator.
* Parameters:
*   u8_t mode - Mode for operator selection (0 for automatic, 1 for manual).
*   Module_Config *operator_info - Pointer to the structure containing operator 
*                                  information (MCC and MNC).
* Return: 
*   int - Returns SUCCESS (0) on successful setting of the operator.
*         FAILURE (-1) if there is an error in the process or if verification 
*         fails.
******************************************************************************/
static int QL_operator_set(u8_t mode, Module_Config *operator_info) {
    at_response_t resp;
    
    // Creating a response object for AT command
    resp = at_create_resp(32, 0, rt_tick_from_millisecond(2000));

    if (mode == 0) {
        // Automatic operator selection
        if (at_exec_cmd(resp, "AT+COPS=0") != 0) {
            LOG_E("Set operator failed.\n");
            for (int i = 0; i < resp->line_counts; i++) {
                LOG_E("resp line [%d]: %s", i, at_resp_get_line(resp, i + 1));
            }
            at_delete_resp(resp);
            return -1;
        }
    } else if (mode == 1) {
        // Manual operator selection
        if (at_exec_cmd(resp, "AT+COPS=1,2,\"%s\"", operator_info->mcc_mnc) != 0) {
            LOG_E("Set operator failed.\n");
            at_delete_resp(resp);
            return -1;
        }

        // Querying to verify the operator information
        at_response_t query_resp; // Creating a new response object for query
        query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(1000));

        if (at_exec_cmd(query_resp, "AT+COPS?") == 0) {
            for (int i = 0; i < query_resp->line_counts; i++) {
                const char *line = at_resp_get_line(query_resp, i + 1);
                LOG_D("query_resp line [%d]: %s", i, line);

                // Check if the returned line contains the expected operator info
                if (strstr(line, operator_info->mcc_mnc)) {
                    LOG_I("Operator settings verification successful.\n");
                    at_delete_resp(resp); // Release the response object created for setting operator
                    at_delete_resp(query_resp); // Release the response object created for querying operator
                    return 0; // Successful setting
                }
            }
        }

        LOG_E("Set operator failed or verification failed.\n");
        at_delete_resp(query_resp); // Release the response object created for querying operator
    }

    at_delete_resp(resp);
    return 0;
}

/******************************************************************************
* Function: QL_air_mode_set
* Description: This function sets the functional mode of the module using the 
*              AT command 'AT+CFUN'. The 'cfun_mode' parameter specifies the 
*              desired functional mode. The function checks for valid mode 
*              values and sends the command to the module. It handles any 
*              memory or command execution failures.
* Parameters:
*   u8_t cfun_mode - The functional mode to set. Valid modes are from 0 to 4.
* Return: 
*   int - Returns SUCCESS (0) on successful setting of the functional mode.
*         FAILURE (-1) if the mode is invalid, if there is a memory allocation 
*         issue, or if the command execution fails.
******************************************************************************/
static int QL_air_mode_set(u8_t cfun_mode)
{
    at_response_t resp;
    char cmd[20];

    // Checking for valid CFUN mode
    if(cfun_mode < 0 || cfun_mode > 4)
    {
        LOG_E("Invalid CFUN mode!\n");
        return -1;
    }

    // Creating a response object for AT command
    resp = at_create_resp(32, 0, rt_tick_from_millisecond(300));
    if (!resp)
    {
        LOG_E("No memory for response object!\n");
        return -1;
    }

    // Preparing and sending the AT command to set CFUN mode
    sprintf(cmd, "AT+CFUN=%d", cfun_mode);
    if (at_exec_cmd(resp, cmd) < 0)
    {
        LOG_E("Set CFUN mode failed!\n");
        at_delete_resp(resp);
        return -1;
    }

    // Log the successful setting of CFUN mode
    LOG_W("Set CFUN mode %d\n", cfun_mode);
    at_delete_resp(resp);
    return 0; // Return success
}


/******************************************************************************
* Function: QL_module_reboot
* Description: This function reboots the module using the AT command 'AT+CFUN=1,1'. 
*              It creates a response object for the AT command and checks for 
*              memory allocation success. The command is sent to the module, 
*              and the function handles any command execution failures.
* Return: 
*   int - Returns SUCCESS (0) on successful execution of the reboot command.
*         FAILURE (-1) if there is a memory allocation issue or if the command 
*         execution fails.
******************************************************************************/
static int QL_module_reboot(void)
{
    at_response_t resp = RT_NULL;

    // Creating a response object for AT command
    resp = at_create_resp(32, 0, rt_tick_from_millisecond(2000));
    if (!resp)
    {
        LOG_E("No memory for AT response object.\n");
        return -1;
    }

    // Sending the AT command to reboot the module
    if (at_exec_cmd(resp, "AT+CFUN=1,1") < 0)
    {
        LOG_E("Restart EC20 command failed.\n");
        at_delete_resp(resp);
        return -1;
    }

    // Cleaning up the response object
    at_delete_resp(resp);

    return 0; // Return success
}


/******************************************************************************
* Function: QL_band_clean
* Description: This function clears and restores the band settings of the 
*              module using the AT command 'AT+QCFG="bandrestore"'. It creates 
*              a response object for the AT command and checks for memory 
*              allocation success. The command is sent to the module, and the 
*              function handles any command execution failures.
* Return: 
*   int - Returns SUCCESS (0) on successful execution of the band restore command.
*         FAILURE (-1) if there is a memory allocation issue or if the command 
*         execution fails.
******************************************************************************/
static int QL_band_clean(void) {
    at_response_t resp = NULL;

    // Creating a response object for AT command
    resp = at_create_resp(32, 0, rt_tick_from_millisecond(500));
    if (resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to clear and restore band settings
    if (at_exec_cmd(resp, "AT+QCFG=\"bandrestore\"") < 0) {
        LOG_E("AT+QCFG=\"bandrestore\" command failed.\n");
        at_delete_resp(resp);
        return -1;
    }

    // Logging successful execution of the command
    LOG_I("QL_band_clean OK\n");
    at_delete_resp(resp);

    return 0; // Return success
}
/******************************************************************************
* Function: QL_band_set
* Description: This function sets the band configurations for GSM, eMTC, and 
*              NB-IoT using the AT command 'AT+QCFG="band"'. It sends the 
*              command with the provided band values, waits for the settings to 
*              take effect, and then queries the current band settings to 
*              verify the changes. The function handles any memory or command 
*              execution failures and verifies if the new settings match the 
*              provided values.
* Parameters:
*   uint32_t GSM_bandval - The band value for GSM.
*   uint32_t eMTC_bandval - The band value for eMTC.
*   uint32_t NB_IoT_bandval - The band value for NB-IoT.
* Return: 
*   int - Returns SUCCESS (0) on successful setting and verification of the bands.
*         FAILURE (-1) if there is a memory allocation issue, if the command 
*         execution fails, or if the verification fails.
******************************************************************************/
static int QL_band_set(uint32_t GSM_bandval, uint32_t eMTC_bandval, uint32_t NB_IoT_bandval) {
    at_response_t resp = NULL;

    // Creating a response object for AT command
    resp = at_create_resp(32, 0, rt_tick_from_millisecond(500));
    if (resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to set the bands
    if (at_exec_cmd(resp, "AT+QCFG=\"band\",%x,%x,%x", GSM_bandval, eMTC_bandval, NB_IoT_bandval) < 0) {
        LOG_E("AT+QCFG=\"band\" set command failed.\n");
        at_delete_resp(resp);
        return -1;
    }

    // Delay to allow settings to take effect
    osDelay(500);

    // Querying the current band settings for verification
    at_response_t query_resp = NULL;
    query_resp = at_create_resp(64, 0, rt_tick_from_millisecond(2000));
    if (query_resp == NULL) {
        LOG_E("No memory for query response object.\n");
        at_delete_resp(resp);
        return -1;
    }

    if (at_exec_cmd(query_resp, "AT+QCFG=\"band\"") == 0) {
        uint32_t query_GSM_bandval = 0;
        uint32_t query_eMTC_bandval = 0;
        uint32_t query_NB_IoT_bandval = 0;

        // Parsing the response to extract band values
        for (int i = 0; i < query_resp->line_counts; i++) {
            const char *line = at_resp_get_line(query_resp, i + 1);
            LOG_D("query_resp line [%d]: %s", i, line);

            if (sscanf(line, "+QCFG: \"band\",%x,%x,%x", &query_GSM_bandval, &query_eMTC_bandval, &query_NB_IoT_bandval) == 3) {
                LOG_I("Query result: GSM_bandval=%x, eMTC_bandval=%x, NB_IoT_bandval=%x\n",
                      query_GSM_bandval, query_eMTC_bandval, query_NB_IoT_bandval);

                // Verifying if the queried values match the set values
                if ((GSM_bandval == 0 || query_GSM_bandval == GSM_bandval) &&
                    (eMTC_bandval == 0 || query_eMTC_bandval == eMTC_bandval) &&
                    (NB_IoT_bandval == 0 || query_NB_IoT_bandval == NB_IoT_bandval)) {
                    LOG_I("Band setting verification successful.\n");
                    at_delete_resp(resp);
                    at_delete_resp(query_resp);
                    return 0; // Successful setting
                }
            }
        }
    }

    LOG_E("Set band failed or verification failed.\n");
    at_delete_resp(resp);
    at_delete_resp(query_resp);
    return -1; // Failure in setting or verification
}

/******************************************************************************
* Function: QL_net_rat_get
* Description: This function queries the current network scan mode using the 
*              AT command 'AT+QCFG="nwscanmode"'. It creates a response object 
*              for the AT command, sends the command, and parses the response 
*              to extract the network scan mode. The function handles any 
*              memory or command execution failures and returns the current 
*              network scan mode on success.
* Return: 
*   QL_rat_mode - Returns the current network scan mode on success.
*                 Returns -1 if there is a memory allocation issue, if the 
*                 command execution fails, or if the response parsing fails.
******************************************************************************/
static QL_rat_mode QL_net_rat_get(void) {
    at_response_t query_resp = NULL;

    // Creating a response object for AT command
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(500));
    if (query_resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to query network scan mode
    if (at_exec_cmd(query_resp, "AT+QCFG=\"nwscanmode\"") < 0) {
        LOG_E("AT+QCFG=\"nwscanmode\" query failed.\n");
        at_delete_resp(query_resp);
        return -1;
    }

    // Parsing the response to extract network scan mode
    for (int i = 0; i < query_resp->line_counts; i++) {
        const char *line = at_resp_get_line(query_resp, i + 1);
        LOG_D("query_resp line [%d]: %s", i, line);

        // Checking if the response line contains the expected format
        int mode;
        if (sscanf(line, "+QCFG: \"nwscanmode\",%d", &mode) == 1) {
            LOG_I("Network scan mode: %d.\n", mode);
            at_delete_resp(query_resp);
            return mode; // Return the network scan mode
        }
    }

    // If the expected format is not found in the response
    LOG_E("Network scan mode query failed.\n");
    at_delete_resp(query_resp);
    return -1; // Return error if query fails
}
/******************************************************************************
* Function: QL_net_rat_set
* Description: This function sets the network scan mode using the AT command 
*              'AT+QCFG="nwscanmode"'. It sends the command with the specified 
*              mode, waits for the setting to take effect, and then queries 
*              the current network scan mode to verify the changes. The 
*              function handles any memory or command execution failures and 
*              verifies if the new setting matches the specified mode.
* Parameters:
*   QL_rat_mode mode - The network scan mode to set.
* Return: 
*   int - Returns SUCCESS (0) on successful setting and verification of the 
*         network scan mode.
*         FAILURE (-1) if there is a memory allocation issue, if the command 
*         execution fails, or if the verification fails.
******************************************************************************/
static int QL_net_rat_set(QL_rat_mode mode) {
    at_response_t resp = NULL;

    // Creating a response object for AT command
    resp = at_create_resp(32, 0, rt_tick_from_millisecond(2000));
    if (resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to set the network scan mode
    if (at_exec_cmd(resp, "AT+QCFG=\"nwscanmode\",%d,1", mode) < 0) {
        LOG_E("AT+QCFG=\"nwscanmode\" set command failed.\n");
        at_delete_resp(resp);
        return -1;
    }

    // Delay to allow settings to take effect
    osDelay(500);

    // Querying the current network scan mode for verification
    at_response_t query_resp;
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(2000));
    char query_result[32];
    snprintf(query_result, sizeof(query_result), "+QCFG: \"nwscanmode\",%d", mode);

    if (at_exec_cmd(query_resp, "AT+QCFG=\"nwscanmode\"") == 0) {
        for (int i = 0; i < query_resp->line_counts; i++) {
            const char *line = at_resp_get_line(query_resp, i + 1);
            LOG_D("query_resp line [%d]: %s", i, line);

            // Check if the returned line contains the set value
            if (strstr(line, query_result) != NULL) {
                LOG_I("Network mode settings verification successful.\n");
                at_delete_resp(resp);
                at_delete_resp(query_resp);
                return 0; // Successful setting
            }
        }
    }

    LOG_E("Set network mode failed or verification failed.\n");
    at_delete_resp(resp);
    at_delete_resp(query_resp);
    return -1; // Failure in setting or verification
}

/******************************************************************************
* Function: QL_net_cell_lock
* Description: This function locks the module to a specific cell by sending the 
*              AT command 'AT+QNWCFG="pci_lock"'. It locks to the cell specified 
*              by EARFCN and PCI values in the 'cells' structure. The function 
*              supports two modes - eMTC and NB-IoT, determined by the 'mode' 
*              parameter. It sends the command, waits for a brief period, and 
*              then queries the current cell lock settings to verify the changes.
* Parameters:
*   u8_t mode - The mode for cell locking (0 for eMTC, 1 for NB-IoT).
*   QL_lock_cell_t *cells - Pointer to the structure containing EARFCN and PCI.
* Return: 
*   int - Returns SUCCESS (0) on successful setting and verification of the cell lock.
*         FAILURE (-1) if there is a memory allocation issue, if the command 
*         execution fails, or if the verification fails.
******************************************************************************/
static int QL_net_cell_lock(u8_t mode, QL_lock_cell_t *cells) {
    at_response_t resp = NULL;

    // Creating a response object for AT command
    resp = at_create_resp(64, 0, rt_tick_from_millisecond(5000));
    if (resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to lock the cell
    if (mode == 0) {
        // eMTC mode
        if (at_exec_cmd(resp, "AT+QNWCFG=\"pci_lock\",\"eMTC\",1,%d,%d", cells->earfcn, cells->pci) < 0) {
            LOG_E("AT+QNWCFG=\"pci_lock\" (eMTC mode) command failed.\n");
            at_delete_resp(resp);
            return -1;
        }
    } else {
        // NB-IoT mode
        if (at_exec_cmd(resp, "AT+QNWCFG=\"pci_lock\",\"NBIoT\",1,%d,%d", cells->earfcn, cells->pci) < 0) {
            LOG_E("AT+QNWCFG=\"pci_lock\" (NB-IoT mode) command failed.\n");
            at_delete_resp(resp);
            return -1;
        }
    }

    // Delay to allow settings to take effect
    osDelay(500);

    // Querying the current cell lock settings for verification
    at_response_t query_resp = at_create_resp(64, 0, rt_tick_from_millisecond(2000));
    if (query_resp == NULL) {
        LOG_E("No memory for query response object.\n");
        at_delete_resp(resp);
        return -1;
    }

    // Sending the query command
    if (at_exec_cmd(query_resp, "AT+QNWCFG=\"pci_lock\"") == 0) {
        char query_result[64];
        snprintf(query_result, sizeof(query_result), "+QNWCFG: \"pci_lock\", \"%s\", 1, %d, %d", 
                 (mode == 0) ? "eMTC" : "NBIoT", cells->earfcn, cells->pci);

        // Verifying the cell lock settings
        for (int i = 0; i < query_resp->line_counts; i++) {
            const char *line = at_resp_get_line(query_resp, i + 1);
            if (strstr(line, query_result) != NULL) {
                LOG_I("Cell lock settings verification successful.\n");
                at_delete_resp(resp);
                at_delete_resp(query_resp);
                return 0; // Successful setting
            }
        }
    }

    LOG_E("Set cell lock failed or verification failed.\n");
    at_delete_resp(resp);
    at_delete_resp(query_resp);
    return -1; // Failure in setting or verification
}


/******************************************************************************
* Function: QL_hplmn_search_get
* Description: This function queries the current setting of the HPLMN (Home 
*              Public Land Mobile Network) search control using the AT command 
*              'AT+QNWCFG="hplmnsearch_ctrl"'. It creates a response object for 
*              the AT command, sends the command, and parses the response to 
*              extract the HPLMN search control setting. The function handles 
*              any memory or command execution failures and returns the current 
*              setting on success.
* Return: 
*   u8_t - Returns the current HPLMN search control setting on success.
*          Returns -1 if there is a memory allocation issue, if the command 
*          execution fails, or if the response parsing fails.
******************************************************************************/
static u8_t QL_hplmn_search_get(void) {
    at_response_t resp = NULL;

    // Creating a response object for AT command
    resp = at_create_resp(64, 0, rt_tick_from_millisecond(500));
    if (resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to query HPLMN search control setting
    if (at_exec_cmd(resp, "AT+QNWCFG=\"hplmnsearch_ctrl\"") < 0) {
        LOG_E("AT+QNWCFG=\"hplmnsearch_ctrl\" query failed.\n");
        at_delete_resp(resp);
        return -1;
    }

    // Parsing the response to extract HPLMN search control setting
    for (u8_t i = 0; i < resp->line_counts; i++) {
        const char *line = at_resp_get_line(resp, i + 1);

        int enable;
        if (sscanf(line, "+QNWCFG: \"hplmnsearch_ctrl\",%d", &enable) == 1) {
            LOG_I("HPLMN search control setting: %d.\n", enable);
            at_delete_resp(resp);
            return enable; // Return the HPLMN search control setting
        }
    }

    // If the expected format is not found in the response
    LOG_E("HPLMN search control setting query failed.\n");
    at_delete_resp(resp);
    return -1; // Return error if query fails
}

/******************************************************************************
* Function: QL_hplmn_search_set
* Description: This function sets the HPLMN (Home Public Land Mobile Network) 
*              search control mode using the AT command 'AT+QNWCFG="hplmnsearch_ctrl"'. 
*              It sends the command with the specified mode, waits for the setting 
*              to take effect, and then queries the current HPLMN search control 
*              setting to verify the changes. The function handles any memory or 
*              command execution failures and verifies if the new setting matches 
*              the specified mode.
* Parameters:
*   u8_t mode - The HPLMN search control mode to set.
* Return: 
*   int - Returns SUCCESS (0) on successful setting and verification of the 
*         HPLMN search control mode.
*         FAILURE (-1) if there is a memory allocation issue, if the command 
*         execution fails, or if the verification fails.
******************************************************************************/
static int QL_hplmn_search_set(u8_t mode) {
    at_response_t resp = NULL;

    // Creating a response object for AT command
    resp = at_create_resp(32, 0, rt_tick_from_millisecond(500));
    if (resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to set the HPLMN search control mode
    if (at_exec_cmd(resp, "AT+QNWCFG=\"hplmnsearch_ctrl\",%d", mode) < 0) {
        LOG_E("AT+QNWCFG=\"hplmnsearch_ctrl\" set command failed.\n");
        at_delete_resp(resp);
        return -1;
    }

    // Delay to allow settings to take effect
    osDelay(500);

    // Querying the current HPLMN search control setting for verification
    at_response_t query_resp = NULL;
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(2000));
    if (query_resp == NULL) {
        LOG_E("No memory for query response object.\n");
        at_delete_resp(resp);
        return -1;
    }

    // Preparing the query result string for verification
    char query_result[32];
    snprintf(query_result, sizeof(query_result), "+QNWCFG: \"hplmnsearch_ctrl\",%d", mode);

    // Sending the query command
    if (at_exec_cmd(query_resp, "AT+QNWCFG=\"hplmnsearch_ctrl\"") == 0) {
        for (int i = 0; i < query_resp->line_counts; i++) {
            const char *line = at_resp_get_line(query_resp, i + 1);
            LOG_D("query_resp line [%d]: %s", i, line);

            // Check if the returned line contains the set value
            if (strstr(line, query_result) != NULL) {
                LOG_I("HPLMN search control setting verification successful.\n");
                at_delete_resp(resp);
                at_delete_resp(query_resp);
                return 0; // Successful setting
            }
        }
    }

    LOG_E("Set HPLMN search control failed or verification failed.\n");
    at_delete_resp(resp);
    at_delete_resp(query_resp);
    return -1; // Failure in setting or verification
}

/******************************************************************************
* Function: QL_snrscan_set
* Description: This function sets the Signal-to-Noise Ratio (SNR) scan level 
*              using the AT command 'AT+QCFG="snrscan"'. It sends the command 
*              with the specified level, waits for the setting to take effect, 
*              and then queries the current SNR scan setting to verify the 
*              changes. The function handles any memory or command execution 
*              failures and verifies if the new setting matches the specified 
*              level.
* Parameters:
*   u8_t level - The SNR scan level to set.
* Return: 
*   int - Returns SUCCESS (0) on successful setting and verification of the 
*         SNR scan level.
*         FAILURE (-1) if there is a memory allocation issue, if the command 
*         execution fails, or if the verification fails.
******************************************************************************/
static int QL_snrscan_set(u8_t level) {
    at_response_t resp = NULL;

    // Creating a response object for AT command
    resp = at_create_resp(32, 0, rt_tick_from_millisecond(500));
    if (resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to set the SNR scan level
    if (at_exec_cmd(resp, "AT+QCFG=\"snrscan\",%d", level) < 0) {
        LOG_E("AT+QCFG=\"snrscan\" set command failed.\n");
        at_delete_resp(resp);
        return -1;
    }

    // Delay to allow settings to take effect
    osDelay(500);

    // Querying the current SNR scan setting for verification
    at_response_t query_resp = NULL;
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(2000));
    if (query_resp == NULL) {
        LOG_E("No memory for query response object.\n");
        at_delete_resp(resp);
        return -1;
    }

    // Preparing the query result string for verification
    char query_result[32];
    snprintf(query_result, sizeof(query_result), "+QCFG: \"snrscan\",%d", level);

    // Sending the query command
    if (at_exec_cmd(query_resp, "AT+QCFG=\"snrscan\"") == 0) {
        for (int i = 0; i < query_resp->line_counts; i++) {
            const char *line = at_resp_get_line(query_resp, i + 1);
            LOG_D("query_resp line [%d]: %s", i, line);

            // Check if the returned line contains the set value
            if (strstr(line, query_result) != NULL) {
                LOG_I("SNR scan setting verification successful.\n");
                at_delete_resp(resp);
                at_delete_resp(query_resp);
                return 0; // Successful setting
            }
        }
    }

    LOG_E("Set SNR scan level failed or verification failed.\n");
    at_delete_resp(resp);
    at_delete_resp(query_resp);
    return -1; // Failure in setting or verification
}
/******************************************************************************
* Function: QL_snrscan_get
* Description: This function queries the current Signal-to-Noise Ratio (SNR) 
*              scan level setting using the AT command 'AT+QCFG="snrscan"'. It 
*              creates a response object for the AT command, sends the command, 
*              and parses the response to extract the SNR scan level setting. 
*              The function handles any memory or command execution failures 
*              and returns the current setting on success.
* Return: 
*   u8_t - Returns the current SNR scan level setting on success.
*          Returns -1 if there is a memory allocation issue, if the command 
*          execution fails, or if the response parsing fails.
******************************************************************************/
static u8_t QL_snrscan_get(void) {
    at_response_t resp = NULL;

    // Creating a response object for AT command
    resp = at_create_resp(32, 0, rt_tick_from_millisecond(500));
    if (resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to query SNR scan level setting
    if (at_exec_cmd(resp, "AT+QCFG=\"snrscan\"") < 0) {
        LOG_E("AT+QCFG=\"snrscan\" query failed.\n");
        at_delete_resp(resp);
        return -1;
    }

    // Parsing the response to extract SNR scan level setting
    for (u8_t i = 0; i < resp->line_counts; i++) {
        const char *line = at_resp_get_line(resp, i + 1);

        u8_t level;
        if (sscanf(line, "+QCFG: \"snrscan\",%d", &level) == 1) {
            LOG_I("SNR scan level setting: %d.\n", level);
            at_delete_resp(resp);
            return level; // Return the SNR scan level setting
        }
    }

    // If the expected format is not found in the response
    LOG_E("SNR scan level setting query failed.\n");
    at_delete_resp(resp);
    return -1; // Return error if query fails
}

ip_addr_t QL_bg95_net_get_ip(void)
{
    return g_module_addr;
}

/******************************************************************************
* Function: QL_AT_conn_ind_cb
* Description: This callback function is called to handle the result of an AT 
*              command connection attempt. It logs the result of the connection 
*              attempt and, in case of success, sends a broadcast message 
*              indicating that the SIM card initialization process should start.
* Parameters:
*   u8_t result - The result of the AT command connection attempt. 
*                 A value of 0 indicates success, while any other value 
*                 indicates an error.
******************************************************************************/
void QL_AT_conn_ind_cb(u8_t result)
{
    // Check if the result indicates a successful AT command connection
    if (result == 0)
    {
        LOG_D("AT connected successfully!\n");
        // Send a broadcast message to indicate SIM card initialization should start
        net_send_bcast_msg(MSG_WHAT_BG95_SIM_START, 0, 0, 0);
    }
    else
    {
        // Log an error if the AT command connection failed
        LOG_E("AT connection error.\n");
    }
}


typedef void (*AT_conn_ind_cb_t)(u8_t result);

/******************************************************************************
* Function: QL_module_init
* Description: This function initializes the module by connecting to the AT 
*              client, disabling the echo function, and saving the settings. It 
*              uses a callback function to notify the result of the connection 
*              attempt. The function handles the connection process, sends the 
*              necessary AT commands to configure the module, and logs the 
*              responses.
* Parameters:
*   AT_conn_ind_cb_t cb - A callback function to be called with the result of 
*                         the module connection attempt.
* Return: 
*   int - Returns SUCCESS (0) on successful initialization of the module.
*         FAILURE (-1) if there is an error in the connection process or 
*         in sending AT commands.
******************************************************************************/
int QL_module_init(AT_conn_ind_cb_t cb)
{
    at_response_t resp;
    at_client_t client = NULL;
    int ret = -1;

    // 1. Get AT client
    client = at_client_get_first();

    // 2. Connect to the module
    ret = at_client_obj_wait_connect(client, rt_tick_from_millisecond(20000));
    if (client->resp_status == AT_RESP_OK)
    {
        LOG_I("Module connection successful %d", ret);
        if(ret != 0)
        {
            return ret; // Return error code if connection is unsuccessful
        }
        if (cb)
        {
            cb(0); // Call the callback function with success
        }
    }
    else
    {
        LOG_E("Module connection failure %d", ret);
        if (cb)
        {
            cb(1); // Call the callback function with failure
        }
        return -1;
    }

    // 3. Disable echo
    resp = at_create_resp(1024, 0, rt_tick_from_millisecond(300));
    at_exec_cmd(resp, "ATE0");
    for (int i = 0; i < resp->line_counts; i++)
    {
        LOG_D("resp line [%d]: %s", i, at_resp_get_line(resp, i + 1));
    }
    if (client->resp_status == AT_RESP_OK)
    {
        LOG_I("ATE0 command successful");
    }
    else
    {
        LOG_E("ATE0 command failed");
        at_delete_resp(resp);
        return -1; // Return error if ATE0 command fails
    }
    at_delete_resp(resp);

    // Save settings
    resp = at_create_resp(1024, 0, rt_tick_from_millisecond(300));
    at_exec_cmd(resp, "AT&W");
    at_delete_resp(resp);
    
    return 0; // Return success
}

/******************************************************************************
* Function: QL_emm_reject_cause_get
* Description: This function queries the EPS Mobility Management (EMM) reject 
*              cause value from the module using the AT command 'AT+QCFG="emmcause"'. 
*              It creates a response object for the AT command, sends the 
*              command, and parses the response to extract the EMM reject cause 
*              value. The function handles any memory or command execution 
*              failures and returns the EMM reject cause value on success.
* Return: 
*   int - Returns the EMM reject cause value on success.
*         Returns -1 if there is a memory allocation issue, if the command 
*         execution fails, or if the response parsing fails.
******************************************************************************/
static int QL_emm_reject_cause_get(void) {
    at_response_t query_resp = NULL;

    // Creating a response object for AT command
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(500));
    if (query_resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to query EMM reject cause value
    if (at_exec_cmd(query_resp, "AT+QCFG=\"emmcause\"") < 0) {
        LOG_E("AT+QCFG=\"emmcause\" query failed.\n");
        at_delete_resp(query_resp);
        return -1;
    }

    // Parsing the response to extract EMM reject cause value
    for (int i = 0; i < query_resp->line_counts; i++) {
        const char *line = at_resp_get_line(query_resp, i + 1);
        LOG_D("query_resp line [%d]: %s", i, line);

        int cause_value;
        if (sscanf(line, "+QCFG: \"emmcause\",%d", &cause_value) == 1) {
            LOG_I("EMM cause value: %d.\n", cause_value);
            at_delete_resp(query_resp);
            return cause_value; // Return the EMM reject cause value
        }
    }

    // If the expected format is not found in the response
    LOG_E("EMM cause value query failed.\n");
    at_delete_resp(query_resp);
    return -1; // Return error if query fails
}


/******************************************************************************
* Function: QL_net_iot_set
* Description: This function sets the network category mode for LTE RAT using 
*              the AT command 'AT+QCFG="iotopmode"'. It sends the command with 
*              the specified mode, waits for the setting to take effect, and 
*              then queries the current network category setting to verify the 
*              changes. The function handles any memory or command execution 
*              failures and verifies if the new setting matches the specified 
*              mode.
* Parameters:
*   QL_lte_mode mode - The LTE RAT network category mode to set.
* Return: 
*   int - Returns SUCCESS (0) on successful setting and verification of the 
*         network category mode.
*         FAILURE (-1) if there is a memory allocation issue, if the command 
*         execution fails, or if the verification fails.
******************************************************************************/
static int QL_net_iot_set(QL_lte_mode mode) {
    at_response_t resp = NULL;

    // Creating a response object for AT command
    resp = at_create_resp(32, 0, rt_tick_from_millisecond(2000));
    if (resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to set the LTE RAT network category mode
    if (at_exec_cmd(resp, "AT+QCFG=\"iotopmode\",%d,1", mode) < 0) {
        LOG_E("AT+QCFG=\"iotopmode\" set command failed.\n");
        at_delete_resp(resp);
        return -1;
    }

    // Delay to allow settings to take effect
    osDelay(500);

    // Querying the current network category setting for verification
    at_response_t query_resp = NULL;
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(2000));
    if (query_resp == NULL) {
        LOG_E("No memory for query response object.\n");
        at_delete_resp(resp);
        return -1;
    }

    // Preparing the query result string for verification
    char query_result[32];
    snprintf(query_result, sizeof(query_result), "+QCFG: \"iotopmode\",%d", mode);

    // Sending the query command
    if (at_exec_cmd(query_resp, "AT+QCFG=\"iotopmode\"") == 0) {
        for (int i = 0; i < query_resp->line_counts; i++) {
            const char *line = at_resp_get_line(query_resp, i + 1);
            LOG_D("query_resp line [%d]: %s", i, line);

            // Check if the returned line contains the set value
            if (strstr(line, query_result) != NULL) {
                LOG_I("LTE RAT network category mode setting verification successful.\n");
                at_delete_resp(resp);
                at_delete_resp(query_resp);
                return 0; // Successful setting
            }
        }
    }

    LOG_E("Set LTE RAT network category mode failed or verification failed.\n");
    at_delete_resp(resp);
    at_delete_resp(query_resp);
    return -1; // Failure in setting or verification
}


/******************************************************************************
* Function: QL_net_iot_get
* Description: This function queries the current network category mode for LTE 
*              RAT using the AT command 'AT+QCFG="iotopmode"'. It creates a 
*              response object for the AT command, sends the command, and 
*              parses the response to extract the LTE RAT network category mode. 
*              The function handles any memory or command execution failures 
*              and returns the current mode on success.
* Return: 
*   QL_lte_mode - Returns the current LTE RAT network category mode on success.
*                 Returns -1 if there is a memory allocation issue, if the 
*                 command execution fails, or if the response parsing fails.
******************************************************************************/
static QL_lte_mode QL_net_iot_get(void) {
    at_response_t query_resp = NULL;

    // Creating a response object for AT command
    query_resp = at_create_resp(32, 0, rt_tick_from_millisecond(300)); // Set max response time to 300ms
    if (query_resp == NULL) {
        LOG_E("No memory for response object.\n");
        return -1;
    }

    // Sending the AT command to query LTE RAT network category mode
    if (at_exec_cmd(query_resp, "AT+QCFG=\"iotopmode\"") < 0) {
        LOG_E("AT+QCFG=\"iotopmode\" query failed.\n");
        at_delete_resp(query_resp);
        return -1;
    }

    // Parsing the response to extract network category mode
    for (int i = 0; i < query_resp->line_counts; i++) {
        const char *line = at_resp_get_line(query_resp, i + 1);
        LOG_D("query_resp line [%d]: %s", i, line);

        int mode;
        if (sscanf(line, "+QCFG: \"iotopmode\",%d", &mode) == 1) {
            LOG_I("LTE RAT network category mode: %d.\n", mode);
            at_delete_resp(query_resp);
            return mode; // Return the LTE RAT network category mode
        }
    }

    // If the expected format is not found in the response
    LOG_E("LTE RAT network category mode query failed.\n");
    at_delete_resp(query_resp);
    return -1; // Return error if query fails
}

/******************************************************************************
* Function: QL_net_server_proc
* Description: This function is a network service process that continuously 
*              listens for messages and performs actions based on the received 
*              messages. It handles module initialization, SIM card state 
*              queries, network start, data call initiation, and network 
*              keep-alive checks. The process includes handling retries, 
*              configuring network settings, and responding to different 
*              network states.
* Parameters:
*   void *argument - Pointer to the argument passed to the thread (unused).
******************************************************************************/
static void QL_net_server_proc(void *argument)
{
    int ret;
    msg_node msgs;
    u8_t retryCount = 0;
    

    u8_t lte_retryCount = 0;
    u8_t rat_set_flag = 0;
    u8_t iot_set_flag = 0;
    int gsm_cs_mode = -1;
    int gsm_ps_mode = -1;
    int lte_mode = -1;
    int gsm_cs_status_flag = -1;
    int gsm_ps_status_flag = -1;
    int lte_status_flag = -1;
    Module_Config myConfig;

    memcpy(&myConfig, (Module_Config *)argument, sizeof(Module_Config));
    // Continuously listen for messages
    while(1)
    {
        // Receive messages from the broadcast thread
        ret = OsalMsgRcv(g_bg95_net_msg_id, (void *)&msgs, NULL, portMAX_DELAY);
		if (ret < 0)
		{
			LOG_E("Receive msg from broadcast thread error!\n");
			continue;
		}
        else
        {
            // Switch between different types of messages
            LOG_D("Receive broadcast msg is what=%d, arg1=%d, arg2=%d, arg3=%d\n", msgs.what, msgs.arg1, msgs.arg2, msgs.arg3);
            switch (msgs.what)
            {
               case MSG_WHAT_BG95_MODULE_INIT:
                    // Handle module initialization
                    ret = QL_module_init(QL_AT_conn_ind_cb);
                    if(ret != 0)
                    {
                        bcast_send_bcast_msg(MSG_WHAT_BG95_MODULE_FAILURE, 0, 0, 0);
                    }
                    break;
                case MSG_WHAT_BG95_SIM_START:
                    // Handle SIM state query and start network process
                    ret = QL_sim_state_query();
                    if (0 == ret)
                    {
                        retryCount=0;
                        net_send_bcast_msg(MSG_WHAT_BG95_NW_START, 0, 0, 0);  //
                    }
                    else
                    {
                        if(retryCount < 3)
                        {
                            QL_air_mode_set(4);
                            osDelay(2000);
                            QL_air_mode_set(1);
                            osDelay(6000);
                            net_send_bcast_msg(MSG_WHAT_BG95_SIM_START, 0, 0, 0);  //
                        }
                        else if(retryCount==3)
                        {
                            QL_module_reboot();
                            osDelay(20000);
                            net_send_bcast_msg(MSG_WHAT_BG95_MODULE_INIT, 0, 0, 0);  //
                        }
                        else
                        {
                            retryCount=0;
                            bcast_send_bcast_msg(MSG_WHAT_BG95_SIM_READY_FAILURE, 0, 0, 0);  //Send data call successful broadcast
                        }
                        retryCount ++ ;
                	}
                    break;

                case MSG_WHAT_BG95_NW_START:
                   // Handle network start process, including RAT and IoT settings
                   //    QL_band_clean();
                 if(QL_net_rat_get() != myConfig.rat_mode )
                 {
                    if(QL_net_rat_set(myConfig.rat_mode) != 0) 
                    {
                        LOG_E("QL_net_rat_set ERROR\n");
                    }
                    else
                    {
                         rat_set_flag = 1;
                    }

                 }

                char *GetAPN = QL_apn_get(1);
                if (GetAPN != NULL) {
                 
                     LOG_I("QL_apn_set\n");
                    if (strcmp(GetAPN, myConfig.apn) != 0) {
                    QL_apn_set(1,1, &myConfig);
                     free(GetAPN); 
                    } 
                }

           
                if(myConfig.workType == Roaming_mode )
                {
                    if(QL_hplmn_search_get() != 0 )
                    {
                        if(QL_hplmn_search_set(0) != 0) 
                        {
                        LOG_E("QL_net_rat_set ERROR\n");
                        }
                    }

                    if(myConfig.lte_mode != lte_eMTC_only )
                    {
                        if(QL_snrscan_get() != myConfig.NB_scan_level )
                        {
                            if(QL_snrscan_set(myConfig.NB_scan_level) != 0) 
                            {
                            LOG_E("QL_net_rat_set ERROR\n");
                            }
                        }
                    }
                }
                if(myConfig.workType == Private_mode )
                {
                    QL_lock_cell_t cell_to_lock;
                    cell_to_lock.pci = 314;      
                    cell_to_lock.earfcn = 1850;  
                
                    if(QL_operator_set(1,&myConfig) != 0)
                   {
                    LOG_E("QL_operator_set ERROR\n"); 
                   }
                   if (QL_net_cell_lock(0, &cell_to_lock) != 0 )
                   {
                    LOG_E("QL_net_cell_lock ERROR\n"); 
                   }

                }
  
                if(myConfig.workType == Fastly_mode )
                {
                   if(QL_net_iot_get() != myConfig.lte_mode )
                    {
                        if(QL_net_iot_set(myConfig.lte_mode) != 0) 
                        {
                         LOG_E("QL_net_rat_set ERROR\n");
                        }
                        else
                        {
                            iot_set_flag = 1;
                        }
                    }
                //   if( QL_band_set(0,0x80000,0x800) != 0 )
                //   {
                //      LOG_E("QL_band_set ERROR\n");
                //   }
                }
                if(QL_net_iot_get() != lte_eMTC_NBlOT && myConfig.rat_mode == rat_set_auto )
                {
                    if(QL_net_iot_set(lte_eMTC_NBlOT) != 0) 
                    {
                    LOG_E("QL_net_rat_set ERROR\n");
                    }
                    else
                    {
                     iot_set_flag = 1;
                    }
                }
                
                if(rat_set_flag==1||iot_set_flag==1)
                {
                    iot_set_flag = 0;
                    rat_set_flag = 0;
                    QL_module_reboot();
                    osDelay(5000);
                    net_send_bcast_msg(MSG_WHAT_BG95_MODULE_INIT, 0, 0, 0);  //
                }
                
NW_START:
                if (  myConfig.rat_mode == rat_set_auto || myConfig.rat_mode == rat_set_lte_only )
                {

                    lte_mode = QL_net_state_lte_get();
                    if (lte_mode == 1 || lte_mode == 5) 
                    {
                     lte_status_flag=1;
                    }
                    else
                    {
                     LOG_W("QL_net_state_lte_get NW ERROR\n"); 
                    }
                    if (lte_mode == 3)
                    {
                    int lte_cause= QL_emm_reject_cause_get();
                    LOG_E("QL_emm_reject_cause_get %d\n",lte_cause); 
                    }

                }
                if( myConfig.rat_mode == rat_set_auto || myConfig.rat_mode == rat_set_gsm_only)
                 {
                    
                   gsm_cs_mode = QL_net_state_2gcs_get();
                   if (gsm_cs_mode == 1 || gsm_cs_mode == 5) 
                   {
                     gsm_cs_status_flag=1;
                   } 
                   else
                   {
                     LOG_E("QL_net_state_2gcs_get NW ERROR\n"); 
                   }
     
                   if( gsm_cs_mode!= 1 || gsm_cs_mode!= 5)
                   {
                    
                   #if 0
                    LOG_E("QL_net_state_2gcs_get retryCount %d\n",retryCount); 
                    if(gsm_cs_retryCount>=1)
                    {
                      gsm_cs_retryCount=0;  
                      bcast_send_bcast_msg(MSG_WHAT_BG95_MODULE_FAILURE, 0, 0, 0);
                    }
                    else
                    {
                        QL_module_reboot();
                        osDelay(5000);
                         gsm_cs_retryCount++;
                          LOG_E("QL_net_state_2gcs_get retryCount %d\n",retryCount); 
                        net_send_bcast_msg(MSG_WHAT_BG95_MODULE_INIT, 0, 0, 0);  //
                    }
                     break;
                     #endif
                   }
                  #if 0
                   gsm_ps_mode = QL_net_state_2gps_get();
                    if(gsm_ps_mode != 0)
                   {
                    LOG_E("QL_net_state_2gps_get NW ERROR\n"); 
                       
                              if(gsm_ps_retryCount>=1)
                    {
                      gsm_ps_retryCount=0;  
                      bcast_send_bcast_msg(MSG_WHAT_BG95_MODULE_FAILURE, 0, 0, 0);
                    }
                    else
                    {
                        QL_module_reboot();
                        osDelay(5000);
                         gsm_ps_retryCount++;
                        net_send_bcast_msg(MSG_WHAT_BG95_MODULE_INIT, 0, 0, 0);  //
                    }
                    break;
                  
                    
                   }
                    #endif
                 }


                if (lte_status_flag ==1 || gsm_cs_status_flag == 1 || gsm_ps_status_flag ==1)
                {
               //     osTimerStart(g_bg95_net_timer_id, 20000);
                    lte_retryCount=0;
                    net_send_bcast_msg(MSG_WHAT_BG95_DATACALL_START, 0, 0, 0);  //Send data call successful broadcast
                     break;
                }
                else
                {
                     if(gsm_ps_mode ==2 || gsm_ps_mode ==4 || gsm_cs_mode ==2 || gsm_cs_mode ==4 || lte_mode == 2|| gsm_ps_mode ==0|| gsm_cs_mode ==0 || lte_mode == 0)
                     {
                       if(lte_retryCount < 20 )
                        {
                       
                          osDelay(2000);
                         lte_retryCount++;
                          goto NW_START;
                       
                        }
                        else if(lte_retryCount == 20)
                        {
                           QL_module_reboot();
                           osDelay(5000);
                           lte_retryCount++;
                           net_send_bcast_msg(MSG_WHAT_BG95_MODULE_INIT, 0, 0, 0);  //
                            break;
                        }
                        else
                        {
                          lte_retryCount=0;
                         bcast_send_bcast_msg(MSG_WHAT_BG95_NET_NET_REG_FAILURE, 0, 0, 0);
                         break;
                        }

                        
                     }
                    else if(gsm_ps_mode ==3  || gsm_cs_mode ==3 || lte_mode == 3)
                    {
                        if(lte_retryCount < 2 )
                        {
                            QL_air_mode_set(4);
                            osDelay(2000);
                            QL_air_mode_set(1);
                            osDelay(20000);
                            lte_retryCount++;
                            goto NW_START;
                        }
                        else if(lte_retryCount ==2)
                        {
                        QL_module_reboot();
                        osDelay(5000);
                        lte_retryCount++;
                        net_send_bcast_msg(MSG_WHAT_BG95_MODULE_INIT, 0, 0, 0);  //
                        break;
                        }
                        else
                        {
                        lte_retryCount=0;
                        bcast_send_bcast_msg(MSG_WHAT_BG95_NET_NET_REG_FAILURE, 0, 0, 0);
                        break;
                        }
                    }
           
                  
                  
                }
                 break;    
        

                case MSG_WHAT_BG95_DATACALL_START:
                     ret = QL_check_datacall_state();
                        if (0 == ret)
                        {
                            bcast_send_bcast_msg(MSG_WHAT_BG95_NET_DATACALL_SUCCESS, 0, 0, 0);  //Send data call successful broadcast
                        }
                       else if(1 == ret)
                     {
                        ret = QL_start_datacall();
                        if (0 == ret)
                        {
                            ret = QL_check_datacall_state();
                            if (0 == ret)
                            {
                                bcast_send_bcast_msg(MSG_WHAT_BG95_NET_DATACALL_SUCCESS, 0, 0, 0);  //Send data call successful broadcast
                            }
                        }
                        else if(-2 == ret)
                        {
                            QL_air_mode_set(0);
                            osDelay(2000);
                            QL_air_mode_set(1);
                            osDelay(6000);
                            net_send_bcast_msg(MSG_WHAT_BG95_NW_START, 0, 0, 0);  //Send data call successful broadcast
                        } 
                     }
  
                    break;

                case MSG_WHAT_BG95_NET_KEEPALIVE:
                	struct QL_CellInfo cellInfo;
                    QL_NeighbourCellInfo neighbor_info[2];//鍙悳�??2涓鍖?
                  
                 if( myConfig.rat_mode == rat_set_auto || myConfig.rat_mode == rat_set_gsm_only)
                 {
                   
                   gsm_cs_mode = QL_net_state_2gcs_get();
                     
                   gsm_ps_mode = QL_net_state_2gps_get();
                 }

                if (  myConfig.rat_mode == rat_set_auto || myConfig.rat_mode == rat_set_lte_only )
                {
                    
                    lte_mode = QL_net_state_lte_get();
                }
					  if(gsm_ps_mode ==1 || gsm_ps_mode ==5 || gsm_cs_mode ==1 || gsm_cs_mode ==5 || lte_mode == 1|| lte_mode == 5)
					{
                        LOG_D("Note that the network is normal\n");
                        osDelay(1000);
                        // 璋冪敤鎮ㄧ殑鍑芥暟鏉ュ～鍏卌ellInfo缁撴�?
                        if (QL_cell_info_get(&cellInfo) == 0) {
                        // 鎵撳嵃鑾峰彇鍒扮殑淇℃伅
                        LOG_D("MCC: %d\n", cellInfo.mcc);
                        LOG_D("MNC: %d\n", cellInfo.mnc);
                        LOG_D("PCI: %d\n", cellInfo.pci);
                        LOG_D("EARFCN: %d\n", cellInfo.earfcn);
                        LOG_D("BAND: %d\n", cellInfo.band);
                        LOG_D("RSRP: %d\n", cellInfo.rsrp);
                        LOG_D("SINR: %d\n", cellInfo.sinr);
						} else {
							// 澶勭悊閿欒
							LOG_D("Failed to retrieve cell information.\n");
						}
						osDelay(1000);
						int parsed_count = QL_neig_info_get(neighbor_info, 2);
						if (parsed_count > 0) {
							for (int i = 0; i < parsed_count; i++) {
								LOG_D("Neighbour %d - EARFCN: %d, PCI: %d, RSRP: %d, SINR: %d\n", i+1, neighbor_info[i].EARFCN, neighbor_info[i].PCI, neighbor_info[i].RSRP, neighbor_info[i].SINR);
							}
						} else {
							LOG_D("Failed to query neighbour cell info.\n");
						}
					}
					else
					{
						LOG_D("NW network disconnected \n");
                            if (lte_mode == 3)
                            {
                            int lte_cause= QL_emm_reject_cause_get();
                            LOG_E("QL_emm_reject_cause_get %d\n",lte_cause); 
                            }
                            QL_air_mode_set(0);
                        osDelay(2000);
                        QL_air_mode_set(1);
                        osDelay(6000);
                        net_send_bcast_msg(MSG_WHAT_BG95_NW_START, 0, 0, 0);  
					   bcast_send_bcast_msg(MSG_WHAT_BG95_NET_NET_REG_FAILURE, 0, 0, 0);  //Send data call successful broadcast
					   break;
					}
                    break;
                case MSG_WHAT_BG95_NET_DATACALL_SUCCESS:
                    LOG_D("Do something MSG_WHAT_BG95_NET_DATACALL_SUCCESS\n");
                    break;

                default:
                    LOG_D("Unrecognized message 0x%x\n", msgs.what);
                    break;
            }
        }
    }
    os_thread_exit();
}
void checkNwCallback(void *argument) {
	 LOG_D("MSG_WHAT_BG95_NET_KEEPALIVE\n");

	net_send_bcast_msg(MSG_WHAT_BG95_NET_KEEPALIVE, 0, 0, 0);  //

}
int QL_bg95_net_create(Module_Config *config)
{
    int ret = 0;
    static osThreadAttr_t thread_attr = {.name = "Net_S", .stack_size = 512*6, .priority = osPriorityNormal};
    
    LOG_V("%s",__FUNCTION__);	

    //1. Creat msg queue
   	g_bg95_net_msg_id = OsalMsgQCreate(MAX_MSG_COUNT, sizeof(msg_node), NULL);
	if (NULL == g_bg95_net_msg_id)
	{
		LOG_E("Create QL_bg95_net_create msg failed!\n"); 
		return -1;			
	}
   // 创建定时�?
    // myTimer = osTimerNew(TimerCallback, osTimerOnce, NULL, &timer_attributes);
    //2. Creat time to keepalive 
    g_bg95_net_timer_id = OsalTimerCreate(checkNwCallback, osTimerPeriodic, NULL, NULL);
    if (g_bg95_net_timer_id == NULL) {
    	LOG_E("Failed to create timer.\n");
    	return -1;
    }
   // osTimerStart(g_bg95_net_timer_id, 1000);
    //3. Register which broadcasts need to be processed
    bcast_reg_receive_msg(MSG_WHAT_BG95_NET_NET_REG_SUCCESS,  g_bg95_net_msg_id);
    bcast_reg_receive_msg(MSG_WHAT_BG95_NET_NET_REG_FAILURE,  g_bg95_net_msg_id);
    bcast_reg_receive_msg(MSG_WHAT_BG95_NET_DATACALL_SUCCESS, g_bg95_net_msg_id);
    bcast_reg_receive_msg(MSG_WHAT_BG95_NET_DATACALL_FAILURE, g_bg95_net_msg_id);

    //4. Create net service
    g_bg95_net_thread_id = os_thread_create(QL_net_server_proc, config, &thread_attr);
	if (NULL == g_bg95_net_thread_id)
	{
		LOG_E ("bg95_net_service_create thread could not start!");
        LOG_V("%s, stack space %d",__FUNCTION__, os_thread_get_stack_space(os_thread_get_thread_id()));	

		return -1;
	}

    //5. Now module start run
    net_send_bcast_msg(MSG_WHAT_BG95_MODULE_INIT, 0, 0, 0);  //

    LOG_I("%s over",__FUNCTION__);	

    return 0;
}

int QL_bg95_net_destroy(void)
{
    int ret = 0;

	LOG_V("%s",__FUNCTION__);	
    
    //1. Deal with del network problems

    //2. UnRegister which broadcasts need to be processed
    bcast_unreg_receive_msg(MSG_WHAT_BG95_NET_NET_REG_SUCCESS,  g_bg95_net_msg_id);
    bcast_unreg_receive_msg(MSG_WHAT_BG95_NET_NET_REG_FAILURE,  g_bg95_net_msg_id);
    bcast_unreg_receive_msg(MSG_WHAT_BG95_NET_DATACALL_SUCCESS, g_bg95_net_msg_id);
    bcast_unreg_receive_msg(MSG_WHAT_BG95_NET_DATACALL_FAILURE, g_bg95_net_msg_id);

    //3. Destroy net service
    if (NULL != g_bg95_net_thread_id)
    {
        ret = os_thread_destroy(g_bg95_net_thread_id);
        if (0 != ret)
        {
            LOG_E("Delete g_bg95_net_thread_id thread failed! %d", ret); 
            return -1;			
        }
    }
    if (NULL != g_bg95_net_msg_id)
     {
         ret = OsalMsgQDelete(g_bg95_net_msg_id);
         if (0 != ret)
         {
             LOG_E("Delete g_bg95_net_msg_id msg failed! %d", ret);
             return -1;
         }
     }

    //4. Delete msg queue
    if (NULL != g_bg95_net_timer_id)
    {
    	OsalTimerStop(g_bg95_net_timer_id);
        ret = OsalTimerDelete(g_bg95_net_timer_id);
        if (0 != ret)
        {
            LOG_E("Delete g_bg95_net_timer_id failed! %d", ret);
            return -1;			
        }
    }
    LOG_V("%s over",__FUNCTION__);

    return 0;
}

int bg95_net_service_test(s32_t argc, char *argv[])
{
    int i;

	//LOG_V("%s",__FUNCTION__);	

    for (i=0; i<argc; i++)
    {
        //LOG_V("%d = %s", i, argv[i]);
    }

	if (strcmp((const char *)argv[0], "help")==0)
	{
        LOG_I("--------------------------------------------");
        LOG_I("| bg95_net:                                |");		
        LOG_I("--------------------------------------------");
        LOG_I("| exit                                     |");		
		LOG_I("| help                                     |");						
		LOG_I("| create                                   |");		
        LOG_I("| destroy                                  |");	
        LOG_I("| send_msg 'what' 'arg1' 'arg2' 'arg3'     |");	
        LOG_I("--------------------------------------------");
	}
    else if (strcmp((const char *)argv[0], "exit")==0)
	{
        LOG_D("exit %s", __FUNCTION__);
        return -1;
	}
	else if (strcmp((const char *)argv[0], "create")==0)
	{
		//QL_bg95_net_create();
	}
	else if (strcmp((const char *)argv[0], "destroy")==0)
	{
		QL_bg95_net_destroy();
	}
    else if (strcmp((const char *)argv[0], "send_msg")==0)
	{
		LOG_I("send_msg 0x%x 0x%x 0x%x 0x%x", strtol(argv[1], NULL, 16), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
        bcast_send_my_msg(g_bg95_net_msg_id, strtol(argv[1], NULL, 16), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
	}
    else
    {
        LOG_E("Invalid parameter:%s", argv[0]);
    }

    return 0;
}
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */