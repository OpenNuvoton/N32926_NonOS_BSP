/****************************************************************************
*                                                                           *
* Copyright (c) 2013 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "w55fa92_reg.h"
#include "w55fa92_sic.h"
#include "w55fa92_gnand.h"
#include "w55fa92_gpio.h"
#include "w55fa92_ts_adc.h"
#include "wblib.h"
#include "nvtfat.h"

#include "nvtloader.h"


BOARD_S s_board;
int g_ibr_boot_sd_port;     // indicate the SD port number which IBR boot from.
UINT8 g_kbuf[CP_SIZE];


void init(BOARD_S* ps_board)
{
	WB_UART_T 	uart;
	UINT32 		u32ExtFreq;
	UINT32 u32Cke = inp32(REG_AHBCLK);
	
	
	
	
	u32ExtFreq = sysGetExternalClock();
	/* enable UART */
    	sysUartPort(1);
    	uart.uart_no = WB_UART_1; 	
	uart.uiFreq = u32ExtFreq;					/* Hz unit */
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	sysInitializeUART(&uart);	
			
#if 0
	{
		UINT32 u32PllOutHz;
		u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
		DBG_PRINTF("UPLL out frequency %d Hz\n", u32PllOutHz);	
		u32PllOutHz = sysGetPLLOutputHz(eSYS_MPLL, u32ExtFreq);    	
		DBG_PRINTF("MPLL out frequency %d Hz\n", u32PllOutHz);	
		u32PllOutHz = sysGetAPBClock();    	
		DBG_PRINTF("APB %d Hz\n", u32PllOutHz);

		gpio_setportpull(GPIO_PORTB, 0x01, 0x01);		
		gpio_setportdir(GPIO_PORTB, 0x01, 0x00);	

	//	sysSetSystemClock(eSYS_EXT, 12000000, 12000000);	
	//	sysSetDramClock(eSYS_EXT, 12000000, 12000000);							
		sysSetDramClock(eSYS_MPLL, 360000000, 360000000);
		sysprintf("Please modify session file address 0xb0003054 to 0x%x\n", (inp32(0xb0003058)|0x10));
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
						240000000,		//UINT32 u32PllKHz,
						240000000);		//UINT32 u32SysKHz,
	}					
#endif	
	
	
	
	/* RTC has been open in NAND/SD Loader	*/
	/* RTC_Ioctl(0,RTC_IOC_SET_POWER_KEY_DELAY, 7, 0); */
	if(ps_board ->spkpower_init != NULL) 
		ps_board ->spkpower_init();
	if(ps_board ->earphone_init != NULL) 
		ps_board ->earphone_init();	

	/* Reset SIC engine to fix USB update kernel and mvoie file */
	outp32(REG_AHBCLK, u32Cke  | (SIC_CKE | NAND_CKE | SD_CKE));
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST )|SIC_RST );
	outp32(REG_AHBIPRST, 0);
	outp32(REG_APBCLK, inp32(REG_APBCLK) | RTC_CKE);
	outp32(REG_APBIPRST, TMR0RST | TMR1RST);
	outp32(REG_APBIPRST, 0);
 	
	sysprintf("PWRON =  0x%x\n", inp32(PWRON));
	outp32(REG_AHBCLK,u32Cke);

	/* init timer */
	sysSetTimerReferenceClock (TIMER0,
								u32ExtFreq);	/* Hz unit */
	sysStartTimer(TIMER0,
					100,
					PERIODIC_MODE);
					

	sysSetLocalInterrupt(ENABLE_IRQ);					
    	


	if(ps_board->mute_init != NULL) 
		ps_board->mute_init();
	if(ps_board->mute_enable != NULL) 
		ps_board->mute_enable();	

	initVPostShowLogo(ps_board);

	DrvADC_Open();
}


INT32 main(void)
{
	// IBR and SDLoader keep the booting SD port number on register SDCR.
	// NVTLoader should load image from same SD port.
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) | SD_CKE);   // SDLoader disable SIC/SD clock before jump to NVTLoader.
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) | SIC_CKE);  // Now, enable clock to read SD register.
	g_ibr_boot_sd_port = (inpw(REG_SDCR) & SDCR_SDPORT) >> 29;
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) & ~SD_CKE);
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) & ~SIC_CKE);
	sysprintf("NVT Loader Start\n");
	
	sysDisableCache(); 	
	sysFlushCache(I_D_CACHE);	
	sysEnableCache(CACHE_WRITE_BACK);  
	
	register_board(&s_board);
	init(&s_board);	
	initVPostShowLogo(&s_board);
	
	sysprintf("NVT Loader: g_ibr_boot_sd_port = %d\n", g_ibr_boot_sd_port);

	fsInitFileSystem();
#ifdef __ENABLE_SD_CARD_0__	
	NVT_LoadKernelFromSD(&s_board, 				/* Board */
						g_ibr_boot_sd_port, 	/* Boot port */	
						g_kbuf);				/* Temp buffer */
#endif						
#if defined(__ENABLE_NAND_0__) || defined(__ENABLE_NAND_1__) || defined(__ENABLE_NAND_2__)	
	NVT_LoadKernelFromNAND(&s_board, 				/* Board */
						g_ibr_boot_sd_port, 	/* Boot port */	
						g_kbuf);				/* Temp buffer */
#endif
	return Successful;
}