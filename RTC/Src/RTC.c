/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------------------------*/
/* Includes of system headers                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wbio.h"
#include "wblib.h"
#include "wbtypes.h"
#include "RTC.h"


//#define __FPGA__
/*---------------------------------------------------------------------------------------------------------*/
/* Macro, type and constant definitions                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define RTC_GLOBALS

//#define RTC_DEBUG
#ifdef RTC_DEBUG
#define RTCDEBUG     sysprintf
#else
#define RTCDEBUG(...)
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Global file scope (static) variables                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
static PFN_RTC_CALLBACK  *g_pfnRTCCallBack_Tick    = NULL, *g_pfnRTCCallBack_Alarm   = NULL, *g_pfnRTCCallBack_PSWI   = NULL, *g_pfnRTCCallBack_Relative_Alarm =NULL;
                   
static UINT32 volatile g_u32RTC_Count  = 0;
static CHAR g_chHourMode = 0;
static BOOL volatile g_bIsEnableTickInt  = FALSE;
static BOOL volatile g_bIsEnableAlarmInt = FALSE;

static UINT32 volatile g_u32Reg, g_u32Reg1,g_u32hiYear,g_u32loYear,g_u32hiMonth,g_u32loMonth,g_u32hiDay,g_u32loDay;
static UINT32 volatile g_u32hiHour,g_u32loHour,g_u32hiMin,g_u32loMin,g_u32hiSec,g_u32loSec;
UINT32 volatile i, Wait;

VOID RTC_Check(void)
{
    i =0;
    Wait = inp32(REG_FLAG) & RTC_REG_FLAG;
    
    while(Wait!=RTC_REG_FLAG)
    {
    	Wait = inp32(REG_FLAG) & RTC_REG_FLAG;
    	i++;
    	if(i > RTC_DELAY)
    	{
    		//sysprintf("Time out\n");
    		break;
    	}
    }      
}
/*---------------------------------------------------------------------------------------------------------*/
/* Functions                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:     <RTC_ISR>                                                                                 */
/*                                                                                                         */
/* Parameter:                                                                                              */
/*               VOID                                                                                      */
/* Returns:                                                                                                */
/*               None                                                                                      */
/* Side effects:                                                                                           */
/*                                                                                                         */
/* Description:                                                                                            */
/*               Install ISR to handle interrupt event                                                     */
/*---------------------------------------------------------------------------------------------------------*/

static VOID RTC_ISR (void)
{ 
    UINT32 volatile u32RegRIIR;
    
#ifdef __LIBRARY__   
	outp32(REG_APBCLK,inp32(REG_APBCLK) | RTC_CKE);
#endif	
    u32RegRIIR = inp32(RIIR);
    if (u32RegRIIR & RTC_TICK_INT)                                     /* tick interrupt occurred */
    {
		outp32(RIIR, RTC_TICK_INT);
		RTC_Check(); 
		
		g_u32RTC_Count++;                                              /* maintain RTC tick count */
#ifdef __LIBRARY__   		
		outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);	
#endif

		if (g_pfnRTCCallBack_Tick != NULL)                             /* execute tick callback function */
		{
		  g_pfnRTCCallBack_Tick();
		}

    }
    if (u32RegRIIR & RTC_ALARM_INT)                                    /* absolute alarm interrupt occurred */
	{    
		outp32(RIIR, RTC_ALARM_INT);
		RTC_Check(); 

#ifdef __LIBRARY__   		
		outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);	
#endif
		
		if (g_pfnRTCCallBack_Alarm != NULL)                           	/* execute absolute alarm callback function */
		{
		  g_pfnRTCCallBack_Alarm();
		}
	}
	if (u32RegRIIR & RTC_RELATIVE_ALARM_INT)                            /* relative alarm interrupt occurred */
	{    
		outp32(RIIR, RTC_RELATIVE_ALARM_INT);
		RTC_Check(); 

		RTC_Ioctl(0,RTC_IOC_DISABLE_INT,RTC_RELATIVE_ALARM_INT,0);

#ifdef __LIBRARY__   		
		outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);	
#endif
		
		if (g_pfnRTCCallBack_Relative_Alarm != NULL)               		/* execute relative alarm callback function */
		{          
			g_pfnRTCCallBack_Relative_Alarm();
			g_pfnRTCCallBack_Relative_Alarm = NULL;
		}

	}   
    if (u32RegRIIR & RTC_PSWI_INT)                                    	/* power key interrupt occurred */
    {    
		outp32(RIIR, RTC_PSWI_INT);
		RTC_Check(); 
		
#ifdef __LIBRARY__   		
		outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);	
