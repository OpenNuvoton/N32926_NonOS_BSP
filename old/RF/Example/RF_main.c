/***************************************************************
 *                                                             *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved. *
 *                                                             *
 ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wblib.h"
#include "nvtfat.h"
#include "W55FA92_SIC.h"
#include "DrvRFCodec.h"

#define MAX_DATA_LENGTH		(1024*1024)

/*-----------------------------------------------------------------------------
 * Define Global Variables
 *---------------------------------------------------------------------------*/
CHAR g_PnctrMode[][4] = { "1/2", "2/3", "3/4", "5/6", "7/8" };
__align(4) UINT8 g_uPlainBuf[MAX_DATA_LENGTH];
__align(4) UINT8 g_uOutputBuf[MAX_DATA_LENGTH];
__align(4) UINT8 g_uCipherBuf[MAX_DATA_LENGTH*2];

void SystemInit(void)
{
	WB_UART_T uart;
	UINT32 u32ExtFreq, u32PllOutHz;

	// initial MMU but disable system cache feature
	//sysEnableCache(CACHE_DISABLE);
	// Cache on
	sysEnableCache(CACHE_WRITE_BACK);

	/* Init UART */
	u32ExtFreq = sysGetExternalClock();
	uart.uart_no = WB_UART_1;
	uart.uiFreq = u32ExtFreq;
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	sysInitializeUART(&uart);
	sysSetTimerReferenceClock(TIMER0, u32ExtFreq);
	sysStartTimer(TIMER0, 1000, PERIODIC_MODE);

	u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	DBG_PRINTF("UPLL out frequency %d Hz\n", u32PllOutHz);
	u32PllOutHz = sysGetPLLOutputHz(eSYS_MPLL, u32ExtFreq);
	DBG_PRINTF("MPLL out frequency %d Hz\n", u32PllOutHz);
}

/*-----------------------------------------------------------------------------
 * RF encryption/decryption test
 * INPUT:
 *	ePnctrMod: puncture mode of RF Codec
 * RETURN:
 *	OK: for test successful
 *	FAIL or error code: for test fail
 *---------------------------------------------------------------------------*/
INT32 RFCodec_test(E_RF_PNCTR_MODE ePnctrMod, UINT32 dataLen)
{
	INT32 result;
	UINT32 startTime, endTime, i;

	// create a random source table
	memset(g_uPlainBuf, 0x0, sizeof(g_uPlainBuf));
	memset(g_uCipherBuf, 0x0, sizeof(g_uCipherBuf));
	memset(g_uOutputBuf, 0x0, sizeof(g_uOutputBuf));
	for (i = 0; i < dataLen; i++) {
		g_uPlainBuf[i] = rand() & 0xFF;
	}

	RF_Set_Puncture(ePnctrMod);

	// 1. do RF encryption
	sysprintf("\nRF encryption input length = %d\n", dataLen);
//	startTime = clock();
	startTime = sysGetTicks(TIMER0);
	result = RF_Encrypt(g_uPlainBuf, g_uCipherBuf, dataLen);
//	endTime = clock();
	endTime = sysGetTicks(TIMER0);
	sysprintf("RF encryption takes %d ms\n", endTime - startTime);
	if (result < 0) {
		sysprintf("ERROR: RF_Encrypt() error with return code %d !!\n", result);
		return result;
	}
	sysprintf("RF encryption output length = %d\n", result);

	// 2. do RF decryption
	sysprintf("RF decryption input length = %d\n", result);
//	startTime = clock();
	startTime = sysGetTicks(TIMER0);
	result = RF_Decrypt(g_uCipherBuf, g_uOutputBuf, dataLen);
//	endTime = clock();
	endTime = sysGetTicks(TIMER0);
	sysprintf("RF decryption takes %d ms\n", endTime - startTime);
	if (result < 0) {
		sysprintf("ERROR: RF_Descrypt() error with return code %d !!\n", result);
		return result;
	}
	sysprintf("RF decryption output length = %d\n", result);

	// 3. compare output data with original plain buffer
	if (memcmp(g_uPlainBuf, g_uOutputBuf, dataLen) != 0) {
		sysprintf("ERROR: RF Decode test is fail !!\n");
		// show error location
		sysprintf("Input address = 0x%08X\n", (UINT32)g_uPlainBuf);
		sysprintf("Output address = 0x%08X\n", (UINT32)g_uOutputBuf);
		for (i = 0; i < dataLen; i++) {
			if (g_uPlainBuf[i] != g_uOutputBuf[i])
				sysprintf("offset = 0x%X, value 0x%X -> 0x%X\n", i, g_uPlainBuf[i], g_uOutputBuf[i]);
		}
		return -1;
	}

	return Successful;
}

INT32 main(void)
{
	INT32 result, i;
	UINT32 dataLen;

	SystemInit();

	sysprintf("\nBegin RF Codec demo...\n");

	// initial RF Codec
	RF_Open();
	RF_Enable_Int();

	srand(time(NULL));
	dataLen = (rand() % MAX_DATA_LENGTH) + 1;
	sysprintf("Total data length is %d\n", dataLen);

	// RF Codec test for different patterns
	sysprintf("RF Codec test\n");
	for (i = E_PNCTR_1_2; i <= E_PNCTR_7_8; i++) {
		result = RFCodec_test((E_RF_PNCTR_MODE)i, dataLen);
		sysprintf("Puncture %s mode test is %s\n", g_PnctrMode[i], (result) ? "Fail" : "Success");
	}

	RF_Disable_Int();
	RF_Close();

	sysprintf("\nRF Codec demo is finished...\n");

	return 0;
}
