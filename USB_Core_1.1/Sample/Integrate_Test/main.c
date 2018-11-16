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
 *     NUC930 USB Host integration test program
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
#include "W55FA92_reg.h"
#include "W55FA92_gpio.h"
/* USB */
#include "usb.h"
#include "usbvideo.h"

#ifdef ECOS
#define sysGetTicks(TIMER0)   cyg_current_time()
#endif

VOID USBKBM_Exit(void);
int  USBH_ClearGlobalPortPower(void);
int USBH_SetGlobalPortPower(void);
void usbSetFeature(void);
INT  USBKeyboardInit(void);
VOID  UMAS_RemoveUmasDriver(void);


UINT8	_JpegImage[256 * 1024];
UINT8	_JpegImageR[256 * 1024];

extern UINT32	_QueuedSize;

extern INT  W99683_HasImageQueued(void);


void  GetJpegImage(UINT8 *image, INT32 *len, INT interval)
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
	INT32	nJpegLen;
	
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
	INT32		nJpegLen;

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
	INT32	nJpegLen;

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
/*=====================================================================================
Test Condition:
	Plug in a pen driver to USBH port 1 or port 2. 



=====================================================================================*/
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
extern int _W99683_Select_Packet_Size;		
int Isochronous_TestPacketSize(int packet_size)
{
	int  nIdx, nJpegLen;
	
	sysprintf("<<< Isochronous-in by W99683 >>> test ...\n");
	sysprintf("Test with packet size %d\n", packet_size);

 	_W99683_Select_Packet_Size = packet_size;
	InitUsbSystem();       	
	//UsbCoreH_Open();

	W99683Cam_Init();

	while (!W99683Cam_IsConnected())
		Hub_CheckIrqEvent();

	if (W99683Cam_Open() < 0)
	{
		sysprintf("Failed to open W99683 device!\n");
		return -1;           /* _W99683_Camera is freed also */
	}

	while (!W99683Cam_IsStreaming())
		;   

	for (nIdx = 0; nIdx < 10; nIdx++)
	{
		sysprintf("[%d] GetJpegImage...\n", nIdx);
		GetJpegImage(_JpegImage, &nJpegLen, 0);
		sysprintf("Image: %d, QSize: %d\n", nJpegLen, _QueuedSize);
	}
	
	sysprintf("Test passed.\n");

	return 0;
}		

