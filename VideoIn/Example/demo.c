/***************************************************************************
 *                                                                         									     *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              					     *
 *                                                                         									     *
 ***************************************************************************/

#include <stdio.h>
#include "wblib.h"
#include "W55FA92_GPIO.h"
#include "W55FA92_VideoIn.h"
#include "demo.h"
#include "NVTFAT.h"
#include "jpegcodec.h"
#include "W55FA92_SIC.h"

#if defined (__GNUC__) 
UINT8 u8PacketFrameBuffer[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2] __attribute__((aligned(4)));		//Keep 640*480*2 RGB565 frame buffer
UINT8 u8PacketFrameBuffer1[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2] __attribute__((aligned(4)));		//Keep 640*480*2 RGB565 frame buffer
UINT8 u8PacketFrameBuffer2[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2] __attribute__((aligned(4)));		//Keep 640*480*2 RGB565 frame buffer
UINT8 u8PlanarFrameBuffer[OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT*2] __attribute__((aligned(256)));		//Keep 640x480*2 PlanarYUV422 frame buffer
UINT8 u8PlanarFrameBuffer1[OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT*2] __attribute__((aligned(256)));
#else
UINT8 __align(4) u8PacketFrameBuffer[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2];		//Keep 640*480*2 RGB565 frame buffer
UINT8 __align(4) u8PacketFrameBuffer1[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2];		//Keep 640*480*2 RGB565 frame buffer
UINT8 __align(4) u8PacketFrameBuffer2[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2];		//Keep 640*480*2 RGB565 frame buffer
UINT8 __align(256) u8PlanarFrameBuffer[OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT*2];		//Keep 640x480*2 PlanarYUV422 frame buffer
UINT8 __align(256) u8PlanarFrameBuffer1[OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT*2];
#endif

VINDEV_T Vin;
VINDEV_T* pVin;

volatile UINT32 g_u32FrameCount = 0;
volatile UINT32 g_u32FrameCount1= 0;
BOOL bIsFrameBuffer0=0,  bIsFrameBuffer1=0, bIsFrameBuffer2=0; /* 0 means buffer is clean */
UINT32 u32VideoInIdx = 0;	//Packet buffer idx
UINT32 u32PlanarIdx = 0;		//Planar buffer idx

