/**************************************************************************//**
 * @file     main.c
 * @brief    Demonstrate AES functions
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "wblib.h"
#include "AES.h"

#define LOG_PREFIX		"[AES Demo]"

// 16-byte aligned data length
#define DATA_LENGTH		0x40000
// KEY_128, KEY_192, KEY_256
#define KEY_LENGTH		KEY_128

#if defined (__GNUC__)
static UINT8 plain_text [DATA_LENGTH] __attribute__((aligned (32)));    // Original plain text.
static UINT8 plain_text2[DATA_LENGTH] __attribute__((aligned (32)));    // Plain text for H/W AES decryption.
static UINT8 cipher_text[DATA_LENGTH] __attribute__((aligned (32)));    // Cipher text for H/W AES encryption.
static UINT8 iv[16] __attribute__((aligned (32)));
static UINT8 cipher_key[32] __attribute__((aligned (32)));
#else
static __align(32) UINT8 plain_text [DATA_LENGTH];      // Original plain text.
static __align(32) UINT8 plain_text2[DATA_LENGTH];      // Plain text for H/W AES decryption.
static __align(32) UINT8 cipher_text[DATA_LENGTH];      // Cipher text for H/W AES encryption.
static __align(32) UINT8 iv[16];
static __align(32) UINT8 cipher_key[32];
#endif
static UINT8 *ptr_plain_text, *ptr_plain_text2, *ptr_cipher_text;
static int errcode = Successful;

int main()
{
    // Cache on.
	if (! sysGetCacheState()) {
		sysInvalidCache();
		sysEnableCache(CACHE_WRITE_THROUGH);
		sysFlushCache(I_D_CACHE);
	}

	{	// Initialize UART.
		UINT32 u32ExtFreq;
		WB_UART_T uart;

		u32ExtFreq = sysGetExternalClock();
		uart.uiFreq = u32ExtFreq;   //use APB clock
		uart.uiBaudrate = 115200;
		uart.uiDataBits = WB_DATA_BITS_8;
		uart.uiStopBits = WB_STOP_BITS_1;
		uart.uiParity = WB_PARITY_NONE;
		uart.uiRxTriggerLevel = LEVEL_1_BYTE;
		uart.uart_no = WB_UART_1;
		sysInitializeUART(&uart);
	}

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
    sysSetCPUClock(240000000/2);
#endif

    // Message gets messed up with above.
    sysprintf("\n\n");

	sysSetLocalInterrupt (ENABLE_IRQ);	// Enable CPSR I bit
	
	do {
		int i;
	
		sysprintf(LOG_PREFIX "Start (data length=%d bytes, key length=%d bits) ...\n", DATA_LENGTH, KEY_LENGTH == KEY_128 ? "128" : KEY_LENGTH == KEY_192 ? "192" : "256");
		
		// Init clock and enable interrupt for AES.
		AES_Initial();
	
		if (! sysGetCacheState()) {	// Cache disabled.
			ptr_plain_text  = plain_text;
			ptr_plain_text2 = plain_text2;
			ptr_cipher_text = cipher_text;
		}
		else {   // Cache enabled.
			ptr_plain_text  = (UINT8 *) ((UINT32) plain_text  | 0x80000000);
			ptr_plain_text2 = (UINT8 *) ((UINT32) plain_text2 | 0x80000000);
			ptr_cipher_text = (UINT8 *) ((UINT32) cipher_text | 0x80000000);
		}

		// Generate IV at random.
		for (i = 0; i < sizeof (iv); i++) {
			iv[i] = rand() & 0xFF;
		}
	
		// Generate cipher key at random.
		for (i = 0; i < 32; i++) {
			cipher_key[i] = rand() & 0xFF;
		}
		
		// Generate plain data at random.
		for (i=0; i<DATA_LENGTH; i++) {
			ptr_plain_text[i] = rand() & 0xFF;
		}
	
		// Encrypt.
		errcode = AES_Encrypt_Async(ptr_plain_text, ptr_cipher_text, DATA_LENGTH, iv, cipher_key, KEY_LENGTH);
		switch(errcode) {
		case Successful:
			break;
			
		case AES_ERR_RUNNING:
			errcode = AES_Flush();	// Wait until complete.
			break;
		
		default:
			sysprintf(LOG_PREFIX "AES encrypt error: 0x%08x\n", errcode);
		}
		if (errcode != Successful) {
			break;
		}
		
		// Decrypt.
		errcode = AES_Decrypt_Async(ptr_cipher_text, ptr_plain_text2, DATA_LENGTH, iv, cipher_key, KEY_LENGTH);
		switch(errcode) {
			case Successful:
			break;
			
		case AES_ERR_RUNNING:
			errcode = AES_Flush();	// Wait until complete.
			break;
			
		default:
			sysprintf(LOG_PREFIX "AES decrypt error: 0x%08x\n", errcode);
		}
		if (errcode != Successful) {
			break;
		}
		
		
		// Compare original plain text with AES decrypted text.
		if (memcmp(ptr_plain_text, ptr_plain_text2, DATA_LENGTH) != 0) {
			sysprintf(LOG_PREFIX "Compare original plain text with AES decrypted text error\n");
			break;
		}
		
		sysprintf(LOG_PREFIX "OK\n");
	}
	while (0);
	
	AES_Final();
	
	return 0;
}
