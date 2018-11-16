/***************************************************************************
 *                                                                         *
 * Copyright (c) 2007 - 2009 Nuvoton Technology Corp. All rights reserved.*
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     FA92_VPOST_TOPPLY.c
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
#include "w55fa92_vpost.h"

extern void LCDDelay(unsigned int nCount);

#if defined(__HAVE_TOPPLY_320x240__)

static UINT32 g_nScreenWidth;
static UINT32 g_nScreenHeight;

typedef enum 
{
	eEXT 	= 0,
	eX32K 	= 1,
	eAPLL  	= 2,
	eUPLL  	= 3
}E_CLK;


static void BacklightControl(int OnOff)
{	
	// GPA[11] set OUTPUT mode  => control the backlight
	outpw(REG_GPIOA_OMD, (inpw(REG_GPIOA_OMD) & 0x0000FFFF)| 0x00000800);
	if(OnOff==TRUE) {
		// GPA[11] turn on the backlight
		outpw(REG_GPIOA_DOUT, (inpw(REG_GPIOA_DOUT) & 0x0000FFFF)| 0x00000800);
	} else {
		// GPA[11] diable backlight
		outpw(REG_GPIOA_DOUT, (inpw(REG_GPIOA_DOUT) & 0x0000FFFF) & 0xFFFFF7FF);
	}
}

void TOPPLY_Init(void)
{



}


//#define OPT_ASYNC_TV		// async with TV timing

INT vpostLCMInit_TOPPLY_320x240(PLCDFORMATEX plcdformatex, UINT32 *pFramebuf)
{

	UINT32 nBytesPixel,u32PLLclk, u32ClockDivider, u32Clkin;	
	
	volatile S_DRVVPOST_SYNCLCM_HTIMING sHTiming = {4,235,64};			// Horizontal direction is decreased by 1
	volatile S_DRVVPOST_SYNCLCM_VTIMING sVTiming = {18,3,3};			// Vertical direction is not decreased by 1
	volatile S_DRVVPOST_SYNCLCM_WINDOW sWindow = {360,240,0};			
	volatile S_DRVVPOST_SYNCLCM_POLARITY sPolarity = {TRUE,TRUE,FALSE,FALSE};

	volatile S_DRVVPOST_FRAME_SIZE sFSize;
	volatile S_DRVVPOST_SCALING_OUTPUT sScal;	

	outpw(REG_AHBCLK, inpw(REG_AHBCLK) | VPOST_CKE | HCLK4_CKE);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) | VPOST_RST);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) & ~VPOST_RST);	

	
	u32Clkin = sysGetExternalClock();

	u32PLLclk = sysGetPLLOutputHz(eUPLL, u32Clkin);		// CLK_IN = 12 MHz
	u32ClockDivider = u32PLLclk / 27000000;
	u32ClockDivider--;
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_N0 );						
	outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_N1) | ((u32ClockDivider & 0xFF) << 8));						
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_S);
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) | (3<<3));		// VPOST clock from UPLL		
	
	vpostVAStopTrigger();	


	// Enable VPOST function pins
	vpostSetDataBusPin(eDRVVPOST_DATA_8BITS);	

#ifdef OPT_ASYNC_TV
	// configure LCD timing sync or async with TV timing	
	vpostsetLCM_TimingType(eDRVVPOST_ASYNC_TV);
#else
	vpostsetLCM_TimingType(eDRVVPOST_SYNC_TV);
#endif	

	// LCD image source select
	vpostSetLCM_ImageSource(eDRVVPOST_FRAME_BUFFER);

	// LCD type (sync/MPU/High definition)
	vpostSetLCM_TypeSelect(eDRVVPOST_SYNC);	

    // Configure Serial LCD interface (8-bit data bus)
    vpostSetSerialSyncLCM_Interface(eDRVVPOST_SRGB_YUV422);    

	outpw(REG_LCM_LCDCCtl, inpw(REG_LCM_LCDCCtl) | LCDCCtl_YUVBL); 	// little-endian

#ifdef OPT_ASYNC_TV
	// scaling 320x240 to 360x240
	
	sFSize.u16HSize = 320;
	sFSize.u16VSize = 240;  
	sScal.u16HSize = 360;
	sScal.u16VSize = 240;  
	vpostSetScalingOutput_Size((S_DRVVPOST_SCALING_OUTPUT*)&sScal);
	outpw(REG_LCM_LCDCCtl, inpw(REG_LCM_LCDCCtl) | LCDCCtl_SC_EN); 	// enable scaling feature   			
	outpw(REG_LCM_TVCtl, inpw(REG_LCM_TVCtl) & ~TVCtl_TV_D1);        					
	vpostSetFrameBuffer_Size((S_DRVVPOST_FRAME_SIZE*)&sFSize);
	
	// set both "active pixel per line" and "active lines per screen" for Syn type LCD   
	vpostSetSyncLCM_ImageWindow((S_DRVVPOST_SYNCLCM_WINDOW *)&sWindow);

#else
    // TV control register
    vpostSetTVEnableConfig(	eDRVVPOST_LCD_QVGA, 	/* Frame Buffer Size in TV */
    						eDRVVPOST_FRAME_BUFFER, 		/* LCD Color Source */
    						eDRVVPOST_FRAME_BUFFER, 		/* TV Color Source */
    						0,									/* TV DAC 1:Disable 0:Enable */
    						1, 									/* 1:Interlance 0:Non-Interlance */
//    						0, 									/* 1:Interlance 0:Non-Interlance */    						
    						0, 									/* TV System Select 1:PAL 0:NTSC */
    						1									/* TV Encoder 1:enable 0:disable */
    						);
    						
	// TV display position adjustment
  	outpw(REG_TVOUT_ADJ, 0x10000000);		
  	outpw(REG_LCM_TVDisCtl, 0x00C91593);		  	
#endif    						

	// Topply LCD panel		
    //  Topply_LCD_INIT();

	// set Horizontal scanning pixel timing for Syn type LCD   
    vpostSetSyncLCM_HTiming((S_DRVVPOST_SYNCLCM_HTIMING *)&sHTiming);

	// set Vertical scanning line timing for Syn type LCD   
    vpostSetSyncLCM_VTiming((S_DRVVPOST_SYNCLCM_VTIMING *)&sVTiming);
	
  	// set Hsync/Vsync/Vden/Pclk poalrity
	vpostSetSyncLCM_SignalPolarity((S_DRVVPOST_SYNCLCM_POLARITY *)&sPolarity);  	

#if 0
	vpostSetFrameBuffer_BaseAddress(pFramebuf);

#else    
    // set frambuffer base address
    if(pFramebuf != NULL) {
		vpostAllocVABufferFromAP(pFramebuf);
	} else {
    	if( vpostAllocVABuffer(plcdformatex, nBytesPixel)==FALSE)
    		return ERR_NULL_BUF;
    }
#endif    

	// set frame buffer data format
	vpostSetFrameBuffer_DataType(plcdformatex->ucVASrcFormat);

	// trigger to display
	vpostVAStartTrigger();

	return 0;
}

INT32 vpostLCMDeinit_TOPPLY_320x240(VOID)
{
	vpostVAStopTrigger();
	vpostFreeVABuffer();
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) & ~VPOST_CKE);	
	return 0;
}
#endif    //HAVE_TOPPLY
