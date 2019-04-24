/***************************************************************************
 *                                                                         									     *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              					     *
 *                                                                         									     *
 ***************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wblib.h"
#include "demo.h"





#define MEM_SIZE			0x100000
#define NON_CACHE_BIT		0x80000000
void DemoClockMemCpy(UINT32 u32Count)
{
	UINT32 tmp;
	char *pUINT8Buf0, *pUINT8Buf1;
	volatile unsigned int  u32Btime, u32Etime;
	
	pUINT8Buf0 = (char*)0x800000;//(char*)malloc(MEM_SIZE);
	pUINT8Buf1 = (char*)0x1000000;//(char*)malloc(MEM_SIZE);
	
	//pUINT8Buf0 = (UINT8*)MEM_BUF1;
	for(tmp=0;tmp<MEM_SIZE; tmp = tmp+1)
	{
		*pUINT8Buf0++ = u32Count+tmp*123;
	}	
	sysSetTimerReferenceClock(TIMER0, 12000000); 		//External Crystal
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);			/* 100 ticks/per sec ==> 1tick/10ms */
	
	sysSetLocalInterrupt(ENABLE_IRQ);	
	pUINT8Buf0 = (char*)0x800000;
	DBG_PRINTF("Src Buf = 0x%x.  Dst Buf= 0x%x\n",(UINT32)pUINT8Buf0 , (UINT32)pUINT8Buf1);
	u32Btime = sysGetTicks(TIMER0);	
	memcpy((UINT8*) ((UINT32)pUINT8Buf0 | NON_CACHE_BIT), 
			(UINT8*)((UINT32)pUINT8Buf1 | NON_CACHE_BIT), 
			MEM_SIZE);
	u32Etime = sysGetTicks(TIMER0);
	DBG_PRINTF("Copy 0x%x bytes memory takes %d ms\n",MEM_SIZE, (u32Etime-u32Btime)*10);
				
}			
void	DemoClockMemCmp(UINT32 u32Count)		
{
//	UINT32 tmp;
	char *pUINT8Buf0, *pUINT8Buf1;
	volatile unsigned int  u32Btime, u32Etime;
	pUINT8Buf0 = (char*)0x800000;//(char*)malloc(MEM_SIZE);
	pUINT8Buf1 = (char*)0x1000000;//(char*)malloc(MEM_SIZE);						
	
	/* Compare time */
	u32Btime = sysGetTicks(TIMER0);
	if(memcmp((UINT8*) ((UINT32)pUINT8Buf0 | NON_CACHE_BIT), 
				(UINT8*)((UINT32)pUINT8Buf1 | NON_CACHE_BIT), 
				MEM_SIZE)!=0)
	{
		DBG_PRINTF("Compare error\n");
		while(1);
	}
	u32Etime = sysGetTicks(TIMER0);
	DBG_PRINTF("Compare 0x%x bytes memory takes %d ms\n", MEM_SIZE, (u32Etime-u32Btime)*10);

}


/* =============================================================
	This enumation is only for clock test mode. 
	Specified clock will be outputed through pin SEN_CLK 
	The register is located CLK_BA[0x30]
==============================================================*/


