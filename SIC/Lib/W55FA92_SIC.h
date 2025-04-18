/**************************************************************************//**
 * @file     W55FA92_SIC.h
 * @version  V3.00
 * @brief    N3292x series SIC driver header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#ifndef _W55FA92_SIC_H
#define _W55FA92_SIC_H

//#define OPT_SW_WP         // use GPA7 as software Write Protect pin; 1 is normal; 0 is protected

#define OPT_MARK_BAD_BLOCK_WHILE_ERASE_FAIL

#define FMI_SD_CARD     0
#define FMI_SM_CARD     1

#define FMI_ERR_ID      0xFFFF0100

#define FMI_TIMEOUT             (FMI_ERR_ID|0x01)
#define FMI_NO_MEMORY           (FMI_ERR_ID|0x02)
/* SD error */
#define FMI_NO_SD_CARD          (FMI_ERR_ID|0x10)
#define FMI_ERR_DEVICE          (FMI_ERR_ID|0x11)
#define FMI_SD_INIT_TIMEOUT     (FMI_ERR_ID|0x12)
#define FMI_SD_SELECT_ERROR     (FMI_ERR_ID|0x13)
#define FMI_SD_WRITE_PROTECT    (FMI_ERR_ID|0x14)
#define FMI_SD_INIT_ERROR       (FMI_ERR_ID|0x15)
#define FMI_SD_CRC7_ERROR       (FMI_ERR_ID|0x16)
#define FMI_SD_CRC16_ERROR      (FMI_ERR_ID|0x17)
#define FMI_SD_CRC_ERROR        (FMI_ERR_ID|0x18)
#define FMI_SD_CMD8_ERROR       (FMI_ERR_ID|0x19)
#define FMI_SD_DITO_ERROR       (FMI_ERR_ID|0x1A)
#define FMI_SD_RITO_ERROR       (FMI_ERR_ID|0x1B)

/* NAND error */
#define FMI_SM_INIT_ERROR       (FMI_ERR_ID|0x20)
#define FMI_SM_RB_ERR           (FMI_ERR_ID|0x21)
#define FMI_SM_STATE_ERROR      (FMI_ERR_ID|0x22)
#define FMI_SM_ECC_ERROR        (FMI_ERR_ID|0x23)
#define FMI_SM_STATUS_ERR       (FMI_ERR_ID|0x24)
#define FMI_SM_ID_ERR           (FMI_ERR_ID|0x25)
#define FMI_SM_INVALID_BLOCK    (FMI_ERR_ID|0x26)
#define FMI_SM_MARK_BAD_BLOCK_ERR   (FMI_ERR_ID|0x27)
#define FMI_SM_REGION_PROTECT_ERR   (FMI_ERR_ID|0x28)

/* MS error */
#define FMI_NO_MS_CARD          (FMI_ERR_ID|0x30)
#define FMI_MS_INIT_ERROR       (FMI_ERR_ID|0x31)
#define FMI_MS_INT_TIMEOUT      (FMI_ERR_ID|0x32)
#define FMI_MS_BUSY_TIMEOUT     (FMI_ERR_ID|0x33)
#define FMI_MS_CRC_ERROR        (FMI_ERR_ID|0x34)
#define FMI_MS_INT_CMDNK        (FMI_ERR_ID|0x35)
#define FMI_MS_INT_ERR          (FMI_ERR_ID|0x36)
#define FMI_MS_INT_BREQ         (FMI_ERR_ID|0x37)
#define FMI_MS_INT_CED_ERR      (FMI_ERR_ID|0x38)
#define FMI_MS_READ_PAGE_ERROR  (FMI_ERR_ID|0x39)
#define FMI_MS_COPY_PAGE_ERR    (FMI_ERR_ID|0x3a)
#define FMI_MS_ALLOC_ERR        (FMI_ERR_ID|0x3b)
#define FMI_MS_WRONG_SEGMENT    (FMI_ERR_ID|0x3c)
#define FMI_MS_WRONG_PHYBLOCK   (FMI_ERR_ID|0x3d)
#define FMI_MS_WRONG_TYPE       (FMI_ERR_ID|0x3e)
#define FMI_MS_WRITE_DISABLE    (FMI_ERR_ID|0x3f)

#define NAND_TYPE_SLC       0x01
#define NAND_TYPE_MLC       0x00

#define NAND_PAGE_512B      512
#define NAND_PAGE_2KB       2048
#define NAND_PAGE_4KB       4096
#define NAND_PAGE_8KB       8192

