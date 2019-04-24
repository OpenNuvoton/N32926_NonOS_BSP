/*************************************************************************
 * Nuvoton Electronics Corporation confidential
 *
 * Copyright (c) 2008 by Nuvoton Electronics Corporation
 * All rights reserved
 *
 * FILENAME
 *     main.c
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     NUC900 USB Host integration test program
 *
 * HISTORY
 *     2008.06.24       Created
 *
 * REMARK
 *     None
 **************************************************************************/
#ifdef ECOS
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "drv_api.h"
#include "diag.h"
#include "wbtypes.h"
#include "wbio.h"
#else
#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include "wbtypes.h"
#include "wblib.h"
#endif

#include "nvtfat.h"
#include "w55fa92_reg.h"
/* USB */
#include "usb.h"
#include "usbvideo.h"
#include "nvtfat.h"
#include "umas.h"

INT  USBKeyboardInit(void);

#define DUMMY_BUFFER_SIZE		(64 * 1024)

#ifdef ECOS
#define sysGetTicks(TIMER0)   cyg_current_time()
#endif

UINT8	_JpegImage[256 * 1024];

extern UINT32	_QueuedSize;

extern INT  W99683_HasImageQueued(void);


void  GetJpegImage(UINT8 *image, UINT32 *len, INT interval)
{
	UINT8   *bufPTR;
	INT     bufLEN;
	UINT32	idx = 0;
	
	*len = 0;
	/* Skip frames */
	while (1)
	{
		if (W99683_GetFramePiece(&bufPTR, &bufLEN) < 0) 
			;           /* W99683 is not enabled, we wait */

		if ((bufPTR[0] == 0xFF) && (bufPTR[1] == 0xD8))
		{
			if (interval == 0)
				break;
			interval--;
		}
	}
	
	while (1)
	{	
		memcpy(&image[idx], bufPTR, bufLEN);
		idx += bufLEN;
		*len += bufLEN;
		
		if (W99683_GetFramePiece(&bufPTR, &bufLEN) < 0) 
			continue;		/* W99683 is not enabled, we wait */
		
		if ((bufPTR[0] == 0xFF) && (bufPTR[1] == 0xD8))
			return;
	}
}


static INT  Action_Compare(CHAR *suFileName1, CHAR *szAsciiName1, 
							CHAR *suFileName2, CHAR *szAsciiName2)
{
	INT		hFile1, hFile2;
	INT		nLen1, nLen2, nCount, nStatus1, nStatus2;
	UINT8	buffer1[8192], buffer2[8192];
	UINT32	nJpegLen;
	
	hFile1 = fsOpenFile(suFileName1, szAsciiName1, O_RDONLY);
	if (hFile1 < 0)
		return hFile1;
	
	hFile2 = fsOpenFile(suFileName2, szAsciiName2, O_RDONLY);
	if (hFile2 < 0)
		return hFile2;
	
	sysprintf("\nComparing file ...\n");
	nCount = 0;
	while (1)
	{
		nStatus1 = fsReadFile(hFile1, buffer1, 1024, &nLen1);
		nStatus2 = fsReadFile(hFile2, buffer2, 1024, &nLen2);

		GetJpegImage(_JpegImage, &nJpegLen, 0);
		
		if ((nStatus1 == ERR_FILE_EOF) && (nStatus2 == ERR_FILE_EOF))
		{
			sysprintf("\ncompare ok!\n");
			fsCloseFile(hFile1);
			fsCloseFile(hFile2);
			return 0;
		}
		
		if (nLen1 != nLen2)
			break;
			
		if (memcmp(buffer1, buffer2, 1024))
			break;
		
		nCount++;
		//if ((nCount % 1024) == 0)
		//	sysprintf("%d KB    \r", nCount);
	}
	
	sysprintf("\nFile Compare failed at offset %x\n", nCount * 1024);
	
	fsCloseFile(hFile1);
	fsCloseFile(hFile2);
	return -1;
}



