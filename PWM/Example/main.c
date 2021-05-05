/**************************************************************************//**
 * @file     main.c
 * @brief    Demonstrate how to use PWM counter output/capture waveform.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wbtypes.h"
#include "wblib.h"

#include "PWM.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
BOOL	g_bCapInt = FALSE;
UINT8 volatile g_u8PWMCount = 1;
UINT16	g_u16Frequency;
static	UINT32	s_u32Pulse = 0;
UINT16 g_u16RisingTime, g_u16FallingTime, g_u16HighPeroid, g_u16LowPeroid, g_u16TotalPeroid;
/*---------------------------------------------------------------------------------------------------------*/
/* PWM Timer Callback function                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
VOID PWM_PwmIRQHandler()
{
	if(s_u32Pulse == 1 * g_u16Frequency /10)
	{
	    /*--------------------------------------------------------------------------------------*/
	    /* Stop PWM Timer 0 (Recommended procedure method 2)		                            */
	    /* Set PWM Timer counter as 0, When interrupt request happen, disable PWM Timer			*/				    
	    /*--------------------------------------------------------------------------------------*/			
		PWM_SetTimerCounter(PWM_TIMER0,0);		
	}
	if (s_u32Pulse == 1 * g_u16Frequency /10 + 1)
		g_u8PWMCount = FALSE;
	s_u32Pulse++;
	   
} 

/*---------------------------------------------------------------------------------------------------------*/
/* PWM Capture Callback function                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
VOID PWM_CapIRQHandler()
{
	g_bCapInt = TRUE;	
}

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
VOID CalPeriodTime(UINT8 u8Capture);

/*---------------------------------------------------------------------------------------------------------*/
/* PWM Test Sample 				                                                                           */
/* Test Item					                                                                           */
/*  0. PWM Timer Waveform Test																			   */	 
/*	   Use PWM Timer to create a specified frequency waveform and output to Buzzer.						   */	
/*  1. PWM Caputre Test																					   */
/*     Use PWM Capture to capture PWM timer waveform's property.										   */
/*---------------------------------------------------------------------------------------------------------*/
int main()
{
	WB_UART_T uart;
	PWM_TIME_DATA_T sPt;
	UINT8 u8Item;
	BOOL bLoop = TRUE;
	BOOL bTestLoop = TRUE;
	UINT32 u32ExtFreq,u32PWMClock;
	E_SYS_SRC_CLK eSrcClk;
	PFN_PWM_CALLBACK pfnOldcallback;	
	UINT32 u32Frequency;
	
	u32ExtFreq = sysGetExternalClock();    	/* Hz unit */	
	uart.uart_no = WB_UART_1;
	uart.uiFreq = u32ExtFreq;
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    sysInitializeUART(&uart);
    
    /********************************************************************************************** 
     * Clock Constraints: 
     * (a) If Memory Clock > System Clock, the source clock of Memory and System can come from
     *     different clock source. Suggestion MPLL for Memory Clock, UPLL for System Clock   
     * (b) For Memory Clock = System Clock, the source clock of Memory and System must come from 
     *     same clock source	 
     *********************************************************************************************/
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
    sysSetCPUClock(240000000);
    sysSetAPBClock(60000000);	
#endif	

	sysprintf("\nPWM Sample Demo.\n");   
	
	/* Enable PWM clock */
	PWM_Open();	
	
	u32PWMClock = PWM_GetEngineClock(&eSrcClk);
	sysprintf("Get PWM Clock Source Setting\n");	
	sysprintf("->PWM Default Clock Source is ");	
	
	switch(eSrcClk)
	{
		case eSYS_EXT:
			sysprintf("External Crystal\n");	
			break;
		case eSYS_APLL:
			sysprintf("APLL\n");	
			break;
		case eSYS_UPLL:
			sysprintf("UPLL\n");	
			break;	
		default:		
			return FALSE;			
	}
	sysprintf("->PWM Default Clock is %dkHz\n",u32PWMClock/1000);		
	
	sysprintf("Change PWM Clock Source Setting\n");	
		
	PWM_SetClockSetting(eSYS_UPLL, 4, 2);	/* PWM Egine clock is UPLL / 4, and Engine Clock divider is 2 */
	
	u32PWMClock = PWM_GetEngineClock(&eSrcClk);	
	
	sysprintf("->PWM Clock Source is ");	

	switch(eSrcClk)
	{
		case eSYS_EXT:
			sysprintf("External Crystal\n");	
			break;
		case eSYS_APLL:
			sysprintf("APLL\n");	
			break;
		case eSYS_UPLL:
			sysprintf("UPLL\n");	
			break;			
	}
	sysprintf("->PWM Clock is %dkHz\n",u32PWMClock/1000);		

	while(bLoop)
	{		
		sysprintf("\nPlease Select Test Item\n");
		sysprintf(" 0 : PWM Timer Waveform Test\n");
		sysprintf(" 1 : PWM Caputre Test\n");
		sysprintf(" 2 : Exit\n");		
	
		u8Item = sysGetChar();


		switch(u8Item)
		{
			case '0':		
			{			
				UINT8 u8ItemOK;		
				sysprintf("\nPWM Timer Waveform Test. Waveform output(GPD0) to Buzzer\n");  
				
				bTestLoop = TRUE; 
							
				sysprintf("Select Test Item\n");  
				sysprintf(" 1: Do (256Hz)\n");
				sysprintf(" 2: Re (287Hz)\n");
				sysprintf(" 3: Mi (322Hz)\n");
				sysprintf(" 4: Fa (341Hz)\n");
				sysprintf(" 5: Sol(383Hz) \n");
				sysprintf(" 6: La (430Hz)\n");
				sysprintf(" 7: Si (483Hz)\n");
				sysprintf(" 8: D0 (512Hz)\n");
				sysprintf(" 0: Exit\n");	
										
				while(bTestLoop)
				{					
					u8ItemOK = TRUE;
					u8Item = sysGetChar(); 						

					switch(u8Item)
					{
						case '1':
								g_u16Frequency = 256;
								break;									
						case '2':
								g_u16Frequency = 287;
								break;
						case '3':
								g_u16Frequency = 322;
								break;
						case '4':
								g_u16Frequency = 341;
								break;
						case '5':
								g_u16Frequency = 383;
								break;
						case '6':
								g_u16Frequency = 430;
								break;
						case '7':
								g_u16Frequency = 483;
								break;	
						case '8':
								g_u16Frequency = 512;
								break;											
						case '0':
								bTestLoop = FALSE;
								break;		
						default:
								u8ItemOK = FALSE;
								break;																																																									
				 	}
				 	if(bTestLoop && u8ItemOK)
				 	{
				 		s_u32Pulse = 0;
				 		g_u8PWMCount = TRUE;
				 		
						/* PWM Timer property */ 
						sPt.u8Mode = PWM_TOGGLE_MODE;
						sPt.fFrequency = g_u16Frequency;
						sPt.u8HighPulseRatio = 1;	/* High Pulse peroid : Total Pulse peroid = 1 : 100 */ 
						sPt.bInverter = FALSE;				
					
						/* Set PWM Timer 0 Configuration */
						PWM_SetTimerClk(PWM_TIMER0,&sPt);							

						/* Enable Output for PWM Timer 0 */
						PWM_SetTimerIO(PWM_TIMER0,TRUE);				

						/* Enable Interrupt Sources of PWM Timer 0  */
						PWM_EnableInt(PWM_TIMER0,0);				
							
						/* Install Callback function */	
						PWM_InstallCallBack(PWM_TIMER0, PWM_PwmIRQHandler, &pfnOldcallback);		
							
						/* Enable the PWM Timer 0 */
						PWM_Enable(PWM_TIMER0,TRUE);	
							
						while(g_u8PWMCount);	
							
					    /*--------------------------------------------------------------------------------------*/
					    /* Stop PWM Timer 0 (Recommended procedure method 2)		                            */
					    /* Set PWM Timer counter as 0, When interrupt request happen, disable PWM Timer			*/			
					    /* Set PWM Timer counter as 0 in Call back function										*/					    	    
					    /*--------------------------------------------------------------------------------------*/							
							
						/* Disable the PWM Timer 0 */
						PWM_Enable(PWM_TIMER0,FALSE);	

						/* Disable I & F bit */ 
						PWM_DisableInt(PWM_TIMER0, 0);								
					}
				}	
				break;
			}
			case '1':
			{
				sysprintf("PWM Capture Test\n");
				sysprintf("Use PWM Capture 3 (GPD[3]) to capture the PWM Timer 0 Waveform (GPD[0])\n");		
				sysprintf("PWM Timer 0 Waveform is 250 Hz, and High Pulse Ratio is 30%%\n");	
			    /*--------------------------------------------------------------------------------------*/
			    /* Set the PWM Timer 0 as output function.    				                            */
			    /*--------------------------------------------------------------------------------------*/
			    				    
				/* PWM Timer property for Output waveform */ 
				sPt.u8Mode = PWM_TOGGLE_MODE;
				sPt.fFrequency = 250;			/* 250 Hz */
				sPt.u8HighPulseRatio = 30; 		/* High Pulse peroid : Total Pulse peroid = 30 : 100 */ 
				sPt.bInverter = FALSE;	
								    
			    /* Set PWM Timer 0 Configuration */
				PWM_SetTimerClk(PWM_TIMER0,&sPt);

				/* Enable Output for PWM Timer 0 */
				PWM_SetTimerIO(PWM_TIMER0,TRUE);		
													
				/* Enable the PWM Timer0 */
				PWM_Enable(PWM_TIMER0,TRUE);	
				
			    /*--------------------------------------------------------------------------------------*/
			    /* Set the PWM Capture 3 for capture function 				                            */
			    /*--------------------------------------------------------------------------------------*/					

				/* PWM Timer property for Capture */ 
				sPt.u8Mode = PWM_TOGGLE_MODE;
				sPt.fFrequency = 125;			/* Set the proper frequency to capture data (Less than the input data)*/
				sPt.u8HighPulseRatio = 50;		/* High Pulse peroid : Total Pulse peroid = 50 : 100 (Set a non-zero value) */
				sPt.u32Duty = 0x10000;			/* Set the counter to the maximum value */
				sPt.bInverter = FALSE;		
	
				/* Set PWM Timer 3 for Capture */
				PWM_SetTimerClk(PWM_CAP3,&sPt);				
			
				/* Enable Interrupt Sources of PWM Capture3 */
				PWM_EnableInt(PWM_CAP3, PWM_CAP_FALLING_INT);
							
				/* Enable Input function for PWM Capture 3 */
				PWM_SetTimerIO(PWM_CAP3,TRUE);	
			
				/* Enable the PWM Capture3 */
				PWM_Enable(PWM_CAP3,TRUE);		
			
				/* Capture the Input Waveform Data */
				CalPeriodTime(PWM_CAP3);							

			    /*--------------------------------------------------------------------------------------*/
			    /* Stop PWM Timer 0 (Recommended procedure method 1)		                            */
			    /* Set PWM Timer counter as 0, When PWM internal counter reaches to 0, disable PWM Timer*/				    
			    /*--------------------------------------------------------------------------------------*/					
								
				/* Set PWM Timer 0 counter as 0 */
				PWM_SetTimerCounter(PWM_TIMER0,0);	
							
				 /* Loop when Counter of PWM Timer0 isn't 0 */							
				while(PWM_GetTimerCounter(PWM_TIMER0));										
					
				/* Disable PWM Timer 0 */	
				PWM_Enable(PWM_TIMER0,FALSE);	
			
				/* Disable Output function for PWM Timer 0 */
				PWM_SetTimerIO(PWM_TIMER0,FALSE);					
			
			    /*--------------------------------------------------------------------------------------*/
			    /* Stop PWM Timer 3 (Recommended procedure method 1)		                            */
			    /* Set PWM Timer counter as 0, When PWM internal counter reaches to 0, disable PWM Timer*/				    
			    /*--------------------------------------------------------------------------------------*/								
			
				/* Set PWM Capture 0 counter as 0 */
				PWM_SetTimerCounter(PWM_CAP3,0);
				
				//Wait PWM Capture 3 Counter reach to 0
				while(PWM_GetTimerCounter(PWM_CAP3));				
				
				/* Clear the PWM Capture 3 Interrupt */
				PWM_ClearInt(PWM_CAP3);		
					
										
				/* Disable PWM Capture 3 */	
				PWM_Enable(PWM_CAP3,FALSE);	

				/* Disable Input function for PWM Capture 3 */
				PWM_SetTimerIO(PWM_CAP3,FALSE);		
				
				/* Disable Capture Interrupt */												
				PWM_DisableInt(PWM_CAP3,PWM_CAP_ALL_INT);
				
				u32Frequency = (u32PWMClock / sPt.u8ClockSelector / sPt.u16PreScale)  / g_u16TotalPeroid;				
				
				sysprintf("Captured Signal frequency is %dHz High Pulse Ratio is %d%%\n",u32Frequency, (UINT32)g_u16HighPeroid * 100 / g_u16TotalPeroid);
				
				break;			
			}
			case '2':
			{
				bLoop = FALSE;
				break;			
			}		
		}			
	}	
	sysprintf("PWM Sample Demo End.\n");
	
	PWM_Close();
		
	return 0;
}

