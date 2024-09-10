#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_PSM__
#include <at.h>
#include "cmsis_os.h"
#include "bg95_socket.h"
#include "bg95_net.h"
#include "at_osal.h"
#include "at_socket.h"
#include "bg95_psm.h"
extern psm_setting user_psm_setting = {NULL};
extern psm_threshold_setting user_psm_threshold_setting = {NULL};
extern psm_ext_cfg user_psm_ext_cfg = {NULL};

int QL_psm_settings_read()
{
	// Create an AT response object with a 3000ms timeout and space for 4 lines.
	at_response_t resp = at_create_resp(64, 0, rt_tick_from_millisecond(3000));
	if (!resp) {
		LOG_E("No memory for response object.\n");
		return -1;
	}
//	psm_setting psm_setting_t;
	// Execute the AT command to get PSM settings
	at_exec_cmd(resp, "AT+CPSMS?");
	LOG_I("AT+CPSMS?\n");
	for (int i = 0; i < resp->line_counts; i++) {
		const char *line = at_resp_get_line(resp, i + 1);
		LOG_I("query_resp line [%d]: %s", i, line);
		if (i == 1)
		{
			sscanf(line, "+CPSMS: %d,,,\"%d\",\"%d\" ", &user_psm_setting.Mode, &user_psm_setting.Requested_Periodic_TAU, &user_psm_setting.Requested_Active_Time);
		}

		// Check if there is a "+CME ERROR" line.
		if (strstr(line, "ERROR") != NULL) {
			sscanf(line, "ERROR");
			at_delete_resp(resp);
			return -1;
		}
	}


	return 0;
}

int QL_psm_settings_write(psm_setting *val)
{
	// Create an AT response object with a 3000ms timeout and space for 4 lines.
	at_response_t resp = at_create_resp(64, 0, rt_tick_from_millisecond(3000));
	if (!resp) {
		LOG_E("No memory for response object.\n");
		return -1;
	}

	// Execute the AT command to set PSM settings
	at_exec_cmd(resp, "AT+CPSMS=%d,,,\"%08d\",\"%08d\"", val->Mode, val->Requested_Periodic_TAU, val->Requested_Active_Time);
	for (int i = 0; i < resp->line_counts; i++) {
		const char *line = at_resp_get_line(resp, i + 1);
		LOG_I("query_resp line [%d]: %s", i, line);

		// Check if there is a "+CME ERROR" line.
		if (strstr(line, "ERROR") != NULL) {
			sscanf(line, "ERROR");
			at_delete_resp(resp);
			return -1;
		}
	}

	return 0;
}
int QL_psm_threshold_settings_read()
{
	// Create an AT response object with a 3000ms timeout and space for 4 lines.
	at_response_t resp = at_create_resp(64, 0, rt_tick_from_millisecond(3000));
	if (!resp) {
		LOG_E("No memory for response object.\n");
		return -1;
	}

	// Execute the AT command to get PSM extented settings
	at_exec_cmd(resp, "AT+QPSMCFG?");
	for (int i = 0; i < resp->line_counts; i++) {
		const char *line = at_resp_get_line(resp, i + 1);
		LOG_I("query_resp line [%d]: %s", i, line);

		if (i == 1)
		{
			sscanf(line, "+QPSMCFG: %d,%d", &user_psm_threshold_setting.threshold, &user_psm_threshold_setting.psm_version);
		}

		// Check if there is a "+CME ERROR" line.
		if (strstr(line, "ERROR") != NULL) {
			sscanf(line, "ERROR");
			at_delete_resp(resp);
			return -1;
		}
	}
	return 0;
}
int QL_psm_threshold_settings_write(psm_threshold_setting *val)
{
	// Create an AT response object with a 3000ms timeout and space for 4 lines.
	at_response_t resp = at_create_resp(64, 0, rt_tick_from_millisecond(3000));
	if (!resp) {
		LOG_E("No memory for response object.\n");
		return -1;
	}

	// Execute the AT command to set PSM extented settings
	at_exec_cmd(resp, "AT+QPSMCFG=%d", val->threshold);
	LOG_I("enable PSM mode");
	for (int i = 0; i < resp->line_counts; i++) {
		const char *line = at_resp_get_line(resp, i + 1);
		LOG_I("query_resp line [%d]: %s", i, line);

		// Check if there is a "+CME ERROR" line.
		if (strstr(line, "ERROR") != NULL) {
			sscanf(line, "ERROR");
			at_delete_resp(resp);
			return -1;
		}
	}
	QL_psm_threshold_settings_read();
	return 0;
}
int QL_psm_ext_cfg_read()
{
	// Create an AT response object with a 3000ms timeout and space for 4 lines.
	at_response_t resp = at_create_resp(64, 0, rt_tick_from_millisecond(3000));
	if (!resp) {
		LOG_E("No memory for response object.\n");
		return -1;
	}

	// Execute the AT command to get PSM extented config
	at_exec_cmd(resp, "AT+QPSMEXTCFG?");
	for (int i = 0; i < resp->line_counts; i++) {
		const char *line = at_resp_get_line(resp, i + 1);
		LOG_I("query_resp line [%d]: %s", i, line);

		if (i == 1)
		{
			sscanf(line, "+QPSMEXTCFG: %d,%d,%d,%d,%d,%d", &user_psm_ext_cfg.PSM_opt_mask, &user_psm_ext_cfg.max_oos_full_scans, &user_psm_ext_cfg.PSM_duration_due_to_oos, &user_psm_ext_cfg.PSM_randomization_window, &user_psm_ext_cfg.max_oos_time, &user_psm_ext_cfg.early_wakeup_time);
			LOG_V(" %d %d %d %d %d %d", user_psm_ext_cfg.PSM_opt_mask, user_psm_ext_cfg.max_oos_full_scans, user_psm_ext_cfg.PSM_duration_due_to_oos, user_psm_ext_cfg.PSM_randomization_window, user_psm_ext_cfg.max_oos_time, user_psm_ext_cfg.early_wakeup_time);
		}

		// Check if there is a "+CME ERROR" line.
		if (strstr(line, "ERROR") != NULL) {
			sscanf(line, "ERROR");
			at_delete_resp(resp);
			return -1;
		}
	}
	return 0;
}
int QL_psm_ext_cfg_write(psm_ext_cfg *val)
{
	// Create an AT response object with a 3000ms timeout and space for 4 lines.
	at_response_t resp = at_create_resp(64, 0, rt_tick_from_millisecond(3000));
	if (!resp) {
		LOG_E("No memory for response object.\n");
		return -1;
	}

	// Execute the AT command to set PSM extented config
	at_exec_cmd(resp, "AT+QPSMEXTCFG=%d,%d,%d,%d,%d,%d", val->PSM_opt_mask,val->max_oos_full_scans,val->PSM_duration_due_to_oos,val->PSM_randomization_window,val->max_oos_time,val->early_wakeup_time);
	LOG_I("enable PSM mode");
	for (int i = 0; i < resp->line_counts; i++) {
		const char *line = at_resp_get_line(resp, i + 1);
		LOG_I("query_resp line [%d]: %s", i, line);

		// Check if there is a "+CME ERROR" line.
		if (strstr(line, "ERROR") != NULL) {
			sscanf(line, "ERROR");
			at_delete_resp(resp);
			return -1;
		}
	}
	QL_psm_ext_cfg_read();
	return 0;
}
int QL_psm_ext_timer_read()
{
	// Create an AT response object with a 3000ms timeout and space for 4 lines.
	at_response_t resp = at_create_resp(64, 0, rt_tick_from_millisecond(3000));
	if (!resp) {
		LOG_E("No memory for response object.\n");
		return -1;
	}

	// Execute the AT command to get PSM timer
	at_exec_cmd(resp, "AT+QCFG=\"psm/urc\"");
	for (int i = 0; i < resp->line_counts; i++) {
		const char *line = at_resp_get_line(resp, i + 1);
		LOG_I("query_resp line [%d]: %s", i, line);

		// Check if there is a "+CME ERROR" line.
		if (strstr(line, "ERROR") != NULL) {
			sscanf(line, "ERROR");
			at_delete_resp(resp);
			return -1;
		}
	}
	return 0;
}
int QL_psm_ext_timer_write(bool val)
{
	// Create an AT response object with a 3000ms timeout and space for 4 lines.
	at_response_t resp = at_create_resp(64, 0, rt_tick_from_millisecond(3000));
	if (!resp) {
		LOG_E("No memory for response object.\n");
		return -1;
	}

	// Execute the AT command to set PSM timer
	at_exec_cmd(resp, "AT+QCFG=\"psm/urc\",%d", val);
	for (int i = 0; i < resp->line_counts; i++) {
		const char *line = at_resp_get_line(resp, i + 1);
		LOG_I("query_resp line [%d]: %s", i, line);

		// Check if there is a "+CME ERROR" line.
		if (strstr(line, "ERROR") != NULL) {
			sscanf(line, "ERROR");
			at_delete_resp(resp);
			return -1;
		}
	}
	QL_psm_ext_timer_read();
	return 0;
}