void  Isochronous_Test()
{
	CHAR 	szFileName[32] = {'C',0,':',0,'\\',0,'1',0,0,0 };
	CHAR 	szAsciiStr[32]={0};
	CHAR 	suFileName[128];
	
	
	INT     nIdx = 0;
	INT32	nJpegLen;
	INT    	hFile;
	INT     nWriteBytes;

	W99683Cam_Init();

	while (!W99683Cam_IsConnected())
#ifdef ECOS
		Hub_CheckIrqEvent(0);
#else	
		Hub_CheckIrqEvent();
#endif

	if (W99683Cam_Open() < 0)
	{
		sysprintf("Failed to open W99683 device!\n");
		return;           /* _W99683_Camera is freed also */
	}

	while (!W99683Cam_IsStreaming())
		;   

	/* Drop 5 pictures */
	for (nIdx = 0; nIdx < 5; nIdx++)
	//for (nIdx = 0; ; nIdx++)
	{
		sysprintf("%d GetJpegImage...\n", nIdx);
		GetJpegImage(_JpegImage, &nJpegLen, 0);
		sysprintf("ImageSize: %d, _QueuedSize: %d\n", nJpegLen, _QueuedSize);
	}
	
	sprintf(szAsciiStr, "C:\\Test");
	fsAsciiToUnicode(szAsciiStr, suFileName, TRUE);
	fsMakeDirectory(suFileName, NULL);
	
	for (nIdx = 0; nIdx < 300; nIdx++)
	{
reget:		
		GetJpegImage(_JpegImage, &nJpegLen, 0);
		if (_QueuedSize > 200000)
		{
			goto reget;
		}
		sysprintf("ImageSize: %d, _QueuedSize: %d\n", nJpegLen, _QueuedSize);
		
		/* Open a new file for writing */
		sprintf(szFileName, "C:\\Test\\%04d.jpg", nIdx);
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
}
void Keyboard_Test(void)
{
	INT			t0;
	t0 = sysGetTicks(TIMER0);
	while (sysGetTicks(TIMER0) - t0 < 1000);							//Wait 10sec. 
}
/*
	Before power down set


*/
void RemoteWakup_Test(void)
{
	INT			t0;	
	sysprintf("Please plug in a HID device that support remote wakeup like mouse before test\n");
	while(1)
	{	
		t0 = sysGetTicks(TIMER0);
		while (sysGetTicks(TIMER0) - t0 < 300);							//Wait 3 sec. 	
		outp32(REG_HC_RH_STATUS, inp32(REG_HC_RH_STATUS) | BIT15 );	//Device remote wake up enable
		usbSetFeature();											//For device signal host
		outp32(REG_HC_RH_PORT_STATUS2, 0x4); 						//Port suspend
		sysprintf("Enter Power Down\n");
		sysPowerDown(WE_UHC);									//Wake up from USBH
		///usbClearFeature();											//Next power down
	}	
}
void PortSuspendResume_Test(void)
{
	INT			t0;	
	sysprintf("Please plug in a pen driver before test\n");
	sysprintf("Wake up system by GPIO A0 \n");
	while(1)
	{		
	
		PenDriverAccess(2);	
		t0 = sysGetTicks(TIMER0);
		while (sysGetTicks(TIMER0) - t0 < 300);	//Wait 3sec. 
		
		gpio_setportpull(GPIO_PORTA, 0x01, 0x01);		/*Set GPIOA-0 to pull high 		 */	
		gpio_setportdir(GPIO_PORTA, 0x01, 0x00);		/*Correct	Set GPIOA-0 as input port	 */			
		//gpio_setportdir(GPIO_PORTA, 0x01, 0x01);		/*Wrong Set GPIOA-0 as output port	 */
		gpio_setsrcgrp(GPIO_PORTA, 0x01, 0x00);		/*Group GPIOA-0 to EXT_GPIO0	 */
		gpio_setintmode(GPIO_PORTA, 0x01, 0x01, 0x01);	/*Rising/Falling				 	 */
		outp32(REG_IRQTGSRC0, 0xFFFFFFFF);
		outp32(REG_IRQLHSEL, 0x11);
		/* Set gpio wake up source */
		sysprintf("Enter power down, GPIO Int status 0x%x\n", inp32(REG_IRQTGSRC0));		
						
		outp32(REG_HC_RH_STATUS, inp32(REG_HC_RH_STATUS) | BIT15 );	//Device remote wake up enable
		outp32(REG_HC_RH_PORT_STATUS2, 0x4); 						//Port suspend
		sysprintf("Enter Power Down, Please toggle GPIOA0\n");
		sysPowerDown(WE_GPIO);	
		sysprintf("Exit Power Down\n");
		outp32(REG_HC_RH_PORT_STATUS2, inp32(REG_HC_RH_PORT_STATUS2) | BIT3);	//Write 1 for resume port
		 while( (inp32(REG_HC_RH_PORT_STATUS2)&BIT18)==0)
		 {		 	
		 	sysprintf("Port is still suspend\n");
		 }
		 sysprintf("Port is still resume complete\n");
		 
		t0 = sysGetTicks(TIMER0);
		while (sysGetTicks(TIMER0) - t0 < 300);	//Wait 3sec. 		 
		PenDriverAccess(4);										
	}	
}
INT IntegrateTest(void)
{
	INT		t0;
	UINT32	uBlockSize, uFreeSize, uDiskSize;
	UINT32 	u32Item;
	fsInitFileSystem();
	fsAssignDriveNumber('C', DISK_TYPE_USB_DEVICE, 0, 1);
		
	sysprintf("=============================================================================================\n");
	sysprintf("Please select the test item																	\n");
	sysprintf("[1] Pen driver access																		\n");
	sysprintf("[2] Isochronous test																			\n");
	sysprintf("[3] Keyboard test																			\n");	
	sysprintf("[4].Integration test [W99683+Keyboard+Disk]													\n");
	sysprintf("[5].Integration test [W99683+Disk under a hub that connect to port 1 and port 2 connect a mouse]\n");
	sysprintf("[6].Port suspend, system power down, remote wake-up by HID mouse							\n");					
	sysprintf("[7].Port suspend, system power down, wake-up by gpio then resume device						\n");	
	sysprintf("=============================================================================================\n");
	u32Item = sysGetChar();
	switch(u32Item)
	{
			case '1': 		
				/* Mass-storage */ 		
				InitUsbSystem();       					
				UMAS_InitUmasDriver();	
				t0 = sysGetTicks(TIMER0);
				while (sysGetTicks(TIMER0) - t0 < 300);	// wait hard disk ready 
												
				if (fsDiskFreeSpace('C', &uBlockSize, &uFreeSize, &uDiskSize) == 0)
					sysprintf("Disk size = %d KB, Free speace = %d KB\n", uDiskSize, uFreeSize);
				else
					sysprintf("fsDiskFreeSpace failed!!\n");
						
				PenDriverAccess(60);		
				UMAS_RemoveUmasDriver();			
				DeInitUsbSystem();								
				break;
    		case '2': 	
    				/* Mass-storage */ 						
    				InitUsbSystem();       	
				UMAS_InitUmasDriver();	
				t0 = sysGetTicks(TIMER0);
				while (sysGetTicks(TIMER0) - t0 < 300);	// wait hard disk ready 
												
				if (fsDiskFreeSpace('C', &uBlockSize, &uFreeSize, &uDiskSize) == 0)
					sysprintf("Disk size = %d KB, Free speace = %d KB\n", uDiskSize, uFreeSize);
				else
					sysprintf("fsDiskFreeSpace failed!!\n");
    				Isochronous_Test();	
    				W99683Cam_Exit();
    				UMAS_RemoveUmasDriver();			
				DeInitUsbSystem();	
    				break;
    		case '3': 	    
    				/* Key Board */
    				InitUsbSystem();       		
					USBKeyboardInit();				
    				Keyboard_Test();
    				USBKBM_Exit();	
    				DeInitUsbSystem();		
    				break;    
    		case '4':

    				/* Mass-storage */ 	
    				InitUsbSystem();  					
					UMAS_InitUmasDriver();	
					t0 = sysGetTicks(TIMER0);
					while (sysGetTicks(TIMER0) - t0 < 300);	// wait hard disk ready 
												
					if (fsDiskFreeSpace('C', &uBlockSize, &uFreeSize, &uDiskSize) == 0)
						sysprintf("Disk size = %d KB, Free speace = %d KB\n", uDiskSize, uFreeSize);
					else
						sysprintf("fsDiskFreeSpace failed!!\n");
    				/* Key board */
    				USBKeyboardInit();						
					Isochronous_Test();			
    				break;
    		case '5':			
    				/* Mass-storage */ 						
    				InitUsbSystem();       	
    				USBKeyboardInit();	
					UMAS_InitUmasDriver();	
					t0 = sysGetTicks(TIMER0);
					while (sysGetTicks(TIMER0) - t0 < 300);	// wait hard disk ready 
													
					if (fsDiskFreeSpace('C', &uBlockSize, &uFreeSize, &uDiskSize) == 0)
						sysprintf("Disk size = %d KB, Free speace = %d KB\n", uDiskSize, uFreeSize);
					else
						sysprintf("fsDiskFreeSpace failed!!\n");
    				Isochronous_Test();	
    				W99683Cam_Exit();
    				USBKBM_Exit();	
    				UMAS_RemoveUmasDriver();			
					DeInitUsbSystem();			
				
    				break;								   				
    		case '6':	
    				InitUsbSystem();   
    				USBKeyboardInit();	
    				RemoteWakup_Test();
    				break;	
    					
    		case '7':	
    			InitUsbSystem(); 
					UMAS_InitUmasDriver();	
					t0 = sysGetTicks(TIMER0);
					while (sysGetTicks(TIMER0) - t0 < 300);	// wait hard disk ready 
													
					if (fsDiskFreeSpace('C', &uBlockSize, &uFreeSize, &uDiskSize) == 0)
						sysprintf("Disk size = %d KB, Free speace = %d KB\n", uDiskSize, uFreeSize);
					else
						sysprintf("fsDiskFreeSpace failed!!\n");						    		
	    		
	    			PortSuspendResume_Test();
	    			break;						 	
    }		
    return 0;
}		
UINT32 u32TMP;		

INT main()
{
		
	WB_UART_T uart;
	UINT32 u32Item, u32ExtFreq;
	UINT32 u32UsbhPort1 = HOST_LIKE_PORT1_DISABLE;
	u32ExtFreq = sysGetExternalClock();
	sysUartPort(1);
	uart.uiFreq = u32ExtFreq;
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uart_no=WB_UART_1;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	sysInitializeUART(&uart);    	    	
	sysSetTimerReferenceClock (TIMER0, u32ExtFreq);
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);

	do
	{
		sysprintf("============================================================================================\n");
		sysprintf("Please select the USB host port 1 through GPIO											   \n");
		sysprintf("[1] GPA10 and GPA11	(Conflict with UART 1)							   			  		   \n");
		sysprintf("[2] Disable																			  	   \n");
		sysprintf("============================================================================================\n");
		u32Item = sysGetChar();
		switch(u32Item)
		{
			case '1': 	u32UsbhPort1 = HOST_LIKE_PORT1_0;
					break;
	   		case '2':   u32UsbhPort1 = HOST_LIKE_PORT1_DISABLE;	
	    				break;		
		}
	}while((u32Item> '3'));	
	
	do
	{
		sysprintf("============================================================================================\n");
		sysprintf("Please select the USB host port 2 through GPIO											   \n");
		sysprintf("[1] GPD3 and GPD4																		   \n");
		sysprintf("[2] GPA3 and GPA4																		   \n");
		sysprintf("[3] GPD14 and GPD15																	       \n");
		sysprintf("[5] Disable																				   \n");
		sysprintf("============================================================================================\n");
		u32Item = sysGetChar();
		switch(u32Item)
		{
			case '1': 	
						USB_PortInit(u32UsbhPort1, HOST_LIKE_PORT2_0);			
						goto start;
//						break;
	    		case '2': 		
	    				USB_PortInit(u32UsbhPort1, HOST_LIKE_PORT2_1);	
	    				goto start;
//	    				break;
	    		case '3': 
	    				USB_PortInit(u32UsbhPort1, HOST_LIKE_PORT2_2);	
	    				goto start;
//	    				break;		    		
	    		case '5':		
	    				USB_PortInit(u32UsbhPort1, HOST_LIKE_PORT2_DISABLE);	
	    				goto start;
//	    				break;	    				
		}
	}while((u32Item> '5'));	
		
start:	
	
	IntegrateTest(); 		
}

