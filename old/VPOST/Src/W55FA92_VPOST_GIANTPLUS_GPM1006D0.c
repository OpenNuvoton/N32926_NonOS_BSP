/***************************************************************************
 *                                                                         *
 * Copyright (c) 2007 - 2009 Nuvoton Technology Corp. All rights reserved.*
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     FA92_VPOST_GIANTPLUS_GPM1006D0.c
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
//#include "NUC930_VPOST_Regs.h"
#include "w55fa92_vpost.h"

extern void LCDDelay(unsigned int nCount);

#if defined(__HAVE_GIANTPLUS_GPM1006D0__)

//#define OPT_SPI_CS_BY_GPD2

//static UINT32 g_nScreenWidth;
//static UINT32 g_nScreenHeight;

typedef enum 
{
	eEXT 	= 0,
	eX32K 	= 1,
	eAPLL  	= 2,
	eUPLL  	= 3
}E_CLK;

#if 0
static delay_loop(UINT8 u8delay)
{
	volatile UINT8 ii, jj;
	for (jj=0; jj<u8delay; jj++)
		for (ii=0; ii<200; ii++);
}
#endif

//=====================================================================
//=====================================================================
#if 0
static void BacklightControl(int OnOff)
{	
}
#endif

extern VOID vpostVAStopTrigger(void);
extern VOID vpostSetDataBusPin(E_DRVVPOST_DATABUS eDataBus);
extern VOID vpostSetLCM_ImageSource
(
	E_DRVVPOST_IMAGE_SOURCE eSource
);
extern VOID vpostSetFrameBuffer_Size
(
	S_DRVVPOST_FRAME_SIZE* psSize
);
extern VOID vpostSetLCM_TypeSelect
(
	E_DRVVPOST_LCM_TYPE eType
);
extern VOID vpostsetLCM_TimingType
(
	E_DRVVPOST_TIMING_TYPE eTimingTpye
);
extern VOID vpostSetSerialSyncLCM_Interface
(
	E_DRVVPOST_8BIT_SYNCLCM_INTERFACE eInterface
);
extern VOID vpostSetSyncLCM_HTiming
(
	S_DRVVPOST_SYNCLCM_HTIMING *psHTiming
);
extern VOID vpostSetSyncLCM_VTiming
(
	S_DRVVPOST_SYNCLCM_VTIMING *psVTiming
);
extern VOID vpostSetSyncLCM_ImageWindow
(
	S_DRVVPOST_SYNCLCM_WINDOW *psWindow
);
extern VOID vpostSetSyncLCM_SignalPolarity
(
	S_DRVVPOST_SYNCLCM_POLARITY *psPolarity
);
extern BOOL vpostAllocVABufferFromAP(UINT32 *pFramebuf);
extern BOOL vpostAllocVABuffer(PLCDFORMATEX plcdformatex,UINT32 nBytesPixel);
extern VOID vpostSetFrameBuffer_DataType
(
	E_DRVVPOST_FRAME_DATA_TYPE eType
);
extern VOID vpostSetYUVEndianSelect
(
	E_DRVVPOST_ENDIAN eEndian
);
extern VOID vpostVAStartTrigger(void);
extern BOOL vpostFreeVABuffer(void);

INT vpostLCMInit_GIANTPLUS_GPM1006D0(PLCDFORMATEX plcdformatex, UINT32 *pFramebuf)
{
	volatile S_DRVVPOST_FRAME_SIZE sFSize = {320,240};	
	S_DRVVPOST_SYNCLCM_WINDOW sWindow = {320,240,320};	
	S_DRVVPOST_SYNCLCM_HTIMING sHTiming = {2,0xCA,(UINT8)0x3C};
//	volatile S_DRVVPOST_SYNCLCM_VTIMING sVTiming = {0x12,0x13,3};
//	volatile S_DRVVPOST_SYNCLCM_VTIMING sVTiming = {0x2,0x8,1};
	S_DRVVPOST_SYNCLCM_VTIMING sVTiming = {0x2,0x10,1};
//	volatile S_DRVVPOST_SYNCLCM_POLARITY sPolarity = {FALSE,FALSE,FALSE,TRUE};
	S_DRVVPOST_SYNCLCM_POLARITY sPolarity = {TRUE,TRUE,FALSE,FALSE};

	UINT32 nBytesPixel, u32PLLclk, u32ClockDivider, u32Clkin;
	
//	volatile S_DRVVPOST_SCALING_OUTPUT sScal;		

	// VPOST clock control
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) | VPOST_CKE | HCLK4_CKE);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) | VPOST_RST);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) & ~VPOST_RST);	

	u32Clkin = sysGetExternalClock();

	u32PLLclk = sysGetPLLOutputHz((E_SYS_SRC_CLK)eUPLL, u32Clkin);		// CLK_IN = 12 MHz
	u32ClockDivider = u32PLLclk / 24000000;
	u32ClockDivider--;
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_N0 );						
	outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_N1) | ((u32ClockDivider & 0xFF) << 8));						
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_S);
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) | (3<<3));		// VPOST clock from UPLL		

	vpostVAStopTrigger();	

	// Enable VPOST function pins
	vpostSetDataBusPin(eDRVVPOST_DATA_8BITS);	
		  
	// LCD image source select
	vpostSetLCM_ImageSource(eDRVVPOST_FRAME_BUFFER);
	
	// set frame buffer size	
	sFSize.u16HSize = 320;
	sFSize.u16VSize = 240;  
	outpw(REG_LCM_TVCtl, inpw(REG_LCM_TVCtl) & ~TVCtl_TV_D1);        					
	vpostSetFrameBuffer_Size((S_DRVVPOST_FRAME_SIZE*)&sFSize);

    // configure LCD interface
	vpostSetLCM_TypeSelect(eDRVVPOST_SYNC);

	// configure LCD timing sync or async with TV timing	
	vpostsetLCM_TimingType(eDRVVPOST_ASYNC_TV);
	
    // Configure Serial LCD interface (8-bit data bus)
    vpostSetSerialSyncLCM_Interface(eDRVVPOST_SRGB_RGBTHROUGH);    
    
//	ILI_I9322_RGBThrough_NTSC_INIT();

    // set Horizontal scanning line timing for Syn type LCD 
    vpostSetSyncLCM_HTiming(&sHTiming);

	// set Vertical scanning line timing for Syn type LCD   
    vpostSetSyncLCM_VTiming(&sVTiming);
	
	// set both "active pixel per line" and "active lines per screen" for Syn type LCD   
	vpostSetSyncLCM_ImageWindow(&sWindow);

  	// set Hsync/Vsync/Vden/Pclk poalrity
	vpostSetSyncLCM_SignalPolarity(&sPolarity);  	
    
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
	vpostSetFrameBuffer_DataType((E_DRVVPOST_FRAME_DATA_TYPE)plcdformatex->ucVASrcFormat);
	
	vpostSetYUVEndianSelect(eDRVVPOST_YUV_LITTLE_ENDIAN);
	
	// enable LCD controller
	vpostVAStartTrigger();
	
//	BacklightControl(TRUE);			
	return 0;
}

INT32 vpostLCMDeinit_GIANTPLUS_GPM1006D0(void)
{
	vpostVAStopTrigger();
	vpostFreeVABuffer();
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) & ~VPOST_CKE);	
	return 0;
}
#endif    //__HAVE_GIANTPLUS_GPM1006D0__
