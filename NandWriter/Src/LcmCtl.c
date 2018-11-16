#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
//#include "nvtloader.h"
#include "w55fa92_gpio.h"
//#include "w55fa95_kpi.h"
#include "w55fa92_reg.h"
/*
	Mute Control : 
	LGE: LCM power : GPA5   = 1 (Enable LCM power)
			             		= 0 (Disable LCM power)
*/
void LcmPowerInit(void)
{
#ifdef __KLE_DEMO__
	gpio_configure(GPIO_PORTA, 
					//(1<<5));	// pin number	
						5);	// pin number	
	gpio_setportdir(GPIO_PORTA, 
					(1<<5), 	// Mask 
					(1<<5));	// 1 output. 0 input.				
#endif					
}
void LcmPowerEnable(void)
{
#ifdef __KLE_DEMO__
	gpio_setportval(GPIO_PORTA, 
					(1<<5), 	//Mask
					(1<<5));	//High
#endif								
}
void LcmPowerDisable(void)
{
#ifdef __KLE_DEMO__
	gpio_setportval(GPIO_PORTA, 
					(1<<5), 	//Mask
					(0<<5));	//Low
#endif					
}
/*
	Backlight Control : 
	LGE: LCM power : GPG5   = 1 (Enable LCM backlight)
			             		= 0 (Disable LCM backlight)
*/
void LcmBacklightInit(void)
{
#ifdef __KLE_DEMO__
	gpio_configure(GPIO_PORTG, 
					//(1<<5));	// pin number	
						5);	// pin number	
	gpio_setportdir(GPIO_PORTG, 
					(1<<5), 	// Mask 
					(1<<5));	// 1 output. 0 input.				
#endif		
#ifdef __CWC_DEMO__
	gpio_configure(GPIO_PORTD, 
					//(1<<5));	// pin number	
						1);	// pin number	
	gpio_setportdir(GPIO_PORTD, 
					(1<<1), 	// Mask 
					(1<<1));	// 1 output. 0 input.	
#endif			
}
void LcmBacklightEnable(void)
{
#ifdef __KLE_DEMO__
	gpio_setportval(GPIO_PORTG, 
					(1<<5), 	//Mask
					(1<<5));	//High			
#endif					
#ifdef __CWC_DEMO__
	gpio_setportval(GPIO_PORTD, 
					(1<<1), 	//Mask
					(1<<1));	//High	
#endif					
}
void LcmBacklightDisable(void)
{
#ifdef __KLE_DEMO__
	gpio_setportval(GPIO_PORTG, 
					(1<<5), 	//Mask
					(0<<5));	//Low
#endif	
#ifdef __CWC_DEMO__
	gpio_setportval(GPIO_PORTD, 
					(1<<1), 	//Mask
					(0<<1));	//Low
#endif					
}


void LcmSaturationDelay(void)
{
#ifdef __KLE_DEMO__
	UINT32 u32Delay;	
	for (u32Delay=0;u32Delay<=10000;u32Delay++);
#endif	
}

void LcmSaturationInc(UINT32 u32Value)
{
#ifdef __KLE_DEMO__
	UINT32 u32Counter;	
	gpio_setportval(GPIO_PORTA, 0x40, 0x40);
	gpio_setportval(GPIO_PORTE, 0x2, 0x2);
	LcmSaturationDelay();	
			
	gpio_setportval(GPIO_PORTE, 0x2, 0x0);		//CS=L
	LcmSaturationDelay();
	
	for (u32Counter=1;u32Counter<=u32Value;u32Counter++)
	{		
		gpio_setportval(GPIO_PORTA, 0x40, 0x0);	//U/D=L	
		LcmSaturationDelay();
		
		gpio_setportval(GPIO_PORTA, 0x40, 0x40);	//U/D=H	
		LcmSaturationDelay();
	}	
	gpio_setportval(GPIO_PORTE, 0x2, 0x2);		//CS=H
#endif
}

void LcmSaturationDec(UINT32 u32Value)
{
#ifdef __KLE_DEMO__
	UINT32 u32Counter;
	gpio_setportval(GPIO_PORTA, 0x40, 0x0);
	gpio_setportval(GPIO_PORTE, 0x2, 0x2);
	LcmSaturationDelay();
			
	gpio_setportval(GPIO_PORTE, 0x2, 0x0);		//CS=L
	LcmSaturationDelay();
	
	for (u32Counter=1;u32Counter<=u32Value;u32Counter++)
	{
		gpio_setportval(GPIO_PORTA, 0x40, 0x0);	//U/D=L	
		LcmSaturationDelay();
		
		gpio_setportval(GPIO_PORTA, 0x40, 0x40);	//U/D=H	
		LcmSaturationDelay();
	}

	gpio_setportval(GPIO_PORTA, 0x40, 0x0);	//U/D=L	
	LcmSaturationDelay();
	gpio_setportval(GPIO_PORTE, 0x2, 0x2);		//CS=H
#endif	
}

void LcmSaturationInit(void)
{
#ifdef __KLE_DEMO__
	gpio_configure(GPIO_PORTA, 6);	//U/D
	gpio_configure(GPIO_PORTE, 1);	//CS

	gpio_setportval(GPIO_PORTA, 0x40, 0x40);
	gpio_setportval(GPIO_PORTE, 0x2, 0x2);

	gpio_setportdir(GPIO_PORTA, 0x40, 0x40);
	gpio_setportdir(GPIO_PORTE, 0x2, 0x2);

	LcmSaturationDec(64);
#endif	
}