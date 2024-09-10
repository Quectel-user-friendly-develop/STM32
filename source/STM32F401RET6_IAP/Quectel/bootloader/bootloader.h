#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

typedef unsigned char                   u8_t;
typedef signed   char                   s8_t;
typedef unsigned short                  u16_t;
typedef signed   short                  s16_t;
typedef unsigned int                    u32_t;
typedef unsigned long long              u64_t;
typedef signed   int                    s32_t;
typedef __IO u64_t                      vu64_t;
typedef __IO u32_t                      vu32_t;
typedef __IO u16_t                      vu16_t;
typedef __IO u8_t                       vu8_t;

#define ALIGN_UP(x, a) ( ( ((x) + ((a) - 1) ) / a ) * a )
#define ALIGN_DOWN(x, a) ( ( (x) / (a)) * (a) )

//Start address of the FLASH sector
//Main memory
#define ADDR_FLASH_SECTOR_0     ((u32_t)0x08000000) 	//Sector 0 start address, 16 Kbytes 
#define ADDR_FLASH_SECTOR_1     ((u32_t)0x08004000) 	//Sector 1 start address, 16 Kbytes 
#define ADDR_FLASH_SECTOR_2     ((u32_t)0x08008000) 	//Sector 2 start address, 16 Kbytes 
#define ADDR_FLASH_SECTOR_3     ((u32_t)0x0800C000) 	//Sector 3 start address, 16 Kbytes 
#define ADDR_FLASH_SECTOR_4     ((u32_t)0x08010000) 	//Sector 4 start address, 64 Kbytes
#define ADDR_FLASH_SECTOR_5     ((u32_t)0x08020000) 	//Sector 5 start address, 128 Kbytes 
#define ADDR_FLASH_SECTOR_6     ((u32_t)0x08040000) 	//Sector 6 start address, 128 Kbytes  
#define ADDR_FLASH_SECTOR_7     ((u32_t)0x08060000) 	//Sector 7 start address, 128 Kbytes  
//System memory
#define ADDR_FLASH_SYSTEM       ((u32_t)0x1FFFF0000) 	//System   start address, 30  Kbytes  
//OTP area
#define ADDR_FLASH_otp          ((u32_t)0x1FFFF7800) 	//System   start address, 528 bytes  
//Option bytes
#define ADDR_FLASH_otp          ((u32_t)0x1FFFFC000) 	//System   start address, 16  bytes  

#define STM32_FLASH_SIZE                (64*1024) 	    //FLASH capacity of the selected STM32 (in bytes)
#define STM_SECTOR_SIZE		            (16*1024)	    //Note: The FLASH sector size of STM32F401ret6 is 16K
#define STM32_FLASH_BASE        ADDR_FLASH_SECTOR_0     //STM32 FLASH start address
#define FLASH_APP1_ADDR		    ADDR_FLASH_SECTOR_4  	//Application start address 

#endif
