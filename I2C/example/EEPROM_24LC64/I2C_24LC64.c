/**************************************************************************//**
 * @file     i2c_24LC64.c
 * @brief    Read/write EEPROM via I2C interface
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/ 
 
#include <stdio.h>
#include <string.h>
#include "wblib.h"

#include "W55FA92_I2C.h"


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

	i2cExample();
	
	return 0;
}




