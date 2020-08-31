/**************************************************************************//**
 * @file     demo.c
 * @brief    Demostrate keypad, touch pannel, voltage detection and wake up system by touch event
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include "wblib.h"
#include "W55FA92_ADC.h"
#include "demo.h"

/*
	1. There are max 3 AIN. They are SA-AHS(Channel 1), SA-AIN2(Channel 2) and SA-SEN(channel 3)
	2. SA-SEN is used in 5 wire touch panel. So there are 2 chanel for AIN for 5-wire touch panel
*/
int main(VOID)
{
	//unsigned int volatile i;
	WB_UART_T uart;
	UINT32 u32Item, u32ExtFreq;
	UINT32 u32PllOutHz;
	
	u32ExtFreq = sysGetExternalClock();
	sysUartPort(1);
	uart.uart_no = WB_UART_1; 
	uart.uiFreq = u32ExtFreq;	//use APB clock
  uart.uiBaudrate = 115200;
  uart.uiDataBits = WB_DATA_BITS_8;
  uart.uiStopBits = WB_STOP_BITS_1;
  uart.uiParity = WB_PARITY_NONE;
  uart.uiRxTriggerLevel = LEVEL_1_BYTE;
  uart.uart_no = WB_UART_1;
  sysInitializeUART(&uart);
  sysSetLocalInterrupt(ENABLE_FIQ_IRQ);	
    	
  DrvADC_Open();
   
  u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	DBG_PRINTF("PLL out frequency %d Hz\n", u32PllOutHz);	
	do
	{    	
		DBG_PRINTF("================================================================\n");
		DBG_PRINTF("						Touch library demo code									  	\n");
		DBG_PRINTF(" [1] KeyPad demo 																\n");
		DBG_PRINTF(" [2] Voltage detection demo 														\n");
		DBG_PRINTF(" [3] Touch panel demo 															\n");
		DBG_PRINTF(" [4] Integration demo 																\n");
		DBG_PRINTF(" [5] Touch wakeup																\n");
		DBG_PRINTF(" [6] Key wakeup		 															\n");		
		DBG_PRINTF(" [7] Pen up/down detect														\n");		
		DBG_PRINTF("================================================================\n");
		
		u32Item = sysGetChar();
		//u32Item = '3';
		switch(u32Item)
		{
						
			case '1': 	
					DBG_PRINTF("Please input test channel AIN1, AIN2 or AIN3 (DEV:Default AIN2, HMI: Change to GPIO IP)\n");
					u32Item = sysGetChar();
					if( (u32Item>'0')||(u32Item<'4') )
						KeyPad((u32Item-0x30));		
					else
						DBG_PRINTF("Input Wrong Channel\n");	
					break; 	
			case '2': 	
					DBG_PRINTF("Please input test channel AIN1, AIN2 or AIN3 (DEV:Default AIN1, HMI: Don't Support)\n");
					u32Item = sysGetChar();
					if( (u32Item>'0')||(u32Item<'4') )
						VoltageDetection((u32Item-0x30));		
					else
						DBG_PRINTF("Input Wrong Channel\n");	
					break;
			case '3':	
					Raw_TouchPanel();
					break;	
	    case '4':	
	    		Integration();
	    		break;
			case '5':			
					Raw_TouchPanel();		
					DrvADC_Wakeup(eADC_WAKEUP_TOUCH);
					sysPowerDown(WE_ADC);
					break;
			case '6':			
					KeyPad(2);		
					DrvADC_Wakeup(eADC_WAKEUP_KEY);
					sysPowerDown(WE_ADC);
					break;		
			case '7':		
					Polling_Processed_TouchPanel();
				  break;
	    		case 'Q':	break;
	    		case 'q':	break;
			}
	}while((u32Item!= 'q') || (u32Item!= 'Q'));	
    	return 0;
} /* end main */
