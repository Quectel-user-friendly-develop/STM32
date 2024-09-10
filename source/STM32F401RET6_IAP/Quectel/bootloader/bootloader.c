#include <stm32f4xx.h>
#include "bootloader.h"
#include "diskio.h"
#include "ff.h"
#include "stm32f4xx_hal_flash.h" 
#include "fatfs.h"

/******************************
Function name:STMFLASH_GetFlashSector
Function     :Gets the flash sector in which an address resides
argument     :
    addr     :flash address
return       :The value ranges from 0 to 7, that is, the sector where addr resides
*******************************/
static uint16_t STMFLASH_GetFlashSector(u32_t addr)
{
	if(addr<ADDR_FLASH_SECTOR_1)return 0;
	else if(addr<ADDR_FLASH_SECTOR_2)return 1;
	else if(addr<ADDR_FLASH_SECTOR_3)return 2;
	else if(addr<ADDR_FLASH_SECTOR_4)return 3;
	else if(addr<ADDR_FLASH_SECTOR_5)return 4;
	else if(addr<ADDR_FLASH_SECTOR_6)return 5;
	else if(addr<ADDR_FLASH_SECTOR_7)return 6;
	return 7;	
}

/******************************
Function name:iap_write_appbin
Function     :Firmware upgrade function
argument     :
	appxaddr :The start address of the application program must be the start address of a sector
	appbuf   :Application CODE
	appsize  :Application size (in bytes).
return       :NULL
*******************************/
static void iap_write_appbin(u32_t appxaddr,u8_t *appbuf,u32_t appsize)
{
	u32_t i = 0, align_num;
	vu32_t temp_32 = 0;
	vu8_t temp_8 = 0, sector = 0;
	u32_t WriteAddr = appxaddr, NumToWrite = appsize;
	
    if ((WriteAddr < STM32_FLASH_BASE) || (WriteAddr >= (STM32_FLASH_BASE + 1024 * STM32_FLASH_SIZE)) || (WriteAddr%4))
        return;   

    HAL_FLASH_Unlock();
	align_num = ALIGN_DOWN(NumToWrite, 4);
	for(i = 0; i < align_num; i += 4)
	{
		temp_32 = 0;
		temp_32 |= appbuf[3] << 24;
		temp_32 |= appbuf[2] << 16;
		temp_32 |= appbuf[1] << 8;
		temp_32 |= appbuf[0] << 0;
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,WriteAddr,temp_32)!=HAL_OK)//Write data
			break;
		// printf("0x%x = 0x%x, 0x%x, 0x%x, 0x%x\r\n", 		WriteAddr, appbuf[0], appbuf[1], appbuf[2], appbuf[3]); 
		// printf("0x%x = 0x%x, 0x%x, 0x%x, 0x%x\r\n\r\n", 	WriteAddr, *(vu8_t*)WriteAddr, *(vu8_t*)(WriteAddr+1), *(vu8_t*)(WriteAddr+2), *(vu8_t*)(WriteAddr+3)); 
		WriteAddr += 4;
		appbuf += 4;
		NumToWrite -= 4;
	}
	// printf("NumToWrite = %d\r\n", NumToWrite);
	if (NumToWrite != 0)
	{
		for(i = 0; i < NumToWrite; i ++)
		{
			temp_8  = appbuf;
			if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,WriteAddr,temp_8)!=HAL_OK)//Write data
				break;
			WriteAddr += 1;
			appbuf += 1;
			NumToWrite -= 1;
			// printf("0x%x = 0x%x, 0x%x, 0x%x, 0x%x\r\n", 		WriteAddr, appbuf[0], appbuf[1], appbuf[2], appbuf[3]); 
			// printf("0x%x = 0x%x, 0x%x, 0x%x, 0x%x\r\n\r\n", 	WriteAddr, *(vu8_t*)WriteAddr, *(vu8_t*)(WriteAddr+1), *(vu8_t*)(WriteAddr+2), *(vu8_t*)(WriteAddr+3)); 
		}
	}
	HAL_FLASH_Lock(); //????
}

/******************************
Function name:iap_erase_appbin_flash
Function     :
argument     :NULL
return       :NULL
*******************************/
static void iap_erase_appbin_flash(u32_t app_size)
{
	//Erase flash
	HAL_FLASH_Unlock();
	if (FLASH_APP1_ADDR + app_size >= ADDR_FLASH_SECTOR_7){
	FLASH_Erase_Sector(STMFLASH_GetFlashSector(ADDR_FLASH_SECTOR_7),FLASH_VOLTAGE_RANGE_3);printf("Erase ADDR_FLASH_SECTOR_7\r\n");}
	if (FLASH_APP1_ADDR + app_size >= ADDR_FLASH_SECTOR_6){
	FLASH_Erase_Sector(STMFLASH_GetFlashSector(ADDR_FLASH_SECTOR_6),FLASH_VOLTAGE_RANGE_3);printf("Erase ADDR_FLASH_SECTOR_6\r\n");}
	if (FLASH_APP1_ADDR + app_size >= ADDR_FLASH_SECTOR_5){
	FLASH_Erase_Sector(STMFLASH_GetFlashSector(ADDR_FLASH_SECTOR_5),FLASH_VOLTAGE_RANGE_3);printf("Erase ADDR_FLASH_SECTOR_5\r\n");}
	if (FLASH_APP1_ADDR + app_size >= ADDR_FLASH_SECTOR_4){
	FLASH_Erase_Sector(STMFLASH_GetFlashSector(ADDR_FLASH_SECTOR_4),FLASH_VOLTAGE_RANGE_3);printf("Erase ADDR_FLASH_SECTOR_4\r\n");}
	HAL_FLASH_Lock(); 
}

/******************************
Function name:FirmwareUpdate
Function     :Firmware upgrade function
argument     :NULL
return       :NULL
*******************************/
static void FirmwareUpdate(void)
{
    u8_t i=0;
    u8_t res;
    u32_t br;
    u16_t readlen;
    u32_t addrx;
    u32_t Receive_data=0; 								//Calculate the total amount of data received
    u32_t file_size=0;    								//File size
    u8_t Receive_dat_buff[STM_SECTOR_SIZE];		   		//Data receive cache array
    u8_t percent=0;       								//Percentage of firmware upgrades
	
	/*Find if the BIN file you want to upgrade exists*/
    res = f_open(&SDFile, "0:update.bin", FA_OPEN_EXISTING | FA_READ);
    file_size=f_size(&SDFile);    					//Read file size Byte
    printf("The size of the file read is:%d Byte\r\n",file_size);

	if(res!=FR_OK) return;
    addrx = FLASH_APP1_ADDR;
    
	/*Perform major IAP functions*/
    printf("Start updating firmware....\r\n");
	while(1)
	{
		/*Read 16K of data each time into the memory buffer buffer*/
	    res = f_read(&SDFile, Receive_dat_buff, STM_SECTOR_SIZE, &br);
        readlen=br;
        Receive_data+=br;  								 //Total number of bytes read
        if (res || br == 0) 
        {
			//printf("\r\nres = %d, br = %d\r\n", res, br);
            break; 
        }

		if (i++ == 0)//The first packet determines the validity of the data and erases the corresponding partition
		{
			if(((*(vu32_t*)(Receive_dat_buff+4))&0xFF000000)!=0x08000000)//Check whether the value is 0X08XXXXXX
			{
				printf("Invalid upgrade package 0x%x\r\n", Receive_dat_buff);
				break;
			}
			iap_erase_appbin_flash(file_size);
		}
        
        iap_write_appbin(addrx,Receive_dat_buff,readlen);//The read data is written to the Flash
        addrx+=STM_SECTOR_SIZE;
        percent = Receive_data*100/file_size;
		printf("[%2d\%] recive %d, total %d\r", percent, Receive_data, file_size);
    }
    f_close(&SDFile);
}

#define FF_MAX_SS 512
extern SD_HandleTypeDef hsd;
BYTE work[FF_MAX_SS] = {0};
void SD_hardware_init(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
}

void SD_INIT(void)
{
	uint8_t RES, fnum;  
	FRESULT f_res;
	char WriteBuffer[16] =  "whz"; 

	SD_hardware_init();
	RES = f_mount(&SDFatFS, (TCHAR const*)SDPath, 1);
	if(RES ==FR_OK)
	{
		/* FatFs Initialization Error */
		printf("Fat System OK\r\n");

		if(HAL_SD_GetCardState(&hsd) == HAL_SD_CARD_TRANSFER)
		{
			printf("Initialize SD card successfully!\r\n");

			printf("SD card information! \r\n");
			printf("CardCapacity  : %llu \r\n", (unsigned long long)hsd.SdCard.BlockSize * hsd.SdCard.BlockNbr);
			printf("CardBlockSize : %d \r\n", hsd.SdCard.BlockSize);  
			printf("LogBlockNbr   : %d \r\n", hsd.SdCard.LogBlockNbr);	
			printf("LogBlockSize  : %d \r\n", hsd.SdCard.LogBlockSize);
			printf("RCA           : %d \r\n", hsd.SdCard.RelCardAdd);  
			printf("CardType      : %d \r\n", hsd.SdCard.CardType);   

			HAL_SD_CardCIDTypeDef sdcard_cid;
			HAL_SD_GetCardCID(&hsd,&sdcard_cid);
			printf("ManufacturerID: %d \r\n",sdcard_cid.ManufacturerID);
		}
		else
		{
			printf("SD card init fail!\r\n" );
		}
	}
	else
	{
		printf("Fat System Err,RES=%d!!!\r\n",RES);
	}
	if(f_mount(&SDFatFS,"0:",1) == FR_NO_FILESYSTEM)		
	{
		printf("sd card NO_FILESYSTEM! \r\n");
	}
	else if(RES == FR_OK)
	{
		printf("sd card mount success! \r\n");
	#if 0
		f_res = f_open(&SDFile, "0:FatFs.txt", FA_CREATE_ALWAYS | FA_WRITE);
		if(f_res == FR_OK)
		{
			printf(" open file sucess!!! \r\n");
			#if 0
			printf("\r\n****** Write data to the text files ******\r\n");
			f_res = f_write(&SDFile, WriteBuffer, sizeof(WriteBuffer), &fnum);
			if(f_res == FR_OK)
			{
				printf(" write file sucess!!! (%d)\n", fnum);
				printf(" write Data : %s\r\n", WriteBuffer);
			}
			else
			{
				printf(" write file error : %d\r\n", f_res);
			}

			f_close(&SDFile);
			/*************************************************************************/
			f_res = f_open(&SDFile, "0:update.bin", FA_OPEN_EXISTING | FA_READ);
			//f_res = f_open(&SDFile, "0:FatFs.txt", FA_OPEN_EXISTING | FA_READ);
			if(f_res == FR_OK)
			{
				printf(" open file sucess!!! \r\n");
				memset(WriteBuffer, 0, sizeof(WriteBuffer));
				printf("\r\n****** read data from the text files ******\r\n");
				f_res = f_read(&SDFile, WriteBuffer, sizeof(WriteBuffer), &fnum);
				if(f_res == FR_OK)
				{
					printf(" read file sucess!!! (%d)\n", fnum);
					printf(" read Data : %s\r\n", WriteBuffer);
				}
				else
				{
					printf(" read file error : %d,%d\r\n", f_res, fnum);
				}

				f_close(&SDFile);
			}
			#endif
		}
		else
		{
			printf(" open file error : %d\r\n", f_res);
		}
		#endif
	}
	else
	{
		printf("sd card mount valit falil! \r\n");
	    return;
	}
}

/******************************
Function name:Jump2App
Function     :Jump from the Bootloader to the user APP address space
argument     :NULL
return       :NULL
*******************************/
void Jump2App(void)
{
typedef  void (*fun)(void);				  
    fun AppStart; 
	__IO uint32_t AppStartAddr = FLASH_APP1_ADDR;

	if(((*(vu32_t*)FLASH_APP1_ADDR)&0x2FFE0000) == 0x20000000)	//Check whether the top address of the stack is valid.
	{ 
		if(f_mount(NULL,"0:",0) == FR_OK)		
		{
			printf("f_mount \r\n");
		}
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
		
		printf("Now jump to APP...\r\n");
        /* The first address is MSP, and address +4 is the address of the reset interrupt service procedure */
		AppStart = (fun)(*(vu32_t*)(AppStartAddr+4));		
            
         /* Close global interrupt */
        __set_PRIMASK(1); 
        
		HAL_NVIC_DisableIRQ(DMA2_Stream3_IRQn);
		HAL_NVIC_DisableIRQ(DMA2_Stream6_IRQn);
		HAL_NVIC_DisableIRQ(SDIO_IRQn);
		HAL_NVIC_DisableIRQ(USART6_IRQn);
 
        /* Close the tick timer and reset it to the default value */
        SysTick->CTRL = 0;
        SysTick->LOAD = 0;
        SysTick->VAL = 0;
        
        /* Set all clocks to the default state */
        HAL_RCC_DeInit();
        
        /* Close all interrupts and clear all interrupt suspension flags */  
        for (int i = 0; i < 8; i++)
        {
            NVIC->ICER[i]=0xFFFFFFFF;
            NVIC->ICPR[i]=0xFFFFFFFF;
        }

        /* Description Global interrupt was enabled */ 
        //__set_PRIMASK(0);

        /* In RTOS engineering, this statement is very important, set to privileged mode, using the MSP pointer */
        __set_CONTROL(0);
        
        __set_MSP(*(vu32_t*)AppStartAddr);		    //Initializes the APP stack pointer (the first word in the user code area is used to store the stack top address)
		
		AppStart();								   //Jump to APP
        
        /* If the jump is successful, it will not be executed here, and the user can add code here */
        while (1)
        {
			printf("Skip APP faile\r\n");
        }
	}
}



int iap_main(void)
{
	if (BSP_SD_IsDetected() == SD_PRESENT)
	{
		printf("An SD card is detected and starts checking to see if an upgrade is required\r\n");
		SD_INIT();
		FirmwareUpdate();
	}
	else
	{
		printf("No SD card detected\r\n");
	}
	Jump2App();
	return 0;
}

