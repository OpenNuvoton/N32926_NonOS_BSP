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
	UINT32 uWriteAddr, uReadAddr;
    WB_UART_T uart;
    UINT32 u32ExtFreq, u32PllOutKHz;

    u32ExtFreq = sysGetExternalClock();
    uart.uart_no = WB_UART_1;
    uart.uiFreq = u32ExtFreq;   //use APB clock
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    sysInitializeUART(&uart);
    sysSetLocalInterrupt(ENABLE_FIQ_IRQ);

    u32PllOutKHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
    sysprintf("Power on UPLL out frequency %d Khz\n", u32PllOutKHz);
    u32PllOutKHz = sysGetPLLOutputHz(eSYS_MPLL, u32ExtFreq);
    sysprintf("Power on MPLL out frequency %d Khz\n", u32PllOutKHz);

    u32PllOutKHz = sysGetAPBClock();
    sysprintf("APB %d Khz\n", u32PllOutKHz);
		
    /********************************************************************************************** 
     * Clock Constraints: 
     * (a) If Memory Clock > System Clock, the source clock of Memory and System can come from
     *     different clock source. Suggestion MPLL for Memory Clock, UPLL for System Clock   
     * (b) For Memory Clock = System Clock, the source clock of Memory and System must come from 
     *     same clock source	 
     *********************************************************************************************/
#if 0 
    /********************************************************************************************** 
     * Slower down system and memory clock procedures:
     * If current working clock fast than desired working clock, Please follow the procedure below  
     * 1. Change System Clock first
     * 2. Then change Memory Clock
     * 
     * Following example shows the Memory Clock = System Clock case. User can specify different 
     * Memory Clock and System Clock depends on DRAM bandwidth or power consumption requirement. 
     *********************************************************************************************/
    sysSetSystemClock(eSYS_EXT, 12000000, 12000000);
    sysSetDramClock(eSYS_EXT, 12000000, 12000000);
#else 
    /********************************************************************************************** 
     * Speed up system and memory clock procedures:
     * If current working clock slower than desired working clock, Please follow the procedure below  
     * 1. Change Memory Clock first
     * 2. Then change System Clock
     * 
     * Following example shows to speed up clock case. User can specify different 
     * Memory Clock and System Clock depends on DRAM bandwidth or power consumption requirement.
     *********************************************************************************************/
    sysSetDramClock(eSYS_MPLL, 360000000, 360000000);
    sysSetSystemClock(eSYS_UPLL,            //E_SYS_SRC_CLK eSrcClk,
                      240000000,            //UINT32 u32PllKHz,
                      240000000);           //UINT32 u32SysKHz,
    sysSetCPUClock(240000000/2);
#endif
	
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
	
}

































