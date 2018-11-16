/*-----------------------------------------------------------------------------------*/
/* Nuvoton Electronics Corporation confidential                                      */
/*                                                                                   */
/* Copyright (c) 2009 by Nuvoton Electronics Corporation                             */
/* All rights reserved                                                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
/* File Contents:                                                                    */
/*   spiflash.c                 		                                             */
/*                                                                                   */
/* This file contains:                                                               */
/*                                                                                   */
/* Project:                                                                          */
/*                                                                                   */
/* Remark:                                                                           */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "wblib.h"
#include "w55fa92_spi.h"


#define TEST_SIZE	512 * 2 * 64
__align(4096) UINT8 WriteBuffer[TEST_SIZE];
__align(4096) UINT8 ReadBuffer[TEST_SIZE];


#define DBG_PRINTF	sysprintf
//#define DBG_PRINTF	printf

int main()
{
	WB_UART_T uart;
	UINT32 u32ExtFreq;
	unsigned char *pSrc, *pDst;
	int volatile i;
	BOOL bLoop = TRUE;
	UINT8 u8Item, u8data;
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

	DBG_PRINTF("SpiFlash Test...\n");

	pSrc = (UINT8 *)((UINT32)WriteBuffer | 0x80000000);
	pDst = (UINT8 *)((UINT32)ReadBuffer | 0x80000000);	

	for (i=0; i<TEST_SIZE; i++)
		*(pSrc+i) = i & 0xff;

	spiFlashInit();

	spiFlashEraseAll();

	while(bLoop)
	{
		sysprintf("\nPlease Select Test Item\n");
		sysprintf(" 1 : Read/Write Test with Winbond W25Q256FV\n");
		sysprintf(" 2 : Qude Read/Write Test with Winbond W25Q256FV\n");
		sysprintf(" 3 : Qude Read/Write Test with EON EN25QH256\n");
		sysprintf(" 9 : Exit\n>");

		u8Item = sysGetChar();

		switch(u8Item)
		{
			case '1':	
				usiStatusWrite1(0x00, 0x00);	// clear block protect , disable QE
				usiStatusRead(0x35, &u8data);
				sysprintf("SpiFlash Register-2 Status 0x%x \n", u8data);
				
				DBG_PRINTF("\tErase SpiFlash Block 0\n");
				spiFlashEraseBlock(0, 1);
				
				DBG_PRINTF("\tWrite SpiFlash\n");
				spiFlashWrite(0, TEST_SIZE, pSrc);

				memset(pDst, 0, TEST_SIZE);

				DBG_PRINTF("\tRead and Compare SpiFlash\n");
				spiFlashRead(0, TEST_SIZE, pDst);

				for (i=0; i<TEST_SIZE; i++)
				{
					if (*(pSrc+i) != *(pDst+i))
					{
						DBG_PRINTF("error!! Src[%d] = 0x%X, Dst[%d] = 0x%X\n", i, *(pSrc+i), i, *(pDst+i));
						break;
					}
				}
					
				break;

			case '2':	
				usiStatusWrite1(0x00, 0x02);	// clear block protect , set QE
				usiStatusRead(0x35, &u8data);
				sysprintf("SpiFlash Register-2 Status 0x%x \n", u8data);
				
				DBG_PRINTF("\tErase SpiFlash Block 0\n");
				spiFlashEraseBlock(0, 1);
				
				DBG_PRINTF("\tWrite SpiFlash\n");
				spiFlashQuadWrite(0, TEST_SIZE, pSrc);

				memset(pDst, 0, TEST_SIZE);

				DBG_PRINTF("\tRead and Compare SpiFlash\n");
				spiFlashFastReadQuad(0, TEST_SIZE, pDst);

				for (i=0; i<TEST_SIZE; i++)
				{
					if (*(pSrc+i) != *(pDst+i))
					{
						DBG_PRINTF("error!! Src[%d] = 0x%X, Dst[%d] = 0x%X\n", i, *(pSrc+i), i, *(pDst+i));
						break;
					}
				}
					
				break;

			case '3':	
				DBG_PRINTF("\tErase SpiFlash Block 0\n");
				spiFlashEraseBlock(0, 1);
				
				DBG_PRINTF("\tWrite SpiFlash\n");
				spiEONFlashQuadWrite(0, TEST_SIZE, pSrc);

				memset(pDst, 0, TEST_SIZE);

				DBG_PRINTF("\tRead and Compare SpiFlash\n");
				spiEONFlashFastReadQuad(0, TEST_SIZE, pDst);

				for (i=0; i<TEST_SIZE; i++)
				{
					if (*(pSrc+i) != *(pDst+i))
					{
						DBG_PRINTF("error!! Src[%d] = 0x%X, Dst[%d] = 0x%X\n", i, *(pSrc+i), i, *(pDst+i));
						break;
					}
				}
					
				break;

			case '9':	

				bLoop = FALSE;
				break;

			default:
					sysprintf("Write Item\n");
					break;
		}			
				
	}

	DBG_PRINTF("finish SPI test\n");
	return 0;
}

