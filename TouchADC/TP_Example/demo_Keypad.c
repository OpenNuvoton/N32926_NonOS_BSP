#include <stdio.h>
#include "wblib.h"
#include "demo.h"
#include "w55fa92_ts_adc.h"
/*
	EVB use AIN2(GPIOG 8) for key pad input.
*/
static volatile BOOL Keypad_time = FALSE;
//static volatile UINT16 u16Keycode = FALSE;
//static volatile BOOL bIsValidKey = FALSE;
static void Keypad_timer(void)
{
	Keypad_time = TRUE;
}



INT32 KeyPad(UINT32 u32Channel)
{
	UINT32 tmp, btime, etime;
	UINT32 u32ExtFreq;
	UINT32 u32KeyCode;
	
	DBG_PRINTF("ADC Keypad Demo...\n");	
	u32ExtFreq = sysGetExternalClock();
	sysSetTimerReferenceClock(TIMER0, u32ExtFreq); 			//External Crystal
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);			/* 100 ticks/per sec ==> 1tick/10ms */
	tmp = sysSetTimerEvent(TIMER0, 2, (PVOID)Keypad_timer);		/* 2 ticks call back */
	DBG_PRINTF("No. of Event [%d]\n", tmp);
	
	btime = sysGetTicks(TIMER0);
	etime = btime;
	while ((etime - btime) <= 1000)
	{

		if(DrvADC_KeyDetection(u32Channel, &u32KeyCode)==Successful){	//ADC is ready					
			if( u32KeyCode!=0x00 ) 
				sysprintf("Key Scan code = 0x%x\n", u32KeyCode);	
		}							
		etime = sysGetTicks(TIMER0);
	}	
	sysClearTimerEvent(TIMER0, tmp);
	return Successful;
}