/*--------------------------------------------------------------------------------------*/
/* Capture function to calculate the input waveform information                         */
/* u32Count[4] : Keep the internal counter value when input signal rising / falling 	*/
/*				 happens																*/
/*																					    */
/* time	   A    B	  C		D    														*/
/*           ___   ___   ___   ___   ___   ___   ___   ___  							*/
/*      ____|	|_|	  |_|   |_|   |_|	|_|   |_|	|_|   |_____						*/
/* index	          0 1   2 3 														*/
/*																						*/
/* The capture internal counter down count from 0x10000, and reload to 0x10000 after 	*/
/* input signal falling happens	(Time B/C/D)											*/
/*--------------------------------------------------------------------------------------*/								
			
VOID CalPeriodTime(UINT8 u8Capture)
{
	UINT16 u32Count[4];
	UINT32 u32i;	
	
	/* Clear the Capture Interrupt Flag (Time A) */
	PWM_ClearCaptureIntStatus(u8Capture,PWM_CAP_FALLING_FLAG);			
	
	/* Wait for Interrupt Flag  (Falling) */
	while(PWM_GetCaptureIntStatus(u8Capture,PWM_CAP_FALLING_FLAG)!=TRUE);
	
	/* Clear the Capture Interrupt Flag (Time B)*/
	PWM_ClearCaptureIntStatus(u8Capture,PWM_CAP_FALLING_FLAG);
	
	u32i = 0;
	
	while(u32i<4)
	{
		/* Wait for Interrupt Flag (Falling) */
		while(PWM_GetCaptureIntStatus(u8Capture,PWM_CAP_FALLING_FLAG)!=TRUE);
	
		/* Clear the Capture Interrupt Flag */
		PWM_ClearCaptureIntStatus(u8Capture,PWM_CAP_FALLING_FLAG);

		/* Clear the Capture Rising Interrupt Flag */
		PWM_ClearCaptureIntStatus(u8Capture,PWM_CAP_RISING_FLAG);
				
		/* Get the Falling Counter Data */
		u32Count[u32i++] = PWM_GetFallingCounter(u8Capture);
		
		/* Wait for Capture Rising Interrupt Flag */
		while(PWM_GetCaptureIntStatus(u8Capture,PWM_CAP_RISING_FLAG) != TRUE);
		
		/* Clear the Capture Rising Interrupt Flag */
		PWM_ClearCaptureIntStatus(u8Capture,PWM_CAP_RISING_FLAG);		
		
		/* Get the Rising Counter Data */
		u32Count[u32i++] = PWM_GetRisingCounter(u8Capture);	
	}	
	
	g_u16RisingTime = u32Count[1];
	
	g_u16FallingTime = u32Count[0];
	
	g_u16HighPeroid = u32Count[1] - u32Count[2];
	
	g_u16LowPeroid = 0x10000 - u32Count[1];
	
	g_u16TotalPeroid = 0x10000 - u32Count[2];
	
	
	sysprintf("Test Result:\nRising Time = %d, Falling Time = %d.\nHigh Period = %d, Low  Period = %d, Total Period = %d.\n\n",
		g_u16RisingTime, g_u16FallingTime, g_u16HighPeroid, g_u16LowPeroid, g_u16TotalPeroid);
}
