//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "wblib.h"
#include "aes.h"

#define LOG_PREFIX		"[AES Demo]"

// 16-byte aligned data length
#define DATA_LENGTH		0x40000
// KEY_128, KEY_192, KEY_256
#define KEY_LENGTH		KEY_128

static __align(32) UINT8 plain_text [DATA_LENGTH];      // Original plain text.
static __align(32) UINT8 plain_text2[DATA_LENGTH];      // Plain text for H/W AES decryption.
static __align(32) UINT8 cipher_text[DATA_LENGTH];      // Cipher text for H/W AES encryption.
static __align(32) UINT8 iv[16];
static __align(32) UINT8 cipher_key[32];
static UINT8 *ptr_plain_text, *ptr_plain_text2, *ptr_cipher_text;
static int errcode = Successful;

int main()
{	
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
	
	// Cache on.
	if (! sysGetCacheState()) {
		sysInvalidCache();
		sysEnableCache(CACHE_WRITE_THROUGH);
		sysFlushCache(I_D_CACHE);
	}

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