#endif
		//RTC_Ioctl(0,RTC_IOC_DISABLE_INT,RTC_PSWI_INT,0);

		if (g_pfnRTCCallBack_PSWI != NULL)                            	/* execute power key callback function */
		{
		  g_pfnRTCCallBack_PSWI();
		}
	}    
	
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:     RTC_DoFrequencyCompensation                 	                                           */
/*                                                                                                         */
/* Parameter:                                                                                              */
/*               float number                                                                              */
/* Returns:                                                                                                */
/*               E_SUCCESS					Success.                                                       */
/*               E_RTC_ERR_FCR_VALUE		Wrong Compenation VALUE                                        */
/* Side effects:                                                                                           */
/*                                                                                                         */
/* DESCRIPTION                                                                                             */
/*                                                                                                         */
/*               Set Frequecy Compenation Data                                                             */
/*---------------------------------------------------------------------------------------------------------*/
UINT32 RTC_DoFrequencyCompensation(void)
{
   	UINT32 u32Clock,integer, fraction,count;
   	float  fTmp;
#ifdef __LIBRARY__      	
	outp32(REG_APBCLK,inp32(REG_APBCLK) | RTC_CKE);
#endif

	outp32(RTC_FCR,inp32(RTC_FCR) | FC_EN);
	RTC_Check();
	while(inp32(RTC_FCR) & FC_EN);	

#ifdef __FPGA__
	u32Clock = 27000000;
#else
	u32Clock = sysGetAPBClock();
#endif		
	count = inp32(REG_1Hz_CNT);

	//("\nOSC_32K_CNT (APB %d) 0x%X\n\n",u32Clock,count); 
	
	if(count > 0)
	{
		fTmp = ((float) u32Clock / count) * 32768;
		
		integer = (UINT32) fTmp - 1;
		
		fraction = (UINT32)((fTmp - (integer + 1)) * 60) - 1;
		
		sysprintf("OSC_32K_CNT (APB %d) 0x%X 0x%X\n\n",u32Clock,integer,fraction); 
		/*-----------------------------------------------------------------------------------------------------*/
	    /* Judge Interger part is reasonable                                                                   */
	    /*-----------------------------------------------------------------------------------------------------*/

	  /*  if ((integer > 0xFFFF))
	    {
	        return E_RTC_ERR_FCR_VALUE ;
	    }*/
	}
        			
	outp32(RTC_FCR, (inp32(RTC_FCR) & ~(INTEGER |FRACTION)) | (((integer & 0xFFFF) << 8) | (fraction & 0x3F)));
	RTC_Check();
#ifdef __LIBRARY__   
	outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif	
    return E_RTC_SUCCESS;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:     RTC_WriteEnable                                                                           */
/*                                                                                                         */
/* Parameter:                                                                                              */
/*               bEnable					TRUE/FALSE                                                     */
/* Returns:                                                                                                */
/*               E_SUCCESS		            Success.                                                       */
/*               E_RTC_ERR_FAILED       	FAILED                                                         */
/* Side effects:                                                                                           */
/*                                                                                                         */
/* DESCRIPTION                                                                                             */
/*                                                                                                         */
/*               Access PW to AER to make access other register enable                                     */
/*---------------------------------------------------------------------------------------------------------*/
UINT32 RTC_WriteEnable (BOOL bEnable)
{
    INT32 volatile i32i;
#ifdef __LIBRARY__   	
	outp32(REG_APBCLK,inp32(REG_APBCLK) | RTC_CKE);
#endif	
	
	if(!bEnable)
		RTC_Check();
	
	if(bEnable)
	{
	    outp32(AER, RTC_WRITE_KEY);


	    for (i32i = 0 ; i32i < RTC_WAIT_COUNT ; i32i++)
	    {
	        /*-------------------------------------------------------------------------------------------------*/
	        /* check RTC_AER[16] to find out RTC write enable                                                  */
	        /*-------------------------------------------------------------------------------------------------*/
	        if ( inp32(AER) & 0x10000)
	        {
	            break;
	        }
	    }

	    if (i32i == RTC_WAIT_COUNT)
	    {
	        RTCDEBUG ("\nRTC: 3, set write enable FAILED!\n");
#ifdef __LIBRARY__   	        
	        outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif
	        return E_RTC_ERR_EIO;
	    }
	}
	else
	{
		outp32(AER, 0);	
	    for (i32i = 0 ; i32i < RTC_WAIT_COUNT ; i32i++)
	    {
	        /*-------------------------------------------------------------------------------------------------*/
	        /* check RTC_AER[16] to find out RTC write enable                                                  */
	        /*-------------------------------------------------------------------------------------------------*/
	        if ( inp32(AER) == 0)
	        {
	            break;
	        }
	    }		
		
		
		
	}	
	RTC_Check();
#ifdef __LIBRARY__   	
	outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif	
    return E_RTC_SUCCESS;
}

/*---------------------------------------------------------------------------------------------------------*/
UINT32 RTC_WriteEnabled (BOOL bEnable)
{
    INT32 volatile i32i;
#ifdef __LIBRARY__   	
	outp32(REG_APBCLK,inp32(REG_APBCLK) | RTC_CKE);
#endif
	
	if(bEnable)
	{
	    outp32(AER, RTC_WRITE_KEY);


	    for (i32i = 0 ; i32i < RTC_WAIT_COUNT ; i32i++)
	    {
	        /*-------------------------------------------------------------------------------------------------*/
	        /* check RTC_AER[16] to find out RTC write enable                                                  */
	        /*-------------------------------------------------------------------------------------------------*/
	        if ( inp32(AER) & 0x10000)
	        {
	            break;
	        }
	    }

	    if (i32i == RTC_WAIT_COUNT)
	    {
	        RTCDEBUG ("\nRTC: 3, set write enable FAILED!\n");
#ifdef __LIBRARY__   	        
	        outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif
	        return E_RTC_ERR_EIO;
	    }
	}
	else
		outp32(AER, 0);	
#ifdef __LIBRARY__   
	outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif
    return E_RTC_SUCCESS;
}
/*---------------------------------------------------------------------------------------------------------*/
/* Function:     RTC_Init        	                                                                       */
/*                                                                                                         */
/* Parameter:                                                                                              */
/*               VOID                                                                                      */
/* Returns:                                                                                                */
/*               E_SUCCESS            	Success.                                                           */
/*               E_RTC_ERR_EIO			Initial RTC FAILED.    	                                           */
/* Side effects:                                                                                           */
/*                                                                                                         */
/* DESCRIPTION                                                                                             */
/*                                                                                                         */
/*               Initial RTC and install ISR                                                               */
/*---------------------------------------------------------------------------------------------------------*/


UINT32 RTC_Init (void)
{
    INT32 i32i;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Initial time data struct and some parameters.                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    g_pfnRTCCallBack_Alarm = NULL;
    g_pfnRTCCallBack_Tick = NULL;
	g_pfnRTCCallBack_PSWI = NULL;
	g_pfnRTCCallBack_Relative_Alarm = NULL;
    g_u32RTC_Count = 0;
    /*-----------------------------------------------------------------------------------------------------*/
    /* When RTC is power on, write 0xa5eb1357 to RTC_INIR to reset all logic.                              */
    /*-----------------------------------------------------------------------------------------------------*/
#ifdef __LIBRARY__       
    outp32(REG_APBCLK,inp32(REG_APBCLK) | RTC_CKE);
#endif
    
    outp32(INIR, RTC_INIT_KEY);

    for (i32i = 0 ; i32i < RTC_WAIT_COUNT ; i32i++)
    {
        if ( inp32(INIR) & 0x01 )
        {            /* Check RTC_INIR[0] to find out RTC reset signal */
            break;

        }
    }
        
    if (i32i == RTC_WAIT_COUNT)
    {
        RTCDEBUG("\nRTC: RTC_Init, initial RTC FAILED!\n");
        return E_RTC_ERR_EIO;
    }
    Wait = inp32(REG_FLAG) & RTC_REG_FLAG;
    
    while(1)
    {
    	 Wait = inp32(REG_FLAG) & RTC_REG_FLAG;
    	 if(Wait)
    	 {
	    	 break;
	   	}
    }  	
    /*-----------------------------------------------------------------------------------------------------*/
    /* Install RTC ISR                                                              					   */
    /*-----------------------------------------------------------------------------------------------------*/

  	outp32(AER, RTC_WRITE_KEY);

    for (i32i = 0 ; i32i < RTC_WAIT_COUNT ; i32i++)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* check RTC_AER[16] to find out RTC write enable                                                  */
        /*-------------------------------------------------------------------------------------------------*/
        if ( inp32(AER) & 0x10000)
        {
            break;
        }
    }

    if (i32i == RTC_WAIT_COUNT)
    {
        RTCDEBUG ("\nRTC: 3, set write enable FAILED!\n");
        return E_RTC_ERR_EIO;
    }
    Wait = inp32(REG_FLAG) & RTC_REG_FLAG;
    
    while(1)
    {
    	 Wait = inp32(REG_FLAG) & RTC_REG_FLAG;
    	 if(Wait)
    	 {
	    	 break;
	   	}
    }  	 	       
#ifdef __LIBRARY__   
    outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif
    
	sysInstallISR(IRQ_LEVEL_1, IRQ_RTC, (PVOID)RTC_ISR);	
	sysSetLocalInterrupt(ENABLE_IRQ);
	sysEnableInterrupt(IRQ_RTC);	    
	
    return E_RTC_SUCCESS;
}


