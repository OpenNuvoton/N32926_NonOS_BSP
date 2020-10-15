/**************************************************************************//**
 * @file     DrvAAC.h
 * @version  V3.00
 * @brief    N3292x series AAC driver header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#ifndef _DRVACC_H
#define _DRVACC_H
 

// SPU open
ERRCODE
DrvAAC_Open(void);
void DrvAAC_Close(void);

INT32
DrvAAC_Encoder(
	INT32 *pi32inbuf, 
	INT32 *pi32outbuf,
	INT32  i32Size	
);

INT32
DrvAAC_Decoder(
	INT32  i32Size,
	INT32 *pi32inbuf, 
	INT32 *pi32outbuf 
	
);

#endif