void QL_psm_stat()
{
	QL_psm_settings_read();
	QL_psm_threshold_settings_read();
	QL_psm_ext_cfg_read();
	QL_psm_ext_timer_read();
	LOG_I("================================");
	LOG_I("\t\tPSM Stat");
	LOG_I("================================");

	if (user_psm_setting.Mode)
	{
		LOG_I("PSM Mode is enabled");
	}
	else
	{
		LOG_I("PSM Mode is disabled");
	}
	LOG_I("PSM Mode TAU : %08d", user_psm_setting.Requested_Periodic_TAU);
	LOG_I("PSM Mode Requested Active Time: %08d", user_psm_setting.Requested_Active_Time);
	LOG_I("================================");
	LOG_I("PSM Mode opt mask: %d", user_psm_ext_cfg.PSM_opt_mask);
	LOG_I("PSM Mode max oos full scans: %d", user_psm_ext_cfg.max_oos_full_scans);
	LOG_I("PSM Mode duration due to oos: %d", user_psm_ext_cfg.PSM_duration_due_to_oos);
	LOG_I("PSM Mode randomization window: %d", user_psm_ext_cfg.PSM_randomization_window);
	LOG_I("PSM Mode max oos time: %d", user_psm_ext_cfg.max_oos_time);
	LOG_I("PSM Mode early wakeup time: %d", user_psm_ext_cfg.early_wakeup_time);
	LOG_I("================================");
	LOG_I("PSM threshold: %d", user_psm_threshold_setting.threshold);
	LOG_I("PSM version: %d", user_psm_threshold_setting.psm_version);
	LOG_I("================================");
}

void QL_PSM_example()
{

	QL_psm_settings_read();
	QL_psm_threshold_settings_read();
	QL_psm_ext_cfg_read();
	QL_psm_ext_timer_read();

	QL_psm_settings_write(&user_psm_setting);
	QL_psm_threshold_settings_write(&user_psm_threshold_setting);
	QL_psm_ext_cfg_write(&user_psm_ext_cfg);
	QL_psm_ext_timer_write(true);

	QL_psm_stat();


}
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_PSM__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */