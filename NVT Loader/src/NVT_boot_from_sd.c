/****************************************************************************
*                                                                           *
* Copyright (c) 2013 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "w55fa92_reg.h"
#include "w55fa92_sic.h"
#include "w55fa92_gnand.h"
#include "W55fa92_AudioRec.h"
#include "spu.h"
#include "usbd.h"
#include "wblib.h"
#include "nvtfat.h"

#include "nvtloader.h"


typedef struct sd_info{
	unsigned int startSector;
	unsigned int endSector;
	unsigned int fileLen;
	unsigned int executeAddr;
}NVT_SD_INFO_T;
/* read image information */
UINT8 dummy_buffer[512];
unsigned char *buf;
unsigned int *pImageList;
NVT_SD_INFO_T image;
UINT32 ParsingReservedArea(int g_ibr_boot_sd_port)
{
	UINT32 u32ReservedSector=0;

	int count, i;

	buf = (UINT8 *)((UINT32)dummy_buffer | 0x80000000);
	pImageList=((unsigned int *)(((unsigned int)dummy_buffer)|0x80000000));

	if (g_ibr_boot_sd_port == 0)
		sicSdRead0(33, 1, (UINT32)dummy_buffer);
	else if (g_ibr_boot_sd_port == 1)
	    sicSdRead1(33, 1, (UINT32)dummy_buffer);
	else if (g_ibr_boot_sd_port == 2)
	    sicSdRead2(33, 1, (UINT32)dummy_buffer);

	pImageList=((unsigned int *)(((unsigned int)dummy_buffer)|0x80000000));
	if( (*(pImageList+2)) !=0xFFFFFFFF){
		sysprintf("Turbo writter reserved area %dKB\n",u32ReservedSector/2);
		u32ReservedSector = (*(pImageList+2));
	}
	if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963)){
		count = *(pImageList+1);

		pImageList = pImageList+4;
		for (i=0; i<count; i++){
			if (((*(pImageList) >> 16) & 0xffff) < 4){
				image.startSector = *(pImageList + 1) & 0xffff;
				image.endSector = (*(pImageList + 1) & 0xffff0000) >> 16;
				if(image.endSector>u32ReservedSector)
					u32ReservedSector = image.endSector;
				image.executeAddr = *(pImageList + 2);
				image.fileLen = *(pImageList + 3);
			}
			/* pointer to next image */
			pImageList = pImageList+12;
		}
	}
	return (u32ReservedSector+1);
}

