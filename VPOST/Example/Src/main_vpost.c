/****************************************************************************
 *
 **************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wbio.h"
#include "wbtypes.h"
#include "w55fa92_vpost.h"
#include "w55fa92_reg.h"

UINT32 u32SecCnt;
UINT32 u32backup[10];
extern int DemoAPI_HUART(void);

__align(32) UINT8 Vpost_Frame[]=
{

#ifdef __LCD_1024x768__
//	#include "house_1024x768_rgb565.dat"		// for SVGA size test
	#include "house_1024x768_RGBx888.dat"		// for SVGA size test	
#endif
	
#ifdef __LCD_800x600__
	#include "roof_800x600_rgb565.dat"		// for SVGA size test
#endif

#ifdef __LCD_800x480__
	#include "sea_800x480_rgb565.dat"		
//	#include "roof_800x480_rgb565.dat"				
#endif

#ifdef __LCD_720x480__
	#include "lake_720x480_rgb565.dat"		// for D1 size test
//	#include "roof_720x480_rgb565.dat"		// for D1 size test
#endif

#ifdef __LCD_640x480__
    #include "mountain_640x480_rgb565.dat"	// for VGA size test	
#endif

#ifdef __LCD_480x272__
	#include "river_480x272_rgb565.dat"
#endif

#ifdef __LCD_320x240__	
	#include "roof_320x240_rgb565.dat"	
//	#include "lin_320x240_rgb565.dat"	
//	#include "roof_320x240_yuv422.dat"	
//	#include "roof_320x240_rgbx888.dat"		
//  #include "Dbg_QVGA_RGB565.dat"
#endif

#ifdef __LCD_480x854__	
	#include "480x854_RGBx888.dat"	
#endif

//    #include "mountain_640x480_rgb565.dat"	// for VGA size test	
//	#include "river_480x272_rgb565.dat"
};

LCDFORMATEX lcdFormat;

void initPLL(void)
{
#ifdef __TV_OUTPUT__

	sysSetSystemClock(eSYS_UPLL, 		//E_SYS_SRC_CLK eSrcClk,
					2160000000,		//UINT32 u32PllKHz,
					2160000000);		//UINT32 u32SysKHz,	
	sysSetDramClock(eSYS_UPLL, 216000000, 321600000);

#else
	/* */
//    sysSetDramClock(eSYS_UPLL, 300000000, 300000000);
	sysSetSystemClock(eSYS_UPLL, 		//E_SYS_SRC_CLK eSrcClk,
					300000000,		//UINT32 u32PllKHz,
					300000000);		//UINT32 u32SysKHz,	
	sysSetDramClock(eSYS_UPLL, 300000000, 300000000);
	
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