/*---------------------------------------------------------------------------------------------------------*/
/* Function:     RTC_Open                                                                                  */
/*                                                                                                         */
/* Parameter:    RTC_TIME_DATA_T *sPt            Just Set Current_Timer   		                           */
/*                                                                                                         */
/* Returns:                                                                                                */
/*               E_SUCCESS        		Success.                                                           */
/*               E_RTC_ERR_EIO		Initial RTC FAILED.                             	                   */
/* Side effects:                                                                                           */
/*                                                                                                         */
/* DESCRIPTION                                                                                             */
/*               Just Set Current_Timer .                                                                  */
//*--------------------------------------------------------------------------------------------------------*/

UINT32 RTC_Open (RTC_TIME_DATA_T *sPt)
{
    UINT32 volatile u32Reg;
    
    /*-----------------------------------------------------------------------------------------------------*/
    /* DO BASIC JUDGEMENT TO Check RTC time data value is reasonable or not.                               */
    /*-----------------------------------------------------------------------------------------------------*/
    if ( ((sPt->u32Year - RTC_YEAR2000) > 99)|
         ((sPt->u32cMonth == 0) || (sPt->u32cMonth > 12))|
         ((sPt->u32cDay   == 0) || (sPt->u32cDay   > 31)))
    {
        return E_RTC_ERR_CALENDAR_VALUE;
    }

    if (sPt->u8cClockDisplay == RTC_CLOCK_12)
    {
        if ( (sPt->u32cHour == 0) || (sPt->u32cHour > 12) )
        {
            return E_RTC_ERR_TIMESACLE_VALUE ;
        }
    }
    else if (sPt->u8cClockDisplay == RTC_CLOCK_24)
    {
        if (sPt->u32cHour > 23)
        {
            return E_RTC_ERR_TIMESACLE_VALUE ;
        }
    }
    else
    {
        return E_RTC_ERR_TIMESACLE_VALUE ;
    }

    if ((sPt->u32cMinute > 59) |
        (sPt->u32cSecond > 59) |
        (sPt->u32cSecond > 59))
    {
        return E_RTC_ERR_TIME_VALUE ;
    }
    if (sPt->u32cDayOfWeek > 6)
    {
        return E_RTC_ERR_DWR_VALUE ;
    }
#ifdef __LIBRARY__       
    outp32(REG_APBCLK,inp32(REG_APBCLK) | RTC_CKE);
#endif
    
    /*-----------------------------------------------------------------------------------------------------*/
    /* Second, set RTC time data.                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    if (sPt->u8cClockDisplay == RTC_CLOCK_12)
    {
        g_chHourMode = RTC_CLOCK_12;
 	
        outp32(TSSR, RTC_CLOCK_12);
        
 		RTC_Check();        
 		 		
        /*-------------------------------------------------------------------------------------------------*/
        /* important, range of 12-hour PM mode is 21 upto 32                                               */
        /*-------------------------------------------------------------------------------------------------*/
        if (sPt->u8cAmPm == RTC_PM)
            sPt->u32cHour += 20;
    }
    else                                                                               /* RTC_CLOCK_24 */
    {
        g_chHourMode = RTC_CLOCK_24;
                
        outp32(TSSR, RTC_CLOCK_24);
    
    	RTC_Check();
          
        RTCDEBUG ("RTC: 24-hour\n");
    }


    g_u32hiHour  = sPt->u32cHour / 10;
    g_u32loHour  = sPt->u32cHour % 10;
    g_u32hiMin   = sPt->u32cMinute / 10;
    g_u32loMin   = sPt->u32cMinute % 10;
    g_u32hiSec   = sPt->u32cSecond / 10;
    g_u32loSec   = sPt->u32cSecond % 10;
    u32Reg     = (g_u32hiHour << 20);
    u32Reg    |= (g_u32loHour << 16);
    u32Reg    |= (g_u32hiMin << 12);
    u32Reg    |= (g_u32loMin << 8);
    u32Reg    |= (g_u32hiSec << 4);
    u32Reg    |= g_u32loSec;
    g_u32Reg = u32Reg;
    
    outp32(TLR, (UINT32)g_u32Reg);
	    
    RTC_Check();
     
    if (sPt->u8cClockDisplay == RTC_CLOCK_12)
    {
        if (sPt->u8cAmPm == RTC_PM)
            sPt->u32cHour -= 20;
    }
             
    g_u32hiYear  = (sPt->u32Year - RTC_YEAR2000) / 10;
    g_u32loYear  = (sPt->u32Year - RTC_YEAR2000) % 10;
    g_u32hiMonth =  sPt->u32cMonth              / 10;
    g_u32loMonth =  sPt->u32cMonth              % 10;
    g_u32hiDay   =  sPt->u32cDay                / 10;
    g_u32loDay   =  sPt->u32cDay                % 10;
    u32Reg    = (g_u32hiYear << 20);
    u32Reg    |= (g_u32loYear << 16);
    u32Reg    |= (g_u32hiMonth << 12);
    u32Reg    |= (g_u32loMonth << 8);
    u32Reg    |= (g_u32hiDay << 4);
    u32Reg    |= g_u32loDay;
    g_u32Reg = u32Reg;
    outp32 (CLR, (UINT32)g_u32Reg);    
    
    RTC_Check();
     
   	RTCDEBUG ("Init RTC: REG_RTC_CLR[0x%08x]\n",g_u32Reg);       

    outp32(DWR, (UINT32)sPt->u32cDayOfWeek);
    
    RTC_Check();
