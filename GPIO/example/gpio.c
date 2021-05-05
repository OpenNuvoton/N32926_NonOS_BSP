/**************************************************************************//**
 * @file     gpio.c
 * @brief    Show how to set GPIO pin mode and output control.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wblib.h"
#include "W55FA92_GPIO.h"

void EXTINT0_IRQHandler(void)
{
	unsigned int read0;
	
	outpw(REG_IRQTGSRC0 , inpw(REG_IRQTGSRC0) & BIT18);	/* Clear GPB2 interrupt flag */
	sysprintf("EXTINT0 INT occurred.\n");

	gpio_readport(GPIO_PORTB, (unsigned short *)&read0);
	sysprintf("GPB2 pin status %d\n", (read0 & BIT2) >> 2);
}

void GPIOOutputTest(void)
{
	int i;
	
	gpio_setportval(GPIO_PORTB, (1 << 0), 0);
	gpio_setportdir(GPIO_PORTB, (1 << 0), (1 << 0));		/* Set GPB0 output mode */
	
	sysprintf("start gpio output test... \n");
	
	for(i = 0; i < 2; i++) {
		gpio_setportval(GPIO_PORTB, (1 << 0), (1 << 0));	/* Set GPB0 output high */
		sysDelay(50);	
		
		gpio_setportval(GPIO_PORTB, (1 << 0), 0);					/* Set GPB0 output low */
		sysDelay(50);		
	}	
	
	sysprintf("exit gpio output test\n");	
}

void GPIOInterruptTest(void)
{
	int i;
	
	sysprintf("Connect GPB2 and GPB0 to test interrupt ......\n");

	gpio_setportval(GPIO_PORTB, BIT0, BIT0);			/* Set GPB0 output high */
	gpio_setportdir(GPIO_PORTB, BIT0, BIT0);			/* Set GPB0 output mode */	
	
	sysInstallISR(IRQ_LEVEL_7, (INT_SOURCE_E)IRQ_EXTINT0, (PVOID)EXTINT0_IRQHandler);
	sysSetInterruptType((INT_SOURCE_E)IRQ_EXTINT0, HIGH_LEVEL_SENSITIVE);
	sysEnableInterrupt((INT_SOURCE_E)IRQ_EXTINT0);

	gpio_setportdir(GPIO_PORTB, BIT2, 0x0);				/* Set GPB2 input mode */
	gpio_setportpull(GPIO_PORTB, BIT2, BIT2);			/* Set GPB2 pull-low */
	gpio_setsrcgrp(GPIO_PORTB, BIT2, 0x0);				/* set to IRQ_EXTINT0 */
	gpio_setintmode(GPIO_PORTB, BIT2, BIT2, 0);		/* Set GPB2 falling edge trigger */

	sysSetLocalInterrupt(ENABLE_IRQ);

	for(i = 0; i < 2; i++) {
		gpio_setportval(GPIO_PORTB, BIT0, BIT0);		/* Set GPB0 output high */
		sysDelay(50);	
		
		gpio_setportval(GPIO_PORTB, BIT0, 0);				/* Set GPB0 output low */
		sysDelay(50);		
	}	

}

int main(void)
{
	WB_UART_T uart;
	UINT32 u32ExtFreq;	
	INT8 i8Item;

	u32ExtFreq = sysGetExternalClock();    	/* Hz unit */	
	uart.uart_no = WB_UART_1;
	uart.uiFreq = u32ExtFreq;
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	sysInitializeUART(&uart);		

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
    sysSetCPUClock(240000000);
    sysSetAPBClock(60000000);	
#endif	
	
	do
	{
		sysprintf("\n");	
		sysprintf("==================================================================\n");
		sysprintf("[1] GPIO Output Test \n");
		sysprintf("[2] GPIO Interrupt Test \n");			
		sysprintf("==================================================================\n");

		i8Item = sysGetChar();
		
		switch(i8Item) 
		{
			case '1': 
				GPIOOutputTest();			
				break;
					
			case '2':
				GPIOInterruptTest();					
				break;		
								
			case 'Q':
			case 'q': 
				i8Item = 'Q';
				sysprintf("quit GPIO test...\n");				
				break;	
				
			}
		
	}while(i8Item!='Q');
	
	return(0);
}
