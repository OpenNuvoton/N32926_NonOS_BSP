#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#pragma import(__use_no_semihosting_swi)

#include "wbio.h"
#include "wblib.h"

#include "w55fa92_sic.h"
#include "W55fa92_AudioRec.h"
#include "nvtfat.h"
#include "spu.h"

#include "AviLib.h"

extern VOID spuSetDacSlaveMode(void);

//#pragma import(__use_no_semihosting_swi)

MV_CFG_T	_tMvCfg;
int iii = 0;

void  mp3_play_callback(MV_CFG_T *ptMvCfg)
{
	MV_INFO_T 	*ptMvInfo;
	static INT	last_time = 0;

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
	sysEnableCache(CACHE_WRITE_BACK);

	/*-----------------------------------------------------------------------*/
	/*  CPU/HCLK/APB:  192/96/48                                             */
	/*-----------------------------------------------------------------------*/
  sysSetDramClock(eSYS_MPLL, 288000000, 288000000);
	sysSetSystemClock(eSYS_UPLL, 		//E_SYS_SRC_CLK eSrcClk,
					192000000,		//UINT32 u32PllHz,
					192000000);		//UINT32 u32SysHz,

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
