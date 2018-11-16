/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/

/****************************************************************************
* FILENAME
*   main.c
*
* VERSION
*   1.0
*
* DESCRIPTION
*   SPU sample application using SPU library. 
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*
* HISTORY
*
* REMARK
*   None
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wblib.h"
#include "w55fa92_spu.h"

__align (32) UINT8 g_AudioPattern[] = {
		#include "pcm16_raw.dat"
};
#define SMIC_CTRL_REG	0xB1000050

void delay(UINT32 kk)
{
	UINT32 ii, jj;
	
	for(ii=0; ii < kk; ii++)
	{
		for(jj=0; jj < 0x1000; jj++);	
	}
}

#if 0
static void delay_2(UINT32 kk)
{
	UINT32 ii, jj;
	
	for(ii=0; ii < kk; ii++)
	{
		for(jj=0; jj < 0x10; jj++);	
	}
}
#endif

extern VOID spuDacOn(UINT8 level);
extern VOID spuSetDacSlaveMode(void);
int main(void)
{
	UINT32 u32TestChannel, uSamplingRate;	
	UINT8 u8SrcFormat;

		DrvSPU_Open();
		
//		uSamplingRate = eDRVSPU_FREQ_32000;
		uSamplingRate = eDRVSPU_FREQ_44100;		
		
		// Right channel 
		u32TestChannel = 0;
		DrvSPU_ChannelOpen((E_DRVSPU_CHANNEL)u32TestChannel);
		DrvSPU_SetBaseAddress((E_DRVSPU_CHANNEL)u32TestChannel, (UINT32)g_AudioPattern);
		DrvSPU_SetThresholdAddress((E_DRVSPU_CHANNEL)u32TestChannel, (UINT32)g_AudioPattern + sizeof(g_AudioPattern));
		DrvSPU_SetEndAddress((E_DRVSPU_CHANNEL)u32TestChannel, (UINT32)g_AudioPattern + sizeof(g_AudioPattern));
		DrvSPU_SetChannelVolume((E_DRVSPU_CHANNEL)u32TestChannel, 100);	
		DrvSPU_SetSampleRate((E_DRVSPU_SAMPLING)uSamplingRate);		
		DrvSPU_SetPAN((E_DRVSPU_CHANNEL)u32TestChannel, 0x1F00);	// MSB 8-bit = right channel; LSB 8-bit = left channel			
		DrvSPU_SetDFA((E_DRVSPU_CHANNEL)u32TestChannel, 0x400);	
		u8SrcFormat = DRVSPU_STEREO_PCM16_RIGHT;		
		DrvSPU_SetSrcType((E_DRVSPU_CHANNEL)u32TestChannel, (E_DRVSPU_FORMAT)u8SrcFormat);

		// left channel 		
		u32TestChannel++;
		DrvSPU_ChannelOpen((E_DRVSPU_CHANNEL)u32TestChannel);
		DrvSPU_SetBaseAddress((E_DRVSPU_CHANNEL)u32TestChannel, (UINT32)g_AudioPattern);
		DrvSPU_SetThresholdAddress((E_DRVSPU_CHANNEL)u32TestChannel, (UINT32)g_AudioPattern + sizeof(g_AudioPattern));
		DrvSPU_SetEndAddress((E_DRVSPU_CHANNEL)u32TestChannel, (UINT32)g_AudioPattern + sizeof(g_AudioPattern));
		DrvSPU_SetChannelVolume((E_DRVSPU_CHANNEL)u32TestChannel, 100);	
		DrvSPU_SetSampleRate((E_DRVSPU_SAMPLING)uSamplingRate);		
		DrvSPU_SetPAN((E_DRVSPU_CHANNEL)u32TestChannel, 0x001F);	// MSB 8-bit = right channel; LSB 8-bit = left channel			
		DrvSPU_SetDFA((E_DRVSPU_CHANNEL)u32TestChannel, 0x400);	

		u8SrcFormat = DRVSPU_STEREO_PCM16_LEFT;		
		DrvSPU_SetSrcType((E_DRVSPU_CHANNEL)u32TestChannel, (E_DRVSPU_FORMAT)u8SrcFormat);

        spuDacOn(2);
        sysDelay(30);
        spuSetDacSlaveMode();

//		DrvSPU_SetVolume(0x1a1a);
		DrvSPU_StartPlay();

//    while(1);	
		
	return(0);
}