void CoWork_VideoIn0_InterruptHandler(void)
{	
	//sysprintf("VIN0 done\n");
	g_u32FrameCount = g_u32FrameCount+1;
}
void CoWork_VideoIn1_InterruptHandler(void)
{
	//sysprintf("VIN1 done\n");
	g_u32FrameCount1 = g_u32FrameCount1+1;
}
void CoWork_VideoIn_InterruptHandler(void )
{

}
void VideoIn_InterruptHandler(void)
{
	UINT16 u16Width, u16Height;
#if 1
	switch(u32VideoInIdx)
	{//Current Packet Frame
		case 0:		
			if(bIsFrameBuffer1==0)
			{/* Change frame buffer 1 if Frame Buffer 1 is clean, Otherwise, do nothing */	
			#ifdef __TV__
				pVin->SetBaseStartAddress(eVIDEOIN_PACKET,	0, (UINT32)((UINT32)u8PacketFrameBuffer1 ) );		
			#else			
				pVin->SetBaseStartAddress(eVIDEOIN_PACKET,	 (E_VIDEOIN_BUFFER)0, (UINT32)((UINT32)u8PacketFrameBuffer1 + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );																	
			 #endif		
			 	bIsFrameBuffer0 = 1; u32VideoInIdx = 1; 
			}				
			break; 	 				
		case 1:		
			if(bIsFrameBuffer2==0)
			{/* Change frame buffer 2 if Frame Buffer 2 is clean, Otherwise, do nothing */
			#ifdef __TV__
				pVin->SetBaseStartAddress(eVIDEOIN_PACKET,	0, (UINT32)((UINT32)u8PacketFrameBuffer2 ) );				
			#else				
				pVin->SetBaseStartAddress(eVIDEOIN_PACKET,	 (E_VIDEOIN_BUFFER)0, (UINT32)((UINT32)u8PacketFrameBuffer2 + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );										
			#endif						
 				bIsFrameBuffer1 = 1; u32VideoInIdx = 2;
			}	
				break; 	 				
		case 2:		
			if(bIsFrameBuffer0==0)
			{/* Change frame buffer 0 if Frame Buffer 0 is clean, Otherwise, do nothing */
			#ifdef __TV__
				pVin->SetBaseStartAddress(eVIDEOIN_PACKET,	0, (UINT32)((UINT32)u8PacketFrameBuffer ) );				
			#else				
				pVin->SetBaseStartAddress(eVIDEOIN_PACKET,	 (E_VIDEOIN_BUFFER)0, (UINT32)((UINT32)u8PacketFrameBuffer + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );										
			#endif						
 				bIsFrameBuffer2 = 1; u32VideoInIdx = 0;						
			}	
				break; 	 
	}
	
	u16Width = OPT_ENCODE_WIDTH;
	u16Height = OPT_ENCODE_HEIGHT;
	switch(u32PlanarIdx)
	{//Current Planar Frame
		case 0:		
			/* Change frame buffer 1 if Frame Buffer 1 is clean, Otherwise, do nothing */
			pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,	 (E_VIDEOIN_BUFFER)0, (UINT32)((UINT32)u8PlanarFrameBuffer1 ) );	
			pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,	 (E_VIDEOIN_BUFFER)1, (UINT32)u8PlanarFrameBuffer1 +u16Width*u16Height);	    		
		
		#ifdef _MACRO_BLOCK_
			pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,	 2, (UINT32)u8PlanarFrameBuffer1 +u16Width*u16Height);	 
		#else	
	    		pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,	 (E_VIDEOIN_BUFFER)2, (UINT32)u8PlanarFrameBuffer1 +u16Width*u16Height+u16Width*u16Height/2);	
	    	#endif		    		   				
	    	
			u32PlanarIdx = 1;
			break; 	 	
		case 1:		
			/* Change frame buffer 1 if Frame Buffer 1 is clean, Otherwise, do nothing */
			pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,	 (E_VIDEOIN_BUFFER)0, (UINT32)((UINT32)u8PlanarFrameBuffer ) );	
			pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,	 (E_VIDEOIN_BUFFER)1, (UINT32)u8PlanarFrameBuffer+u16Width*u16Height);
			
		#ifdef _MACRO_BLOCK_	
			pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,	 2, (UINT32)u8PlanarFrameBuffer +u16Width*u16Height);	    		
		#else	
	    		pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,	 (E_VIDEOIN_BUFFER)2, (UINT32)u8PlanarFrameBuffer +u16Width*u16Height+u16Width*u16Height/2);					
	    	#endif		    
	    	
			u32PlanarIdx = 0;
			break; 	 				
	}
	pVin->SetShadowRegister();	
	pVin->SetOperationMode(TRUE);	
#endif				
	g_u32FrameCount = g_u32FrameCount+1;
}
UINT32 VideoIn_GetCurrFrameCount(void)
{
	return g_u32FrameCount;
}
void VideoIn_ClearFrameCount(void)
{
	g_u32FrameCount = 0;
}
#if defined (__GNUC__) 
UINT8 u8PacketFrameBuffer2_0[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2] __attribute__((aligned(4)));
UINT8 u8PacketFrameBuffer2_1[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2] __attribute__((aligned(4)));
UINT8 u8PacketFrameBuffer2_2[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2] __attribute__((aligned(4)));
UINT8 u8PlanarFrameBuffer2_0[OPT_ENCODE_WIDTH_2*OPT_ENCODE_HEIGHT_2*2] __attribute__((aligned(256)));		
UINT8 u8PlanarFrameBuffer2_1[OPT_ENCODE_WIDTH_2*OPT_ENCODE_HEIGHT_2*2] __attribute__((aligned(256)));