void DemoAPI_CLK(void)
{
	while(1)
	{//Adjust PLL   
		UINT32 u32Clk;					    	
	   
	    	DBG_PRINTF("***UPLL = 240MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							240000000,		//UINT32 u32PllKHz,
							240000000);		//UINT32 u32SysKHz,
				
		DBG_PRINTF("***UPLL = 324MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							324000000,		//UINT32 u32PllKHz,
							324000000);		//UINT32 u32SysKHz,					
							
	    	u32Clk = 300000000;
	    	while(1)
	    	{
	    		UINT32 u32SysClock;
	    		DBG_PRINTF("***UPLL = %d MHz\n", u32Clk/1000000);
	    		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							u32Clk,		//UINT32 u32PllKHz,
							u32Clk/2);		//UINT32 u32SysKHz,	
			u32Clk = u32Clk - 16000000;	
			u32SysClock = sysGetSystemClock();
			sysprintf("HCLK clock = %d Hz\n", u32SysClock/2);
			if(u32Clk<128000000)
				break;					
		}
		u32Clk = 128000000;
	    	while(1)
	    	{
	    		DBG_PRINTF("***UPLL = %d MHz\n", u32Clk/1000000);
	    		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							u32Clk,		//UINT32 u32PllKHz,
							u32Clk/5);		//UINT32 u32SysKHz,	
			u32Clk = u32Clk + 16000000;	
			if(u32Clk>300000000)
				break;					
		}
							 						
		DBG_PRINTF("***UPLL = 300MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							300000000,		//UINT32 u32PllKHz,
							300000000);		//UINT32 u32SysKHz,		
		
		DBG_PRINTF("***UPLL = 264MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							264000000,		//UINT32 u32PllKHz,
							264000000);		//UINT32 u32SysKHz,												
	 
	 	DBG_PRINTF("***UPLL = 300MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							300000000,		//UINT32 u32PllKHz,
							300000000);		//UINT32 u32SysKHz,		
	#if 1						
		DBG_PRINTF("***UPLL = 264MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							264000000,		//UINT32 u32PllKHz,
							264000000);		//UINT32 u32SysKHz,								
	#endif						
	#if 1
		DBG_PRINTF("***UPLL = 216MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							216000000,		//UINT32 u32PllKHz,
							216000000);		//UINT32 u32SysKHz,
	#endif						
	  	DBG_PRINTF("***UPLL = 264MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							264000000,		//UINT32 u32PllKHz,
							264000000);		//UINT32 u32SysKHz,

		DBG_PRINTF("***UPLL = 240MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							240000000,		//UINT32 u32PllKHz,
							240000000);		//UINT32 u32SysKHz,
		
		DBG_PRINTF("***UPLL = 300MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							300000000,		//UINT32 u32PllKHz,
							300000000);		//UINT32 u32SysKHz,
							
		DBG_PRINTF("***UPLL = 264MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							264000000,		//UINT32 u32PllKHz,
							264000000);		//UINT32 u32SysKHz,			
							
		DBG_PRINTF("***UPLL = 216MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							216000000,		//UINT32 u32PllKHz,
							216000000);		//UINT32 u32SysKHz,					
		
		DBG_PRINTF("***UPLL = 264MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							264000000,		//UINT32 u32PllKHz,
							264000000);		//UINT32 u32SysKHz,					

		DBG_PRINTF("***UPLL = 216MHz								  	\n");				
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							216000000,		//UINT32 u32PllKHz,
							216000000);		//UINT32 u32SysKHz,	
							
		DBG_PRINTF("***UPLL = 300MHz								  	\n");				
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							300000000,		//UINT32 u32PllKHz,
							300000000);		//UINT32 u32SysKHz,	
		
		DBG_PRINTF("***UPLL = 216MHz								  	\n");				
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							216000000,		//UINT32 u32PllKHz,
							216000000);		//UINT32 u32SysKHz,						
		
		DBG_PRINTF("***UPLL = 264MHz								  	\n");			
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							264000000,		//UINT32 u32PllKHz,
							264000000);		//UINT32 u32SysKHz,
							
		DBG_PRINTF("***UPLL = 240MHz								  	\n");				
		sysSetSystemClock(eSYS_UPLL, 	//E_SYS_SRC_CLK eSrcClk,
							240000000,		//UINT32 u32PllKHz,
							240000000);		//UINT32 u32SysKHz,											
		DBG_PRINTF("Done								  	\n");	
	}									    		
}
void Delay(UINT32 u32DelaySec)
{
	volatile unsigned int btime, etime, tmp, tsec;
	btime = sysGetTicks(TIMER0);
	tsec = 0;
	tmp = btime;
	while (1)
	{			
		etime = sysGetTicks(TIMER0);
		if ((etime - btime) >= (100*u32DelaySec))
		{	
			break;		
		}
	}
}

void DemoAPI_ChangeMemoryClock(void);

