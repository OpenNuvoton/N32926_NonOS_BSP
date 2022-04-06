/**************************************************************************//**
 * @file     spiloader.h
 * @brief    Spiloader header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

/* PLL clock option */
#define __UPLL_240__

/* Enable 4 bit mode */
//#define __OTP_4BIT__
//#define __N32926O2DN__

//#define __RTC_HW_PWOFF__

#ifdef __N32923__

	#ifdef __OTP_4BIT__
		#ifdef __Security__
			#define DATE_CODE   "20210507 4-Bit Mode with Security"
		#else
			#define DATE_CODE   "20210507 4-Bit Mode"
		#endif
	#else
		#ifdef __Security__
			#define DATE_CODE   "20210507 with Security"	
		#else			
			#define DATE_CODE   "20210507"
		#endif			
	#endif

    #define IMAGE_BUFFER		0x500000

#else		//N3295 or N3296

	#ifdef __OTP_4BIT__
		#ifdef __Security__
			#define DATE_CODE   "20210507 4-Bit Mode with Security"	
		#else	
			#define DATE_CODE   "20210507 4-Bit Mode"
		#endif			
	#else
		#ifdef __Security__
			#define DATE_CODE   "20210507 with Security"	
		#else
			#define DATE_CODE   "20210507"
		#endif			
	#endif
		
    #define IMAGE_BUFFER		0xA00000

#endif


/* Start for option for VPOST frame buffer */
#if defined(__TV__)
	#ifdef __TV_QVGA__ 
	#define PANEL_WIDTH		320
	#define PANEL_HEIGHT	240
	#else
	#define PANEL_WIDTH		640
	#define PANEL_HEIGHT	480
	#endif
#elif defined( __LCM_800x600__) 
	#define PANEL_WIDTH		800
	#define PANEL_HEIGHT	600
#elif defined( __LCM_480x272__)
	#define PANEL_WIDTH		480
	#define PANEL_HEIGHT	272
#elif defined( __LCM_800x480__)
	#define PANEL_WIDTH		800
	#define PANEL_HEIGHT	480
#elif defined( __LCM_QVGA__)
	#define PANEL_WIDTH		320
	#define PANEL_HEIGHT	240
#elif defined( __LCM_128x64__)
	#define PANEL_WIDTH		128
	#define PANEL_HEIGHT	64	
#else 	
	#define PANEL_WIDTH	480
	#define PANEL_HEIGHT	272
#endif
/* End for option for VPOST frame buffer */

#define PANEL_BPP		2
#define FB_ADDR		0x500000


#ifdef __DEBUG__
#define DBG_PRINTF		sysprintf
#else
#define DBG_PRINTF(...)		
#endif


 void SPI_OpenSPI(void);
 int SPIReadFast(BOOL bEDMAread, UINT32 addr, UINT32 len, UINT32 *buf);
 INT spiFlashFastReadQuad(UINT32 addr, UINT32 len, UINT32* buf);
 void JEDEC_Probe (void);
 
 /* Turn on the optional. Back light enable */
 /* Turn off the optional, ICE can connect to */
 /* Default Demo Board  GPD1 keep pull high */
 /* 								*/ 
//#define __BACKLIGHT__
 
