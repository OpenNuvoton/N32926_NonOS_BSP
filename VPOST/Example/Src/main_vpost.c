/**************************************************************************//**
 * @file     main_vpost.c
 * @version  V3.00
 * @brief    N329xx series VPOST demo code
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wbio.h"
#include "wbtypes.h"
#include "W55FA92_VPOST.h"
#include "W55FA92_reg.h"

UINT32 u32SecCnt;
UINT32 u32backup[10];
extern int DemoAPI_HUART(void);

#if defined (__GNUC__) && !(__CC_ARM)
__attribute__ ((aligned (32))) UINT8 Vpost_Frame[]=
#else
__align(32) UINT8 Vpost_Frame[]=
#endif
{

#ifdef __LCD_1024x768__
	#include "house_1024x768_RGBx888.dat"		// for SVGA size test	
#endif
	
#ifdef __LCD_800x600__
	#include "roof_800x600_RGB565.dat"		// for SVGA size test
#endif

#ifdef __LCD_800x480__
	#include "sea_800x480_RGB565.dat"		
#endif

#ifdef __LCD_720x480__
	#include "lake_720x480_RGB565.dat"		// for D1 size test
#endif

#ifdef __LCD_640x480__
    #include "mountain_640x480_RGB565.dat"	// for VGA size test	
#endif

#ifdef __LCD_480x272__
	#include "river_480x272_RGB565.dat"
#endif

#ifdef __LCD_320x240__	
	#include "roof_320x240_RGB565.dat"	
//	#include "lin_320x240_RGB565.dat"	
//	#include "roof_320x240_RGB422.dat"	
//	#include "roof_320x240_RGBx888.dat"		
//  #include "Dbg_QVGA_RGB565.dat"
#endif

#ifdef __LCD_480x854__	
	#include "480x854_RGBx888.dat"	
#endif
};

LCDFORMATEX lcdFormat;

void initPLL(void)
{
	/********************************************************************************************** 
	 * Clock Constraints: 
	 * (a) If Memory Clock > System Clock, the source clock of Memory and System can come from
	 *     different clock source. Suggestion MPLL for Memory Clock, UPLL for System Clock   
	 * (b) For Memory Clock = System Clock, the source clock of Memory and System must come from 
	 *     same clock source	 
	 *********************************************************************************************/

#ifdef __TV_OUTPUT__
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
		sysSetSystemClock(eSYS_UPLL, 216000000, 216000000);
		sysSetDramClock(eSYS_UPLL, 216000000, 216000000);
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
		sysSetDramClock(eSYS_UPLL, 216000000, 216000000);
		sysSetSystemClock(eSYS_UPLL,            //E_SYS_SRC_CLK eSrcClk,
						  216000000,            //UINT32 u32PllKHz,
						  216000000);           //UINT32 u32SysKHz,
		sysSetCPUClock(216000000/2);
	#endif
#else
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
#endif
}

int main(void)
{
	initPLL();
	
//	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB555;	
	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
//	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;
//	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGBx888;

	vpostLCMInit(&lcdFormat, (UINT32*)Vpost_Frame);

	
	while(1);
}


