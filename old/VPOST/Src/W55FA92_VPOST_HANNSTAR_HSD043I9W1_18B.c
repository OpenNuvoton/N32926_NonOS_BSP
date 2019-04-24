/***************************************************************************
 *                                                                         *
 * Copyright (c) 2007 - 2009 Nuvoton Technology Corp. All rights reserved.*
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     W55FA92_VPOST_HANNSTAR_HSD043I9W1_18B.c
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
 *     2010.09.21		Created by xxx
 *
 *
 * REMARK
 *     None
 *
 *
 **************************************************************************/
#include "stdio.h"
#include "stdlib.h"
#include "wblib.h"

#include "w55FA92_vpost.h"

extern void LCDDelay(unsigned int nCount);

#ifdef __HAVE_HANNSTAR_HSD043I9W1_18B__

static UINT32 g_nScreenWidth;
static UINT32 g_nScreenHeight;

typedef enum 
{
	eEXT 	= 0,
	eX32K 	= 1,
	eAPLL  	= 2,
	eUPLL  	= 3
}E_CLK;


static UINT32 GetPLLOutputKhz(
	E_CLK eSysPll,
	UINT32 u32FinKHz
	)
{
	UINT32 u32Freq, u32PllCntlReg;
	UINT32 NF, NR, NO;
	
	UINT8 au8Map[4] = {1, 2, 2, 4};
	if(eSysPll==eSYS_APLL)
		u32PllCntlReg = inp32(REG_APLLCON);
	else if(eSysPll==eSYS_UPLL)	
		u32PllCntlReg = inp32(REG_UPLLCON);		
	
	NF = (u32PllCntlReg&FB_DV)+2;
	NR = ((u32PllCntlReg & IN_DV)>>9)+2;
	NO = au8Map[((u32PllCntlReg&OUT_DV)>>14)];
//	sysprintf("PLL regster = 0x%x\n", u32PllCntlReg);	
//	sysprintf("NF = %d\n", NF);
//	sysprintf("NR = %d\n", NR);
//	sysprintf("NOr = %d\n", NO);
		
	u32Freq = u32FinKHz*NF/NR/NO;
//	sysprintf("PLL Freq = %d\n", u32Freq);
	return u32Freq;
}

static INT Clock_Control(void)
{
}

INT vpostLCMInit_HANNSTAR_HSD043I9W1_18B(PLCDFORMATEX plcdformatex, UINT32 *pFramebuf)
{
	volatile S_DRVVPOST_FRAME_SIZE sFSize = {480,272};	
	volatile S_DRVVPOST_SYNCLCM_WINDOW sWindow = {480,272,480};	
	volatile S_DRVVPOST_SYNCLCM_HTIMING sHTiming = {2,0x28,(UINT8)0x33};
//	volatile S_DRVVPOST_SYNCLCM_VTIMING sVTiming = {3,0x8,8};
	volatile S_DRVVPOST_SYNCLCM_VTIMING sVTiming = {3,0x8,0x28};
//	volatile S_DRVVPOST_SYNCLCM_POLARITY sPolarity = {FALSE,FALSE,TRUE,TRUE};
	volatile S_DRVVPOST_SYNCLCM_POLARITY sPolarity = {TRUE,TRUE,FALSE,TRUE};
	
	UINT32 nBytesPixel, u32PLLclk, u32ClockDivider, u32Clkin;
	
	volatile S_DRVVPOST_SCALING_OUTPUT sScal;	


//#define OPT_24BIT_MODE


	// VPOST clock control
//	Clock_Control();
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) | VPOST_CKE | HCLK4_CKE);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) | VPOST_RST);
	LCDDelay(10);
	outpw(REG_AHBIPRST, inpw(REG_AHBIPRST) & ~VPOST_RST);	
	
#define FA92_FPGA
#ifndef FA92_FPGA

	u32Clkin = sysGetExternalClock();
	u32PLLclk = sysGetPLLOutputHz(eUPLL, u32Clkin);		// CLK_IN = 12 MHz
	u32ClockDivider = u32PLLclk / 30000000;
	u32ClockDivider--;
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_N0 );						
	outpw(REG_CLKDIV1, (inpw(REG_CLKDIV1) & ~VPOST_N1) | ((u32ClockDivider & 0xFF) << 8));						
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) & ~VPOST_S);
	outpw(REG_CLKDIV1, inpw(REG_CLKDIV1) | (3<<3));		// VPOST clock from UPLL		

#endif

	vpostVAStopTrigger();	

	// Enable VPOST function pins
#ifdef	OPT_24BIT_MODE		
	vpostSetDataBusPin(eDRVVPOST_DATA_24BITS);
#else
//	vpostSetDataBusPin(eDRVVPOST_DATA_16BITS);
	vpostSetDataBusPin(eDRVVPOST_DATA_18BITS);	
#endif	

	// set frame buffer size	
	sFSize.u16HSize = 480;
	sFSize.u16VSize = 272;  
	outpw(REG_LCM_TVCtl, inpw(REG_LCM_TVCtl) & ~TVCtl_TV_D1);        					
	vpostSetFrameBuffer_Size((S_DRVVPOST_FRAME_SIZE*)&sFSize);
		  
	// LCD image source select
	vpostSetLCM_ImageSource(eDRVVPOST_FRAME_BUFFER);
	
	// configure LCD interface
	vpostSetLCM_TypeSelect(eDRVVPOST_HIGH_RESOLUTINO_SYNC);

	// configure LCD timing sync or async with TV timing	
	vpostsetLCM_TimingType(eDRVVPOST_ASYNC_TV);
	
    // Configure Parallel LCD interface (16/18/24-bit data bus)
#ifdef	OPT_24BIT_MODE		
    vpostSetParalelSyncLCM_Interface(eDRVVPOST_PRGB_24BITS);
#else    
//    vpostSetParalelSyncLCM_Interface(eDRVVPOST_PRGB_16BITS);
    vpostSetParalelSyncLCM_Interface(eDRVVPOST_PRGB_18BITS);    
#endif    
    
//	HANNSTAR_HSD043I9W1_18B_Init();
	
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
	vpostSetFrameBuffer_DataType(plcdformatex->ucVASrcFormat);
	
	vpostSetYUVEndianSelect(eDRVVPOST_YUV_LITTLE_ENDIAN);
	
	// enable LCD controller
	vpostVAStartTrigger();
	
//	BacklightControl(TRUE);			
	return 0;
}

INT32 vpostLCMDeinit_HANNSTAR_HSD043I9W1_18B(VOID)
{
	vpostVAStopTrigger();
	vpostFreeVABuffer();
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) & ~VPOST_CKE);	
	return 0;
}
#endif    //__HAVE_HANNSTAR_HSD043I9W1_18B__
