/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _FLASH_MAP_H_
#define _FLASH_MAP_H_

#define IMG_HDR_SIZE                    0x00000400  //Changable, default 4K, modify in eFuse if needed

#define CFG_FILE_PAYLOAD_LEN            0x00001000  //Fixed

#define FLASH_SIZE                      0x01000000  //16384K Bytes


/* ========== High Level Flash Layout Configuration ========== */
#define EQ_FITTING_ADDR                 0x02000000
#define EQ_FITTING_SIZE                 0x00000400  //1K Bytes
#define RSV_ADDR                        0x02000400
#define RSV_SIZE                        0x00001C00  //7K Bytes
#define OEM_CFG_ADDR                    0x02002000
#define OEM_CFG_SIZE                    0x00001400  //5K Bytes
#define BOOT_PATCH_ADDR                 0x02004000
#define BOOT_PATCH_SIZE                 0x00002000  //8K Bytes
#define PLATFORM_EXT_ADDR               0x02006000
#define PLATFORM_EXT_SIZE               0x00008000  //32K Bytes
#define LOWERSTACK_EXT_ADDR             0x0200E000
#define LOWERSTACK_EXT_SIZE             0x00007000  //28K Bytes
#define UPPERSTACK_ADDR                 0x02015000
#define UPPERSTACK_SIZE                 0x0003F000  //252K Bytes
#define OTA_BANK0_ADDR                  0x02054000
#define OTA_BANK0_SIZE                  0x00266000  //2456K Bytes
#define OTA_BANK1_ADDR                  0x022BA000
#define OTA_BANK1_SIZE                  0x00266000  //2456K Bytes
#define VP_DATA_ADDR                    0x02520000
#define VP_DATA_SIZE                    0x00032000  //200K Bytes
#define FTL_ADDR                        0x02FEB000
#define FTL_SIZE                        0x00015000  //84K Bytes
#define BKP_DATA1_ADDR                  0x00000000
#define BKP_DATA1_SIZE                  0x00000000  //0K Bytes
#define BKP_DATA2_ADDR                  0x0255D000
#define BKP_DATA2_SIZE                  0x00002000  //8K Bytes
#define OTA_TMP_ADDR                    0x00000000
#define OTA_TMP_SIZE                    0x00000000  //0K Bytes
#define APP_DEFINED_SECTION_ADDR        0x0255F000
#define APP_DEFINED_SECTION_SIZE        0x0000F000  //60K Bytes
#define USER_DATA1_ADDR                 0x0256E000
#define USER_DATA1_SIZE                 0x009A1000  //9860K Bytes
#define USER_DATA1_WITH_HEADER          1
#define USER_DATA2_ADDR                 0x00000000
#define USER_DATA2_SIZE                 0x00000000  //0K Bytes
#define USER_DATA2_WITH_HEADER          0
#define USER_DATA3_ADDR                 0x00000000
#define USER_DATA3_SIZE                 0x00000000  //0K Bytes
#define USER_DATA3_WITH_HEADER          0
#define USER_DATA4_ADDR                 0x00000000
#define USER_DATA4_SIZE                 0x00000000  //0K Bytes
#define USER_DATA4_WITH_HEADER          0
#define USER_DATA5_ADDR                 0x00000000
#define USER_DATA5_SIZE                 0x00000000  //0K Bytes
#define USER_DATA5_WITH_HEADER          0
#define USER_DATA6_ADDR                 0x00000000
#define USER_DATA6_SIZE                 0x00000000  //0K Bytes
#define USER_DATA6_WITH_HEADER          0
#define USER_DATA7_ADDR                 0x00000000
#define USER_DATA7_SIZE                 0x00000000  //0K Bytes
#define USER_DATA7_WITH_HEADER          0
#define USER_DATA8_ADDR                 0x00000000
#define USER_DATA8_SIZE                 0x00000000  //0K Bytes
#define USER_DATA8_WITH_HEADER          0

