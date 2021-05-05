/**************************************************************************//**
 * @file     Demo_MP3.c
 * @brief    Demo how to play MP3 file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#pragma import(__use_no_semihosting_swi)

#include "wbio.h"
#include "wblib.h"

#include "W55FA92_SIC.h"
#include "W55FA92_AUR.h"
#include "NVTFAT.h"
#include "SPU.h"

#include "AVILib.h"

extern VOID spuSetDacSlaveMode(void);

//#pragma import(__use_no_semihosting_swi)

MV_CFG_T	_tMvCfg;
int iii = 0;
int setvolume=1;

void  mp3_play_callback(MV_CFG_T *ptMvCfg)
{
	MV_INFO_T 	*ptMvInfo;
	static INT	last_time = 0;

	if (setvolume ==1)
	{
		aviSetPlayVolume	(100);
		setvolume=0;
	}
	
	mflGetMovieInfo(ptMvCfg, &ptMvInfo);

	if ((sysGetTicks(TIMER0) - last_time > 100) &&
		ptMvInfo->uAuTotalFrames)
	{
		sysprintf("T=%d, Progress = %02d:%02d / %02d:%02d\n", 
					sysGetTicks(TIMER0) / 100,
					ptMvInfo->uPlayCurTimePos / 6000, (ptMvInfo->uPlayCurTimePos / 100) % 60,
					ptMvInfo->uMovieLength / 6000, (ptMvInfo->uMovieLength / 100) % 60);
		last_time = sysGetTicks(TIMER0);
		iii++;
	}
	
	if ( iii >= 15 )
		mflPlayControl(&_tMvCfg, PLAY_CTRL_STOP, 0);
}

int main()
{
    WB_UART_T 	uart;
    UINT32		u32ExtFreq;
    UINT32		u32PllOutHz;
	CHAR		suFileName[128];
	INT			nStatus;
	UINT8		buf;

	
	/* CACHE_ON	*/
		sysInvalidCache(); 	
	sysEnableCache(CACHE_WRITE_BACK);

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

	u32ExtFreq = sysGetExternalClock();

	/*-----------------------------------------------------------------------*/
	/*  Init UART, N,8,1, 115200                                             */
	/*-----------------------------------------------------------------------*/
	sysUartPort(1);
	uart.uiFreq = u32ExtFreq;					//use XIN clock
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	uart.uart_no = WB_UART_1;
    sysInitializeUART(&uart);
	sysprintf("UART initialized.\n");

	/*-----------------------------------------------------------------------*/
	/*  Init timer                                                           */
	/*-----------------------------------------------------------------------*/
	sysSetTimerReferenceClock (TIMER0, u32ExtFreq);
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);		/* 100 ticks/per sec ==> 1tick/10ms */

	/*-----------------------------------------------------------------------*/
	/*  Init FAT file system                                                 */
	/*-----------------------------------------------------------------------*/
	sysprintf("fsInitFileSystem.\n");
	fsInitFileSystem();

	/*-----------------------------------------------------------------------*/
	/*  Init SD card                                                         */
	/*-----------------------------------------------------------------------*/
	u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	sicIoctl(SIC_SET_CLOCK, u32PllOutHz/1000, 0, 0);	// clock from PLL
	sicOpen();
	sysprintf("total sectors (%x)\n", sicSdOpen0());
	
	//FIXME
	outp32(REG_APBCLK, inp32(REG_APBCLK) | ADC_CKE);
	outp32(REG_CLKDIV3, (inp32(REG_CLKDIV3) & ~(ADC_N1 | ADC_S| ADC_N0)) );                              /* Fed to ADC in 12MHz=External clock */
	DrvAUR_AudioI2cRead((E_AUR_ADC_ADDR)0x21, (UINT8*) &buf);
	DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)0x21, buf & ~0x40); // enable ADC VMID bias
	spuOpen(eDRVSPU_FREQ_8000);
	spuDacOn(2);
	sysDelay(30);
	spuSetDacSlaveMode();

	/*-----------------------------------------------------------------------*/
	/*                                                                       */
	/*  MP3 File playback          	                                         */
	/*                                                                       */
	/*-----------------------------------------------------------------------*/
	memset((UINT8 *)&_tMvCfg, 0, sizeof(_tMvCfg));

	fsAsciiToUnicode("c:\\1.mp3", suFileName, TRUE);
	
	_tMvCfg.eInMediaType			= MFL_MEDIA_MP3;
	_tMvCfg.eInStrmType				= MFL_STREAM_FILE;
	_tMvCfg.szIMFAscii				= NULL;
	_tMvCfg.suInMetaFile			= NULL;
	_tMvCfg.szITFAscii				= NULL;
	_tMvCfg.nAudioPlayVolume		= 31;
	_tMvCfg.uStartPlaytimePos		= 0;
	_tMvCfg.nAuABRScanFrameCnt		= 50;
	_tMvCfg.ap_time					= mp3_play_callback;
	_tMvCfg.suInMediaFile			= suFileName;
	
	if ( (nStatus = mflMediaPlayer(&_tMvCfg)) < 0 )
		sysprintf("Playback failed, code = %x\n", nStatus);
	else
		sysprintf("Playback done.\n");
	
	while(1);		
}
