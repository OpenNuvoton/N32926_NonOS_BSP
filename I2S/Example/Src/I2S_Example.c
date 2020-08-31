/**************************************************************************//**
 * @file     I2S_Example.c
 * @version  V3.00
 * @brief    N329xx series I2S demo code
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdio.h>
#include "wbio.h"
#include "wblib.h"
#include "wbtypes.h"

#include "W55FA92_reg.h"
#include "W55FA92_I2S.h"

//#define OPT_FGPA_DEBUG

//#define 	BUFSIZE		0x40
#define 	BUFSIZE		0x100000
//#define 	BUFSIZE		0x1000
//#define 	BUFSIZE		0x800000


volatile S_DRVI2S_PLAY g_sPlay;
volatile S_DRVI2S_RECORD g_sRrecord;

//#include "SONG.h"
#if defined (__GNUC__) && !(__CC_ARM)
__attribute__ ((aligned (32))) char g_baAudioBuf[BUFSIZE];
#else
__align (32) char g_baAudioBuf[BUFSIZE];
#endif


//====================================================================================
//====================================================================================
int main(void)
{
//	UINT32 nAudioState = 0;
	UINT32 /*nBufSize,*/ uWriteAddr, uReadAddr;
//	BOOL bRet;
	unsigned int ii=0;

	uWriteAddr = (UINT32) g_baAudioBuf;
	uReadAddr = (UINT32) g_baAudioBuf;

	DrvI2S_Open();

	// set record	
	g_sRrecord.u32BufferAddr = uWriteAddr;	
	g_sRrecord.u32BufferLength = BUFSIZE;		
	g_sRrecord.eSampleRate = eDRVI2S_FREQ_44100;
	g_sRrecord.eChannel = eDRVI2S_RECORD_STEREO;						
	g_sRrecord.eFormat = eDRVI2S_I2S;	

	DrvI2S_StartRecord((S_DRVI2S_RECORD*) &g_sRrecord);	
	sysprintf(" I2S start Playing stereo in 44.1 kHz sampling rate \n\n");

	// set playback
	g_sPlay.u32BufferAddr = uReadAddr;	
	g_sPlay.u32BufferLength = BUFSIZE;		
	g_sPlay.eSampleRate = eDRVI2S_FREQ_44100;
	g_sPlay.eChannel = eDRVI2S_PLAY_STEREO;						
	g_sPlay.eFormat = eDRVI2S_I2S;			
	DrvI2S_StartPlay((S_DRVI2S_PLAY*) &g_sPlay);		
	sysprintf(" I2S start Recording stereo in 44.1 kHz sampling rate \n\n");

	
	while(1)
//	while(ii++<1000000)
	{
	    while (inp32(REG_I2S_ACTL_RSR) & R_DMA_RIA_IRQ)
	    {
			outp32(REG_I2S_ACTL_RSR, inp32(REG_I2S_ACTL_RSR));
			sysprintf(" I2S current recording address = %6x \n", inp32(REG_I2S_ACTL_RDSTC));	    
			sysprintf(" value of R_DMA_RIA_SN = %1x \n", inp32(REG_I2S_ACTL_RSR) & 0xE0);	    			
	    }

	    while (inp32(REG_I2S_ACTL_PSR) & P_DMA_RIA_IRQ)
	    {
			outp32(REG_I2S_ACTL_PSR, inp32(REG_I2S_ACTL_PSR));	    
			sysprintf(" I2S current playing address = %6x \n", inp32(REG_I2S_ACTL_PDSTC));	    
			sysprintf(" value of P_DMA_RIA_SN = %1x \n", inp32(REG_I2S_ACTL_PSR) & 0xE0);	    			
	    }
	}	    
	
	return 0;
}

































