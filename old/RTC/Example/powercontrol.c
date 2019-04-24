#include <stdio.h>
#include <string.h>
#include "wblib.h"
#include "RTC.h"

extern VOID RTC_Releative_AlarmISR(void);
extern VOID RTC_AlarmISR(void);
extern BOOL volatile g_bAlarm;
BOOL volatile g_bPowerKeyPress = FALSE;

#define HW_POWER_OFF	0
#define SW_POWER_OFF	1
#define SYSTEM_CLOCK	12000000


void Smpl_RTC_Powerdown_Wakeup_Relative(void)
{
	RTC_TIME_DATA_T sCurTime;	
			
	sysprintf("\n2. RTC Powerdown Wakeup Test (Wakeup after 10 seconds)\n");
		
	g_bAlarm = FALSE;	
	
	/* Get the currnet time */
	RTC_Read(RTC_CURRENT_TIME, &sCurTime);
	
	sysprintf("   Current Time:%d/%02d/%02d %02d:%02d:%02d\n",sCurTime.u32Year,sCurTime.u32cMonth,sCurTime.u32cDay,sCurTime.u32cHour,sCurTime.u32cMinute,sCurTime.u32cSecond);
	
	/* Enable RTC Tick Interrupt and install tick call back function */
	RTC_Ioctl(0,RTC_IOC_SET_RELEATIVE_ALARM, 10, (UINT32)RTC_Releative_AlarmISR);	
							
	/* Enable USB Wakeup */
	outp32(REG_MISSR, inp32(REG_MISSR) | (RTC_WS | RTC_WE));
		
	sysSetLocalInterrupt(DISABLE_IRQ);		

	sysPowerDown(WE_RTC);	
	
	outp32(REG_MISSR, inp32(REG_MISSR)| RTC_WS);	/* Clear RTC wakeup status */
	
	
	sysSetLocalInterrupt(ENABLE_IRQ);		

	sysprintf("   Wake up!!!\n");	
			
	RTC_EnableClock(TRUE);
			
	while(!g_bAlarm);
	
	/* Get the currnet time */
	RTC_Read(RTC_CURRENT_TIME, &sCurTime);	
	
	sysprintf("   Current Time:%d/%02d/%02d %02d:%02d:%02d\n",sCurTime.u32Year,sCurTime.u32cMonth,sCurTime.u32cDay,sCurTime.u32cHour,sCurTime.u32cMinute,sCurTime.u32cSecond);
	
	RTC_Ioctl(0,RTC_IOC_DISABLE_INT,RTC_RELATIVE_ALARM_INT,0);
				
}	

void Smpl_RTC_Powerdown_Wakeup(void)
{
	RTC_TIME_DATA_T sCurTime;	
			
	sysprintf("\n2. RTC Powerdown Wakeup Test (Wakeup after 10 seconds)\n");
		
	g_bAlarm = FALSE;	
	
	/* Get the currnet time */
	RTC_Read(RTC_CURRENT_TIME, &sCurTime);
	
	/* Set Alarm call back function */
	sCurTime.pfnAlarmCallBack = RTC_AlarmISR;
	
	/* Disable Alarm Mask */
	sCurTime.u32AlarmMaskSecond = 0;	
	sCurTime.u32AlarmMaskMinute = 0;	
	sCurTime.u32AlarmMaskHour = 0;	
	sCurTime.u32AlarmMaskDay = 0;	
	sCurTime.u32AlarmMaskMonth = 0;	
	sCurTime.u32AlarmMaskYear = 0;	
	sCurTime.u32AlarmMaskDayOfWeek = 0;
	sCurTime.u32AlarmMaskSecond = 0; 			
	
	sysprintf("   Current Time:%d/%02d/%02d %02d:%02d:%02d\n",sCurTime.u32Year,sCurTime.u32cMonth,sCurTime.u32cDay,sCurTime.u32cHour,sCurTime.u32cMinute,sCurTime.u32cSecond);
	
	/* The alarm time setting */	
	sCurTime.u32cSecond = sCurTime.u32cSecond + 10; 	
	
	if(sCurTime.u32cSecond >= 60)
	{
		sCurTime.u32cSecond = sCurTime.u32cSecond - 60;		
		
		sCurTime.u32cMinute++; 	
		if(sCurTime.u32cMinute >= 60)
		{
			sCurTime.u32cMinute = sCurTime.u32cMinute - 60;
			
			sCurTime.u32cHour++; 
			if(sCurTime.u32cHour >= 24)
			{
				sCurTime.u32cHour = 0;
				sCurTime.u32cDay++;
			}			
		}				
	}			
	/* Set the alarm time (Install the call back function and enable the alarm interrupt)*/
	RTC_Write(RTC_ALARM_TIME,&sCurTime);
							
	/* Enable USB Wakeup */
	outp32(REG_MISSR, inp32(REG_MISSR) | (RTC_WS | RTC_WE));
		
	sysSetLocalInterrupt(DISABLE_IRQ);		

	sysPowerDown(WE_RTC);	
	
	outp32(REG_MISSR, inp32(REG_MISSR)| RTC_WS);	/* Clear RTC wakeup status */
	
	
	sysSetLocalInterrupt(ENABLE_IRQ);		

	sysprintf("   Wake up!!!\n");	
	
	RTC_EnableClock(TRUE);
			
	while(!g_bAlarm);
	
	RTC_EnableClock(FALSE);
	
	sysprintf("   Current Time:%d/%02d/%02d %02d:%02d:%02d\n",sCurTime.u32Year,sCurTime.u32cMonth,sCurTime.u32cDay,sCurTime.u32cHour,sCurTime.u32cMinute,sCurTime.u32cSecond);
	
	RTC_Ioctl(0,RTC_IOC_DISABLE_INT,RTC_ALARM_INT,0);
}	