void DramMaxSpeed(void)
{
	sysprintf("REG_CLKDIV0 = 0x%x\n", inp32(REG_CLKDIV0));
	sysprintf("REG_CLKDIV7 = 0x%x\n", inp32(REG_CLKDIV7));
	sysprintf("MPLL to 360MHz, MCLK= 180MHz\n");						
	sysSetDramClock(eSYS_MPLL, 360000000, 360000000);	
	sysprintf("=================================\n");	
}
void DemoAPI_ChangeMPLL(void)
{
	UINT32 tmp;
	while(1)
	{
			static UINT32 u32Toggle=0;
			
			sysprintf("UPLL to 192MHz. SYS/CPU = 48MHz, HCLK1 = 24MHz\n");
			sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/4);	
			sysprintf("MPLL to 360MHz. MCLK= 180MHz\n");						
			sysSetDramClock(eSYS_MPLL, 360000000, 360000000);	
			u32Toggle=1;
			DramMaxSpeed();			
				
			sysprintf("UPLL to 240MHz. SYS/CPU = 120MHz, HCLK1 = 60MHz\n");
			sysSetSystemClock(eSYS_UPLL, 240000000, 240000000/2);	
			sysprintf("MPLL to 312MHz, MCLK = 156MHz\n");						
			sysSetDramClock(eSYS_MPLL, 312000000, 312000000);
			u32Toggle=1;
			DramMaxSpeed();
			
			sysprintf("UPLL to 144MHz. SYS/CPU = 72MHz, HCLK1 = 36MHz\n");
			sysSetSystemClock(eSYS_UPLL, 144000000, 144000000/2);			
			sysprintf("MPLL to 288MHz,  MCLK = 144MHz\n");
			sysSetDramClock(eSYS_MPLL, 288000000, 288000000);
			u32Toggle=1;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
					
			sysprintf("UPLL to 216MHz. SYS/CPU = 72MHz, HCLK1 = 36MHz\n");		
			sysSetSystemClock(eSYS_UPLL, 216000000, 216000000/3);			
			sysprintf("MPLL to 192MHz, MCLK= 96MHz\n");						
			sysSetDramClock(eSYS_MPLL, 192000000, 192000000);				
			u32Toggle=0;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();

			sysprintf("UPLL to 360MHz. SYS/CPU = 60MHz, HCLK1 = 30MHz\n");
			sysSetSystemClock(eSYS_UPLL, 360000000, 360000000/6);	
			sysprintf("MPLL to 336MHz, MCLK= 168MHz\n");						
			sysSetDramClock(eSYS_MPLL, 336000000, 336000000);						
			u32Toggle=0;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();

			sysprintf("UPLL to 360MHz. SYS/CPU = 90MHz, HCLK1 = 45MHz\n");
			sysSetSystemClock(eSYS_UPLL, 360000000, 360000000/4);	;
			sysprintf("MPLL to 240MHz, MCLK= 120MHz\n");						
			sysSetDramClock(eSYS_MPLL, 240000000, 240000000);	
			u32Toggle=0;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();

			sysprintf("UPLL to 240MHz. SYS/CPU = 30MHz, HCLK1 = 15MHz\n");
			sysSetSystemClock(eSYS_UPLL, 240000000, 240000000/8);
			sysprintf("MPLL to 316MHz, MCLK= 158MHz\n");						
			sysSetDramClock(eSYS_MPLL, 316000000, 316000000);	
			u32Toggle=1;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
					
			sysprintf("UPLL to 240MHz. SYS/CPU = 30MHz, HCLK1 = 15MHz\n");		
			sysSetSystemClock(eSYS_UPLL, 240000000, 240000000/8);		
			sysprintf("MPLL to 216MHz,  MCLK= 108MHz\n");						
			sysSetDramClock(eSYS_MPLL, 216000000, 216000000);					
			u32Toggle=0;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();

			sysprintf("UPLL to 316MHz. SYS/CPU = 158MHz, HCLK1 = 79MHz\n");
			sysSetSystemClock(eSYS_UPLL, 316000000, 316000000/2);		
			sysprintf("MPLL to 360MHz, MCLK= 180MHz\n");						
			sysSetDramClock(eSYS_MPLL, 360000000, 360000000);				
			u32Toggle=0;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();

			sysprintf("UPLL to 192MHz. SYS/CPU = 6MHz, HCLK1 = 3MHz\n");
			sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/32);	
			sysprintf("MPLL to 264MHz, , MCLK= 132MHz\n");						
			sysSetDramClock(eSYS_MPLL, 264000000, 264000000);					
			u32Toggle=0;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
												
			sysprintf("UPLL to 316MHz. SYS/CPU = 105.333MHz, HCLK1 = 52.666MHz\n");			
			sysSetSystemClock(eSYS_UPLL, 316000000, 316000000/3);				
			sysprintf("MPLL to 216MHz, MCLK= 108MHz\n");						
			sysSetDramClock(eSYS_MPLL, 216000000, 216000000);					
			u32Toggle=0;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
			sysprintf("UPLL to 360MHz. SYS/CPU = 90MHz, HCLK1 = 45MHz\n");
			sysSetSystemClock(eSYS_UPLL, 360000000, 360000000/4);
			sysprintf("MPLL to 360MHz, MCLK= 180MHz\n");						
			sysSetDramClock(eSYS_MPLL, 360000000,360000000);					
			u32Toggle=0;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
			sysprintf("UPLL to 316MHz. SYS/CPU = 48MHz, HCLK1 = 24MHz\n");
			sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/4);
			sysprintf("MPLL to 360MHz, MCLK= 90MHz\n");						
			sysSetDramClock(eSYS_MPLL, 360000000, 360000000/2);	
		//	u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
						
			sysprintf("UPLL to 192MHz. SYS/CPU = 24MHz, HCLK1 = 12MHz\n");			
			sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/8);			
			sysprintf("MPLL to 360MHz, MCLK= 22.5MHz\n");						
			sysSetDramClock(eSYS_MPLL, 360000000, 360000000/8);					
			u32Toggle=0;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
			sysprintf("UPLL to 192MHz. SYS/CPU = 96MHz, HCLK1 = 48MHz\n");
			sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/2);	
			sysprintf("MPLL to 216MHz, MCLK= 108MHz\n");						
			sysSetDramClock(eSYS_MPLL, 216000000, 216000000);					
			u32Toggle=0;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
			sysprintf("UPLL to 192MHz. SYS/CPU = 9.6MHz, HCLK1 = 4.8MHz\n");
			sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/20);	
			sysprintf("MPLL to 360MHz, MCLK= 22.5MHz\n");						
			sysSetDramClock(eSYS_MPLL, 360000000, 360000000/8);					
			u32Toggle=0;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
								
			sysprintf("UPLL to 360MHz. SYS/CPU = 180MHz, HCLK1 = 90MHz\n");
			sysSetSystemClock(eSYS_UPLL, 360000000, 360000000/2);
			sysprintf("MPLL to 360MHz, MCLK= 180MHz\n");						
			sysSetDramClock(eSYS_MPLL, 360000000, 360000000);					
			u32Toggle=0;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
			
			sysprintf("UPLL to 192MHz. SYS/CPU = 4.8MHz, HCLK1 = 2.4MHz\n");
			sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/40);
			sysprintf("MPLL to 360MHz, MCLK= 9MHz\n");						
			sysSetDramClock(eSYS_MPLL, 360000000, 360000000/20);						
			u32Toggle=0;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
			sysprintf("UPLL to 360MHz, SYS = 5.625MHz, HCLK/HCLK1 = 2.8125MHz\n");
			sysSetSystemClock(eSYS_UPLL, 360000000, 360000000/(8*8));
			sysprintf("MPLL to 320MHz, MCLK= 2.5MHz\n");						
			sysSetDramClock(eSYS_MPLL, 320000000, 320000000/(8*8));					
			u32Toggle=0;
		//	for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
	}	
}

void DemoAPI_ChangeMPLL_FromOtherPLL(void)
{
	UINT32 tmp;
	while(1)
	{
			static UINT32 u32Toggle=0;
			
			sysprintf("UPLL to 192MHz. SYS/CPU = 48MHz, HCLK1 = 24MHz\n");
			sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/4);	
			sysprintf("APLL to 360MHz. MCLK= 90MHz\n");						
			sysSetDramClock(eSYS_APLL, 360000000, 360000000/2);	
			u32Toggle=1;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();			

			sysprintf("UPLL to 240MHz. SYS/CPU = 120MHz, HCLK1 = 60MHz\n");
			sysSetSystemClock(eSYS_UPLL, 240000000, 240000000/2);	
			sysprintf("MPLL to 312MHz, MCLK = 156MHz\n");						
			sysSetDramClock(eSYS_MPLL, 312000000, 312000000);
			u32Toggle=1;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
			sysprintf("UPLL to 144MHz. SYS/CPU = 72MHz, HCLK1 = 36MHz\n");
			sysSetSystemClock(eSYS_UPLL, 144000000, 144000000/2);			
			sysprintf("APLL to 288MHz,  MCLK = 144MHz\n");
			sysSetDramClock(eSYS_APLL, 288000000, 288000000);
			u32Toggle=1;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
					
			sysprintf("UPLL to 360MHz. SYS/CPU = 60MHz, HCLK1 = 30MHz\n");		
			sysSetSystemClock(eSYS_UPLL, 360000000, 360000000/6);			
			sysprintf("MPLL to 192MHz, MCLK= 96MHz\n");						
			sysSetDramClock(eSYS_MPLL, 192000000, 192000000);				
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();

			sysprintf("UPLL to 360MHz. SYS/CPU = 60MHz, HCLK1 = 30MHz\n");	
			sysSetSystemClock(eSYS_UPLL, 360000000, 360000000/6);	
			sysprintf("APLL to 336MHz, MCLK= 168MHz\n");						
			sysSetDramClock(eSYS_APLL, 336000000, 336000000);						
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();

			sysprintf("UPLL to 360MHz. SYS/CPU = 90MHz, HCLK1 = 45MHz\n");
			sysSetSystemClock(eSYS_UPLL, 360000000, 360000000/4);	;
			sysprintf("APLL to 240MHz, MCLK= 120MHz\n");						
			sysSetDramClock(eSYS_APLL, 240000000, 240000000);	
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();

			sysprintf("UPLL to 240MHz. SYS/CPU = 30MHz, HCLK1 = 15MHz\n");
			sysSetSystemClock(eSYS_UPLL, 240000000, 240000000/8);
			sysprintf("MPLL to 316MHz, MCLK= 158MHz\n");						
			sysSetDramClock(eSYS_MPLL, 316000000, 316000000);	
			u32Toggle=1;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
					
			sysprintf("UPLL to 240MHz. SYS/CPU = 30MHz, HCLK1 = 15MHz\n");		
			sysSetSystemClock(eSYS_UPLL, 240000000, 240000000/8);		
			sysprintf("APLL to 216MHz,  MCLK= 108MHz\n");						
			sysSetDramClock(eSYS_APLL, 216000000, 216000000);					
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();

			sysprintf("UPLL to 316MHz. SYS/CPU = 158MHz, HCLK1 = 79MHz\n");
			sysSetSystemClock(eSYS_UPLL, 316000000, 316000000/2);		
			sysprintf("MPLL to 360MHz, MCLK= 180MHz\n");						
			sysSetDramClock(eSYS_MPLL, 360000000, 360000000);				
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();

			sysprintf("UPLL to 192MHz. SYS/CPU = 6MHz, HCLK1 = 3MHz\n");
			sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/32);	
			sysprintf("MPLL to 264MHz, , MCLK= 132MHz\n");						
			sysSetDramClock(eSYS_MPLL, 264000000, 264000000);					
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
												
			sysprintf("UPLL to 316MHz. SYS/CPU = 105.333MHz, HCLK1 = 52.666MHz\n");			
			sysSetSystemClock(eSYS_UPLL, 316000000, 316000000/3);				
			sysprintf("MPLL to 216MHz, MCLK= 108MHz\n");						
			sysSetDramClock(eSYS_MPLL, 216000000, 216000000);					
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
			sysprintf("UPLL to 360MHz. SYS/CPU = 90MHz, HCLK1 = 45MHz\n");
			sysSetSystemClock(eSYS_UPLL, 360000000, 360000000/4);
			sysprintf("MPLL to 360MHz, MCLK= 90MHz\n");						
			sysSetDramClock(eSYS_MPLL, 360000000, 360000000/2);					
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
			sysprintf("UPLL to 192MHz. SYS/CPU = 48MHz, HCLK1 = 24MHz\n");
			sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/4);
			sysprintf("MPLL to 360MHz, MCLK= 90MHz\n");						
			sysSetDramClock(eSYS_MPLL, 360000000, 360000000/2);				
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
						
			sysprintf("UPLL to 192MHz. SYS/CPU = 24MHz, HCLK1 = 12MHz\n");			
			sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/8);			
			sysprintf("MPLL to 360MHz, MCLK= 45MHz\n");						
			sysSetDramClock(eSYS_MPLL, 360000000, 360000000/4);				
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
			sysprintf("UPLL to 316MHz. SYS/CPU = 96MHz, HCLK1 = 48MHz\n");
			sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/2);	
			sysprintf("MPLL to 216MHz, MCLK= 108MHz\n");						
			sysSetDramClock(eSYS_MPLL, 216000000, 216000000);					
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
			sysprintf("UPLL to 192MHz. SYS/CPU = 9.6MHz, HCLK1 = 4.8MHz\n");
			sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/20);	
			sysprintf("MPLL to 360MHz, MCLK= 10MHz\n");	//(MCLK>HCLK)
			sysSetDramClock(eSYS_MPLL, 360000000, 360000000/18);					
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
			DemoAPI_ChangeMemoryClock();
			
			sysprintf("UPLL to 360MHz. SYS/CPU = 180MHz, HCLK1 = 90MHz\n");
			sysSetSystemClock(eSYS_UPLL, 360000000, 360000000/2);
			sysprintf("MPLL to 360MHz, MCLK= 180MHz\n");						
			sysSetDramClock(eSYS_MPLL, 360000000, 360000000);					
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
			
			sysprintf("UPLL to 192MHz. SYS/CPU = 4.8MHz, HCLK1 = 2.4MHz\n");
			sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/40);
			sysprintf("MPLL to 360MHz, MCLK= 5MHz\n");						
			sysSetDramClock(eSYS_MPLL, 360000000, 360000000/36);					
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
			sysprintf("UPLL to 360MHz, SYS = 5.625MHz, HCLK/HCLK1 = 2.8125MHz\n");
			sysSetSystemClock(eSYS_UPLL, 360000000, 360000000/(8*8));
			sysprintf("MPLL to 320MHz, MCLK= 10MHz\n");						
			sysSetDramClock(eSYS_MPLL, 320000000, 320000000/(4*4));					
			u32Toggle=0;
			for(tmp=0; tmp<1000000; tmp++);
			DramMaxSpeed();
			
	}	
}