static int g_kfd, g_mfd;
extern void VolumeConfigFile(void);
extern UINT16 u16Volume;
static void s_delay_10ms(int cnt )
{
    int tick;
    
    tick = sysGetTicks(TIMER0);
    while( (sysGetTicks(TIMER0) - tick) <= cnt);
}
extern VOID spuADCVmidEnable (void);
extern VOID spuDacEnable (int volumeLevel);
UINT32 NVT_LoadKernelFromSD(BOARD_S* ps_board, 
							UINT32 g_ibr_boot_sd_port,
							unsigned char* pkBuf)
{
	INT8 path[64];
	//volatile INT32 i32ErrorCode;
	INT found_kernel = 0;
	INT found_avi = 0;
	PDISK_T		*pDiskList;
	UINT32 block_size, free_size, disk_size;
	INT32 i32BootSD0TotalSector = 0, i32BootSD1TotalSector = 0, i32BootSD2TotalSector = 0;
	UINT32 u32TotalSize, u32PllOutHz, u32ExtFreq;
	void	(*_jump)(void);
	UINT32 u32KpiReport;
	unsigned char *pu8Zero=0;

	DBG_PRINTF("Loader will load conprog.bin from SD card.\n");
	
	/* In here for USB VBus stable. Othwise, USB library can not judge VBus correct  */
	udcOpen();
	
	fsAssignDriveNumber('X', DISK_TYPE_SD_MMC, 0, 1);
	fsAssignDriveNumber('Y', DISK_TYPE_SD_MMC, 0, 2);
	/*-----------------------------------------------------------------------*/
	/*  Init SD card                                                         */
	/*-----------------------------------------------------------------------*/
	u32ExtFreq = sysGetExternalClock();
	u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	sicIoctl(SIC_SET_CLOCK, u32PllOutHz/1000, 0, 0);
	sicOpen();

	if (g_ibr_boot_sd_port == 0){
		sysprintf("Load code from SD0\n");
		i32BootSD0TotalSector = sicSdOpen0();	/* Total sector or error code */
		if(i32BootSD0TotalSector < 0)
	        	sicSdClose0();
		sysprintf("total SD0 sectors number (%x)\n", i32BootSD0TotalSector);
	}
	else if (g_ibr_boot_sd_port == 1){
		sysprintf("Load code from SD1\n");
		i32BootSD1TotalSector = sicSdOpen1();	/* Total sector or error code */
		if(i32BootSD1TotalSector < 0)
	   	     sicSdClose1();
		sysprintf("total SD1 sectors (%x)\n", i32BootSD1TotalSector);
	}
	else if (g_ibr_boot_sd_port == 2){
		sysprintf("Load code from SD2\n");
		i32BootSD2TotalSector = sicSdOpen2();	/* Total sector or error code */
		if(i32BootSD2TotalSector < 0)
	  	      sicSdClose2();
		sysprintf("total SD2 sectors (%x)\n", i32BootSD2TotalSector);
	}

	/* Get SD disk information*/
	pDiskList = fsGetFullDiskInfomation();
	sysprintf("Total Disk Size = %dMB\n", pDiskList->uDiskSize/1024);
	/* Format NAND if necessery */
	if ((fsDiskFreeSpace('X', &block_size, &free_size, &disk_size) < 0) ||
	    (fsDiskFreeSpace('Y', &block_size, &free_size, &disk_size) < 0)){
		UINT32 u32Reserved;
		u32Reserved = ParsingReservedArea(g_ibr_boot_sd_port);
		sysprintf("unknow disk type, format device ...Reserved Area= %dKB \n", u32Reserved/2);
		fsSetReservedArea(u32Reserved);
	#if 1
		if (g_ibr_boot_sd_port == 0)
			u32TotalSize = (i32BootSD0TotalSector-u32Reserved)*512; 
		else if (g_ibr_boot_sd_port == 1)
			u32TotalSize = (i32BootSD1TotalSector-u32Reserved)*512; 
		else if (g_ibr_boot_sd_port == 2)
			u32TotalSize = (i32BootSD2TotalSector-u32Reserved)*512; 
	#endif		
    		if (fsTwoPartAndFormatAll((PDISK_T *)pDiskList->ptSelf, SD1_1_SIZE*1024, (u32TotalSize- SD1_1_SIZE*1024)) < 0){
			sysprintf("Format failed\n");
			goto sd_halt;
		}
    		fsSetVolumeLabel('X', "SD1-1\n", strlen("SD1-1"));
		fsSetVolumeLabel('Y', "SD1-2\n", strlen("SD1-2"));

	}

	/* Read volume config file */
	VolumeConfigFile();

	/* Detect USB */	
	u32KpiReport = kpi_read(KEY_ADC_CHANNEL) & MASS_STORAGE;
	sysprintf("KPI  Key Code = 0x%x\n", u32KpiReport);
	
	if(inp32(0xFF001804) == 0x6D617373){	//AutoWriter
		outp32(0xFF001804, 0);
		u32KpiReport = MASS_STORAGE;
	}
			
	if(u32KpiReport==(MASS_STORAGE)){//Demo board = "B"+"LEFT" Key
		sysprintf("Enter USB\n");
		if(udcIsAttached()){
			//for mass's issue. sicSdClose();
			sysprintf("Detect USB plug in\n");
			mass(NULL, NULL, NULL, i32BootSD0TotalSector, i32BootSD1TotalSector, i32BootSD2TotalSector, 0);					/* ptNDisk is useless for SD mass-storage*/			
			sysprintf("USB plug out\n");
		}
	}
	outp32(PHY_CTL, inp32(PHY_CTL) & (~Phy_suspend)); 
#ifdef __AVI_PLAYBACK__
	fsAsciiToUnicode(MOVIE_PATH_SD, path, TRUE);
	g_mfd = fsOpenFile(path, 0, O_RDONLY);
	if(g_mfd > 0){
		found_avi = 1;
		fsCloseFile(g_mfd);
		sysprintf("animation file found\n");
	}
#endif	
	fsAsciiToUnicode(KERNEL_PATH_SD, path, TRUE);
	g_kfd = fsOpenFile(path, 0, O_RDONLY);
	if(g_kfd > 0){
		found_kernel = 1;
		sysprintf("kernel found\n");
	}
#if 0	 /* Remove depop sound  to */ 
	/* Turn on Audio ADC to avoid wrong DC level for SPU */
	DrvAUR_Open(eAUR_MONO_MIC_IN, TRUE);
	DrvAUR_AudioI2cWrite(0x20, 0x00);
	DrvAUR_AudioI2cWrite(0x21, 0xBF);
#endif	
	/* Initial SPU in advance for linux set volume issue */
#define OPT_DEPOP_20140311
#ifdef OPT_DEPOP_20140311
	spuOpen(eDRVSPU_FREQ_8000);
   	if(found_avi){
    #ifdef __AVI_PLAYBACK__		
		char ucString[64]= MOVIE_PATH_SD;					
		
//      spuDacPrechargeEnable();
		s_delay_10ms(70);   // delay 700 ms
        spuADCVmidEnable();
		s_delay_10ms(100);   // delay 1000 ms        
        spuDacEnable(u16Volume);

		playAnimation(ps_board, g_kfd, ucString);
    #endif		
   	}
   	else
   	{
    #ifdef __AVI_PLAYBACK__	
   		aviSetPlayVolume(u16Volume);
    #else 
   		spuSetVolume(u16Volume, u16Volume);	
    #endif	
   		if(found_kernel)
   			loadKernelCont(g_kfd, 0, pkBuf);
   	}
#else
	spuOpen(eDRVSPU_FREQ_8000);
	spuSetDacSlaveMode();
	if(found_avi){
  #ifdef __AVI_PLAYBACK__		
		char ucString[64]= MOVIE_PATH_SD;					
		playAnimation(ps_board, g_kfd, ucString);
  #endif		
	}else{
  #ifdef __AVI_PLAYBACK__		
		aviSetPlayVolume(u16Volume);
  #else 
		spuSetVolume(u16Volume, u16Volume);	
  #endif			
		if(found_kernel)
			loadKernelCont(g_kfd, 0, pkBuf);
	}
#endif	
	if(g_kfd > 0){
		fsCloseFile(g_kfd);	//Close kernel file
	        if (g_ibr_boot_sd_port == 0)
	        	sicSdClose0();
	        else if (g_ibr_boot_sd_port == 1)
	        	sicSdClose1();
	        else if (g_ibr_boot_sd_port == 2)
	        	sicSdClose2();
		sicClose();
		sysSetGlobalInterrupt(DISABLE_ALL_INTERRUPTS);
		sysSetLocalInterrupt(DISABLE_FIQ_IRQ);


		memcpy(pu8Zero/*0x0*/, pkBuf, CP_SIZE);
		// JUMP to kernel
		//outp32(TTR, 0x7);
		sysprintf("Jump to kernel\n");
#ifdef OPT_DEPOP_20140311
    	if(!found_avi)
    	{
       		sysprintf("Jump to kernel aaaaaa\n");                	
            spuADCVmidEnable();
         }            
#endif			
		//sysprintf( "### 0x%x, 0x%x, 0x%x, 0x%x ###\n", inp32(0xb8001030), inp32(0xb8001034), inp32(0xb8001038), inp32(0xb800103C) );
		//lcmFill2Dark((char *)(FB_ADDR | 0x80000000));
		outp32(REG_AHBIPRST, JPG_RST | SIC_RST | UDC_RST | EDMA_RST);
		outp32(REG_AHBIPRST, 0);
		outp32(REG_APBIPRST, UART1RST | UART0RST | TMR1RST | TMR0RST );
		outp32(REG_APBIPRST, 0);
		sysFlushCache(I_D_CACHE);
		// Invalid and disable cache
		sysDisableCache();
		sysInvalidCache();
	
		outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~(SPU_CKE|SD_CKE|NAND_CKE|USBD_CKE|I2S_CKE|VIN0_CKE|SEN0_CKE));
		outp32(REG_APBCLK, inp32(REG_APBCLK) & ~(KPI_CKE|WDCLK_CKE|TOUCH_CKE|TMR1_CKE|RTC_CKE|I2C_CKE)); //|ADC_CKE));	
		outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) & ~(VIN1_CKE|SEN1_CKE));
	
		_jump = (void(*)(void))(0x0); // Jump to 0x0 and execute kernel
		_jump();

		while(1);
//		return(0); // avoid compilation warning
	}else{
		sysprintf("Cannot find conprog.bin in SD card.(err=0x%x)\n",g_kfd);	
		goto sd_halt;	
	}
//	return Successful;
sd_halt:
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~(SPU_CKE|SD_CKE|NAND_CKE|USBD_CKE|I2S_CKE|VIN0_CKE|SEN0_CKE));
	outp32(REG_APBCLK, inp32(REG_APBCLK) & ~(KPI_CKE|WDCLK_CKE|TOUCH_CKE|TMR1_CKE|RTC_CKE|I2C_CKE|ADC_CKE));	
	outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) & ~(VIN1_CKE|SEN1_CKE));
	sysprintf("systen exit\n");
	while(1); // never return
}
