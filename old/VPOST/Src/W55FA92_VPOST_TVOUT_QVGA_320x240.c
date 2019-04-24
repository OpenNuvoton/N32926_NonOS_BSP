/***************************************************************************
 *                                                                         *
 * Copyright (c) 2007 - 2010 Nuvoton Technology Corp. All rights reserved.*
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     W55FA92_VPOST_TVOUT_320x240.c
 *
 * VERSION
 *     0.1 
 *
 * DESCRIPTION
 *
 *
 *
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *
 *
 *     
 * HISTORY
 *     2009.03.16		Created by Shu-Ming Fan
 *
 *
 * REMARK
 *     None
 *
 *
 **************************************************************************/
#include "stdio.h"
#include "stdlib.h"
#include "w55FA92_vpost.h"

extern void LCDDelay(unsigned int nCount);

#ifdef	__HAVE_TVOUT_320x240__

typedef enum 
{
	eEXT 	= 0,
	eX32K 	= 1,
	eAPLL  	= 2,
	eUPLL  	= 3
}E_CLK;

static UINT32 g_nScreenWidth;
static UINT32 g_nScreenHeight;

static void BacklightControl(int OnOff)
{

}

static INT Clock_Control(void)
{
}

INT vpostLCMInit_TVOUT_320x240(PLCDFORMATEX plcdformatex, UINT32 *pFramebuf)
{
	UINT32 nBytesPixel, u32PLLclk, u32ClockDivider;	
	UINT32 u32Clkin;	
	
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) | VPOST_CKE | HCLK4_CKE);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) | VPOST_RST);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) & ~VPOST_RST);	

	u32Clkin = sysGetExternalClock();

	if(u32Clkin == 27000000)
	{
		// VPOST clock from 27MHz_clkin
		outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_N0);
		outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_N1);
		outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_S);	
	}		
	else
	{
		u32PLLclk = sysGetPLLOutputHz(eUPLL, u32Clkin);		// CLK_IN = 12 MHz
		u32ClockDivider = u32PLLclk / 27000000;
		
		if (!(u32PLLclk % 27000000))
		{
    		u32ClockDivider--;
    		outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_N0);						
    		outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_N1) | ((u32ClockDivider & 0xFF) << 8));						
    		outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_S) | (3<<3));   // VPOST clock from UPLL		
        }
        else
        {
    		u32PLLclk = sysGetPLLOutputHz(eAPLL, u32Clkin);		
    		u32ClockDivider = u32PLLclk / 27000000;
    		u32ClockDivider--;
    		outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_N0);						
    		outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_N1) | ((u32ClockDivider & 0xFF) << 8));						
    		outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_S) | (2<<3));   // VPOST clock from UPLL		
        }    		
	}		

	vpostVAStopTrigger();	
		
	// configure LCD interface  // enable sync with TV, data interface select : YUV422

    // TV control register
    vpostSetTVEnableConfig(	eDRVVPOST_QVGA, 				/* Frame Buffer Size in TV */
    						eDRVVPOST_FRAME_BUFFER, 		/* LCD Color Source */
    						eDRVVPOST_FRAME_BUFFER, 		/* TV Color Source */
    						0,									/* TV DAC 1:Disable 0:Enable */
    						1, 									/* 1:Interlance 0:Non-Interlance */
//    						0, 									/* 1:Interlance 0:Non-Interlance */    						
    						0, 									/* TV System Select 1:PAL 0:NTSC */
    						1									/* TV Encoder 1:enable 0:disable */
    						);
  
  	outpw(REG_TVOUT_ADJ, 0x10000000);		
  
  	// set TV timing
// 	outpw(REG_LCM_TVDisCtl, 0x10f01299);
  	outpw(REG_LCM_TVDisCtl, 0x10ee1593);		  	

  	outpw(REG_LCM_TVDisCtl, (inpw(REG_LCM_TVDisCtl) & (~TVDisCtl_LCDHB)) | 0x00F20000);		
  	
	// enable TV_DAC to mormal mode
  	outpw(REG_LCM_TVCtl, inpw(REG_LCM_TVCtl) | TVCtl_DAC_NORMAL);			
	outpw(REG_LCM_LCDCCtl,inpw(REG_LCM_LCDCCtl) | LCDCCtl_LCDRUN); //va-enable  	

    /*set frambuffer start and end phy addr*/
    if(pFramebuf != NULL) {
		vpostAllocVABufferFromAP(pFramebuf);
	} else {
    	if( vpostAllocVABuffer(plcdformatex, nBytesPixel)==FALSE)
    		return ERR_NULL_BUF;
    }
	
	// set frame buffer data format
	vpostSetFrameBuffer_DataType(plcdformatex->ucVASrcFormat);
	
	// big/little-endian select
	vpostSetYUVEndianSelect(eDRVVPOST_YUV_LITTLE_ENDIAN);
	
	// enable LCD controller
	vpostVAStartTrigger();

	return 0;
}

INT32 vpostLCMDeinit_TVOUT_320x240(VOID)
{
	vpostVAStopTrigger();
	vpostFreeVABuffer();
	return 0;
}

#endif    //__HAVE_TVOUT_320x240__
