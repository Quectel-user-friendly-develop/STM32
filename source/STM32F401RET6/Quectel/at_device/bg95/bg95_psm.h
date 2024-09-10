#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_PSM__
/*
 * Copyright (c) 2024, FAE
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author           Notes
 * 2024-1-5     kartigesan       first version
 */

/*
 * NOTE: DO NOT include this file on the header file.
 */
#include <stdbool.h>
#ifndef __BG95_PSM_H__
#define __BG95_PSM_H__

typedef struct  {
	bool Mode;
	int Requested_Periodic_RAU;
	int Requested_GPRS_Ready_timer;
	int Requested_Periodic_TAU;
	int Requested_Active_Time;
} psm_setting;

typedef struct  {
	int threshold;
	int psm_version;

} psm_threshold_setting;

typedef struct  {
	int PSM_opt_mask;
	int max_oos_full_scans;
	int PSM_duration_due_to_oos;
	int PSM_randomization_window;
	int max_oos_time;
	int early_wakeup_time;
} psm_ext_cfg;

// read fucntions
int QL_psm_settings_read();
int QL_psm_threshold_settings_read();
int QL_psm_ext_cfg_read();
int QL_psm_ext_timer_read();
//write functions
int QL_psm_settings_write(psm_setting*);
int QL_psm_threshold_settings_write(psm_threshold_setting*);
int QL_psm_ext_cfg_write(psm_ext_cfg*);
int QL_psm_ext_timer_write(bool);

void QL_psm_stat();

void QL_PSM_example();

#endif /* __BG95_PSM_H__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_PSM__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */