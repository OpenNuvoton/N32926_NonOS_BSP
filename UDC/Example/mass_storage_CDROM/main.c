#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "w55fa92_reg.h"
#include "usbd.h"
#include "mass_storage_class.h"
#include "ImageISO.h"

//#define DETECT_USBD_PLUG
//#define PLUG_DEVICE_TEST
//#define NON_BLOCK_MODE

#ifdef __TWO_NAND__
	UINT32 u32NAND_EXPORT = (MSC_NAND_CS0 | MSC_NAND_CS2);	
#else
	UINT32 u32NAND_EXPORT = MSC_NAND_CS0;	
#endif
#ifdef __TWO_SD__
	UINT32 u32SD_EXPORT = (MSC_SD_MP_PORT0 | MSC_SD_MP_PORT1);
#else
	UINT32 u32SD_EXPORT = MSC_SD_PORT0;	//Card Detect only work when  u32SD_EXPORT = MSC_SD_PORT0
#endif

#ifndef DETECT_USBD_PLUG
BOOL bPlugStauts = FALSE;
BOOL bHostPlugStauts = FALSE;
#endif

#ifdef PLUG_DEVICE_TEST
UINT32 u32TimerChannel = 0;		
BOOL bTimeOut = FALSE;
/*  */
void Timer0_Callback(void)
{
	bTimeOut = TRUE;
	sysClearTimerEvent(TIMER0, u32TimerChannel);
}
#endif

VOID CDROM_Read(PUINT32 pu32address, UINT32 u32Offset, UINT32 u32LengthInByte)
{

	*pu32address = (UINT32)CD_Tracks + u32Offset;

}

/* Plug detection for mscdMassEvent callback function (Retrun vale - TRUE:Run MSC;FALSE:Exit MSC) */
BOOL PlugDetection(void)
{
#ifdef DETECT_USBD_PLUG		/* Check plug status */
	return udcIsAttached();
#else	
	#ifdef PLUG_DEVICE_TEST	/* Check plug into Host or adaptor */
	if(udcIsAttached())
	{
		
		if(bPlugStauts != udcIsAttached())
		{
			bPlugStauts = TRUE;
			bHostPlugStauts = FALSE;
			sysprintf("<Plug>");				
		}	
		
		if(bHostPlugStauts != udcIsAttachedToHost())
		{
			bHostPlugStauts = udcIsAttachedToHost();
			if(bHostPlugStauts)
			{
				bTimeOut = TRUE;
				sysClearTimerEvent(TIMER0, u32TimerChannel);
				sysprintf("<Host>\n");				
			}
		}
		if(bTimeOut)
		{
			if(bHostPlugStauts)
				return TRUE;
			else
			{
				sysprintf("<Adaptor>\n");	
				return FALSE;
			}
		}
		return TRUE;
	}
	else				
		return FALSE;
	#else					/* Do not check plug status */
		return TRUE;
	#endif	
#endif
}

#ifndef __RAM_DISK_ONLY__
	#include "nvtfat.h"
	#include "w55fa92_sic.h"
	#include "w55fa92_gnand.h"

#ifdef __NAND__
NDISK_T MassNDisk;

NDRV_T _nandDiskDriver0 = 
{
	nandInit0,
	nandpread0,
	nandpwrite0,
	nand_is_page_dirty0,
	nand_is_valid_block0,
	nand_ioctl,
	nand_block_erase0,
	nand_chip_erase0,
	0
};
#endif

