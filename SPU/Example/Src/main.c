/**************************************************************************//**
 * @file     main.c
 * @version  V3.00
 * @brief    N329xx series SPU demo code
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wblib.h"
#include "W55FA92_SPU.h"

#if defined (__GNUC__) && !(__CC_ARM)
__attribute__ ((aligned (32))) UINT8 g_AudioPattern[] = {
		#include "PCM16_raw.dat"
};
#else
__align (32) UINT8 g_AudioPattern[] = {
		#include "PCM16_raw.dat"
};
#endif


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

#if 1
	/* enable U10 ISD8101 in N9H26 HMI demo board */
	outpw(REG_GPAFUN0, inpw(REG_GPAFUN0)&~MF_GPA0);	// enable LPCLK pin
	outpw(REG_GPIOA_OMD, REG_GPIOA_OMD| 0x00000001);
	outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT)| 0x00000001);
#endif
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
		sysprintf("Start Playing...\n");

    while(1);
		
	return(0);
}
