/**************************************************************************//**
 * @file     sdio_fmi.h
 * @version  V3.00
 * @brief    N3292x series SIC/SDIO driver header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#ifndef _SDIO_FMI_H
#define _SDIO_FMI_H

#include <stdio.h>
#include "wbio.h"

// define DATE CODE and show it when running to make maintaining easy.
#define SDIO_DATE_CODE      "20171124"

/*-----------------------------------------------------------------------------
 * To define some flag for emulation or FPGA
 *---------------------------------------------------------------------------*/
// Define OPT_FPGA_DEBUG to run special code only for FPGA board
//#define OPT_FPGA_DEBUG

// Define _SDIO_USE_INT_ to run code that use interrupt function
//#define _SDIO_USE_INT_

// Define DEBUG to show more information for debugging
//#define DEBUG

#define TIMER0  0

//#define _USE_DAT3_DETECT_

//-- function return value
#define    Successful  0
#define    Fail        1

//--- define type of SD card or MMC
#define FMISDIO_TYPE_UNKNOWN            0
#define FMISDIO_TYPE_SD_HIGH            1
#define FMISDIO_TYPE_SD_LOW             2
#define FMISDIO_TYPE_MMC                3   // MMC access mode: Byte mode for capacity <= 2GB
#define FMISDIO_TYPE_MMC_SECTOR_MODE    4   // MMC access mode: Sector mode for capacity > 2GB

#ifdef ECOS
#define sysGetTicks(TIMER0)  cyg_current_time()
#endif

// extern global variables
extern UINT32 _fmiSDIO_uFMIReferenceClock;
extern BOOL volatile _fmiSDIO_bIsSDDataReady;
extern BOOL volatile _fmiSDIO_bIsSMPRegionDetect;

#define STOR_STRING_LEN 32

/* we allocate one of these for every device that we remember */
typedef struct sdio_disk_data_t
{
    struct sdio_disk_data_t  *next; /* next device */

    /* information about the device -- always good */
    unsigned int  totalSectorN;
    unsigned int  diskSize;         /* disk size in Kbytes */
    int           sectorSize;
    char          vendor[STOR_STRING_LEN];
    char          product[STOR_STRING_LEN];
    char          serial[STOR_STRING_LEN];
} SDIO_DISK_DATA_T;


// function declaration

// SDIO functions
INT  fmiSDIOCommand(FMI_SDIO_INFO_T *pSDIO, UINT8 ucCmd, UINT32 uArg);
INT  fmiSDIOCmdAndRsp(FMI_SDIO_INFO_T *pSDIO, UINT8 ucCmd, UINT32 uArg, INT nCount);
INT  fmiSDIOCmdAndRsp2(FMI_SDIO_INFO_T *pSDIO, UINT8 ucCmd, UINT32 uArg, UINT *puR2ptr);
INT  fmiSDIOCmdAndRspDataIn(FMI_SDIO_INFO_T *pSDIO, UINT8 ucCmd, UINT32 uArg);
INT  fmiSDIO_Init(FMI_SDIO_INFO_T *pSDIO);
INT  fmiSDIOSelectCard(FMI_SDIO_INFO_T *pSDIO);
VOID fmiGet_SDIO_info(FMI_SDIO_INFO_T *pSDIO, SDIO_DISK_DATA_T *_info);
INT  fmiSDIO_Read_in(FMI_SDIO_INFO_T *pSDIO, UINT32 uSector, UINT32 uBufcnt, UINT32 uDAddr);
INT  fmiSDIO_Write_in(FMI_SDIO_INFO_T *pSDIO, UINT32 uSector, UINT32 uBufcnt, UINT32 uSAddr);
VOID fmiSDIOCheckRB(void);

/*-----------------------------------------------------------------------------
 * 2011/6/29, declaration more functions
 *---------------------------------------------------------------------------*/
VOID fmiSDIO_Set_clock(UINT32 sd_clock_khz);
VOID fmiSDIO_Show_info(int sdport);
INT  fmiSDIO_Read_in_blksize(FMI_SDIO_INFO_T *pSDIO, UINT32 uSector, UINT32 uBufcnt, UINT32 uDAddr, UINT32 blksize);
VOID fmiSDIO_Change_Driver_Strength(int card_no, int card_type);

#endif  // end of _SDIO_FMI_H