#endif
INT main(void)
{
	UINT32 u32CdromSize;
#ifdef __NAND__
	UINT32 block_size, free_size, disk_size, u32TotalSize;
	#ifdef __TWO_NAND__
		PDISK_T       *pDiskList, *ptPDiskPtr;
	#else
		UINT32 u32NANDsize1;
	#endif			
#endif	
#if !defined(__RAM_DISK_ONLY__)	&& !defined (__SPI_ONLY__)
	UINT32 u32SicRef;
	INT32 status0 = 0,status1 = 0,status2 = 0;	
#endif
#ifdef __SPI_ONLY__	
	INT32 status0 = 0;	
#endif
	UINT32 u32ExtFreq;
	WB_UART_T uart;

	sysDisableCache(); 	
	sysFlushCache(I_D_CACHE);		
		
	/* Enable USB */
	udcOpen();				
	
	sysUartPort(1);
	u32ExtFreq = sysGetExternalClock();    	/* Hz unit */		
	uart.uiFreq = u32ExtFreq;
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;    
	uart.uart_no=WB_UART_1;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    sysInitializeUART(&uart);
	    
	sysEnableCache(CACHE_WRITE_BACK);  

    sysSetTimerReferenceClock (TIMER0, u32ExtFreq);
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);
			
	sysprintf("<MSC>\n");	 		
#if !defined(__RAM_DISK_ONLY__)	&& !defined (__SPI_ONLY__)


	u32SicRef = sysGetHCLK1Clock();	
		
	sicIoctl(SIC_SET_CLOCK, u32SicRef / 1000, 0, 0);

    sicOpen();
	/* initial nuvoton file system */
	fsInitFileSystem();	
		
	#ifdef __NAND__
		sysprintf("<NAND>\n");		

		fsAssignDriveNumber('C', DISK_TYPE_SMART_MEDIA, 0, 1);     // NAND 0, 2 partitions
		fsAssignDriveNumber('D', DISK_TYPE_SMART_MEDIA, 0, 2);     // NAND 0, 2 partitions

		if(GNAND_InitNAND(&_nandDiskDriver0, &MassNDisk, TRUE) < 0) 
		{
			sysprintf("GNAND_InitNAND error\n");
			return 0;		
		}	
		
		if(GNAND_MountNandDisk(&MassNDisk) < 0) 
		{
			sysprintf("GNAND_MountNandDisk error\n");
			return 0;	
		}

		fsSetVolumeLabel('C', "NAND1-1\n", strlen("NAND1-1"));
		fsSetVolumeLabel('D', "NAND1-2\n", strlen("NAND1-2"));

		u32TotalSize = MassNDisk.nZone* MassNDisk.nLBPerZone*MassNDisk.nPagePerBlock*MassNDisk.nPageSize;
		
		if(u32TotalSize > 32 * 0x100000)
			u32NANDsize1 = 32 * 1024;
		else
			u32NANDsize1 = u32TotalSize / 1024 / 5;
			
		/* Format NAND if necessery */
		if ((fsDiskFreeSpace('C', &block_size, &free_size, &disk_size) < 0) || 
		    (fsDiskFreeSpace('D', &block_size, &free_size, &disk_size) < 0)) 			    
		    	{   
			    	if (fsTwoPartAndFormatAll((PDISK_T *)MassNDisk.pDisk, u32NANDsize1, (u32TotalSize - u32NANDsize1)) < 0) {
					sysprintf("Format failed\n");	
				fsSetVolumeLabel('C', "NAND1-1\n", strlen("NAND1-1"));
				fsSetVolumeLabel('D', "NAND1-2\n", strlen("NAND1-2"));	
				return 0;	
		    	}
		}			
	#endif	//#ifdef __NAND__

	#ifdef __SD__
		sysprintf("<SD>\n");
		if(u32SD_EXPORT & MSC_SD_PORT0)
		{
			sicIoctl(SIC_SET_CARD_DETECT, TRUE, 0, 0);  // MUST call sicIoctl() BEFORE sicSdOpen0()	
			status0 = sicSdOpen0();			
			if(status0 < 0)
				sicSdClose0();				
			sysprintf("SD0 %d\n",status0);		
		}
		if(u32SD_EXPORT & MSC_SD_PORT1)
		{
			status1 = sicSdOpen1();			
			if(status1 < 0)
				sicSdClose1();				
			sysprintf("SD1 %d\n",status1);		
		}
		if(u32SD_EXPORT & MSC_SD_PORT2)
		{
			status2 = sicSdOpen2();			
			if(status2 < 0)
				sicSdClose2();				
			sysprintf("SD2 %d\n",status2);		
		}
		
	#endif	