UINT8 u8MDY[OPT_LCM_WIDTH/8*OPT_LCM_HEIGHT/8+4096] __attribute__((aligned(256)));
UINT8 u8MDO[OPT_LCM_WIDTH/8*OPT_LCM_HEIGHT/8+4096] __attribute__((aligned(256)));
#else
UINT8 __align(4) u8PacketFrameBuffer2_0[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2];		
UINT8 __align(4) u8PacketFrameBuffer2_1[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2];	
UINT8 __align(4) u8PacketFrameBuffer2_2[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2];
UINT8 __align(256) u8PlanarFrameBuffer2_0[OPT_ENCODE_WIDTH_2*OPT_ENCODE_HEIGHT_2*2];
UINT8 __align(256) u8PlanarFrameBuffer2_1[OPT_ENCODE_WIDTH_2*OPT_ENCODE_HEIGHT_2*2];

UINT8 __align(256) u8MDY[OPT_LCM_WIDTH/8*OPT_LCM_HEIGHT/8+4096];
UINT8 __align(256) u8MDO[OPT_LCM_WIDTH/8*OPT_LCM_HEIGHT/8+4096];

#endif
VINDEV_T Vin2;
VINDEV_T* pVin2;

UINT32 u32VideoIn2Idx = 0;	//Packet buffer idx
UINT32 u32Planar2Idx = 0;	//Planar buffer idx
BOOL bIsVin2FrameBuffer0=0,  bIsVin2FrameBuffer1=0, bIsVin2FrameBuffer2=0; /* 0 means buffer is clean */
void VideoIn2_InterruptHandler(void)
{
	UINT16 u16Width, u16Height;
#if 1
	switch(u32VideoIn2Idx)
	{//Current Packet Frame
		case 0:		
			if(bIsVin2FrameBuffer1==0)
			{/* Change frame buffer 1 if Frame Buffer 1 is clean, Otherwise, do nothing */	
			#ifdef __TV__
				pVin2->SetBaseStartAddress(eVIDEOIN_PACKET,	0, (UINT32)((UINT32)u8PacketFrameBuffer2_1 ) );		
			#else			
				pVin2->SetBaseStartAddress(eVIDEOIN_PACKET,	 (E_VIDEOIN_BUFFER)0, (UINT32)((UINT32)u8PacketFrameBuffer2_1 + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );																	
			 #endif		
			 	bIsVin2FrameBuffer0 = 1; u32VideoIn2Idx = 1; 
			}				
			break; 	 				
		case 1:		
			if(bIsVin2FrameBuffer2==0)
			{/* Change frame buffer 2 if Frame Buffer 2 is clean, Otherwise, do nothing */
			#ifdef __TV__
				pVin2->SetBaseStartAddress(eVIDEOIN_PACKET,	0, (UINT32)((UINT32)u8PacketFrameBuffer2_2 ) );				
			#else				
				pVin2->SetBaseStartAddress(eVIDEOIN_PACKET,	 (E_VIDEOIN_BUFFER)0, (UINT32)((UINT32)u8PacketFrameBuffer2_2 + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );										
			#endif						
 				bIsVin2FrameBuffer1 = 1; u32VideoIn2Idx = 2;
			}	
				break; 	 				
		case 2:		
			if(bIsVin2FrameBuffer0==0)
			{/* Change frame buffer 0 if Frame Buffer 0 is clean, Otherwise, do nothing */
			#ifdef __TV__
				pVin2->SetBaseStartAddress(eVIDEOIN_PACKET,	0, (UINT32)((UINT32)u8PacketFrameBuffer2_0 ) );				
			#else				
				pVin2->SetBaseStartAddress(eVIDEOIN_PACKET,	 (E_VIDEOIN_BUFFER)0, (UINT32)((UINT32)u8PacketFrameBuffer2_0 + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );										
			#endif						
 				bIsVin2FrameBuffer2 = 1; u32VideoIn2Idx = 0;						
			}	
				break; 	 
	}
	
	u16Width = OPT_ENCODE_WIDTH_2;
	u16Height = OPT_ENCODE_HEIGHT_2;
	switch(u32Planar2Idx)
	{//Current Planar Frame
		case 0:		
			/* Change frame buffer 1 if Frame Buffer 1 is clean, Otherwise, do nothing */
			pVin2->SetBaseStartAddress(eVIDEOIN_PLANAR,	 (E_VIDEOIN_BUFFER)0, (UINT32)((UINT32)u8PlanarFrameBuffer2_1 ) );	
			pVin2->SetBaseStartAddress(eVIDEOIN_PLANAR,	 (E_VIDEOIN_BUFFER)1, (UINT32)u8PlanarFrameBuffer2_1 +u16Width*u16Height);	    		
		
		#ifdef _MACRO_BLOCK_
			pVin2->SetBaseStartAddress(eVIDEOIN_PLANAR,	 2, (UINT32)u8PlanarFrameBuffer2_1 +u16Width*u16Height);	 
		#else	
	    		pVin2->SetBaseStartAddress(eVIDEOIN_PLANAR,	 (E_VIDEOIN_BUFFER)2, (UINT32)u8PlanarFrameBuffer2_1 +u16Width*u16Height+u16Width*u16Height/2);	
	    	#endif		    		   				
	    	
			u32Planar2Idx = 1;
			break; 	 	
		case 1:		
			/* Change frame buffer 1 if Frame Buffer 1 is clean, Otherwise, do nothing */
			pVin2->SetBaseStartAddress(eVIDEOIN_PLANAR,	 (E_VIDEOIN_BUFFER)0, (UINT32)((UINT32)u8PlanarFrameBuffer2_0 ) );	
			pVin2->SetBaseStartAddress(eVIDEOIN_PLANAR,	 (E_VIDEOIN_BUFFER)1, (UINT32)u8PlanarFrameBuffer2_0+u16Width*u16Height);
			
		#ifdef _MACRO_BLOCK_	
			pVin2->SetBaseStartAddress(eVIDEOIN_PLANAR,	 2, (UINT32)u8PlanarFrameBuffer2_0 +u16Width*u16Height);	    		
		#else	
	    		pVin2->SetBaseStartAddress(eVIDEOIN_PLANAR,	 (E_VIDEOIN_BUFFER)2, (UINT32)u8PlanarFrameBuffer2_0 +u16Width*u16Height+u16Width*u16Height/2);					
	    	#endif		    
	    	
			u32Planar2Idx = 0;
			break; 	 				
	}
	pVin2->SetShadowRegister();	
	pVin2->SetOperationMode(TRUE);	
#endif				
	g_u32FrameCount1 = g_u32FrameCount1+1;
}
void Delay(UINT32 nCount)
{
	volatile UINT32 i;
	for(;nCount!=0;nCount--)
		for(i=0;i<5;i++);
}
UINT32 u3210msFlag=0;
void TimerBase(void)
{
	u3210msFlag = u3210msFlag+1;
}

/*-----------------------------------------------------------------------------
 * ISR of Card detect interrupt for card insert
 *---------------------------------------------------------------------------*/
void isr_card_insert()
{
    UINT32 result;
    sysprintf("--- ISR: card inserted on SD port 0 ---\n\n");
    result = sicSdOpen0();
    if (result < FMI_ERR_ID)
    {
        sysprintf("    Detect card on port %d.\n", 0);
    }
    else if (result == FMI_NO_SD_CARD)
    {
        sysprintf("WARNING: Don't detect card on port %d !\n", 0);
    }
    else
    {
        sysprintf("WARNING: Fail to initial SD/MMC card %d, result = 0x%x !\n", 0, result);
    }
    return;
}


/*-----------------------------------------------------------------------------
 * ISR of Card detect interrupt for card remove
 *---------------------------------------------------------------------------*/
void isr_card_remove()
{
    sysprintf("--- ISR: card removed on SD port 0 ---\n\n");
    sicSdClose0();
    return;
}

void init(void)
{
	WB_UART_T uart;
	UINT32 u32ExtFreq;
	UINT32 u32Channel;
	UINT32 u32Item;
	UINT32 u32PllOutHz;
	
	/* Init UART */
//	u32ExtFreq = sysGetExternalClock();
//	u32ExtFreq = 40000000;//27000000;//();	
	u32ExtFreq = sysGetExternalClock();
	sysUartPort(1);
	uart.uiFreq = u32ExtFreq;	
	
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
	uart.uart_no = WB_UART_1;
	sysInitializeUART(&uart);
	sysprintf("UART Init\n");

#if 0								
	sysSetDramClock(eSYS_MPLL, 300000000, 300000000);
	
	sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
					240000000,		//UINT32 u32PllKHz,
					240000000);		//UINT32 u32SysKHz	
#endif

	u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	DBG_PRINTF("UPLL out frequency %d Hz\n", u32PllOutHz);	
	/* Cache on */ 
#ifdef __ENABLE_CACHE__	
	sysDisableCache(); 	
	sysFlushCache(I_D_CACHE);		
	sysEnableCache(CACHE_WRITE_BACK);
#else
	sysDisableCache(); 		
#endif 
	

	/* Init Timer */
	//u32ExtFreq = sysGetExternalClock();	
	
	sysSetTimerReferenceClock(TIMER0, u32ExtFreq); //External Crystal	
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);		/* 100 ticks/per sec ==> 1tick/10ms */	
	u32Channel = sysSetTimerEvent(TIMER0, 1, (PVOID)TimerBase);	/* 1 ticks=10ms call back */	
	sysSetLocalInterrupt(ENABLE_FIQ_IRQ);			
	
	/* Init file system */
	DBG_PRINTF(" Do you want to initialize SD card 0 (y?). If you want to test VideoIn from GPA[1:0], Don't to initialize SD card 0 \n");
	u32Item = sysGetChar();
	if((u32Item=='y') || (u32Item=='Y'))
	{	
		/* Init file system */
		fsInitFileSystem();		
		/* Init Storage Interface Controller */
		sicIoctl(SIC_SET_CLOCK, u32PllOutHz/1000, 0, 0);	
		sicOpen();	
		 //--- Initial callback function for card detection interrupt
		sicIoctl(SIC_SET_CALLBACK, FMI_SD_CARD, (INT32)isr_card_remove, (INT32)isr_card_insert);
		if (sicSdOpen0()<=0)
		{
			sysprintf("Error in initializing SD card !! \n");						
			while(1);
		}			
		fsAssignDriveNumber('C', DISK_TYPE_SD_MMC, 0, 1);
		sicIoctl(SIC_SET_CALLBACK, FMI_SD_CARD, (INT32)isr_card_remove, (INT32)isr_card_insert);	
	}	
}


void MD_Handler(void)
{
	sysprintf("MD\n");
}

IQ_S SensorIQ = {0};
IQ_S* pSensorIQ;
int main()
{
	UINT32 u32Item;
	init();	
	
	pSensorIQ = &SensorIQ;
			 	
	DBG_PRINTF("================================================================\n");
	DBG_PRINTF("Please use LCD Gaintplus GPM1006D													\n");  
	DBG_PRINTF("Please modify file-demo.h for the sensor type on the DEV or DEMO board						\n");   	
	DBG_PRINTF("================================================================\n");
	do
	{ 	   			
		DBG_PRINTF("================================================================\n");
		DBG_PRINTF("                 VideoIn library demo code                      \n"); 
		DBG_PRINTF(" [1] OV9660 VGA demo                                            \n"); /* OK on EVB */
		DBG_PRINTF(" [2] OV9660 SXGA demo                                           \n"); /* */ 
		DBG_PRINTF(" [3] OV7670 demo                                                \n"); /*OK on DEV */
		DBG_PRINTF(" [4] OV7725 VGA                                                 \n");	/* OK on DEV */
		DBG_PRINTF(" [5] NT99141 VGA                                                \n"); /* OK on DEV and Demo board */
		DBG_PRINTF(" [6] NT99141 HD                                                 \n"); /* OK on DEV and Demo board */
		DBG_PRINTF(" [7] NT99050                                                    \n");	/* OK on DEV and Demo board */
		DBG_PRINTF(" [8] NT99160 VGA                                                \n"); /* OK on DEV and Demo board */
		DBG_PRINTF(" [9] NT99161 SVGA                                               \n");	/* OK on DEV and Demo board */
		DBG_PRINTF(" [a] NT99161 HD                                                 \n");	/* OK on DEV and Demo board */
		DBG_PRINTF(" [b] HM1375 VGA                                                 \n"); /* OK on DEV and Demo board */
		DBG_PRINTF(" [c] HM1375 HD                                                  \n");	/* OK on DEV and Demo board */
		DBG_PRINTF(" [d] NT99340 HD                                                 \n");	/* OK on DEV and Demo board */
		DBG_PRINTF(" [e] NT99340 FULL HD                                            \n");	/* OK on DEV and Demo board */
		DBG_PRINTF(" [f] NT99340 QXGA                                               \n");	/* OK on DEV and Demo board */
		DBG_PRINTF(" [g] NT99340 SVGA                                               \n");	/* OK on DEV and Demo board */
		DBG_PRINTF(" [h] NT99340 UXGA                                               \n");	/* OK on DEV and Demo board */
		DBG_PRINTF(" [i] SP1628 HD                                                  \n");	/* OK on DEV and Demo board */
		DBG_PRINTF(" [j] TW9912 HD                                                  \n");	/* OK on DEV and Demo board */
		
		DBG_PRINTF(" [k] WT8861 VGA                                                 \n");	/* OK on EVB  */
		DBG_PRINTF(" [l] SA71113 VGA                                                \n");	/* OK on EVB  */		
		DBG_PRINTF(" [m] GC0308 VGA                                                 \n");	/* OK on DEV  */
		DBG_PRINTF(" [n] NT99142 VGA                                                \n"); /* OK on EVB board */
		DBG_PRINTF(" [o] NT99142 SVGA                                               \n"); /* OK on EVB board */
		DBG_PRINTF(" [p] NT99142 HD                                                 \n"); /* OK on EVB board */
		DBG_PRINTF(" [A] HM2056 VGA                                                 \n"); /* OK on EVB board */
		DBG_PRINTF(" [B] HM2056 HD720P                                              \n"); /* OK on EVB board */
		DBG_PRINTF(" [C] HM2056 Light Mode                                          \n"); /* OK on EVB board */
		DBG_PRINTF(" [D] NT99141 Motion Detection Demo                              \n"); /* OK on EVB board */
		DBG_PRINTF(" [M] Shared Sensor interface For VIN1 and VIN2                  \n"); /* OK on EVB board */
		DBG_PRINTF(" [N] HM1246 VGA                                                 \n"); /* OK on EVB board */
		DBG_PRINTF(" [O] HM1246 HD720P                                              \n"); /* OK on EVB board */
		DBG_PRINTF(" [P] HM1246 SXGA                                                \n"); /* OK on EVB board */
		DBG_PRINTF("================================================================\n");
		u32Item = sysGetChar();
		
#if defined(__1ST_PORT__) && !defined(__2ND_PORT__)
		sysprintf("Plug in sensor to port 0\n");	
#endif
#if !defined(__1ST_PORT__) && defined(__2ND_PORT__)
		sysprintf("Plug in sensor to port 1\n");	
#endif
#if defined(__1ST_PORT__) && defined(__2ND_PORT__)
		sysprintf("Plug in sensor to port 1 and port 2\n");	
#endif
		switch(u32Item)
		{
			case '1':	
				sysprintf("Enable conditional compile  [OV9660_VGA]\n");											
				Smpl_OV9660_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);		
				break; 	
			case '2':	
				sysprintf("Enable conditional compile  [OV9660_SXGA]\n");											
				Smpl_OV9660_SXGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);		
				break; 	
			case '3':	
				sysprintf("Enable conditional compile  [OV7670_VGA]\n");											
				Smpl_OV7670_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);		
				break; 	
			case '4':	
				sysprintf("Enable conditional compile  [OV7725_VGA]\n");											
				Smpl_OV7725_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);		
				break; 									
			case '5':	
				Smpl_NT99141_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
				break;
			case '6':
				sysSetInterruptPriorityLevel(IRQ_VIN,  2);
				sysSetInterruptPriorityLevel(IRQ_VIN1,  1);
				Smpl_NT99141_HD(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;
			case '7':
				Smpl_NT99050(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;	
			case '8':
				Smpl_NT99160_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;			
			case '9':
				Smpl_NT99160_SVGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;
			case 'a':
				Smpl_NT99160_HD(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;
			case 'b':
				Smpl_HM1375_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;						
			case 'c':
				Smpl_HM1375_HD(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;
				
			case 'd':
				Smpl_NT99340_HD(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
				break;		
			case 'e':
				Smpl_NT99340_FULLHD(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
				break;
			case 'f':
				Smpl_NT99340_QXGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
				break;		
				
			case 'g':
				Smpl_NT99252_SVGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
				break;
			case 'h':
				Smpl_NT99252_UXGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
				break;	
			case 'i':
				Smpl_SP1628_HD(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
				break;	
			case 'j':
				Smpl_TW9912_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
				break;	
			case 'k':
				Smpl_WT8861_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
				break;		
			case 'l':
				Smpl_SA71113_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
				break;	
			case 'm':	
				Smpl_GC0308_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
				break;
				
			case 'n':	
				Smpl_NT99142_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
				break;
			case 'o':
				Smpl_NT99142_SVGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;	
			case 'p':
				Smpl_NT99142_HD(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;		
				
			case 'r':
				Smpl_OV10633_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				/* Hue/Contrast/Brightness/Saturation */
				break;			
			case 's':
				Smpl_OV10633_HD(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;						
			case  't':
				Smpl_SC1046_HD(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;			
			case  'u':
				Smpl_GM7150_OneField(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;	
			case  'v':
				Smpl_GM7150_TwoFields(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;	
			case  'w':
				Smpl_TVP5150_OneField(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;	
			case  'x':
				Smpl_TVP5150_TwoFields(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;						
			case 'y':	
				Smpl_TW9900_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
				break;
				
			case 'A':
				Smpl_HM2056_HD(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;
				
			case 'B':
				Smpl_HM2056_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;
			case 'C':
				Smpl_HM2056_LightMode();
				
			case 'D':	
				Smpl_NT99141_VGA_MotionDetection(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
				break;	
				
			case 'N':
				Smpl_HM1246_VGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;	
			case 'O':
				Smpl_HM1246_HD720P(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;
			case 'P':
				Smpl_HM1246_SXGA(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);	
				break;	
					
			case 'E': /* Encode Planar */
				{
					INT32 count = 0;
					do
					{
						sysDelay(100);							
						
						sysprintf("Encode planar pipe, only for performence test\n");
						
						pVin->SetOperationMode(TRUE);	
						while(pVin->GetOperationMode()==TRUE);
						
						jpegOpen ();
						if(u32PlanarIdx==1)
							JpegEncTest(pVin, 0,  (UINT32)(&u8PlanarFrameBuffer));		/* Current 1, encode planar buffer 0*/
						else
							JpegEncTest(pVin, 0,  (UINT32)(&u8PlanarFrameBuffer1));	/* Current 0, encode planar buffer 1*/
							
						pVin->SetPipeEnable(TRUE, eVIDEOIN_BOTH_PIPE_ENABLE);	
						
						count=count+1;
					}while(count<3);	
				}
				break;
			case 'F': /* Encode Packet */
				sysprintf("Encode packet pipe, only for performence test\n");
				jpegOpen ();			
				if(u32VideoInIdx==0)
					JpegEncTest(pVin, 1,  (UINT32)(&u8PacketFrameBuffer2));	/* Current 0, encode packet buffer 2*/
				else if(u32VideoInIdx==1)
					JpegEncTest(pVin, 1,  (UINT32)(&u8PacketFrameBuffer));		/* Current 1, encode packet buffer 0*/
				else if(u32VideoInIdx==2)		
					JpegEncTest(pVin, 1,  (UINT32)(&u8PacketFrameBuffer1));	/* Current 2, encode packet buffer 1*/
				break;	
			
			case 'I':	
	    				if(pSensorIQ->IQ_SetBrightness==NULL)
	    					break;
	    				while(1)
	    				{
	    					static INT16 level = 0;
	    					
		    				pSensorIQ->IQ_SetBrightness(level);
	    					sysDelay(50);
	    					sysprintf("Brightness level =%d\n", level);
	    					level = level+8;	    					
	    					if(level>255)
	    						level = 0;
	    				}
	    				break;
	    		case 'J':	
	    				if(pSensorIQ->IQ_SetHue==NULL)
	    					break;
	    				while(1)
	    				{
	    					static INT16 angle = 0;
	    					pSensorIQ->IQ_SetHue(angle);
	    					sysDelay(50);
	    					sysprintf("Hue Angle =%d\n", angle);
	    					angle = angle+8;	    					
	    					if(angle>255)
	    						angle = 0;
	    				}	    				
	    				pSensorIQ->IQ_SetHue(u32Item);
	    				break;
	    		case 'K':	
	    				if(pSensorIQ->IQ_SetSharpness==NULL)
	    					break;
	    				while(1)
	    				{
	    					static INT16 level = 0;
	    					INT16 regValue;	    					
	    					pSensorIQ->IQ_SetSharpness(level);
	    					sysDelay(50);
	    					regValue = pSensorIQ->IQ_GetSharpness();
	    					sysprintf("Set sharpness level =%d\n", level);
	    					sysprintf("Get sharpness level =%d\n", regValue);
	    					level = level+8;	    					
	    					if(level>255)
	    						level = 0;
	    				}
	    				break;	
	    		case 'L':	
	    				if(pSensorIQ->IQ_SetContrast==NULL)
	    					break;
	    				while(1)
	    				{
	    					static INT16 level = 0;
	    					INT16 regValue;	    					
	    					pSensorIQ->IQ_SetContrast(level);
	    					sysDelay(50);
	    					regValue = pSensorIQ->IQ_GetContrast();
	    					sysprintf("Set Contrast level =%d\n", level);
	    					sysprintf("Get Contrast level =%d\n", regValue);
	    					level = level+8;	    					
	    					if(level>255)
	    						level = 0;
	    				}
	    				break;
			
	    		case 'M':		
	    					sysprintf("Vin1 Packet 0 = 0x%x\n", (UINT32)u8PacketFrameBuffer);
	    					sysprintf("Vin1 Packet 1 = 0x%x\n", (UINT32)u8PacketFrameBuffer1);
	    					sysprintf("Vin1 Packet 2 = 0x%x\n", (UINT32)u8PacketFrameBuffer2);
	    					sysprintf("Vin1 Planar 0 = 0x%x\n", (UINT32)u8PlanarFrameBuffer);
	    					sysprintf("Vin1 Planar 1 = 0x%x\n", (UINT32)u8PlanarFrameBuffer1);
	    					
	    					sysprintf("Vin2 Packet 0 = 0x%x\n", (UINT32)u8PacketFrameBuffer2_0);
	    					sysprintf("Vin2 Packet 1 = 0x%x\n", (UINT32)u8PacketFrameBuffer2_1);
	    					sysprintf("Vin2 Packet 2 = 0x%x\n", (UINT32)u8PacketFrameBuffer2_2);
	    					sysprintf("Vin2 Planar 0 = 0x%x\n", (UINT32)u8PlanarFrameBuffer2_0);
	    					sysprintf("Vin2 Planar 1 = 0x%x\n", (UINT32)u8PlanarFrameBuffer2_1);
	    					
	    					Smpl_NT99141_HD(u8PacketFrameBuffer, u8PacketFrameBuffer1, u8PacketFrameBuffer2);
	    					Smpl_NT99141_DEV1_HD_DEV2_VGA(u8PacketFrameBuffer2_0, u8PacketFrameBuffer2_1, u8PacketFrameBuffer2_2);
	    					outp32(REG_CHIPCFG, inp32(REG_CHIPCFG) | BIT14);	/* Common sensor mode */
	    					
	    					sysprintf(" VIN1 and VIN2 work together now\n");	    				
	    					sysDelay(500);		/* Delay 5s */
	    					
	    					sysprintf("Disable VIN1 Both pipe \n");
	    					pVin->SetPipeEnable(FALSE, eVIDEOIN_BOTH_PIPE_DISABLE);
	   					sysDelay(100);		/* Delay 1s */
	   					
	    					sysprintf(" Change preview to VIN2 \n");		
	    					InitVPOST_2(u8PacketFrameBuffer2_0);	
	    					
	    					sysprintf("Restart VIN1\n");
	    					pVin->SetPipeEnable(TRUE, eVIDEOIN_BOTH_PIPE_ENABLE);
	    					sysDelay(500);		/* Delay 5s */
	    					
	    					sysprintf("Close VIN2 first\n");
	    					pVin2->Close();		
	    					pVin->Close();
	    					break;					
			default:
				break;										
		}		
	}while((u32Item!= 'q') || (u32Item!= 'Q'));	
	
    	return 0;
} /* end main */
