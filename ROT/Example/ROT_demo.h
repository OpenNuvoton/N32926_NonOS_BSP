/**************************************************************************//**
 * @file     ROT_demo.h
 * @brief    N3292x series ROT sample header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#ifndef _ROT_DEMO_H_
#define _ROT_DEMO_H_


#include "W55FA92_ROT.h"

INT32 Emu_DestinationLineOffsetFineTune(UINT8* puDstAddr00, UINT8* puDstAddr1);
#undef DBG_PRINTF
#define DBG_PRINTF		sysprintf
#define __ENABLE_CACHE__

//#define ADDR_ROT_SRC_ADDR			0x1000004	
#define ADDR_ROT_SRC_ADDR			0x500000

#define ADDR_ROT_DST_ADDR			0x2000004
#ifdef __ENABLE_CACHE__
#define E_NONCACHE_BIT			0x80000000
#else
#define E_NONCACHE_BIT			0x00000000
#endif

#define ADDR_ROT_GOLDEN_ADDR		0x1000008

#ifdef __LCM_320x240__
#define OPT_LCM_WIDTH	320
#define OPT_LCM_HEIGHT	240
#endif
#ifdef __LCM_800x480__
#define OPT_LCM_WIDTH	800
#define OPT_LCM_HEIGHT	480
#endif

void rotDoneHandler(void);
void rotClearDoneFlag(void);
BOOL rotGetDoneFlag(void);
void rotAbortHandler(void);
void rotClearAbortFlag(void);
BOOL rotGetAbortFlag(void);
void rotOverHandler(void);
void rotClearOverFlag(void);
BOOL rotGetOverFlag(void) ;
#if 0
INT32 rotGetPacketPixelWidth(E_ROTENG_FMT ePacFormat);
#endif
void InitVPOST(UINT8* pu8FrameBuffer);
INT32 FileSize(char* szAsciiName);
INT32 ReadFile(char* szAsciiName,  PUINT16 pu16BufAddr, INT32 i32Length);

#define 	PACKET_RGB565 		E_ROT_PACKET_RGB565 
#define 	PACKET_RGB888		E_ROT_PACKET_RGB888
#define 	PACKET_YUV422		E_ROT_PACKET_YUV422	

#define 	PACKET_RGB555		(E_ROT_PACKET_YUV422+1)

extern volatile UINT32 bIsBuffer0Dirty;	//0 Means ready for ROT use
extern volatile UINT32 bIsBuffer1Dirty;	//1 means for TV show
extern volatile UINT32 VpostUseBuf;
extern UINT8 u8FrameBuffer0[];
extern UINT8 u8FrameBuffer1[];
#endif /*_ROT_DEMO_H_*/
