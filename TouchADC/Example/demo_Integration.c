#include <stdio.h>
#include "wblib.h"
#include "demo.h"
#include "W55FA92_ADC.h"


//===================================== For Voltage Detection =======================
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
	sysprintf("Voltage Detect Value %d = %d\n", u32Count, u32code);
}
//===================================== For Keypad ==============================
static volatile BOOL Keypad_time = FALSE;
static void Keypad_timer(void)
{
	Keypad_time = TRUE;
}
//===================================== For Touch ==============================
static volatile BOOL TouchPanel_time = FALSE;
static volatile UINT16 u16X, u16Y;

static void TouchPanel_timer(void)
{
	TouchPanel_time = TRUE;
}
static void TouchPanel_callback(UINT32 u32code)
{

}
static void Pressure_callback(UINT32 u32code)
{
	UINT16 u16Z1, u16Z2;
	float Z1, Z2; 
	float Rtouch; 
	UINT32 u32Dec;	
	UINT32 u32Fraction=3;
	
	u16Z1 =  (u32code>>16)&0x7FFF;
	u16Z2 =  u32code & 0x7FFF;
	Z1 = u16Z1;
	Z2 = u16Z2;
	if((u32code&(BIT31 | BIT15)) == (BIT31 | BIT15)){
		sysprintf("(Z1, Z2) = %dx%d\n", u16Z1, u16Z2);
		Rtouch = 671*u16X/4096.0*(Z2/Z1-1); 					
		u32Dec=Rtouch;
		sysprintf("Rtouch = %d.", u32Dec);	
		Rtouch = Rtouch-u32Dec;
		while((Rtouch!=0.) && (u32Fraction!=0))
		{		
			Rtouch = Rtouch*10.;	
			u32Dec = Rtouch;	
			sysprintf("%d",u32Dec); 	
			Rtouch = Rtouch - u32Dec;
			u32Fraction = u32Fraction-1;
		}
		sysprintf("\n\n"); 			
	}else
		sysprintf("Pressure is invalid 0x%x\n", u32code);			
}	
static void Position_callback(UINT32 u32code)	
{		
	u16X =  (u32code>>16)&0x7FFF;
	u16Y =  u32code & 0x7FFF;
	if((u32code&(BIT31 | BIT15)) == (BIT31 | BIT15))
		sysprintf("(X, Y) = %dx%d\n", u16X, u16Y);
	else
		sysprintf("Position is invalid 0x%x\n", u32code);	
}
INT32 Integration(void)
{
	UINT32 tmp, btime, etime;
	UINT32 u32ExtFreq;//, u32Item;
	UINT32 u32KeyCode;
	BOOL bIs5Wire;
	INT32 ret;
	PFN_ADC_CALLBACK pfnOldCallback;
	
	u32ExtFreq = sysGetExternalClock();
	sysSetTimerReferenceClock(TIMER0, u32ExtFreq); 					//External Crystal
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);						/* 100 ticks/per sec ==> 1tick/10ms */
	
	/* Init Keypad */
	tmp = sysSetTimerEvent(TIMER0, 2, (PVOID)Keypad_timer);		/* 2 ticks call back */
	DBG_PRINTF("No. of Event [%d]\n", tmp);
	
	/* Init Voltage Detect */
	tmp = sysSetTimerEvent(TIMER0, 500, (PVOID)VoltageDetect_timer);		/* 100 ticks = 1s call back */
	DBG_PRINTF("No. of Event [%d]\n", tmp);	
	DrvADC_InstallCallback(eADC_AIN,
						VoltageDetect_callback,
						&pfnOldCallback);
						
	/* Init Touch Panel */	
	DBG_PRINTF("ADC Touch Panel Demo...\n");		
	tmp = sysSetTimerEvent(TIMER0, 2, (PVOID)TouchPanel_timer);		/* 2 ticks  call back */
	DBG_PRINTF("No. of Event [%d]\n", tmp);
	DBG_PRINTF("Please use 4 wire touch panel\n");
	bIs5Wire = FALSE;
	
	
	DrvADC_Open();

#if 0 /* For touch panel */
	DrvADC_InstallCallback(eADC_TOUCH,
						TouchPanel_callback,
						&pfnOldCallback);
	
	DrvADC_InstallCallback(eADC_POSITION,
						Position_callback,
						&pfnOldCallback);
						
	DrvADC_InstallCallback(eADC_PRESSURE,
						Pressure_callback,
						&pfnOldCallback);					
#endif

	btime = sysGetTicks(TIMER0);
	etime = btime;
	while ((etime - btime) <= 5000)
	{
		if(Keypad_time==TRUE){
			Keypad_time = FALSE;
			do{
				ret = DrvADC_KeyDetection(2, &u32KeyCode);
				if(ret == Successful)
					sysprintf("Key Scan code = 0x%x\n", u32KeyCode);	
			}while(0);
			if(ret ==E_ADC_BUSY)
				Keypad_time = TRUE; /* do again */
		}
		if(VoltageDetect_time==TRUE){
			VoltageDetect_time = FALSE;
			do{
				ret = DrvADC_VoltageDetection(1);
			}while(0);			
			if(ret == E_ADC_BUSY)
				VoltageDetect_time =  TRUE; /* do again */
		}		
	#if 0 /* For touch panel */
		if(TouchPanel_time==TRUE){
			TouchPanel_time = FALSE;
			do{
				ret = DrvADC_PenDetection(bIs5Wire);
			}while(0);		
			if(ret == E_ADC_BUSY)
				TouchPanel_time =  TRUE; /* do again */	
		}
	#else
		if(TouchPanel_time==TRUE)
		{
			UINT16 x, y;
			TouchPanel_time = FALSE;
			if(IsPenDown()==TRUE)
			{
				if(adc_read(0, &x, &y)==1)
					sysprintf("(x, y)= (0x%x, 0x%x)\n", x, y);
				else
					DBG_PRINTF("Invaliable data\n");	
			}
		}	
	#endif	
		etime = sysGetTicks(TIMER0);
	}	
	sysClearTimerEvent(TIMER0, tmp);
	return Successful;
}
