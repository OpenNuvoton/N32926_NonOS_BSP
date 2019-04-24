#include <stdio.h>
#include "wblib.h"
#include "demo.h"
#include "w55fa92_ts_adc.h"

static volatile BOOL VoltageDetect_time = FALSE;
static volatile UINT32 u32VoltageValue = 0;
static volatile BOOL bIsValidVoltageDet = FALSE;
static UINT32 u32Count=0;
static void VoltageDetect_timer(void)
{
	VoltageDetect_time = TRUE;
}
static void VoltageDetect_callback(UINT32 u32code)
{
	bIsValidVoltageDet = TRUE;
	u32VoltageValue = u32code;
	u32Count = u32Count+1;
	if(u32code==0){
		sysprintf("Voltage Detect Value %d = %d\n", u32Count, u32code);
	}	
	sysprintf("Voltage Detect Value %d = %d\n", u32Count, u32code);
}
INT32 VoltageDetection(UINT32 u32Channel)
{
	UINT32 tmp, btime, etime;
	UINT32 u32ExtFreq;
	PFN_ADC_CALLBACK pfnOldCallback;
	
	u32ExtFreq = sysGetExternalClock();
	DBG_PRINTF("ADC Voltage Detection Demo...\n");	
	sysSetTimerReferenceClock(TIMER0, u32ExtFreq); 			//External Crystal
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);			/* 100 ticks/per sec ==> 1tick/10ms */
	tmp = sysSetTimerEvent(TIMER0, 5, (PVOID)VoltageDetect_timer);		/* 100 ticks = 1s call back */
	DBG_PRINTF("No. of Event [%d]\n", tmp);
	
	DrvADC_InstallCallback(eADC_AIN,
						VoltageDetect_callback,
						&pfnOldCallback);
	
	btime = sysGetTicks(TIMER0);
	etime = btime;
	while ((etime - btime) <= 1000)
	{
		while(VoltageDetect_time==TRUE){
			VoltageDetect_time = FALSE;
			if(DrvADC_VoltageDetection(u32Channel)==Successful){	//ADC is ready					
				if(bIsValidVoltageDet==TRUE){ //Key code has been updated				
					bIsValidVoltageDet = FALSE;
				}
			}
		}
		etime = sysGetTicks(TIMER0);
	}	
	sysClearTimerEvent(TIMER0, tmp);
	return Successful;
}
