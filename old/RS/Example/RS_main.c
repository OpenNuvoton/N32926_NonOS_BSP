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
#include "DrvRSCodec.h"

#define MAX_DATA_LENGTH		(1024*1024)

/*-----------------------------------------------------------------------------
 * Define Global Variables
 *---------------------------------------------------------------------------*/
__align(4) UINT8 g_uPlainBuf[MAX_DATA_LENGTH];
// RS encrypt input length = i, output length = i / 188 * 204 [+ 204 * 11] when interleave in enabled and no padding bytes
__align(4) UINT8 g_uCipherBuf[((MAX_DATA_LENGTH+RS_ENCRYPT_BLOCK_SIZE-1)/RS_ENCRYPT_BLOCK_SIZE+11)*RS_DECRYPT_BLOCK_SIZE];
// RS decrypt output length must be 188 bytes alignment
__align(4) UINT8 g_uOutputBuf[((MAX_DATA_LENGTH+RS_ENCRYPT_BLOCK_SIZE-1)/RS_ENCRYPT_BLOCK_SIZE)*RS_ENCRYPT_BLOCK_SIZE];

void SystemInit(void)
{
	WB_UART_T uart;
	UINT32 u32ExtFreq, u32PllOutHz;;

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

	u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	DBG_PRINTF("UPLL out frequency %d Hz\n", u32PllOutHz);
	u32PllOutHz = sysGetPLLOutputHz(eSYS_MPLL, u32ExtFreq);
	DBG_PRINTF("MPLL out frequency %d Hz\n", u32PllOutHz);
}

/*-----------------------------------------------------------------------------
 * RS encryption/decryption test
 * RETURN:
 *	OK: for test successful
 *	FAIL or error code: for test fail
 *---------------------------------------------------------------------------*/
INT32 RSCodec_test()
{
	INT32 result;
	UINT32 i, j, startTime, endTime, encryptLen;
	UINT32 errorBytes[16], dataLen, num, val;
	UINT8 isInterleave, wrongBytes;
	UINT8 *ptr_start;

	// create a random source table
	srand(time(NULL));
	dataLen = (rand() % MAX_DATA_LENGTH) + 1;
	sysprintf("Total data length is %d\n", dataLen);
	memset(g_uPlainBuf, 0x0, sizeof(g_uPlainBuf));
	memset(g_uOutputBuf, 0x0, sizeof(g_uOutputBuf));
	memset(g_uCipherBuf, 0x0, sizeof(g_uCipherBuf));
	for (i = 0; i < dataLen; i++) {
		g_uPlainBuf[i] = rand() & 0xFF;
	}

	isInterleave = 1;
	wrongBytes = 8;

	// 1. do RS encryption
	sysprintf("RS encryption input length = %d\n", dataLen);
	startTime = clock();
	result = RS_Encrypt(g_uPlainBuf, g_uCipherBuf, dataLen, isInterleave);
	endTime = clock();
	sysprintf("RS encryption takes %d ms\n", endTime - startTime);
	if (result < 0) {
		sysprintf("ERROR: RS_Encrypt() error with return code %d !!\n", result);
		return result;
	}
	sysprintf("RS encryption output length = %d\n", result);
	encryptLen = result;

	// 2. modify data in every block, for testing RS error correction
	if (wrongBytes > 0) {
		ptr_start = g_uCipherBuf;
		if (isInterleave) {
			while ((ptr_start+RS_DECRYPT_BLOCK_SIZE) <= (g_uCipherBuf+encryptLen)) {
				// still not reach end of data
				// select the offset in this block
				while (1) {
					num = rand() & 0xFF;
					if (num < (RS_DECRYPT_BLOCK_SIZE - (wrongBytes * 12)))
						break;
				}
				for (i = 0; i < wrongBytes * 12; i++) {
					// modify wrongBytes * 12 bytes data
					val = rand() & 0xFF;
					// make sure the value is changed
					while (val == (UINT8)(*(ptr_start+num+i)))
						val = rand() & 0xFF;
					*(ptr_start+num+i) = (UINT8)val;
				}
				ptr_start += RS_DECRYPT_BLOCK_SIZE * 12;
			}
		} else {
			while ((ptr_start+RS_DECRYPT_BLOCK_SIZE) <= (g_uCipherBuf+encryptLen)) {
				// still not reach end of data
				for (i = 0; i < wrongBytes; ) {
					num = rand() & 0xFF;
					// select the offset in this block
					if (num < RS_DECRYPT_BLOCK_SIZE) {
						for (j = 0; j < i; j++) {
							if (errorBytes[j] == num) {
								break;
							}
						}
						if (j == i) {
							// modify one byte data
							errorBytes[j] = num;
							val = rand() & 0xFF;
							// make sure the value is changed
							while (val == (UINT8)(*(ptr_start+num)))
								val = rand() & 0xFF;
							*(ptr_start+num) = (UINT8)val;
							i++;
						}
					}
				}
				ptr_start += RS_DECRYPT_BLOCK_SIZE;
			}
		}
	}

	// 3. do RS decryption
	sysprintf("RS decryption input length = %d\n", encryptLen);
	startTime = clock();
	result = RS_Decrypt(g_uCipherBuf, g_uOutputBuf, encryptLen, isInterleave);
	endTime = clock();
	sysprintf("RS decryption takes %d ms\n", endTime - startTime);
	if (result < 0) {
		sysprintf("ERROR: RS_Decrypt() error with return code %d !!\n", result);
		return result;
	}
	sysprintf("RS decryption output length = %d\n", result);

	// 4. compare output data with original plain buffer
	if (wrongBytes <= 8) {
		if (memcmp(g_uPlainBuf, g_uOutputBuf, dataLen) != 0) {
			sysprintf("ERROR: RS Codec test is fail !!\n");
			// show error location
			sysprintf("Input address = 0x%08X\n", (UINT32)g_uPlainBuf);
			sysprintf("Output address = 0x%08X\n", (UINT32)g_uOutputBuf);
			for (i = 0; i < dataLen; i++) {
				if (g_uPlainBuf[i] != g_uOutputBuf[i])
					sysprintf("offset = 0x%X, value 0x%X -> 0x%X\n",i , g_uPlainBuf[i], g_uOutputBuf[i]);
			}
			return -1;
		}
	} else {
		sysprintf("Wrong bytes is %d, but RS Codec can only correct 8 bytes error in every block!\n", wrongBytes);
	}

	return Successful;
}


INT32 main(void)
{
	INT32 result;

	SystemInit();

	sysprintf("\nBegin RS Codec demo...\n");

	// initial RS Codec
	RS_Open();
	RS_Enable_Int();

	sysprintf("RS Codec test with interleave\n");
	result = RSCodec_test();
	sysprintf("RS Codec test is %s\n", (result) ? "Fail" : "Success");

	RS_Disable_Int();
	RS_Close();

	sysprintf("RS Codec demo is finished...\n");

	return 0;
}
