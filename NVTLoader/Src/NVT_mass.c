#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "W55FA92_reg.h"
#include "usbd.h"
#include "mass_storage_class.h"
#include "nvtloader.h"

#define DETECT_USBD_PLUG
//#define PLUG_DEVICE_TEST
#ifdef __NAND__
static UINT32 u32NAND_EXPORT;
#endif
#ifdef __SD__
static UINT32 u32SD_EXPORT;
#endif
extern int g_ibr_boot_sd_port;     // indicate the SD port number which IBR boot from.

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



void mass(NDISK_T *disk0, NDISK_T *disk1, NDISK_T *disk2, INT SDsector0,INT SDsector1,INT SDsector2, INT RamSize)
{
	sysprintf("<MSC>\n");	 		

	mscdInit();		
#ifdef __SD__
	if (g_ibr_boot_sd_port == 0)
		u32SD_EXPORT = MSC_SD_MP_PORT0;
	else if (g_ibr_boot_sd_port == 1)
		u32SD_EXPORT = MSC_SD_MP_PORT1;
	else if (g_ibr_boot_sd_port == 2)
		u32SD_EXPORT = MSC_SD_MP_PORT2;
	//#ifdef __TWO_SD__
	//    u32NAND_EXPORT = MSC_SD_MP_PORT0 | MSC_SD_MP_PORT1; // If want to support 2 devices mass-storage, remember to do sicSdOpenx() for both devices. 
	//#endif
	mscdSdEnable(u32SD_EXPORT);	
#endif
#ifdef __SD_ONLY__
	mscdFlashInitExtend(NULL, NULL, NULL,\
						SDsector0, SDsector1, SDsector2,0);
#else	
  #ifdef __NAND__
	if (g_ibr_boot_sd_port == 0)
		u32NAND_EXPORT = MSC_NAND_CS0;
	else if (g_ibr_boot_sd_port == 1)
		u32NAND_EXPORT = MSC_NAND_CS1;
	//else if (g_ibr_boot_sd_port == 2)
	//	u32NAND_EXPORT = MSC_NAND_CS2;
	//#ifdef __TWO_NAND__
	//    u32NAND_EXPORT = MSC_NAND_CS0 | MSC_NAND_CS1; // If want to support 2 devices mass-storage, remember to do sicSdOpenx() for both devices. 
	//#endif
	mscdNandEnable(u32NAND_EXPORT);			
	mscdFlashInitExtend(disk0, disk1, disk2,\
							SDsector0, SDsector1, SDsector2, 0);	
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

