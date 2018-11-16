#include <stdio.h>
#include "wblib.h"
#include "nvtloader.h"
#include "w55fa92_ts_adc.h"
/*
	EVB use AIN2(GPIOG 8) for key pad input.
*/
static volatile BOOL Keypad_time = FALSE;
static volatile UINT16 u16Keycode = FALSE;
static volatile BOOL bIsValidKey = FALSE;
static void Keypad_timer(void)
{
	Keypad_time = TRUE;
}

static void keypad_callback(UINT32 u32code)
{
	bIsValidKey = TRUE;
	u16Keycode = keymap(u32code);
}
UINT32 u32KeyTimer;
INT32 kpi_read(UINT32 u32Channel)
{
	PFN_ADC_CALLBACK pfnOldCallback;
	UINT32 u32KeyCode=0;
#if defined(__PS_DEMO_SD__) || defined(__PS_DEMO_NAND__)
	if( !(inp32(PWRON) & BIT7))//Check power key pressed
		return POWER_KEY;
	else
		return 0;	
#else
	DBG_PRINTF("ADC Keypad Demo...\n");	

	u32KeyTimer = sysSetTimerEvent(TIMER0, 5, (PVOID)Keypad_timer);		/* 5 ticks call back */
	DBG_PRINTF("No. of Event [%d]\n", u32KeyTimer);
	
	DrvADC_InstallCallback(eADC_KEY,
						keypad_callback,
						&pfnOldCallback);
	if(DrvADC_KeyDetection(u32Channel, &u32KeyCode)==Successful){	//ADC is ready					
		
	}else {
		if(u32KeyCode!=0)
			sysprintf("Key Scan code = 0x%x\n", u32KeyCode);		
	}
	while(u32KeyTimer==FALSE);							
	if(bIsValidKey==TRUE)	
		return u32KeyCode;
	else 	
		return 0;
#endif		
}		
//	sysClearTimerEvent(TIMER0, tmp);
