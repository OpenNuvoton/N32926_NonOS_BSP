/***************************************************************
 *                                                             *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved. *
 *                                                             *
 ***************************************************************/
 
#ifndef __DRVCRC_H__
#define __DRVCRC_H__

#include "wblib.h"

#ifdef  __cplusplus
extern "C"
{
#endif

/*-----------------------------------------------------------------------------
 * Define Macro
 *---------------------------------------------------------------------------*/
// define the debug mode to show more information
//#define DEBUG
#ifdef DEBUG
	#define DBG_PRINTF	sysprintf
#else
	#define DBG_PRINTF(...)
#endif
//#define sysprintf	printf

#define	E_SUCCESS	0

//--- Define Error Code for CRC
#define CRC_ERR_INVAL		(CRC_ERR_ID|0x01)
#define CRC_ERR_NODEV		(CRC_ERR_ID|0x02)
#define CRC_ERR_STATUS		(CRC_ERR_ID|0x03)
#define CRC_ERR_BUSY		(CRC_ERR_ID|0x04)

// the maximum number of CRC channel
#define CRC_CH_NUM		2
// the maximum size in CRC VDMA operation
#define CRC_VDMA_MAX_SIZE	2048

/***************************************************************
	Enumerate and Structure Type
 ***************************************************************/
typedef enum {
	E_CHANNEL_0 = 0,
	E_CHANNEL_1
} E_CRC_CHANNEL_INDEX;

typedef enum {
	E_CH_DISABLE = 0,
	E_CH_ENABLE
} E_CRC_OPERATION;

typedef struct {
	volatile BOOL bInRequest;
	volatile BOOL bInUse;
} S_CRC_CHANNEL_INFO;

typedef enum {
	E_CRCCCITT = 0,
	E_CRC8,
	E_CRC16,
	E_CRC32
} E_CRC_MODE;

typedef enum {
	E_LENGTH_BYTE = 0,
	E_LENGTH_HALF_WORD,
	E_LENGTH_WORD
} E_WRITE_LENGTH;

typedef enum {
	E_1sCOM_OFF = 0,
	E_1sCOM_ON
} E_DATA_1sCOM;

typedef enum {
	E_REVERSE_OFF = 0,
	E_REVERSE_ON
} E_DATA_REVERSE;

typedef enum {
	E_CRC_CPU_PIO = 0,
	E_CRC_VDMA
} E_TRANSFER_MODE;

typedef struct {
	E_CRC_MODE ePolyMode;
	E_WRITE_LENGTH eWriteLength;
	E_DATA_1sCOM eChecksumCom;
	E_DATA_1sCOM eWdataCom;
	E_DATA_REVERSE eChecksumRvs;
	E_DATA_REVERSE eWdataRvs;
	E_TRANSFER_MODE eTransferMode;
	UINT32 uSeed;
} S_CRC_DESCRIPT_SETTING;


/***************************************************************
	APIs declaration
 ***************************************************************/
INT32 CRC_Init(void);
void CRC_Exit(void);
INT32 CRC_Request(INT32 channel);
void CRC_Free(INT32 channel);
INT32 CRC_FindandRequest(void);
UINT32 CRC_Run(INT32 channel, UINT8 *pDataBuf, UINT32 uDataLen, S_CRC_DESCRIPT_SETTING *psCRCDescript);


#ifdef  __cplusplus
}
#endif

#endif	// __DRVCRC_H__
