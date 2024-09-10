/*
 * sd_fatfs.c
 *
 *  Created on: Oct 18, 2023
 *      Author: barry
 */
#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_TFCARD__
#include "sd_fatfs.h"
#include "debug_service.h"
#include "fatfs.h"
// #define FF_MAX_SS 512
 extern SD_HandleTypeDef hsd;
//  BYTE work[FF_MAX_SS] = {0};
void SD_hardware_init(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
}

void SD_INIT(void)
{
	uint8_t RES;

	if (BSP_SD_IsDetected() != SD_PRESENT)
	{
		LOG_E("No SD card detected\r\n");
		return;
	}
	SD_hardware_init();

	RES = f_mount(&SDFatFS, (TCHAR const*)SDPath, 1);
	if(RES ==FR_OK)
	{
		/* FatFs Initialization Error */
		LOG_D("Fat System OK");
		/* 锟�?????娴婼D鍗℃槸鍚︽甯革紙澶勪簬鏁版嵁浼犺緭妯″紡鐨勪紶杈撶姸鎬侊�? */
		if(HAL_SD_GetCardState(&hsd) == HAL_SD_CARD_TRANSFER)
		{
			LOG_D("Initialize SD card successfully!");
			// 鎵撳嵃SD鍗″熀鏈俊锟�??????
			LOG_D("SD card information! ");
			LOG_D("CardCapacity  : %llu ", (unsigned long long)hsd.SdCard.BlockSize * hsd.SdCard.BlockNbr);// 鏄剧ず瀹归�?
			LOG_D("CardBlockSize : %d ", hsd.SdCard.BlockSize);   // 鍧楀ぇ锟�??????
			LOG_D("LogBlockNbr   : %d ", hsd.SdCard.LogBlockNbr);	// 閫昏緫鍧楁暟锟�?????
			LOG_D("LogBlockSize  : %d ", hsd.SdCard.LogBlockSize);// 閫昏緫鍧楀ぇ锟�??????
			LOG_D("RCA           : %d ", hsd.SdCard.RelCardAdd);  // 鍗＄浉瀵瑰湴锟�??????
			LOG_D("CardType      : %d ", hsd.SdCard.CardType);    // 鍗＄被锟�??????
			// 
			HAL_SD_CardCIDTypeDef sdcard_cid;
			HAL_SD_GetCardCID(&hsd,&sdcard_cid);
			LOG_D("ManufacturerID: %d ",sdcard_cid.ManufacturerID);
		}
		else
		{
			LOG_E("SD card init fail!" );
		}
	}
	else
	{
		LOG_E("Fat System Err,RES=%d!!!",RES);
	}
	if(f_mount(&SDFatFS,"0:",1) == FR_NO_FILESYSTEM)		//娌℃湁鏂囦欢绯荤粺锛屾牸寮忓�?
	{
		LOG_W("sd card NO_FILESYSTEM! ");
		// RES = f_mkfs("0:", 0, FF_MAX_SS,work,sizeof(work));
		// if(RES ==FR_OK)
		// {
		// 	LOG_I("sd card mkfs success");
		// 	RES = f_mount(NULL,"0:",1); 		//鏍煎紡鍖栧悗鍏堝彇娑堟寕锟�?????
		// 	RES = f_mount(&SDFatFS,"0:",1);			//閲嶆柊鎸傝浇
		// 	if(RES == FR_OK)
		// 	{
		// 		LOG_I("sd card mount success!");
		// 	}
		// }
		// else
		// {
		// 	LOG_E("sd card mkfs fail=%d!!!",RES);
		// 	return;
		// }
	}
	else if(RES == FR_OK)
	{
		LOG_I("sd card mount success! ");
	}
	else
	{
		LOG_E("sd card mount valit falil! ");
	    return;
	}
}


#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_TFCARD__ */