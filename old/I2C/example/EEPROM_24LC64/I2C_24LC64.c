/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                      */
/*                                                                                   */
/* Copyright (c) 2008 by Nuvoton Technology Corporation                             */
/* All rights reserved                                                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
/* File Contents:                                                                    */
/*   i2c_24LC64.c                                                                    */
/*                                                                                   */
/* This file contains:                                                               */
/*                                                                                   */
/* Project:                                                                          */
/*                                                                                   */
/* Description:                                                                      */
/*   This file is a sample program used to access EEPROM (24LC64) on W90P910         */
/*   EV Board.                                                                       */
/*                                                                                   */
/* Remark:                                                                           */
/*   1. Execute this program on 910 EV board that should set slave addr 0x50 for     */
/*      I2C0 and set slave addr 0x51 for I2C1. It's H/W disign issue.                */
/*   2. Microchip 24LC64 is EEPROM, 64K bits size, page-write buffer for up to       */
/*      32bytes.                                                                     */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/ 
 
#include <stdio.h>
#include <string.h>
#include "wblib.h"

#include "w55fa92_i2c.h"


/* If test I2C on module board, should configure switch of GPIOH pins. */
//#define MODULE_BOARD

#define DBG_PRINTF		sysprintf
//#define DBG_PRINTF(...)

#define RETRY		1000  /* Programming cycle may be in progress. Please refer to 24LC64 datasheet */

#define TXSIZE		512
#define ADDROFFSET  1024


//------------------------- Program -------------------------//
void i2cExample (void)
{
	unsigned char data[TXSIZE], value[TXSIZE];
	int i, j, err, cnt;
	INT32 rtval;
	
	/* initialize test data */
	for(i = 0 ; i < TXSIZE ; i++)
		data[i] = i + 1;
	i2cInit();	
	
	/* Byte Write/Random Read */
	DBG_PRINTF("\nI2C Byte Write/Random Read ...\n");
	
	rtval = i2cOpen();
	if(rtval < 0)
	{
		DBG_PRINTF("Open I2C error!\n");
		goto exit_test;
	}	
	i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, 0x50, 0);  
	i2cIoctl(I2C_IOC_SET_SPEED, 100, 0);
	
	i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0); 

	
	/* Tx porcess */
	DBG_PRINTF("Start Tx\n");
	for(i = 0 ; i < TXSIZE ; i++)
	{
		i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, i, 2);	
		j = RETRY;	
		while(j-- > 0) 
		{
			if(i2cWrite(&data[i], 1) == 1)
				break;
		}						
		if(j <= 0)
			DBG_PRINTF("WRITE ERROR [%d]!\n", i);

		sysDelay(1);
	}
	
	/* Rx porcess */	
	DBG_PRINTF("Start Rx\n");
	memset(value, 0 , TXSIZE);
	for(i = 0 ; i < TXSIZE ; i++)
	{
		i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, i, 2);	
		j = RETRY;
		while(j-- > 0) 
		{
			if(i2cRead(&value[i], 1) == 1)
				break;
		}
		if(j <= 0)
			DBG_PRINTF("Read ERROR [%d]!\n", i);
	}
	
	/* Compare process */
	DBG_PRINTF("Compare data\n");
	err = 0;
	cnt = 0;
	for(i = 0 ; i < TXSIZE ; i++)
	{
		if(value[i] != data[i])	
		{
			DBG_PRINTF("[%d] addr = 0x%08x, val = 0x%02x (should be 0x%02x)\n", i, i + ADDROFFSET, value[i], data[i]);
			err = 1;
			cnt++;			
		}
	}					
	if(err)
		DBG_PRINTF("Test failed [%d]!\n\n", cnt);	
	else
		DBG_PRINTF("Test success\n\n");	

	
	i2cClose();		
	/* Test End */
	
exit_test:
	
	DBG_PRINTF("\nTest finish ...\n");	
	i2cExit();
	
	return;
}	

int main (void)
{
	WB_UART_T uart;
	UINT32 u32ExtFreq;

	u32ExtFreq = sysGetExternalClock();    	/* Hz unit */	
	uart.uart_no = WB_UART_1; 
	uart.uiFreq = u32ExtFreq;
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	sysInitializeUART(&uart);	

/*
	sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
						240000000,		//UINT32 u32PllKHz,
						240000000);		//UINT32 u32SysKHz,									
	sysSetCPUClock(240000000);
	sysSetAPBClock(48000000);
*/

	i2cExample();
	
	return 0;
}




