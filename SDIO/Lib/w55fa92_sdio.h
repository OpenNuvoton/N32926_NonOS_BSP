/***************************************************************************
 * Copyright (c) 2013 Nuvoton Technology. All rights reserved.
 *
 * FILENAME
 *     w55fa92_sdio.h
 * DESCRIPTION
 *     The header file of SIC/SDIO library.
 * FUNCTIONS
 *     None
 **************************************************************************/
#ifndef _W55FA92_SDIO_H
#define _W55FA92_SDIO_H

//#define OPT_SW_WP         // use GPA7 as software Write Protect pin; 1 is normal; 0 is protected

#define FMI_SDIO_CARD   0
#define FMI_SM_CARD     1
#define FMI_SDIO1_CARD  2

#define FMISDIO_ERR_ID              0xFFF00100

#define FMISDIO_TIMEOUT             (FMISDIO_ERR_ID|0x01)
#define FMISDIO_NO_MEMORY           (FMISDIO_ERR_ID|0x02)
/* SD error */
#define FMISDIO_NO_SD_CARD          (FMISDIO_ERR_ID|0x10)
#define FMISDIO_ERR_DEVICE          (FMISDIO_ERR_ID|0x11)
#define FMISDIO_SD_INIT_TIMEOUT     (FMISDIO_ERR_ID|0x12)
#define FMISDIO_SD_SELECT_ERROR     (FMISDIO_ERR_ID|0x13)
#define FMISDIO_SD_WRITE_PROTECT    (FMISDIO_ERR_ID|0x14)
#define FMISDIO_SD_INIT_ERROR       (FMISDIO_ERR_ID|0x15)
#define FMISDIO_SD_CRC7_ERROR       (FMISDIO_ERR_ID|0x16)
#define FMISDIO_SD_CRC16_ERROR      (FMISDIO_ERR_ID|0x17)
#define FMISDIO_SD_CRC_ERROR        (FMISDIO_ERR_ID|0x18)
#define FMISDIO_SD_CMD8_ERROR       (FMISDIO_ERR_ID|0x19)

typedef struct fmi_sdio_info_t
{
    UINT32  uCardType;      // sd2.0, sd1.1, or mmc
    UINT32  uRCA;           // relative card address
    BOOL    bIsCardInsert;
} FMI_SDIO_INFO_T;

extern FMI_SDIO_INFO_T *pSDIO0;
extern FMI_SDIO_INFO_T *pSDIO1;


// function prototype for FMI
VOID fmiSDIOInitDevice(void);
VOID fmiSDIOSetFMIReferenceClock(UINT32 uClock);
VOID fmiSDIOSetCallBack(UINT32 uCard, PVOID pvRemove, PVOID pvInsert);  // callback function
INT  fmiSDIOInitSDDevice(INT cardSel);  // for file system

// function prototype for FMI/SD
INT  fmiSDIO_CardSel(INT cardSel);
INT  fmiSDIO_Read(UINT32 uSector, UINT32 uBufcnt, UINT32 uDAddr);
INT  fmiSDIO_Write(UINT32 uSector, UINT32 uBufcnt, UINT32 uSAddr);
INT  fmiSDIO_CardStatus(FMI_SDIO_INFO_T *pSDIO);

/* extern function */
#define SDIO_SET_CLOCK          0
#define SDIO_SET_CALLBACK       1
#define SDIO_GET_CARD_STATUS    2
#define SDIO0_GET_CARD_STATUS   3
#define SDIO1_GET_CARD_STATUS   4
#define SDIO_SET_CARD_DETECT    5
#define SDIO0_SET_CARD_DETECT   6
#define SDIO1_SET_CARD_DETECT   7
extern INT32 g_SDIO0_card_detect;
extern INT32 g_SDIO1_card_detect;


// MMC run 20MHz under FA92
#define SDIO_MMC_FREQ   20000
#define SDIO_EMMC_FREQ  26000

// SD run 24MHz under FA92
#define SDIO_SD_FREQ    24000

// SDHC run 24MHz under FA92
#define SDIO_SDHC_FREQ  24000


// function prototype for SIC
void sdioOpen(void);
void sdioClose(void);
VOID sdioIoctl(INT32 sdioFeature, INT32 sdioArg0, INT32 sdioArg1, INT32 sdioArg2);

// function prototype for SIC/SDIO
INT sdioSdOpen(void);
INT sdioSdOpen0(void);
INT sdioSdOpen1(void);

VOID sdioSdClose(void);
VOID sdioSdClose0(void);
VOID sdioSdClose1(void);

INT sdioSdRead(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr);
INT sdioSdRead0(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr);
INT sdioSdRead1(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr);

INT sdioSdRead_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sdioSdRead0_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sdioSdRead1_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);

INT sdioSdWrite(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr);
INT sdioSdWrite0(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr);
INT sdioSdWrite1(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr);

INT sdioSdWrite_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sdioSdWrite0_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sdioSdWrite1_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);


/* Declare callback function in waiting loop of SD driver */
__weak void schedule(void);

#endif  //end of _W55FA92_SDIO_H