VOID PowerKeyPress(void)
{
	sysprintf("\nPower Key Press!!\n");	
	g_bPowerKeyPress = TRUE;
	return;
}


VOID Smpl_RTC_PowerOff_Control(UINT32 u32Mode)
{
	UINT32 u32PowerKeyStatus;
	INT32 volatile i;
	UINT32 u32ExtFreq;
  	u32ExtFreq = sysGetExternalClock();
  	sysSetTimerReferenceClock (TIMER0, u32ExtFreq);
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);  	
  	
  	g_bPowerKeyPress = FALSE;
	
	sysprintf("Turn on H/W power off function\n"); 	
	
	/* Press Power Key during 6 sec to Power off */
	RTC_Ioctl(0, RTC_IOC_SET_POWER_OFF_PERIOD, 6, 0);	
	
	/* Install the callback function for Power Key Press */
	RTC_Ioctl(0, RTC_IOC_SET_PSWI_CALLBACK, (UINT32)PowerKeyPress, 0);
	  	  	
	/* Enable Hardware Power off */	  	  	
	RTC_Ioctl(0, RTC_IOC_ENABLE_HW_POWEROFF, 0, 0);  	  	
	  	 	  	
	/* Wait Key Press */  	
	sysprintf("Wait for Key Press\n");
	
	RTC_EnableClock(TRUE);
	
 	while(!g_bPowerKeyPress);

	/* Query Power Key 3 SEC (query a time per 1 sec)*/
  	sysprintf("Press Key 3 seconds (Power off procedure starts after 3 seconds)\n");
  	for(i = 0 ; i< 3;i++)
  	{  	
  		/* Delay 1 second */
  		sysDelay(100);
				
		/* Query Power Key Status */						
		RTC_Ioctl(0, RTC_IOC_GET_POWERKEY_STATUS, (UINT32)&u32PowerKeyStatus, 0);

		if(u32PowerKeyStatus)			
		{
			sysprintf("	Power Key Release\n");
			sysprintf("	Power Off Flow Stop\n");
			return;
		}
		else
			sysprintf("	Power Key Press\n");	
			
	}	  		
		
	/* S/W Power off sequence */
	sysprintf("S/W Power off sequence start (1 second)..\n");	
	
	/* Use time to simulate the S/W Power off sequence (1 sec) */
 	sysDelay(100);
  			
	if(u32Mode == SW_POWER_OFF) 
	{	
		sysprintf("Power off sequence Complete -> Power Off ");	
		
		/* Power Off - S/W can call the API to power off any time he wnats */  	
		RTC_Ioctl(0, RTC_IOC_SET_POWER_OFF, 0, 0);
	}				
	else
	{
		sysprintf("S/W Crash!!\n");
		
		sysprintf("Wait for HW Power off");
	}
	
	i = 0;
	while(1)
	{
		if((i%50000) == 0)
			sysprintf(".");
		i++;	
	}
}
