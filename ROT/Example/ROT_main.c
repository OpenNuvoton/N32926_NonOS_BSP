#include <stdlib.h>
#include <stdio.h>
#include "wblib.h"
#include "rotlib.h"
#include "W55FA92_SIC.h"
#include "nvtfat.h"
#include "ROT_demo.h"

  

__align(4) UINT8 u8FrameBuffer0[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2];
__align(4) UINT8 u8FrameBuffer1[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2];

volatile UINT32 bIsBuffer0Dirty=0;	//0 Means ready for ROT use
volatile UINT32 bIsBuffer1Dirty=1;	//1 means for TV show
volatile UINT32 VpostUseBuf = 1;		//default 1 due to bIsBuffer1Dirty=1. 

static BOOL bIsRotDoneFlag =0;
void rotDoneHandler(void)
{
	bIsRotDoneFlag =1;
}
void rotClearDoneFlag(void)
{
	bIsRotDoneFlag = 0;
}
BOOL rotGetDoneFlag(void) 
{
	return bIsRotDoneFlag;
}
static BOOL bIsRotAbortFlag =0;
void rotAbortHandler(void)
{
	bIsRotAbortFlag =1;
}
void rotClearAbortFlag(void)
{
	bIsRotAbortFlag = 0;
}
BOOL rotGetAbortFlag(void) 
{
	return bIsRotAbortFlag;
}
static BOOL bIsRotOverFlag =0;
void rotOverHandler(void)
{
	bIsRotOverFlag =1;
}
void rotClearOverFlag(void)
{
	bIsRotOverFlag = 0;
}
BOOL rotGetOverFlag(void) 
{
	return bIsRotOverFlag;
}

int main(void)
{
	WB_UART_T uart;
	
	UINT32 u32ExtFreq, u32PllOutHz;
	
#ifdef __ENABLE_CACHE__	
	sysDisableCache(); 	
	sysFlushCache(I_D_CACHE);		
	sysEnableCache(CACHE_WRITE_BACK);
#else
	sysDisableCache(); 		
#endif 
	InitVPOST(u8FrameBuffer0);	
	u32ExtFreq = sysGetExternalClock();
	u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	
	sysSetTimerReferenceClock (TIMER0, u32ExtFreq);
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);
	
	sysUartPort(1);
	uart.uiFreq = u32ExtFreq;
    	uart.uiBaudrate = 115200;
    	uart.uiDataBits = WB_DATA_BITS_8;
    	uart.uiStopBits = WB_STOP_BITS_1;
    	uart.uiParity = WB_PARITY_NONE;
    	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    	uart.uart_no = WB_UART_1;
    	sysInitializeUART(&uart);
    	sysSetLocalInterrupt(ENABLE_FIQ_IRQ);	
    	
    	fsInitFileSystem();
    	sicIoctl(SIC_SET_CLOCK, u32PllOutHz/1000, 0, 0);	
	sicOpen();	
	if (sicSdOpen0()<=0)
	{
		sysprintf("Error in initializing SD card !! \n");						
		while(1);
	}	
	fsAssignDriveNumber('C', DISK_TYPE_SD_MMC, 0, 1);
    	
    	//Create_File_Test();    	    	
	DBG_PRINTF("PLL out frequency %d Khz\n", u32PllOutHz);	
	DBG_PRINTF("Please prepare source pattern in SD card 0. Detail refer readme.txt \n");	
	DBG_PRINTF("Start Conversion\n");
	while(1)
		Emu_DestinationLineOffsetFineTune(u8FrameBuffer0, u8FrameBuffer1);
}    	