#ifdef __LIBRARY__ 
	outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif
	return E_RTC_SUCCESS;
}


/*---------------------------------------------------------------------------------------------------------*/
/* Function:     RTC_Read          	                                                                       */
/*                                                                                                         */
/* Parameter:                                                                                              */
/*               eTime                    		Currnet_Timer/ Alarm_Time                                  */
/*               RTC_TIME_DATA_T *spt   		Time Data                                                  */
/* Returns:                                                                                                */
/*               E_SUCCESS               Success.                                                          */
/*               E_RTC_ERR_EIO			 Initial RTC FAILED.   		                                       */
/* Side effects:                                                                                           */
/*                                                                                                         */
/* DESCRIPTION                                                                                             */
/*               Read current date/time or alarm date/time from RTC                                        */
//*--------------------------------------------------------------------------------------------------------*/

UINT32 RTC_Read (E_RTC_TIME_SELECT eTime, RTC_TIME_DATA_T *sPt)
{
    UINT32 u32Tmp;
#ifdef __LIBRARY__ 
	outp32(REG_APBCLK,inp32(REG_APBCLK) | RTC_CKE);
#endif

    sPt->u8cClockDisplay = inp32(TSSR);                               /* 12/24-hour */
    sPt->u32cDayOfWeek = inp32(DWR);                                   /* Day of week */

    switch (eTime)
    {
        case RTC_CURRENT_TIME:
        {
			g_u32Reg   = inp32(CLR);
            g_u32Reg1  = inp32(TLR);
            break;
		}
        case RTC_ALARM_TIME:
        {
			g_u32Reg   = inp32(CAR);
            g_u32Reg1  = inp32(TAR);
            break;
        }
        default:
        {
#ifdef __LIBRARY__ 
        	outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif
            return E_RTC_ERR_ENOTTY;
        }
    }


    g_u32hiYear  = (g_u32Reg & 0xF00000) >> 20;
    g_u32loYear  = (g_u32Reg & 0xF0000) >> 16;
    g_u32hiMonth = (g_u32Reg & 0x1000) >> 12;
    g_u32loMonth = (g_u32Reg & 0xF00) >> 8;
    g_u32hiDay   = (g_u32Reg & 0x30) >> 4;
    g_u32loDay   =  g_u32Reg & 0xF;

    u32Tmp = (g_u32hiYear * 10);
    u32Tmp+= g_u32loYear;
    sPt->u32Year   =   u32Tmp  + RTC_YEAR2000;
    
    u32Tmp = (g_u32hiMonth * 10);
    sPt->u32cMonth = u32Tmp + g_u32loMonth;
    
    u32Tmp = (g_u32hiDay * 10);
    sPt->u32cDay   =  u32Tmp  + g_u32loDay;


    g_u32hiHour = (g_u32Reg1 & 0x300000) >> 20;    
    g_u32loHour = (g_u32Reg1 & 0xF0000) >> 16;
    g_u32hiMin  = (g_u32Reg1 & 0x7000) >> 12;
    g_u32loMin  = (g_u32Reg1 & 0xF00) >> 8;
    g_u32hiSec  = (g_u32Reg1 & 0x70) >> 4;
    g_u32loSec  =  g_u32Reg1 & 0xF;


    if (sPt->u8cClockDisplay == RTC_CLOCK_12)
    {
        u32Tmp = (g_u32hiHour * 10);
        u32Tmp+= g_u32loHour;
        sPt->u32cHour = u32Tmp;                                /* AM: 1~12. PM: 21~32. */

    	if(eTime ==RTC_CURRENT_TIME)
	    {
            if (sPt->u32cHour >= 21)
	        {
	            sPt->u8cAmPm = RTC_PM;
	            sPt->u32cHour -= 20;
	        }
	        else
	        {
	            sPt->u8cAmPm = RTC_AM;
	        }
        }
        else
        {
            if (sPt->u32cHour < 12)
	        {
	        	if(sPt->u32cHour == 0)
	        		sPt->u32cHour = 12;
	            sPt->u8cAmPm = RTC_AM;
	        }
	        else
	        {
	        	sPt->u32cHour -= 12;
	            sPt->u8cAmPm = RTC_PM;
	        }        
        }
        
        u32Tmp = (g_u32hiMin  * 10);
        u32Tmp+= g_u32loMin;
        sPt->u32cMinute = u32Tmp;
        
        u32Tmp = (g_u32hiSec  * 10);
        u32Tmp+= g_u32loSec;
        sPt->u32cSecond = u32Tmp;

    }
    else
    {   /* RTC_CLOCK_24 */
        u32Tmp = (g_u32hiHour * 10);
        u32Tmp+= g_u32loHour;
        sPt->u32cHour   = u32Tmp;
        
        u32Tmp = (g_u32hiMin  * 10);
        u32Tmp+= g_u32loMin;
        sPt->u32cMinute = u32Tmp;
        
        u32Tmp = (g_u32hiSec  * 10);
        u32Tmp+= g_u32loSec;
        sPt->u32cSecond = u32Tmp;
    }
#ifdef __LIBRARY__    
	outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif
    return E_RTC_SUCCESS;

}



