/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/

/****************************************************************************
* FILENAME
*   gpio.c
*
* VERSION
*   1.0
*
* DESCRIPTION
*   GPIO sample application using GPIO library. Pull up and low port D pins
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
#include "w55fa92_gpio.h"


int main(void)
{
	WB_UART_T uart;
	UINT32 u32ExtFreq;	
	int i;
	unsigned int ii=0;
/*
	sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
						240000000,		//UINT32 u32PllKHz,
						240000000);		//UINT32 u32SysKHz,									
	sysSetCPUClock(240000000);
	sysSetAPBClock(48000000);
*/
	u32ExtFreq = sysGetExternalClock();    	/* Hz unit */	
	uart.uart_no = WB_UART_1;
	uart.uiFreq = u32ExtFreq;
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	sysInitializeUART(&uart);		
	
	gpio_setportval(GPIO_PORTB, 0xf, 0);
	gpio_setportpull(GPIO_PORTB, 0xf, 0);
	gpio_setportdir(GPIO_PORTB, 0xf, 0xf);
	
	sysprintf("start gpio test... \n");
//	while(1) {
	while(ii++<1000000) {
		for(i = 0; i < 0xf; i++) {
			gpio_setportval(GPIO_PORTB, 0xf, i);
			sysDelay(100);	
		}	
	}
	
	sysprintf("quit gpio test\n");
	return(0);
}
