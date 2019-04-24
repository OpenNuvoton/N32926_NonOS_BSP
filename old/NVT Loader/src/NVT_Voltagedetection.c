#include <stdio.h>
#include "wblib.h"
#include "nvtloader.h"
#include "w55fa92_ts_adc.h"

//static volatile BOOL VoltageDetect_time = FALSE;
static volatile INT32 i32VoltageValue = 0;
static volatile BOOL bIsValidVoltageDet = FALSE;
static UINT32 u32Count=0;
//static void VoltageDetect_timer(void)
//{
//	VoltageDetect_time = TRUE;
//}
static void VoltageDetect_callback(UINT32 u32code)
{
	bIsValidVoltageDet = TRUE;
	i32VoltageValue = u32code;
	u32Count = u32Count+1;
	if(u32code==0){
		sysprintf("Voltage Detect Value %d = %d\n", u32Count, u32code);
	}	
	sysprintf("Voltage Detect Value %d = %d\n", u32Count, u32code);
}
INT32 read_voltage(UINT32 u32Channel)
{
	PFN_ADC_CALLBACK pfnOldCallback;
	

	
	DrvADC_InstallCallback(eADC_AIN,
						VoltageDetect_callback,
						&pfnOldCallback);
		
	if(DrvADC_VoltageDetection(u32Channel)==Successful){	//ADC is ready					
		if(bIsValidVoltageDet==TRUE){ //Key code has been updated				
			bIsValidVoltageDet = FALSE;
		}
	}
	while(bIsValidVoltageDet==FALSE);
	return i32VoltageValue;
}