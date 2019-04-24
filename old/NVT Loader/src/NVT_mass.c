#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "w55fa92_reg.h"
#include "usbd.h"
#include "mass_storage_class.h"
#include "nvtloader.h"

#define DETECT_USBD_PLUG
//#define PLUG_DEVICE_TEST

#ifdef __TWO_NAND__
	UINT32 u32NAND_EXPORT = (MSC_NAND_CS0 | MSC_NAND_CS2);	
#else
	UINT32 u32NAND_EXPORT = MSC_NAND_CS0;	
#endif
#ifdef __TWO_SD__
	UINT32 u32SD_EXPORT = (MSC_SD_MP_PORT0 | MSC_SD_MP_PORT1);
#else
	UINT32 u32SD_EXPORT = MSC_SD_MP_PORT0;
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
	
#ifdef SW_REMOVE
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

#endif
void mass(NDISK_T *disk0, NDISK_T *disk1, NDISK_T *disk2, INT SDsector0,INT SDsector1,INT SDsector2, INT RamSize)
{
#ifdef SW_REMOVE
	#ifdef __NAND__
		UINT32 block_size, free_size, disk_size, u32TotalSize;
		#ifdef __TWO_NAND__
			PDISK_T       *pDiskList, *ptPDiskPtr;
		#else
			UINT32 u32NANDsize1;
		#endif			
	#endif	
	
	UINT32 u32SicRef;
	INT32 status0 = 0,status1 = 0,status2 = 0;	
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
#endif /* SW_REMOVE */
			
	sysprintf("<MSC>\n");	 		
#ifndef __RAM_DISK_ONLY__	
#ifdef SW_REMOVE
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
#endif /* SW_REMOVE */
#ifdef SW_REMOVE
	#ifdef __SD__
		sysprintf("<SD>\n");
		if(u32SD_EXPORT & MSC_SD_PORT0)
		{
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
#endif /* SW_REMOVE */		
#else
	sysprintf("MSC Ram Disk Test\n");		
#endif    //#ifndef __RAM_DISK_ONLY__	

	mscdInit();		
	
#ifdef __RAM_DISK_ONLY__		
	mscdFlashInitExtend(NULL,NULL,NULL, 0,0,0,MSC_RAMDISK_8M);	
#else	
	#ifdef __SD__
		mscdSdEnable(u32SD_EXPORT);	
	#endif
	#ifdef __SD_ONLY__
		mscdFlashInitExtend(NULL,NULL,NULL, SDsector0,SDsector1,SDsector2,0);
	#else	
		#ifdef __NAND__	
			mscdNandEnable(u32NAND_EXPORT);			
			#ifdef __TWO_NAND__	
				mscdFlashInitExtend(disk0,disk1,disk2,SDsector0,SDsector1,SDsector2,0);
			#else
				mscdFlashInitExtend(disk0,disk1,disk2,SDsector0,SDsector1,SDsector2,0);		
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
	mscdMassEvent(PlugDetection);	
#endif	
	mscdDeinit();	
	udcDeinit();	
	udcClose();
	sysprintf("Sample code End\n");	
}

