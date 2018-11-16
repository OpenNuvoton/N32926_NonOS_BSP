/***************************************************************
 *                                                             *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved. *
 *                                                             *
 ***************************************************************/
 
#ifndef __DRVRF_H__
#define __DRVRF_H__

#include "wblib.h"

#ifdef  __cplusplus
extern "C"
{
#endif

/*-----------------------------------------------------------------------------
 * Define Macro
 *---------------------------------------------------------------------------*/
// define FPGA board debug or not
//#define OPT_FPGA_DEBUG

// it is used to enlarge block size for emulation patterns, and should be commented for release version
//#define EMULATION_ONLY

// define the debug mode to show more information
//#define DEBUG
#ifdef DEBUG
	#define DBG_PRINTF	sysprintf
#else
	#define DBG_PRINTF(...)
#endif
//#define sysprintf	printf

#define	E_SUCCESS	0

//--- Define Error Code for RF
#define RFC_ERR_FAIL		(RFC_ERR_ID|0x01)
#define RFC_ERR_PNCTR_MODE	(RFC_ERR_ID|0x02)
#define RFC_ERR_DATA_BUF	(RFC_ERR_ID|0x03)

// the maximum size in RF operation
#define RF_BLOCK_SIZE		24
#ifdef EMULATION_ONLY
	#define RF_MAX_SIZE		65535
#else
	//#define RF_MAX_SIZE		5040	// Must be divisible by [2/3/5/7] * 24
	#define RF_MAX_SIZE		4096
#endif

// define RF opearation mode
#define RF_ENCRYPT	0
#define RF_DECRYPT	1


/***************************************************************
	Enumerate Type
 ***************************************************************/
typedef enum {
	E_PNCTR_1_2 = 0,
	E_PNCTR_2_3,
	E_PNCTR_3_4,
	E_PNCTR_5_6,
	E_PNCTR_7_8
} E_RF_PNCTR_MODE;


/***************************************************************
	APIs declaration
 ***************************************************************/
INT32 RF_Open(void);

void RF_Close(void);

void RF_Enable_Int(void);

void RF_Disable_Int(void);

INT32 RF_Set_Puncture(E_RF_PNCTR_MODE ePnctrMod);

E_RF_PNCTR_MODE RF_Get_Puncture(void);

INT32 RF_Encrypt(UINT8* plainBuf, UINT8* cipherBuf, INT32 plainDataLen);

INT32 RF_Decrypt(UINT8* cipherBuf, UINT8* plainBuf, UINT32 plainDataLen);


#ifdef  __cplusplus
}
#endif

#endif	// __DRVRF_H__