INT  copy_file(CHAR *suSrcName, CHAR *szSrcAsciiName, 
					CHAR *suDstName, CHAR *szDstAsciiName)
{
	INT			hFileSrc, hFileDst, nByteCnt, nStatus;
	UINT8		*pucBuff, *szFNameSrc, *szFNameDst;
	UINT32		nJpegLen;

	pucBuff = (UINT8 *)malloc(4096 + MAX_FILE_NAME_LEN + 512);
	if (pucBuff == NULL)
		return ERR_NO_FREE_MEMORY;
		
	szFNameSrc = pucBuff + 4096;
	szFNameDst = szFNameSrc + MAX_FILE_NAME_LEN/2;

	hFileSrc = fsOpenFile(suSrcName, szSrcAsciiName, O_RDONLY);
	if (hFileSrc < 0)
	{
		free(pucBuff);
		return hFileSrc;
	}

	hFileDst = fsOpenFile(suDstName, szDstAsciiName, O_RDONLY);
	if (hFileDst > 0)
	{
		fsCloseFile(hFileSrc);
		fsCloseFile(hFileDst);
		free(pucBuff);
		return ERR_FILE_EXIST;
	}

	hFileDst = fsOpenFile(suDstName, szDstAsciiName, O_CREATE);
	if (hFileDst < 0)
	{
		fsCloseFile(hFileSrc);
		free(pucBuff);
		return hFileDst;
	}

   	while (1)
   	{
   		nStatus = fsReadFile(hFileSrc, pucBuff, 4096, &nByteCnt);
   		if (nStatus < 0)
   			break;

		nStatus = fsWriteFile(hFileDst, pucBuff, nByteCnt, &nByteCnt);
   		if (nStatus < 0)
   			break;

		GetJpegImage(_JpegImage, &nJpegLen, 0);
	}
	fsCloseFile(hFileSrc);
	fsCloseFile(hFileDst);
	free(pucBuff);

	if (nStatus == ERR_FILE_EOF)
		nStatus = 0;
	return nStatus;
}



INT Test()
{
	INT     nStatus;
	//CHAR	szSrcA[24] = "C:\\tape001.mpg";
	CHAR	szSrcA[24] = "C:\\1.mp4";
	CHAR	szDstA[24] = "C:\\copya";
	CHAR	suFileName1[64], suFileName2[64];
	UINT32	nJpegLen;

	while (1)
	{
		sysprintf("Delete file: %s\n", szDstA);
		fsAsciiToUnicode(szDstA, suFileName1, TRUE);
		nStatus = fsDeleteFile(suFileName1, NULL);
		if (nStatus < 0)
			sysprintf("Failed, status = %x\n", nStatus);

		while (_QueuedSize > 16*1024)
		{
			GetJpegImage(_JpegImage, &nJpegLen, 0);
			sysprintf(".");
		}

		sysprintf("Copy file: %s\n", szSrcA);
		fsAsciiToUnicode(szSrcA, suFileName1, TRUE);
		fsAsciiToUnicode(szDstA, suFileName2, TRUE);
		nStatus = copy_file(suFileName1, NULL, suFileName2, NULL);
		if (nStatus < 0)
		{
			sysprintf("Failed, status = %x\n", nStatus);
			exit(0);
		}

		sysprintf("Compare file: %s and %s\n", szSrcA, szDstA);
		fsAsciiToUnicode(szSrcA, suFileName1, TRUE);
		fsAsciiToUnicode(szDstA, suFileName2, TRUE);
		
		if (Action_Compare(suFileName1, NULL, suFileName2, NULL) < 0)
			break;
	}	
	return 0;
}