typedef struct fmi_sm_info_t
{
    UINT32  uSectorPerFlash;
    UINT32  uBlockPerFlash;
    UINT32  uPagePerBlock;
    UINT32  uSectorPerBlock;
    UINT32  uLibStartBlock;     // the number of block that system area used
    UINT32  nPageSize;
    UINT32  uBadBlockCount;
    UINT32  uRegionProtect;     // the page number for Region Protect End Address
    UINT32  uIBRBlock;          // the number of block that IBR will read with different BCH rule
    BOOL    bIsMulticycle;
    BOOL    bIsMLCNand;
    BOOL    bIsNandECC4;
    BOOL    bIsNandECC8;
    BOOL    bIsNandECC12;
    BOOL    bIsNandECC15;
    BOOL    bIsNandECC24;
    BOOL    bIsCheckECC;
    BOOL    bIsRA224;           // TRUE to use 224 bytes in Redundancy Area
} FMI_SM_INFO_T;

extern FMI_SM_INFO_T *pSM0, *pSM1;

typedef struct fmi_sd_info_t
{
    UINT32  uCardType;      // sd2.0, sd1.1, or mmc
    UINT32  uRCA;           // relative card address
    BOOL    bIsCardInsert;
} FMI_SD_INFO_T;

extern FMI_SD_INFO_T *pSD0;
extern FMI_SD_INFO_T *pSD1;
extern FMI_SD_INFO_T *pSD2;


// function prototype for FMI
VOID fmiInitDevice(void);
VOID fmiSetFMIReferenceClock(UINT32 uClock);
VOID fmiSetCallBack(UINT32 uCard, PVOID pvRemove, PVOID pvInsert);  // callback function
INT  fmiInitSDDevice(INT cardSel);  // for file system

// function prototype for FMI/SD
INT  fmiSD_CardSel(INT cardSel);
INT  fmiSD_Read(UINT32 uSector, UINT32 uBufcnt, UINT32 uDAddr);
INT  fmiSD_Write(UINT32 uSector, UINT32 uBufcnt, UINT32 uSAddr);

// function prototype for FMI/SM
VOID fmiSMClose(INT chipSel);

/* extern function */
#define SIC_SET_CLOCK       0
#define SIC_SET_CALLBACK    1
#define SIC_GET_CARD_STATUS 2
#define SIC_SET_CARD_DETECT 3
extern INT32 g_SD0_card_detect;

// MMC run 20MHz
#define MMC_FREQ    20000
#define EMMC_FREQ   26000

// SD run 24MHz
#define SD_FREQ     24000

// SDHC run 24MHz
#define SDHC_FREQ   24000


// function prototype for SIC
void sicOpen(void);
void sicClose(void);
VOID sicIoctl(INT32 sicFeature, INT32 sicArg0, INT32 sicArg1, INT32 sicArg2);

// function prototype for SIC/SD
INT sicSdOpen(void);
INT sicSdOpen0(void);
INT sicSdOpen1(void);
INT sicSdOpen2(void);

VOID sicSdClose(void);
VOID sicSdClose0(void);
VOID sicSdClose1(void);
VOID sicSdClose2(void);

INT sicSdRead(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr);
INT sicSdRead0(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr);
INT sicSdRead1(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr);
INT sicSdRead2(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr);

INT sicSdRead_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sicSdRead0_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sicSdRead1_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sicSdRead2_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);

INT sicSdWrite(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr);
INT sicSdWrite0(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr);
INT sicSdWrite1(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr);
INT sicSdWrite2(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr);

INT sicSdWrite_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sicSdWrite0_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sicSdWrite1_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sicSdWrite2_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);


// function prototype for SIC/SM
INT sicSMRegionProtect(INT chipSel, INT PBA, INT page);


/* gnand use */
#include "W55FA92_GNAND.h"

// function prototype for SM that end user can called
INT nand_ioctl(INT param1, INT param2, INT param3, INT param4);

INT nandInit0(NDISK_T *NDISK_info);
INT nandpread0(INT PBA, INT page, UINT8 *buff);
INT nandpwrite0(INT PBA, INT page, UINT8 *buff);
INT nand_is_page_dirty0(INT PBA, INT page);
INT nand_is_valid_block0(INT PBA);
INT nand_block_erase0(INT PBA);
INT nand_chip_erase0(void);
INT nandRegionProtect0(INT PBA, INT page);

INT nandInit1(NDISK_T *NDISK_info);
INT nandpread1(INT PBA, INT page, UINT8 *buff);
INT nandpwrite1(INT PBA, INT page, UINT8 *buff);
INT nand_is_page_dirty1(INT PBA, INT page);
INT nand_is_valid_block1(INT PBA);
INT nand_block_erase1(INT PBA);
INT nand_chip_erase1(void);
INT nandRegionProtect1(INT PBA, INT page);

INT nand_is_card_inserted(void);


/* Declare callback function in waiting loop of SD driver */
#if defined (__GNUC__)
    __attribute__((weak)) void schedule(void);
#else
    __weak void schedule(void);
#endif

#endif  //end of _W55FA92_SIC_H
