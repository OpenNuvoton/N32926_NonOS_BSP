/**************************************************************************//**
 * @file     Demo_AVI.c
 * @brief    Demo how to play AVI file
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

#include "W55FA92_VPOST.h"
#include "W55FA92_SIC.h"
#include "NVTFAT.h"
#include "AVILib.h"
#include "SPU.h"


//#pragma import(__use_no_semihosting_swi)
//#define VPOST_FRAME_BUFSZ		(640*480*2)
#define VPOST_FRAME_BUFSZ		(720*480*2)

#if defined (__GNUC__)
UINT8  _VpostFrameBufferPool[VPOST_FRAME_BUFSZ] __attribute__((aligned (256)));
#else
static __align(256) UINT8  _VpostFrameBufferPool[VPOST_FRAME_BUFSZ];
#endif
static UINT8   *_VpostFrameBuffer;


void  avi_play_control(AVI_INFO_T *aviInfo)
{
	static INT	last_time;
	int    frame_rate;
	
	if (aviInfo->uPlayCurTimePos != 0)
		frame_rate = ((aviInfo->uVidFramesPlayed - aviInfo->uVidFramesSkipped) * 100) / aviInfo->uPlayCurTimePos;
	
	if (aviInfo->uPlayCurTimePos - last_time > 100)
	{
		sysprintf("%02d:%02d / %02d:%02d  Vid fps: %d / %d\n", 
			aviInfo->uPlayCurTimePos / 6000, (aviInfo->uPlayCurTimePos / 100) % 60,
			aviInfo->uMovieLength / 6000, (aviInfo->uMovieLength / 100) % 60,
			frame_rate, aviInfo->uVideoFrameRate);
		last_time = aviInfo->uPlayCurTimePos;
	}
}


int main()
{
    WB_UART_T 	uart;
	LCDFORMATEX lcdformatex;
	CHAR		suFileName[128];
	INT			nStatus;

	
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

	/*-----------------------------------------------------------------------*/
	/*  Init UART, N,8,1, 115200                                             */
	/*-----------------------------------------------------------------------*/
	uart.uart_no = WB_UART_1;	
#if 1	
	uart.uiFreq = 12000000;					//use XIN clock
#else
	uart.uiFreq = 27000000;	
#endif	
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    sysInitializeUART(&uart);
	sysprintf("UART initialized.\n");

	_VpostFrameBuffer = (UINT8 *)((UINT32)_VpostFrameBufferPool | 0x80000000);

	/*-----------------------------------------------------------------------*/
	/*  Init timer                                                           */
	/*-----------------------------------------------------------------------*/
	sysSetTimerReferenceClock (TIMER0, 12000000);
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);

	/*-----------------------------------------------------------------------*/
	/*  Init FAT file system                                                 */
	/*-----------------------------------------------------------------------*/
	sysprintf("fsInitFileSystem.\n");
	fsInitFileSystem();

	/*-----------------------------------------------------------------------*/
	/*  Init SD card                                                         */
	/*-----------------------------------------------------------------------*/
	sicIoctl(SIC_SET_CLOCK, sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock())/1000, 0, 0);    // clock from PLL
	sicOpen();
	sysprintf("total sectors (%x)\n", sicSdOpen0());
	
	spuOpen(eDRVSPU_FREQ_8000);
	spuDacOn(1);	
//	sysDelay(100);	
	spuSetDacSlaveMode();	

#if 0
	/*-----------------------------------------------------------------------*/
	/*                                                                       */
	/*  Direct RGB555 AVI playback 	               							 */
	/*                                                                       */
	/*-----------------------------------------------------------------------*/
	lcdformatex.ucVASrcFormat = DRVVPOST_FRAME_RGB555;
    vpostLCMInit(&lcdformatex, (UINT32 *)_VpostFrameBuffer);

	fsAsciiToUnicode("c:\\480x272.avi", suFileName, TRUE);	

   	if (aviPlayFile(suFileName, 0, 0, DIRECT_RGB555, avi_play_control) < 0)
		sysprintf("Playback failed, code = %x\n", nStatus);
	else
		sysprintf("Playback done.\n");

#endif
#if 1
	/*-----------------------------------------------------------------------*/
	/*                                                                       */
	/*  Direct RGB565 AVI playback 	                                         */
	/*                                                                       */
	/*-----------------------------------------------------------------------*/
	lcdformatex.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
    vpostLCMInit(&lcdformatex, (UINT32 *)_VpostFrameBuffer);

	fsAsciiToUnicode("c:\\480x272.avi", suFileName, TRUE);	

   	if (aviPlayFile(suFileName, 0, 0, DIRECT_RGB565, avi_play_control) < 0)
		sysprintf("Playback failed, code = %x\n", nStatus);
	else
		sysprintf("Playback done.\n");
#endif
#if 0
	/*-----------------------------------------------------------------------*/
	/*                                                                       */
	/*  Direct YUV422 AVI playback 	                                         */
	/*                                                                       */
	/*-----------------------------------------------------------------------*/
	lcdformatex.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;
    vpostLCMInit(&lcdformatex, (UINT32 *)_VpostFrameBuffer);

	fsAsciiToUnicode("c:\\480x272.avi", suFileName, TRUE);	

   	if (aviPlayFile(suFileName, 0, 0, DIRECT_YUV422, avi_play_control) < 0)
		sysprintf("Playback failed, code = %x\n", nStatus);
	else
		sysprintf("Playback done.\n");
#endif	

	while(1);		
}