/* ========== OTA Bank0 Flash Layout Configuration ========== */
#define BANK0_OTA_HDR_ADDR              0x02054000
#define BANK0_OTA_HDR_SIZE              0x00000400  //1K Bytes
#define BANK0_FSBL_ADDR                 0x02054400
#define BANK0_FSBL_SIZE                 0x00001C00  //7K Bytes
#define BANK0_STACK_PATCH_ADDR          0x02056000
#define BANK0_STACK_PATCH_SIZE          0x0003D000  //244K Bytes
#define BANK0_SYS_PATCH_ADDR            0x02093000
#define BANK0_SYS_PATCH_SIZE            0x0001D000  //116K Bytes
#define BANK0_APP_ADDR                  0x020B0000
#define BANK0_APP_SIZE                  0x00187000  //1564K Bytes
#define BANK0_DSP_SYS_ADDR              0x02237000
#define BANK0_DSP_SYS_SIZE              0x00020000  //128K Bytes
#define BANK0_DSP_APP_ADDR              0x02257000
#define BANK0_DSP_APP_SIZE              0x00038000  //224K Bytes
#define BANK0_DSP_CFG_ADDR              0x0228F000
#define BANK0_DSP_CFG_SIZE              0x0000A000  //40K Bytes
#define BANK0_APP_CFG_ADDR              0x02299000
#define BANK0_APP_CFG_SIZE              0x00002000  //8K Bytes
#define BANK0_EXT_IMG0_ADDR             0x0229B000
#define BANK0_EXT_IMG0_SIZE             0x0000A000  //40K Bytes
#define BANK0_EXT_IMG1_ADDR             0x022A5000
#define BANK0_EXT_IMG1_SIZE             0x00004000  //16K Bytes
#define BANK0_EXT_IMG2_ADDR             0x00000000
#define BANK0_EXT_IMG2_SIZE             0x00000000  //0K Bytes
#define BANK0_EXT_IMG3_ADDR             0x00000000
#define BANK0_EXT_IMG3_SIZE             0x00000000  //0K Bytes

/* ========== OTA Bank1 Flash Layout Configuration ========== */
#define BANK1_OTA_HDR_ADDR              0x022BA000
#define BANK1_OTA_HDR_SIZE              0x00000400  //1K Bytes
#define BANK1_FSBL_ADDR                 0x022BA400
#define BANK1_FSBL_SIZE                 0x00001C00  //7K Bytes
#define BANK1_STACK_PATCH_ADDR          0x022BC000
#define BANK1_STACK_PATCH_SIZE          0x0003D000  //244K Bytes
#define BANK1_SYS_PATCH_ADDR            0x022F9000
#define BANK1_SYS_PATCH_SIZE            0x0001D000  //116K Bytes
#define BANK1_APP_ADDR                  0x02316000
#define BANK1_APP_SIZE                  0x00187000  //1564K Bytes
#define BANK1_DSP_SYS_ADDR              0x0249D000
#define BANK1_DSP_SYS_SIZE              0x00020000  //128K Bytes
#define BANK1_DSP_APP_ADDR              0x024BD000
#define BANK1_DSP_APP_SIZE              0x00038000  //224K Bytes
#define BANK1_DSP_CFG_ADDR              0x024F5000
#define BANK1_DSP_CFG_SIZE              0x0000A000  //40K Bytes
#define BANK1_APP_CFG_ADDR              0x024FF000
#define BANK1_APP_CFG_SIZE              0x00002000  //8K Bytes
#define BANK1_EXT_IMG0_ADDR             0x02501000
#define BANK1_EXT_IMG0_SIZE             0x0000A000  //40K Bytes
#define BANK1_EXT_IMG1_ADDR             0x0250B000
#define BANK1_EXT_IMG1_SIZE             0x00004000  //16K Bytes
#define BANK1_EXT_IMG2_ADDR             0x00000000
#define BANK1_EXT_IMG2_SIZE             0x00000000  //0K Bytes
#define BANK1_EXT_IMG3_ADDR             0x00000000
#define BANK1_EXT_IMG3_SIZE             0x00000000  //0K Bytes


#endif /* _FLASH_MAP_H_ */
