/***************************************************************************
 *                                                                         									     *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              					     *
 *                                                                         									     *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "wblib.h"

UINT32 g_u32Idx=0;
volatile BOOL bIsTimeOut=0;
char pi8UartBuf[10010];

UINT32 g_u32Len;
UINT32 g_u32Valid = 0;
UINT32 g_u32Timeout = 0;

UINT32 u32LenR[30]={0};
UINT32 u32LenPtr = 0;
/*
	Receive file with ASCII code from PC. The program excape if receive the 'q' letter
	Remember taht if using high speed UART, it is UART port 0 shared with ICE mode.

*/
void UartDataValid_Handler(UINT8* buf, UINT32 u32Len)
{
	UINT32 u32Idx = 0;
	g_u32Len = u32Len;
	g_u32Valid = g_u32Valid+1;

	//u32LenR[u32LenPtr] = u32Len;
	//u32LenPtr = u32LenPtr+1;

	memcpy(&(pi8UartBuf[g_u32Idx]), buf, u32Len);
	g_u32Idx = g_u32Idx+u32Len;


	while(u32Idx++<u32Len)
	{
		if(*buf++ =='q')
		{
			bIsTimeOut = 1;
			//u8RevBuf[(g_u32Idx-1)] = 0;	/* Cover the "quit " pattern */
			break;
		}
	}
}
void UartDataTimeOut_Handler(UINT8* buf, UINT32 u32Len)
{
	UINT32 u32Idx = 0;
	g_u32Timeout = g_u32Timeout+1;

	//u32LenR[u32LenPtr] = u32Len;
	//u32LenPtr = u32LenPtr+1;

	memcpy(&(pi8UartBuf[g_u32Idx]), buf, u32Len);
	g_u32Idx = g_u32Idx+u32Len;

	while(u32Idx++<u32Len)
	{
		if(*buf++ =='q')
		{
			bIsTimeOut = 1;
			//u8RevBuf[(g_u32Idx-1)] = 0;	/* Cover the "quit " pattern */
			break;
		}
	}
}
int DemoAPI_HUART(void)
{
	WB_UART_T uart;
	char* pi8String;
	UINT8 u8Item;
	UINT32 u32Len;
	UINT32 /*u32Item,*/ u32ExtFreq;//,i;
	UINT32 u32Count;
	INT8 i8UartData=0;
	memset(pi8UartBuf, 0, 1024);
	u32ExtFreq = sysGetExternalClock();

	/* Init UART 0 (High speed) */
	sysUartPort(0);		/* Message will be show in HUART*/
	
	//sysUartPort(1);
	uart.uiFreq = u32ExtFreq;
	uart.uiBaudrate = 921600;
    	uart.uart_no = WB_UART_0;
	
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	
	sysInitializeHUART(&uart);
	sysUartEnableInt(UART_INT_NONE);
	sysUartInstallcallback(UART_INT_RDA, UartDataValid_Handler);
	sysUartInstallcallback(UART_INT_RDTO, UartDataTimeOut_Handler);
	sysprintf("\n\nDemo Start, please input some characters\n");
	sysprintf("Receive data one by one by manual input on hyper-terminal until [q] ASCII code received\n");
	do
	{
        	i8UartData = sysHuartReceive();
        	sysprintf("%c", i8UartData);
        }while (i8UartData!='q');
         sysprintf("\nSingle character input from HUART is done \n");

	/* Hyper-terminal will converse 0x0D,0x0A to 0x0D only. Following code converse the carriage return code 0x0D to 0x0 */
	pi8String = pi8UartBuf;
	u32Len = strlen(pi8String);
	while(*pi8String!=0)
	{
		if(*pi8String==0x0d)
			*pi8String = 0;
		pi8String ++;
	}
	pi8String = pi8UartBuf;
	do
	{
		u32Len = strlen(pi8String);
		sysprintf("%s\n", pi8String);
		pi8String = (char*)((UINT32)pi8String+(u32Len+1));
	}while(u32Len!=0);

	/* Transfer data to PC */

	u32Count = 0;
	while(1)
	{
		for(u8Item='0'; u8Item<='Z'; u8Item=u8Item+1)
		{
			pi8UartBuf[u32Count] = u8Item;
			u32Count = u32Count+1;
			if(u32Count>10000)
				break;
		}
		if(u32Count>10000)
				break;
		for(u8Item='Z'; u8Item>='0'; u8Item=u8Item-1)
		{
			pi8UartBuf[u32Count] = u8Item;
			u32Count = u32Count+1;
			if(u32Count>10000)
				break;
		}
		if(u32Count>10000)
				break;
	}
	sysHuartTransfer(pi8UartBuf,  u32Count);

	/* Init UART 1 (Normal speed) */
	sysUartPort(1);		/* Message will be show in normal speed UART*/
    	return 0;
} /* end main */
