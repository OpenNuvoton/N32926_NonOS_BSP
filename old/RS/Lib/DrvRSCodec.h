/***************************************************************
 *                                                             *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved. *
 *                                                             *
 ***************************************************************/
 
#ifndef __DRVRS_H__
#define __DRVRS_H__

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

// define the debug mode to show more information
//#define DEBUG
#ifdef DEBUG
	#define DBG_PRINTF	sysprintf
#else
	#define DBG_PRINTF(...)
#endif
//#define sysprintf	printf

#define	E_SUCCESS	0

//--- Define Error Code for RS
#define RSC_ERR_FAIL		(RSC_ERR_ID|0x01)
#define RSC_ERR_DATA_LEN	(RSC_ERR_ID|0x02)
#define RSC_ERR_DATA_BUF	(RSC_ERR_ID|0x03)
#define RSC_ERR_DEC_ERROR	(RSC_ERR_ID|0x04)

// the maximum size in RS operation
#define RS_ENCRYPT_BLOCK_SIZE		188
#define RS_DECRYPT_BLOCK_SIZE		204
#define RS_ENCRYPT_MAX_SIZE		RS_ENCRYPT_BLOCK_SIZE * 32
#define RS_DECRYPT_MAX_SIZE		RS_DECRYPT_BLOCK_SIZE * 32

// define RS opearation mode
#define RS_ENCRYPT	0
#define RS_DECRYPT	1


/***************************************************************
	APIs declaration
 ***************************************************************/
INT32 RS_Open(void);

void RS_Close(void);

void RS_Enable_Int(void);

void RS_Disable_Int(void);

INT32 RS_Encrypt(UINT8* plainBuf, UINT8* cipherBuf, INT32 dataLen, UINT8 isInterleave);

INT32 RS_Decrypt(UINT8* cipherBuf, UINT8* plainBuf, UINT32 dataLen, UINT8 isInterleave);


#ifdef  __cplusplus
}
#endif

#endif	// __DRVRS_H__
