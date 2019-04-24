/***************************************************************************
 *                                                                         *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include "wblib.h"

#include "W55FA92_GPIO.h"
#include "W55FA92_VideoIn.h"
#include "demo.h"
#include "jpegcodec.h"
#include "usbd.h"
#include "videoclass.h"

VINDEV_T Vin;
VINDEV_T* pVin;
	
void init(void)
{
	WB_UART_T uart;
	UINT32 u32ExtFreq;
	
	/* Cache on */ 
	sysInvalidCache();
	sysDisableCache();
	sysEnableCache(CACHE_WRITE_BACK);
	
	/* Init UART */
	sysUartPort(1);
	u32ExtFreq = sysGetExternalClock();    	/* Hz unit */		
	uart.uiFreq = u32ExtFreq;
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;    
	uart.uart_no  = WB_UART_1;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    sysInitializeUART(&uart);
    
	sysprintf("UART Init\n");
	/* Init Timer */
	u32ExtFreq = sysGetExternalClock();	
	sysSetTimerReferenceClock(TIMER0, u32ExtFreq); //External Crystal	
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);		/* 100 ticks/per sec ==> 1tick/10ms */	
	sysSetLocalInterrupt(ENABLE_FIQ_IRQ);			
		
	sysSetDramClock(eSYS_MPLL, 360000000, 360000000);
	
	sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
					240000000,		//UINT32 u32PllKHz,
					240000000);		//UINT32 u32SysKHz,		
}			
int main()
{
	UINT32 u32Item;
	/*Due to I2C share pins with UART, the SerialIO can not be used*/
	init();	
#ifdef __UVC_VIN__	
	uvcPuInfo.PU_BACKLIGHT_COMPENSATION_MIN = 0;
	uvcPuInfo.PU_BACKLIGHT_COMPENSATION_MAX = 255;
	uvcPuInfo.PU_BACKLIGHT_COMPENSATION_DEF = 128;
	uvcPuInfo.PU_BRIGHTNESS_MIN = 0;
	uvcPuInfo.PU_BRIGHTNESS_MAX = 255;
	uvcPuInfo.PU_BRIGHTNESS_DEF =64;
	uvcPuInfo.PU_CONTRAST_MIN = 0;
	uvcPuInfo.PU_CONTRAST_MAX = 255;
	uvcPuInfo.PU_CONTRAST_DEF = 32;
	uvcPuInfo.PU_HUE_MIN = 0;
	uvcPuInfo.PU_HUE_MAX = 255;
	uvcPuInfo.PU_HUE_DEF = 16;
	uvcPuInfo.PU_SATURATION_MIN = 0;
	uvcPuInfo.PU_SATURATION_MAX = 0;
	uvcPuInfo.PU_SATURATION_DEF = 0;
	uvcPuInfo.PU_SHARPNESS_MIN = 0;
	uvcPuInfo.PU_SHARPNESS_MAX =255;
	uvcPuInfo.PU_SHARPNESS_DEF = 192;
	uvcPuInfo.PU_GAMMA_MIN = 0;
	uvcPuInfo.PU_GAMMA_MAX = 0;
	uvcPuInfo.PU_GAMMA_DEF = 0;
	uvcPuInfo.PU_POWER_LINE_FREQUENCY_MIN = 0;
	uvcPuInfo.PU_POWER_LINE_FREQUENCY_MAX = 0;
	uvcPuInfo.PU_POWER_LINE_FREQUENCY_DEF = 0; 			    				
 Menu:  			
	DBG_PRINTF("================================================================\n");
	DBG_PRINTF("				VideoIn library demo code						\n"); 
	DBG_PRINTF(" [1] OV9660			 											\n");
	DBG_PRINTF(" [2] OV7670 													\n");
	DBG_PRINTF(" [3] OV7725 	  												\n");
	DBG_PRINTF(" [4] NT99141 													\n"); /* OK on DEV */
	DBG_PRINTF(" [5] NT99050	     	 										\n"); /* OK on DEV */
	DBG_PRINTF(" [6] NT99160	 												\n"); /* OK on DEV */
	DBG_PRINTF("================================================================\n");
	u32Item = sysGetChar();
#if defined(__1ST_PORT__) && !defined(__2ND_PORT__)
		sysprintf("Plug in sensor to port 1\n");	
#endif
#if !defined(__1ST_PORT__) && defined(__2ND_PORT__)
		sysprintf("Plug in sensor to port 2\n");	
#endif
#if defined(__1ST_PORT__) && defined(__2ND_PORT__)
		sysprintf("Plug in sensor to port 1 and port 2\n");	
#endif
	switch(u32Item)
	{
		case '1':	
			sysprintf("Enable conditional compile  [OV9660]\n");											
			Smpl_OV9660();		
			break; 	
		case '2':	
			sysprintf("Enable conditional compile  [OV7670]\n");											
			Smpl_OV7670();		
			break; 	
		case '3':	
			sysprintf("Enable conditional compile  [OV7725]\n");											
			Smpl_OV7725();		
			break; 									
		case '4':
			sysprintf("Enable conditional compile  [NT99141]\n");	
			Smpl_NT99141();
			break;
		case '5':
			sysprintf("Enable conditional compile  [NT99050]\n");	
			Smpl_NT99050();	
			break;	
		case '6':
			sysprintf("Enable conditional compile  [NT99160]\n");
			Smpl_NT99160();	
			break;				
		default:
			sysprintf("Wrong item!!\n");	
			goto Menu;										
	}		
	jpegOpen ();    
#endif		
	uvc_main();		
//	while(1);
    return 0;
} /* end main */