#else
#ifdef __RAM_DISK_ONLY__
	sysprintf("MSC Ram Disk Test\n");	
#endif
#ifdef __SPI_ONLY__
	sysprintf("MSC Spi Disk Test\n");	
#endif		
#endif    //#ifndef __RAM_DISK_ONLY__	

	mscdInit();	
	
	u32CdromSize = sizeof(CD_Tracks);
		
	#ifdef __SPI_ONLY__	
	{
		UINT32 block_size, free_size, disk_size, reserved_size;
		PDISK_T		*pDiskList;	
		
		
		fsInitFileSystem();
		fsAssignDriveNumber('C', DISK_TYPE_SD_MMC, 0, 1);
		
        reserved_size = 64*1024;			// SPI reserved size before FAT = 64KB
        status0 = SpiFlashOpen(reserved_size);
        
		if (fsDiskFreeSpace('C', &block_size, &free_size, &disk_size) < 0)  
		{
			UINT32 u32BlockSize, u32FreeSize, u32DiskSize;
			PDISK_T		*pDiskList;	
			
			//printf("Total SPI size = %d KB\n", u32TotalSectorSize/2);

            fsSetReservedArea(reserved_size/512);
			pDiskList = fsGetFullDiskInfomation();
			fsFormatFlashMemoryCard(pDiskList);
			fsReleaseDiskInformation(pDiskList);
			fsDiskFreeSpace('C', &u32BlockSize, &u32FreeSize, &u32DiskSize);   
			sysprintf("block_size = %d\n", u32BlockSize);   
			sysprintf("free_size = %d\n", u32FreeSize);   
			sysprintf("disk_size = %d\n", u32DiskSize); 		
		}
	}	
			
	mscdFlashInitExtendCDROM(NULL,NULL,NULL, 0,0,0,0,CDROM_Read,u32CdromSize);	
#elif defined (__RAM_DISK_ONLY__)
	mscdFlashInitExtendCDROM(NULL,NULL,NULL, 0,0,0,MSC_RAMDISK_8M,CDROM_Read,u32CdromSize);
#else	
	#ifdef __SD__
		mscdSdEnable(u32SD_EXPORT);	
	#endif
	#ifdef __SD_ONLY__
		mscdFlashInitExtendCDROM(NULL,NULL,NULL, status0,status1,status2,0,CDROM_Read,u32CdromSize);
	#else	
		#ifdef __NAND__	
			mscdNandEnable(u32NAND_EXPORT);			
			#ifdef __TWO_NAND__	
				mscdFlashInitExtendCDROM(&MassNDisk,NULL,&MassNDisk2,status0,status1,status2,0,CDROM_Read,u32CdromSize);
			#else
				mscdFlashInitExtendCDROM(&MassNDisk,NULL,NULL,status0,status1,status2,0,CDROM_Read,u32CdromSize);		
			#endif	
		#endif		
	#endif
#endif
	udcInit();	
#ifdef PLUG_DEVICE_TEST	
	while(1)
	{
		bTimeOut = FALSE;
		bPlugStauts = FALSE;
		bHostPlugStauts = FALSE;
		
		sysStartTimer(TIMER0, 
						100, 
						PERIODIC_MODE);
						
	   	u32TimerChannel = sysSetTimerEvent(TIMER0,150,(PVOID)Timer0_Callback);		
		
		mscdMassEvent(PlugDetection);
		
		if(bPlugStauts != udcIsAttached() && udcIsAttached() == FALSE)
			sysprintf("<Unplug>\n");
			
		while(udcIsAttached());			
	}
#else
#ifdef NON_BLOCK_MODE
	mscdBlcokModeEnable(FALSE);		// Non-Block mode
	while(1)
	{
		if(!PlugDetection())
			break;
		mscdMassEvent(NULL);	
	}
#else
	mscdMassEvent(PlugDetection);	// Default : Block mode
#endif	
#endif	
	mscdDeinit();	
	udcDeinit();	
	udcClose();
	sysprintf("Sample code End\n");	
}