void  Isochronous_Test()
{
	CHAR    szFileName[32] = {'C',0,':',0,'\\',0,'1',0,0,0 };
	CHAR    suFileName[128];
	INT     nIdx = 0;
	UINT32	nJpegLen;
	INT    	hFile;
	INT     nWriteBytes;

	W99683Cam_Init();

	while (!W99683Cam_IsConnected())
		Hub_CheckIrqEvent();

	if (W99683Cam_Open() < 0)
	{
		sysprintf("Failed to open W99683 device!\n");
		return;           /* _W99683_Camera is freed also */
	}

	while (!W99683Cam_IsStreaming())
		;   

	/* Drop 5 pictures */
	for (nIdx = 0; nIdx < 10; nIdx++)
	//for (nIdx = 0; ; nIdx++)
	{
		sysprintf("%d GetJpegImage...\n", nIdx);
		GetJpegImage(_JpegImage, &nJpegLen, 0);
		sysprintf("ImageSize: %d, _QueuedSize: %d\n", nJpegLen, _QueuedSize);
	}
	
	//Test();
	
	//Action_WriteSpeedTest(szFileName, NULL);
	
	for (nIdx = 0; nIdx < 30; nIdx++)
	{
reget:		
		GetJpegImage(_JpegImage, &nJpegLen, 0);
		if (_QueuedSize > 200000)
		{
			goto reget;
		}
		sysprintf("ImageSize: %d, _QueuedSize: %d\n", nJpegLen, _QueuedSize);
		
		/* Open a new file for writing */
		sprintf(szFileName, "C:\\%04d.jpg", nIdx);
		fsAsciiToUnicode(szFileName, suFileName, 1);
		hFile = fsOpenFile(suFileName, NULL, O_CREATE);
		if (hFile > 0)
			sysprintf("Opene file:[%s], file handle:%d\n", szFileName, hFile);
		else
		{
			sysprintf("Failed to open file: %s (%x)\n", szFileName, hFile);
			continue;
		}
		if ((fsWriteFile(hFile, _JpegImage, nJpegLen, &nWriteBytes) < 0) ||
			(nWriteBytes != nJpegLen))
			sysprintf("File write error! %d %d\n", nWriteBytes);
		else
			sysprintf("%d bytes\n", nWriteBytes);
		fsCloseFile(hFile);
	}
	
	while (1)
		GetJpegImage(_JpegImage, &nJpegLen, 0);
}
UINT8	_JpegImage[256 * 1024];
UINT8	_JpegImageR[256 * 1024];

void PenDriverAccess(UINT32 u32Count)
{
	CHAR szFileName[32] = {'C',0,':',0,'\\',0,'1',0,0,0 };
	CHAR szAsciiStr[32]={0};
	CHAR suFileName[128];
	INT     nIdx = 0, nIdy=0;
	UINT32	nJpegLen=256*1024;
	INT    	hFile;
	INT     nWriteBytes;
	
	sprintf(szAsciiStr, "C:\\Test");
	fsAsciiToUnicode(szAsciiStr, suFileName, TRUE);
	fsMakeDirectory(suFileName, NULL);
	//for (nIdx = 0; nIdx < 60; nIdx++)
	for (nIdx = 0; nIdx < u32Count; nIdx++)
	{	
		for (nIdy = 0; nIdy < (256*1024); nIdy=nIdy+1)
			_JpegImage[nIdy] = (nIdy+(nIdy>>8)+(nIdy>>16))+nIdx;
		
		sysprintf("ImageSize: %d\n", nJpegLen);		
		/* Open a new file for writing */
		sprintf(szFileName, "C:\\Test\\%07d.jpg", nIdx);
		fsAsciiToUnicode(szFileName, suFileName, 1);
		hFile = fsOpenFile(suFileName, NULL, O_CREATE);
		if (hFile > 0)
			sysprintf("Opene file:[%s], file handle:%d\n", szFileName, hFile);
		else
		{
			sysprintf("Failed to open file: %s (%x)\n", szFileName, hFile);
			continue;
		}
		if ((fsWriteFile(hFile, _JpegImage, nJpegLen, &nWriteBytes) < 0) ||
			(nWriteBytes != nJpegLen))
			sysprintf("File write error! %d %d\n", nWriteBytes);
		else
			sysprintf("%d bytes\n", nWriteBytes);
		fsCloseFile(hFile);
		hFile = fsOpenFile(suFileName, NULL, O_RDONLY );
		if (hFile > 0)
			sysprintf("Opene file:[%s], file handle:%d\n", szFileName, hFile);
		else
		{
			sysprintf("Opene file Error\n");
			while(1);
		}
		if ((fsReadFile(hFile, _JpegImageR, nJpegLen, &nWriteBytes) < 0) ||
			(nWriteBytes != nJpegLen))
			sysprintf("File read error! %d %d\n", nWriteBytes);
		fsCloseFile(hFile);		
			
		if(memcmp(_JpegImage, _JpegImageR, 256*1024)  != 0 )
		{
			sysprintf("Compare file error\n");
			while(1);
		}	
					
	}	
	sysprintf("Done\n");
}	

INT main()
{
	INT			t0;
	INT volatile i;
	UINT32		uBlockSize, uFreeSize, uDiskSize;
#ifndef ECOS	
    WB_UART_T 	uart;
#endif

	UINT32 u32ExtFreq;
	/* CACHE_ON	*/
//	sysInvalidCache();
//	sysEnableCache(CACHE_WRITE_BACK);

#ifndef ECOS	
	u32ExtFreq = sysGetExternalClock();    	/* Hz unit */	
	uart.uiFreq = u32ExtFreq;
	uart.uart_no= WB_UART_1;
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    sysInitializeUART(&uart);
    sysSetTimerReferenceClock (TIMER0, u32ExtFreq);
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);	
	
#endif

	fsInitFileSystem();
	sysprintf("Init USB...\n");

	InitUsbSystem();       

	UMAS_InitUmasDriver();

	USBKeyboardInit();

	/* wait hard disk ready */
	t0 = sysGetTicks(TIMER0);
	while (sysGetTicks(TIMER0) - t0 < 300)
		;
	
	if (fsDiskFreeSpace('C', &uBlockSize, &uFreeSize, &uDiskSize) == 0)
		sysprintf("Disk size = %d KB, Free speace = %d KB\n", uDiskSize, uFreeSize);
	else
		sysprintf("fsDiskFreeSpace failed!!\n");


	for(i=0;i<1000000;i++)
		;

	PenDriverAccess(60);		

	UMAS_RemoveUmasDriver();		
	Isochronous_Test();
}



#ifndef ECOS	
/*
 * standalone.c - minimal bootstrap for C library
 * Copyright (C) 2000 ARM Limited.
 * All rights reserved.
 */

/*
 * RCS $Revision: 1 $
 * Checkin $Date: 08/08/22 9:59p $ 0
 * Revising $Author: Mncheng $
 */

/*
 * This code defines a run-time environment for the C library.
 * Without this, the C startup code will attempt to use semi-hosting
 * calls to get environment information.
 */
 extern unsigned int Image$$ZI$$Limit;

void _sys_exit(int return_code)
{
label:  goto label; /* endless loop */
}

void _ttywrch(int ch)
{
    char tempch = (char)ch;
    (void)tempch;
}


__value_in_regs struct R0_R3 {unsigned heap_base, stack_base, heap_limit, stack_limit;} 
    __user_initial_stackheap(unsigned int R0, unsigned int SP, unsigned int R2, unsigned int SL)
{
    struct R0_R3 config;

    config.heap_base = (unsigned int)&Image$$ZI$$Limit;
    config.stack_base = 0x1000000; //Stack base;

/*
To place heap_base directly above the ZI area, use:
    extern unsigned int Image$$ZI$$Limit;
    config.heap_base = (unsigned int)&Image$$ZI$$Limit;
(or &Image$$region_name$$ZI$$Limit for scatterloaded images)

To specify the limits for the heap & stack, use e.g:
    config.heap_limit = SL;
    config.stack_limit = SL;
*/

    return config;
}

/* end of file standalone.c */

/* end of file standalone.c */
#endif