void DemoAPI_ChangeSystemClock(void)
{
	while(1)
	{//		
		sysprintf("UPLL to 240MHz. SYS= 240MHz\n");
		sysSetSystemClock(eSYS_UPLL, 240000000, 240000000/16);	
		sysprintf("REG_UPLL = %d\n", inp32(REG_UPLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("UPLL to 144MHz. SYS= 18MHz\n");		
		sysSetSystemClock(eSYS_UPLL, 144000000, 144000000/8);	
		sysprintf("REG_UPLL = %x\n", inp32(REG_UPLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("UPLL to 240MHz. SYS= 60MHz\n");
		sysSetSystemClock(eSYS_UPLL, 240000000, 240000000/4);	
		sysprintf("REG_UPLL = %x\n", inp32(REG_UPLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));		
		
		sysprintf("APLL to 240MHz. SYS= 240MHz\n");
		sysSetSystemClock(eSYS_APLL, 240000000, 240000000/16);	
		sysprintf("REG_APLL = %d\n", inp32(REG_APLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("APLL to 144MHz. SYS= 18MHz\n");		
		sysSetSystemClock(eSYS_APLL, 144000000, 144000000/8);	
		sysprintf("REG_APLL = %x\n", inp32(REG_APLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("APLL to 240MHz. SYS= 60MHz\n");
		sysSetSystemClock(eSYS_APLL, 240000000, 240000000/4);	
		sysprintf("REG_APLL = %x\n", inp32(REG_APLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		sysprintf("======================\n");
		
		sysprintf("UPLL to 144MHz. SYS= 72MHz\n");		
		sysSetSystemClock(eSYS_UPLL, 144000000, 144000000/2);	
		sysprintf("REG_UPLL = %x\n", inp32(REG_UPLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("UPLL to 240MHz. SYS= 3.75MHz\n");		
		sysSetSystemClock(eSYS_UPLL, 240000000, 240000000/(16*8));	
		sysprintf("REG_UPLL = %x\n", inp32(REG_UPLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("UPLL to 240MHz. SYS= 3.75MHz\n");		
		sysSetSystemClock(eSYS_UPLL, 240000000, 240000000/(16*8));	
		sysprintf("REG_UPLL = %x\n", inp32(REG_UPLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("APLL to 144MHz. SYS= 72MHz\n");		
		sysSetSystemClock(eSYS_APLL, 144000000, 144000000/2);	
		sysprintf("REG_APLL = %x\n", inp32(REG_APLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("APLL to 240MHz. SYS= 3.75MHz\n");		
		sysSetSystemClock(eSYS_APLL, 240000000, 240000000/(16*8));	
		sysprintf("REG_APLL = %x\n", inp32(REG_APLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("APLL to 240MHz. SYS= 3.75MHz\n");		
		sysSetSystemClock(eSYS_APLL, 240000000, 240000000/(16*8));	
		sysprintf("REG_APLL = %x\n", inp32(REG_APLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		sysprintf("======================\n");
		
		sysprintf("UPLL to 144MHz. SYS= 24MHz\n");		
		sysSetSystemClock(eSYS_UPLL, 144000000, 144000000/6);	
		sysprintf("REG_UPLL = %x\n", inp32(REG_UPLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("UPLL to 240MHz. SYS= 40MHz\n");		
		sysSetSystemClock(eSYS_UPLL, 240000000, 240000000/6);	
		sysprintf("REG_UPLL = %x\n", inp32(REG_UPLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("UPLL to 192MHz. SYS= 24MHz\n");		
		sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/8);	
		sysprintf("REG_UPLL = %x\n", inp32(REG_UPLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("APLL to 144MHz. SYS= 24MHz\n");		
		sysSetSystemClock(eSYS_APLL, 144000000, 144000000/6);	
		sysprintf("REG_APLL = %x\n", inp32(REG_APLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("APLL to 240MHz. SYS= 40MHz\n");		
		sysSetSystemClock(eSYS_APLL, 240000000, 240000000/6);	
		sysprintf("REG_APLL = %x\n", inp32(REG_APLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("APLL to 192MHz. SYS= 24MHz\n");		
		sysSetSystemClock(eSYS_APLL, 192000000, 192000000/8);	
		sysprintf("REG_APLL = %x\n", inp32(REG_APLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		sysprintf("======================\n");
		
		sysprintf("UPLL to 240MHz. SYS= 240MHz\n");		
		sysSetSystemClock(eSYS_UPLL, 240000000, 240000000);	
		sysprintf("REG_UPLL = %x\n", inp32(REG_UPLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
		
		sysprintf("APLL to 240MHz. SYS= 15MHz\n");		
		sysSetSystemClock(eSYS_APLL, 240000000, 15000000);	
		sysprintf("REG_APLL = %x\n", inp32(REG_APLLCON));
		sysprintf("REG_CLKDIV0 = %x\n\n", inp32(REG_CLKDIV0));
		
	}
}
void DemoAPI_ChangeMemoryClock(void)
{
	INT32 tmp, i32Mdiv;
	sysprintf("UPLL to 192MHz. SYS/CPU = 9.6MHz, HCLK1 = 4.8MHz\n");
	sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/20);	
	
	for(i32Mdiv=20; i32Mdiv>=2; i32Mdiv=i32Mdiv-1)
	{						
		sysSetDramClock(eSYS_MPLL, 360000000, 360000000/i32Mdiv);	
		sysprintf("MPLL to 360MHz, MCLK= %dHz\n", 360000000/2/i32Mdiv);				
		for(tmp=0; tmp<1000000; tmp++);
		DramMaxSpeed();
	}
}	