/*---------------------------------------------------------------------------------------------------------*/
/* Function:     RTC_Write                                             	                                   */
/*                                                                                                         */
/* Parameter:                                                                                              */
/*               eTime                    		Currnet_Timer/ Alarm_Time                                  */
/*               RTC_TIME_DATA_T *sPt     		Time Data                                                  */
/* Returns:                                                                                                */
/*               E_SUCCESS               Success.                                                          */
/*               E_RTC_ERR_EIO      	 Initial RTC FAILED.                                               */
/* Side effects:                                                                                           */
/*                                                                                                         */
/* DESCRIPTION                                                                                             */
/*               Read current date/time or alarm date/time from RTC                                        */
//*--------------------------------------------------------------------------------------------------------*/
UINT32 RTC_Write(E_RTC_TIME_SELECT eTime, RTC_TIME_DATA_T *sPt)
{
    UINT32 u32Reg;
    
    /*-----------------------------------------------------------------------------------------------------*/
    /* Check RTC time data value is reasonable or not.                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    if ( ((sPt->u32Year - RTC_YEAR2000) > 99)|
         ((sPt->u32cMonth == 0) || (sPt->u32cMonth > 12))|
         ((sPt->u32cDay   == 0) || (sPt->u32cDay   > 31)))
    {
        RTCDEBUG ("\nRTC: Year value is incorrect\n");
        return E_RTC_ERR_CALENDAR_VALUE;
    }

    if ( (sPt->u32Year - RTC_YEAR2000) > 99 )
    {
        RTCDEBUG ("\nRTC: Year value is incorrect\n");
        return E_RTC_ERR_CALENDAR_VALUE;
    }

    if ( (sPt->u32cMonth == 0) || (sPt->u32cMonth > 12) )
    {
        RTCDEBUG ("\nRTC: Month value is incorrect\n");
        return E_RTC_ERR_CALENDAR_VALUE;
    }

    if ( (sPt->u32cDay == 0) || (sPt->u32cDay > 31) )
    {
        RTCDEBUG ("\nRTC: Day value is incorrect\n");
        return E_RTC_ERR_CALENDAR_VALUE;
    }

    if (sPt->u8cClockDisplay == RTC_CLOCK_12)
    {
        if ( (sPt->u32cHour == 0) || (sPt->u32cHour > 12) )
        {
            RTCDEBUG ("\nRTC: Hour value is incorrect\n");
            return E_RTC_ERR_TIME_VALUE;
        }
    }
    else if (sPt->u8cClockDisplay == RTC_CLOCK_24)
    {
        if (sPt->u32cHour > 23)
        {
            RTCDEBUG ("\nRTC: Hour value is incorrect\n");
            return E_RTC_ERR_TIME_VALUE;
        }
    }
    else
    {
        RTCDEBUG ("\nRTC: Clock mode is incorrect\n");
        return E_RTC_ERR_TIME_VALUE;
    }

    if (sPt->u32cMinute > 59)
    {
        RTCDEBUG ("\nRTC: Minute value is incorrect\n");
        return E_RTC_ERR_TIME_VALUE;
    }

    if (sPt->u32cSecond > 59)
    {
        RTCDEBUG ("\nRTC: Second value is incorrect\n");
        return E_RTC_ERR_TIME_VALUE;
    }

    if (sPt->u32cDayOfWeek > 6)
    {
        RTCDEBUG ("\nRTC: Day of week value is incorrect\n");
        return E_RTC_ERR_DWR_VALUE;
    }
    
    switch (eTime)
    {

        case RTC_CURRENT_TIME:
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Second, set RTC time data.                                                                  */
            /*---------------------------------------------------------------------------------------------*/
#ifdef __LIBRARY__ 
			outp32(REG_APBCLK,inp32(REG_APBCLK) | RTC_CKE);
#endif

            if (sPt->u8cClockDisplay == RTC_CLOCK_12)
            {
                g_chHourMode = RTC_CLOCK_12;
                outp32(TSSR, RTC_CLOCK_12);			    
			    RTC_Check();
			                       
                RTCDEBUG ("RTC: 12-hour\n");
                /*-----------------------------------------------------------------------------------------*/
                /* important, range of 12-hour PM mode is 21 upto 32                                       */
                /*-----------------------------------------------------------------------------------------*/
                if (sPt->u8cAmPm == RTC_PM)
                {                
                    sPt->u32cHour += 20;
                 	RTCDEBUG("RTC_CURRENT_TIME %d\n", sPt->u32cHour);   
                }
            }
            else                                                                  /* RTC_CLOCK_24 */
            {
                g_chHourMode = RTC_CLOCK_24;
                outp32(TSSR, RTC_CLOCK_24);			    
			    RTC_Check();
			                       
                RTCDEBUG ("RTC: 24-hour\n");
            }


		     
            g_u32hiHour  = sPt->u32cHour / 10;
            g_u32loHour  = sPt->u32cHour % 10;
            g_u32hiMin   = sPt->u32cMinute / 10;
            g_u32loMin   = sPt->u32cMinute % 10;
            g_u32hiSec   = sPt->u32cSecond / 10;
            g_u32loSec   = sPt->u32cSecond % 10;
            
            u32Reg = (g_u32hiHour << 20);
            u32Reg|= (g_u32loHour << 16);
            u32Reg|= (g_u32hiMin << 12);
            u32Reg|= (g_u32loMin << 8);
            u32Reg|= (g_u32hiSec << 4);
            u32Reg|= g_u32loSec;
            g_u32Reg = u32Reg;
            
			
            outp32(TLR, (UINT32)g_u32Reg);            
		    RTC_Check();                
                      
			g_u32hiYear  = (sPt->u32Year - RTC_YEAR2000) / 10;
            g_u32loYear  = (sPt->u32Year - RTC_YEAR2000) % 10;
            g_u32hiMonth = sPt->u32cMonth / 10;
            g_u32loMonth = sPt->u32cMonth % 10;
            g_u32hiDay   = sPt->u32cDay / 10;
            g_u32loDay   = sPt->u32cDay % 10;
            
            u32Reg = (g_u32hiYear << 20);
            u32Reg|= (g_u32loYear << 16);
            u32Reg|= (g_u32hiMonth << 12);
            u32Reg|= (g_u32loMonth << 8);
            u32Reg|= (g_u32hiDay << 4);
            u32Reg|= g_u32loDay;
            g_u32Reg = u32Reg;
              	  
            outp32 (CLR, (UINT32)g_u32Reg);             
		    RTC_Check();                      
                                    			
            outp32(DWR,(UINT32) sPt->u32cDayOfWeek);
            RTC_Check();            
            
            if (sPt->u8cClockDisplay == RTC_CLOCK_12)
            {
	            if (sPt->u8cAmPm == RTC_PM)
	            {
    	              sPt->u32cHour -= 20;			
    	              RTCDEBUG("After RTC_CURRENT_TIME %d\n", sPt->u32cHour);   	
    	        }
			}	
#ifdef __LIBRARY__ 			
			outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif
            return E_RTC_SUCCESS;

		 }
         case RTC_ALARM_TIME:
         {
  
  			outp32(PWRON,inp32(PWRON) & ~ALARM_EN);
			RTC_Check(); 
            g_pfnRTCCallBack_Alarm = NULL;                                         /* Initial call back function.*/
            /*---------------------------------------------------------------------------------------------*/
            /* Second, set alarm time data.                                                                */
            /*---------------------------------------------------------------------------------------------*/
            g_u32hiYear = (sPt->u32Year - RTC_YEAR2000) / 10;
            g_u32loYear = (sPt->u32Year - RTC_YEAR2000) % 10;
            g_u32hiMonth = sPt->u32cMonth / 10;
            g_u32loMonth = sPt->u32cMonth % 10;
            g_u32hiDay = sPt->u32cDay / 10;
            g_u32loDay = sPt->u32cDay % 10;
            
            u32Reg = ((sPt->u32AlarmMaskDayOfWeek & 0x1) << 31);
            u32Reg|= ((sPt->u32cDayOfWeek & 0x7) << 28);
            u32Reg|= ((sPt->u32AlarmMaskYear & 0x1) << 24);          
            u32Reg|= (g_u32hiYear << 20);
            u32Reg|= (g_u32loYear << 16);
            u32Reg|= ((sPt->u32AlarmMaskMonth & 0x1) << 15);     
            u32Reg|= (g_u32hiMonth << 12);
            u32Reg|= (g_u32loMonth << 8);
            u32Reg|= ((sPt->u32AlarmMaskDay & 0x1) << 7);  
            u32Reg|= (g_u32hiDay << 4);
            u32Reg|= g_u32loDay;
                        
            g_u32Reg = u32Reg;
#ifdef __LIBRARY__             
	        outp32(REG_APBCLK,inp32(REG_APBCLK) | RTC_CKE);
#endif
            
            outp32(CAR, (UINT32)g_u32Reg);
			RTC_Check();              
            
			
            if (g_chHourMode == RTC_CLOCK_12)
            {
                if (sPt->u8cAmPm == RTC_PM)       /* important, range of 12-hour PM mode is 21 upto 32 */
                {
                    sPt->u32cHour += 20;                     
                }
            }
            g_u32hiHour   = sPt->u32cHour / 10;
            g_u32loHour   = sPt->u32cHour % 10;
            g_u32hiMin  = sPt->u32cMinute / 10;
            g_u32loMin  = sPt->u32cMinute % 10;
            g_u32hiSec  = sPt->u32cSecond / 10;
            g_u32loSec  = sPt->u32cSecond % 10;
           
           	u32Reg = ((sPt->u32AlarmMaskHour & 0x1) << 23); 
            u32Reg|= (g_u32hiHour << 20);
            u32Reg|= (g_u32loHour << 16);
            u32Reg|= ((sPt->u32AlarmMaskMinute & 0x1) << 15); 
            u32Reg|= (g_u32hiMin << 12);
            u32Reg|= (g_u32loMin << 8);
            u32Reg|= ((sPt->u32AlarmMaskSecond & 0x1) << 7); 
            u32Reg|= (g_u32hiSec << 4);
            u32Reg|= g_u32loSec;
            
            g_u32Reg = u32Reg;
            
            outp32(TAR, (UINT32)g_u32Reg);               
		    RTC_Check();                
		    
		    

            if (sPt->u8cClockDisplay == RTC_CLOCK_12)
            {
	            if (sPt->u8cAmPm == RTC_PM)
	            {
    	              sPt->u32cHour -= 20;			
    	        }
			}	            
            /*---------------------------------------------------------------------------------------------*/
            /* Third, install alarm callback function.                                                     */
            /*---------------------------------------------------------------------------------------------*/
            if (sPt->pfnAlarmCallBack != NULL)
            {
                g_pfnRTCCallBack_Alarm = sPt->pfnAlarmCallBack;                
             }
            /*---------------------------------------------------------------------------------------------*/
            /* Finally, enable alarm interrupt.                                                            */
            /*---------------------------------------------------------------------------------------------*/

            RTC_Ioctl(0,RTC_IOC_ENABLE_INT,RTC_ALARM_INT,0);
  
  			outp32(PWRON,inp32(PWRON) | ALARM_EN);
			RTC_Check(); 
#ifdef __LIBRARY__ 			  
  			outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif
            return E_RTC_SUCCESS;
		}
		default:
        {
            return E_RTC_ERR_ENOTTY;
        }
    }

}



