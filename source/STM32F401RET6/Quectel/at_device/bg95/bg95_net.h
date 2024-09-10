#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__
/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-10-12     armink       first version
 */

/*
 * NOTE: DO NOT include this file on the header file.
 */
#ifndef __BG95_NET_H__
#define __BG95_NET_H__

#include "at_osal.h"


// ����洢С����Ϣ�Ľṹ��?
struct QL_CellInfo {
    int mcc;   // �ƶ����Ҵ��� (MCC)
    int mnc;   // �ƶ��������? (MNC)
    int pci;   // λ�������� (LAC)
    int earfcn; // С��ʶ���� (Cell ID)
    int band; // С��ʶ���� (Cell ID)
    int rsrp;  // �����ź�ǿ�� (RSSI)
    int sinr;  // �����ź�ǿ�� (RSSI)
};
// 定义结构体来存储领区信息
typedef struct {
    int EARFCN;    // E-UTRA Absolute Radio Frequency Channel Number
    int PCI;       // Physical Cell ID
    int RSRP;      // Reference Signal Received Power
    int SINR;      // Reference Signal Received Power
} QL_NeighbourCellInfo;

typedef struct {
    uint32_t bandval_high;    // ��32λ
    uint32_t bandval_low;     // ��32λ
    uint32_t ltebandval_high; // LTE bandֵ�ĸ�32λ
    uint32_t ltebandval_low;  // LTE bandֵ�ĵ�32λ
} QL_band_configuration_t;

typedef struct {
    int earfcn;
    int pci;
} QL_lock_cell_t;

typedef enum
{
	rat_set_auto =0,
	rat_set_gsm_only,
	rat_set_wcdma_only,
	rat_set_lte_only,
	rat_set_td_scdma_only,
	rat_set_umts_only,
	rat_set_cdma_only,
	rat_set_hdr_only,
	rat_set_cdma_and_hdr_only,
} QL_rat_mode;

// ������ö��
typedef enum {
    Normal_mode,    // ������
    Roaming_mode,   // ���ο�
    Private_mode, // ˽������
    Fastly_mode
} Work_Mode;

typedef enum
{
	lte_eMTC_only =0,
	lte_NB_IoT_only,
	lte_eMTC_NBlOT,
} QL_lte_mode;

// EC20 ���ýṹ��
typedef struct {
	Work_Mode workType;  // ������
    char apn[50];           // Access Point Name
    char mcc_mnc[7];         // MCC��MNC�����磺"46000"
    QL_rat_mode rat_mode ;        // auto =0 2g only =1  4g only =2
    QL_lte_mode lte_mode ;        // auto =0 2g only =1  4g only =2
    u8_t NB_scan_level ;

} Module_Config;

ip_addr_t QL_bg95_net_get_ip(void);
int bg95_net_service_create(void);
int bg95_net_service_destroy(void);



int bg95_net_service_test(s32_t argc, char *argv[]);

#endif /* __BG95_NET_H__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_NETWORK__ */
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_MODULE_SUPPORT_BG95__ */