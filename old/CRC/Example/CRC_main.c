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
#include "w55fa92_edma.h"
#include "DrvCRC.h"

#define MAX_DATA_LENGTH		(16*1024)

/*-----------------------------------------------------------------------------
 * Define Global Variables
 *---------------------------------------------------------------------------*/
CHAR g_CrcMode[][10] = { "CRC-CCITT", "CRC-8", "CRC-16", "CRC-32" };
UINT32 g_CrcMask[] = { 0xFFFF, 0xFF, 0xFFFF, 0xFFFFFFFF };
__align (4) UINT8 g_uSrcMemBuffer[MAX_DATA_LENGTH];

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

	u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	DBG_PRINTF("UPLL out frequency %d Hz\n", u32PllOutHz);
	u32PllOutHz = sysGetPLLOutputHz(eSYS_MPLL, u32ExtFreq);
	DBG_PRINTF("MPLL out frequency %d Hz\n", u32PllOutHz);
}

void CRC_Test()
{
	S_CRC_DESCRIPT_SETTING sCRCDescript;
	INT32 iCrcCh, i;
	UINT32 uDataLen, uVal;

	// create a random source table
	srand(time(NULL));
	uDataLen = (rand() % MAX_DATA_LENGTH) + 1;
	sysprintf("Total data length is %d\n", uDataLen);
	for (i = 0; i < uDataLen; i++)
		g_uSrcMemBuffer[i] = rand() & 0xFF;

	CRC_Init();
	// allocate a free CRC channel
	iCrcCh = CRC_FindandRequest();
	while (iCrcCh < 0) {
		iCrcCh = CRC_FindandRequest();
	}
	sysprintf("Allocate CRC ch%d\n", iCrcCh);

	// ePolyMode could be E_CRCCCITT / E_CRC8 / E_CRC16 / E_CRC32
	sCRCDescript.ePolyMode = E_CRCCCITT;
	// eWriteLength could be E_LENGTH_BYTE / E_LENGTH_HALF_WORD / E_LENGTH_WORD
	sCRCDescript.eWriteLength = E_LENGTH_WORD;
	// eChecksumCom could be E_1sCOM_OFF / E_1sCOM_ON
	sCRCDescript.eChecksumCom = E_1sCOM_OFF;
	// eWdataCom could be E_1sCOM_OFF / E_1sCOM_ON
	sCRCDescript.eWdataCom = E_1sCOM_OFF;
	// eChecksumRvs could be E_REVERSE_OFF / E_REVERSE_ON
	sCRCDescript.eChecksumRvs = E_REVERSE_OFF;
	// eWdataRvs could be E_REVERSE_OFF / E_REVERSE_ON
	sCRCDescript.eWdataRvs = E_REVERSE_OFF;
	// eTransferMode could be E_CRC_CPU_PIO / E_CRC_VDMA
	sCRCDescript.eTransferMode = E_CRC_VDMA;
	// uSeed could be any value
	sCRCDescript.uSeed = rand() & g_CrcMask[sCRCDescript.ePolyMode];

	sysprintf("PolyMode	= %s\n", g_CrcMode[sCRCDescript.ePolyMode]);
	sysprintf("WriteLength	= %d\n", 1 << sCRCDescript.eWriteLength);
	sysprintf("ChecksumCom	= %s\n", (sCRCDescript.eChecksumCom) ? "Enable" : "Disable");
	sysprintf("WdataCom	= %s\n", (sCRCDescript.eWdataCom) ? "Enable" : "Disable");
	sysprintf("ChecksumRvs	= %s\n", (sCRCDescript.eChecksumRvs) ? "Enable" : "Disable");
	sysprintf("WdataRvs	= %s\n", (sCRCDescript.eWdataRvs) ? "Enable" : "Disable");
	sysprintf("TransferMode	= %s\n", (sCRCDescript.eTransferMode) ? "VDMA" : "CPU");
	sysprintf("Seed		= 0x%x\n", sCRCDescript.uSeed);

	// run a CRC computation
	uVal = CRC_Run(iCrcCh, g_uSrcMemBuffer, uDataLen, &sCRCDescript);
	sysprintf("CRC return value is 0x%x\n", uVal);

	// free a allocated CRC channel
	CRC_Free(iCrcCh);
	CRC_Exit();
}

INT32 main(void)
{
	SystemInit();

	sysprintf("\nBegin CRC demo...\n");
	CRC_Test();
	sysprintf("CRC demo is finished...\n");

	return 0;
}