/*---------------------------------------------------------------------------------------------------------*/
/* Function:     RTC_Ioctl                   	                                                           */
/*                                                                                                         */
/* Parameter:                                                                                              */
/*               i32Num             Interface number.   always set 0                                       */
/*               eCmd           	Command.                                                               */
/*               u32Arg0            Arguments for the command.                                             */
/*               u32Arg1            Arguments for the command.                                             */
/* Returns:                                                                                                */
/*               E_SUCCESS        		 Success.                          		                           */
/*               E_RTC_ERR_ENOTTY        Command not support, or parameter incorrect.          		       */
/*               E_RTC_ERR_ENODEV        Interface number incorrect.                           		       */
/* Side effects:                                                                                           */
/*                                                                                                         */
/* DESCRIPTION                                                                                             */
/*               Support some commands for application.                                                    */
//*--------------------------------------------------------------------------------------------------------*/

UINT32 RTC_Ioctl (INT32 i32Num, E_RTC_CMD eCmd, UINT32 u32Arg0, UINT32 u32Arg1)
{
    INT32 i32Ret;
    UINT32 u32Reg;
    RTC_TICK_T *ptick;
	UINT32 u32Tmp;
	
    if (i32Num != 0)
        return E_RTC_ERR_ENODEV;
#ifdef __LIBRARY__         
	outp32(REG_APBCLK,inp32(REG_APBCLK) | RTC_CKE);
#endif

    switch (eCmd)
    {

        case RTC_IOC_IDENTIFY_LEAP_YEAR:
        {
            u32Reg = inp32(LIR);
            if (u32Reg & 0x01)
            {
                *(PUINT32)u32Arg0 = RTC_LEAP_YEAR;
                RTCDEBUG("\nRTC: It's a leap year\n");
            }
            else
            {
                *(PUINT32)u32Arg0 = 0;
                RTCDEBUG("\nRTC: It's not a leap year\n");
            }
            break;
        }
        case RTC_IOC_SET_TICK_MODE:
        {
            ptick = (RTC_TICK_T *) u32Arg0;
            
            if (g_bIsEnableTickInt== TRUE)
            {            
            	RTC_Ioctl(0,RTC_IOC_DISABLE_INT,RTC_TICK_INT,0);
            	g_bIsEnableTickInt = TRUE;
	        }    
            g_u32RTC_Count = 0;

            if (ptick->ucMode > RTC_TICK_1_128_SEC)                            /*Tick mode 0 to 7 */
            {
                return E_RTC_ERR_ENOTTY ;
            }
            
            outp32(TTR, ptick->ucMode);     
            RTC_Check();       
                       
            if (ptick->pfnTickCallBack != NULL)
            {
                g_pfnRTCCallBack_Tick = ptick->pfnTickCallBack;
            }
            else
            {
                g_pfnRTCCallBack_Tick = NULL;
            }
            /*---------------------------------------------------------------------------------------------*/
            /* Reset tick interrupt status if program enable tick interrupt before.                        */
            /*---------------------------------------------------------------------------------------------*/
            if (g_bIsEnableTickInt== TRUE)
            {

                RTC_Ioctl(0,RTC_IOC_ENABLE_INT,RTC_TICK_INT,0);
#ifdef __LIBRARY__                 
                outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif
                return E_RTC_SUCCESS;
            }
            break;
        }

        case RTC_IOC_GET_TICK:
        {
            (*(PUINT32)u32Arg0) = g_u32RTC_Count;
            break;
        }

        case RTC_IOC_RESTORE_TICK:
        {
            g_u32RTC_Count = 0;
            break;
        }

        case RTC_IOC_ENABLE_INT:
        {            
        	
            switch ((RTC_INT_SOURCE)u32Arg0)
            {

                case RTC_TICK_INT:
                {
                    g_bIsEnableTickInt   = TRUE;
                    u32Tmp = inp32(RIER) | RTC_TICK_INT;
                    break;
                }
                case RTC_ALARM_INT:
                {
                    g_bIsEnableAlarmInt  = TRUE;               
                      			
		  			u32Tmp = inp32(PWRON) | ALARM_EN;
		  				
                    outp32(PWRON, u32Tmp);                    
                    RTC_Check();  
                                        
                    u32Tmp = inp32(RIER) | RTC_ALARM_INT;      
                                  
                    break;
                }
				case RTC_RELATIVE_ALARM_INT:
                {
                    g_bIsEnableAlarmInt  = TRUE;                
		  			
		  			u32Tmp = inp32(PWRON) | REL_ALARM_EN;
		  			
                    outp32(PWRON, u32Tmp);                    
                    RTC_Check(); 
                                        
                    u32Tmp = inp32(RIER) | RTC_RELATIVE_ALARM_INT;     
                    break;
                }                
                case RTC_PSWI_INT:
                {
                    g_bIsEnableAlarmInt  = TRUE;
                    u32Tmp = inp32(RIER) | RTC_PSWI_INT;
                    break;
                }              
                default:
                {
                    return E_RTC_ERR_ENOTTY;

                }
            }
            outp32(RIER, u32Tmp); 
		    RTC_Check();
		              
            while( inp32(RIER) !=  u32Tmp);            
            
            break;
        }
        case RTC_IOC_DISABLE_INT:
        {
         
			switch ((RTC_INT_SOURCE)u32Arg0)
            {
                case RTC_TICK_INT:
                {
                    g_bIsEnableTickInt   = FALSE;
                                     
                    u32Tmp = inp32(RIER) & (~RTC_TICK_INT);
                                        
					outp32(RIER, u32Tmp);
                    
				    RTC_Check();
                                       
					outp32(RIIR, RTC_TICK_INT);					                    
				    RTC_Check();   					
					
		            while((inp32(RIIR) & RTC_TICK_INT)  !=  0); 
		                                
                    break;
                }
                case RTC_ALARM_INT:
                {
                    g_bIsEnableAlarmInt  = FALSE;
                    

                    
                    u32Tmp = inp32(RIER) & (~RTC_ALARM_INT);
					
					outp32(RIER, u32Tmp);					
					RTC_Check();					
										
					u32Tmp = inp32(PWRON) & ~ALARM_EN;
										
				   	outp32(PWRON,u32Tmp );					                    
				    RTC_Check();  
		                                
					outp32(RIIR, RTC_ALARM_INT);					
                    RTC_Check();  					
					                    
                    break;
                }
                case RTC_RELATIVE_ALARM_INT:
                {
                    g_bIsEnableAlarmInt  = FALSE;
                    
                    u32Tmp = inp32(RIER) & (~RTC_RELATIVE_ALARM_INT);
                    	
					outp32(RIER, u32Tmp);					
					RTC_Check();					
					 									
					u32Tmp = inp32(PWRON) & ~REL_ALARM_EN;
					
				   	outp32(PWRON,u32Tmp );					                    
				    RTC_Check();  
							                                
					outp32(RIIR, RTC_RELATIVE_ALARM_INT);					
                    RTC_Check();  					
					
                    break;
                }                
                case RTC_PSWI_INT:
                {
                    g_bIsEnableAlarmInt  = FALSE;
                    u32Tmp = inp32(RIER) & (~RTC_PSWI_INT);
                    
					outp32(RIER, u32Tmp);
                    RTC_Check();  					
					   
					outp32(RIIR, RTC_PSWI_INT);    
                  	RTC_Check();
                  	 
                    break;
                }                

                case RTC_ALL_INT:
                {
                    g_bIsEnableTickInt   = FALSE;
                    g_bIsEnableAlarmInt  = FALSE;
                    outp32(RIER, 0 );                    				
				    RTC_Check();                                       
                    		                                
                    outp32(RIIR, RTC_ALL_INT);                    
				    RTC_Check();
                    
                    break;
                }
                default:
                {
#ifdef __LIBRARY__                 
                	outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif
                    return E_RTC_ERR_ENOTTY;
                }
            }


            break;
        }

        case RTC_IOC_SET_FREQUENCY:
        {
            i32Ret= RTC_DoFrequencyCompensation() ;
            if (i32Ret != 0)
            {
#ifdef __LIBRARY__             
            	outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif
                return E_RTC_ERR_ENOTTY;
            }
            break;
        }
		case RTC_IOC_SET_POWER_ON:
		{
		    u32Tmp = inp32(PWRON) | 0x01;
		    
		    outp32(PWRON,u32Tmp);		                       
		    RTC_Check();  		    
		    
		    while((inp32(PWRON) & 0x01) != 0x1);
		    
		    break;
		}
		case RTC_IOC_SET_POWER_OFF:
		{
		    outp32(PWRON, (inp32(PWRON) & ~0x01) | 2);		                        
		    RTC_Check();
#ifndef __FPGA__			    
		    outp32(REG_AHBCLK,0);
	    
		    while(1);
#else		    
			while(inp32(PWRON) & 0x01);
#endif
//		    break;
		}
		case RTC_IOC_SET_POWER_OFF_PERIOD:
		{		    
			outp32(PWRON, (inp32(PWRON) & ~0xF0000) | ((u32Arg0 & 0xF) << 16));			
            RTC_Check();
			
			while(((inp32(PWRON) & 0xF0000)) != ((u32Arg0 & 0xF) << 16) );

		    break;
		}
		case RTC_IOC_ENABLE_HW_POWEROFF:
		{	    
			outp32(PWRON, (inp32(PWRON) | 0x04));			
            RTC_Check();  
			
			while(!(inp32(PWRON) & 0x04));
			
		    break;
		}
		case RTC_IOC_DISABLE_HW_POWEROFF:
		{
     		    
			outp32(PWRON, (inp32(PWRON) & ~0x04));			
            RTC_Check();
			
			while((inp32(PWRON) & 0x04));
			
		    break;
		}		
		case RTC_IOC_SET_PSWI_CALLBACK:
		{         
		
			RTC_Ioctl(0, RTC_IOC_ENABLE_INT, RTC_PSWI_INT, 0);
			
            if (((PFN_RTC_CALLBACK  *) u32Arg0) != NULL)
            {
                g_pfnRTCCallBack_PSWI = (PFN_RTC_CALLBACK  *) u32Arg0;
            }
            else
            {
                g_pfnRTCCallBack_PSWI = NULL;
            }
           	break;
        }            				
		case RTC_IOC_GET_POWERKEY_STATUS:
		{
			if(inp32(PWRON) & 0x80)
				 *(PUINT32)u32Arg0 = 1;
			else
				 *(PUINT32)u32Arg0 = 0;	

		    break;
		}
		case RTC_IOC_GET_SW_STATUS:
		{
			*(PUINT32)u32Arg0 = (inp32(PWRON) & 0xFF00) >> 8; 
			break;
		}
		case RTC_IOC_SET_SW_STATUS:
		{
			u32Tmp = (inp32(PWRON) & ~0xFF00);
			
			outp32(PWRON, (u32Tmp | ((u32Arg0 & 0xFF) << 8)));			
			RTC_Check(); 
			
			break;
		}
		
		case RTC_IOC_SET_RELEATIVE_ALARM:
		{
            g_bIsEnableAlarmInt  = TRUE;
           
          	outp32(PWRON,  (inp32(PWRON) & ~0xFFF00010));
            RTC_Check();            
               
			u32Tmp = (inp32(PWRON) & ~0xFFF00000)| ((u32Arg0 & 0xFFF) <<20) | REL_ALARM_EN;

            outp32(PWRON, u32Tmp);
            RTC_Check(); 
          
            g_bIsEnableAlarmInt  = TRUE;    
                                
            u32Tmp = inp32(RIER) | RTC_RELATIVE_ALARM_INT;

             outp32(RIER, u32Tmp);          
			 RTC_Check(); 
			 
            if(u32Arg1 != 0)
	            g_pfnRTCCallBack_Relative_Alarm = 	(PFN_RTC_CALLBACK  *)u32Arg1;
                    
			break;
		
		}
		case RTC_IOC_SET_POWER_KEY_DELAY:
		{
			if(u32Arg0 > 7)
				  return E_RTC_ERR_ENOTTY; 
				  
			u32Tmp = inp32(RTC_FCR) & ~POWER_KEY_DURATION;
 	  
				  
			outp32(RTC_FCR, u32Tmp | ((u32Arg0 & 0x07) << 24) ); 
			RTC_Check(); 
			break;
		}		
		case RTC_IOC_SET_CLOCK_SOURCE:
		{
			if(u32Arg0 != RTC_INTERANL && u32Arg0 != RTC_EXTERNAL)
				  return E_RTC_ERR_ENOTTY; 
 	  
			outp32(REG_OSC_32K, u32Arg0); 
			RTC_Check(); 
			
			RTC_DoFrequencyCompensation();
			break;
		}		
		case RTC_IOC_GET_CLOCK_SOURCE:
		{			
			UINT32 u32Count = ((inp32(RTC_FCR) & INTEGER) >> 8);
			
			if((u32Count >= (0x7FFF - 10)) && (u32Count <= (0x7FFF + 10)))
				*(PUINT32)u32Arg0 = RTC_EXTERNAL; 
			else
				*(PUINT32)u32Arg0 = RTC_INTERANL; 
			break;
		}				
        default:
        {
#ifdef __LIBRARY__         
        	outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif
            return E_RTC_ERR_ENOTTY;
        }
    }
#ifdef __LIBRARY__     
    outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif

    return E_RTC_SUCCESS;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:     RTC_Close      	                                                                        */
/*                                                                                                         */
/* Parameter:                                                                                              */
/*               VOID            							                                               */
/* Returns:                                                                                                */
/*               E_SUCCESS                Success.                                                         */
/*               E_RTC_ERR_ENODEV 	      Interface number incorrect.                                      */
/* Side effects:                                                                                           */
/*                                                                                                         */
/* DESCRIPTION                                                                                             */
/*               Disable AIC channel of RTC and both tick and alarm interrupt..                             */
/*---------------------------------------------------------------------------------------------------------*/

UINT32 RTC_Close (void)
{

    g_bIsEnableTickInt = FALSE;
        
#ifdef __LIBRARY__         
    outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);
#endif
    
   	sysDisableInterrupt(IRQ_RTC);
   

    RTC_Ioctl(0,RTC_IOC_DISABLE_INT,RTC_ALL_INT,0);


    return E_RTC_SUCCESS;
}

VOID RTC_EnableClock(BOOL bEnable)
{
	if(bEnable)
		outp32(REG_APBCLK,inp32(REG_APBCLK) | RTC_CKE);	
	else
		outp32(REG_APBCLK,inp32(REG_APBCLK) & ~RTC_CKE);	

}

extern UINT32 RTC_Init(void);
extern UINT32 RTC_Open(RTC_TIME_DATA_T *sPt);
extern UINT32 RTC_Ioctl(INT32 nNum, E_RTC_CMD uCmd, UINT32 uArg0, UINT32 u32Arg1);
extern UINT32 RTC_Read(E_RTC_TIME_SELECT eTime, RTC_TIME_DATA_T *sPt);
extern UINT32 RTC_Write(E_RTC_TIME_SELECT eTime, RTC_TIME_DATA_T *sPt);
extern UINT32 RTC_Close(void);



