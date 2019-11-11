/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved. *
 *                                                              *
 ****************************************************************/
 
#ifndef __W55FA92_TOUCH_H__
#define __W55FA92_TOUCH_H__

#include "wblib.h"
#ifdef  __cplusplus
extern "C"
{
#endif

/* Define data type (struct, union¡K) */
// #define Constant


//Error message
// E_VIDEOIN_INVALID_INT					Invalid interrupt
// E_VIDEOIN_INVALID_BUF					Invalid buffer
// E_VIDEOIN_INVALID_PIPE					Invalid pipe
// E_VIDEOIN_INVALID_COLOR_MODE			Invalid color mode
#define ADC_ERROR_CODE					0xB800F000
#define E_ADC_BUSY  						(ADC_ERROR_CODE | 0x01)


#define E_TOUCH_UP					1
#define E_KEYPAD_UP					1

typedef VOID (*PFN_ADC_CALLBACK)(UINT32);


typedef enum{
	eADC_KEY 	= 0,	/* Hardware */
	eADC_TOUCH   = 1,
	eADC_AIN 	= 2,
	
	eADC_POSITION = 3,	/* Software */
	eADC_PRESSURE = 4 			
}E_ADC_INT_TYPE;

typedef enum{
	eADC_WAKEUP_KEY 	= 1,		/* Hardware */
	eADC_WAKEUP_TOUCH   = 2	
}E_ADC_WAKEUP_TYPE;

INT32 DrvADC_Open(void);
INT32 DrvADC_Close(void);
INT32 DrvADC_InstallCallback(E_ADC_INT_TYPE eIntType,
	PFN_ADC_CALLBACK pfnCallback,
	PFN_ADC_CALLBACK* pfnOldCallback);
//INT32 DrvADC_EnableInt(E_ADC_INT_TYPE eIntType);
//INT32 DrvADC_DisableInt(E_ADC_INT_TYPE eIntType);

void DrvADC_Wakeup(E_ADC_WAKEUP_TYPE eWakeupSrc); /* Only support eADC_KEY & eADC_TOUCH */
INT32 DrvADC_PenDetection(BOOL bIs5Wire);
INT32 DrvADC_KeyDetection(UINT32 u32Channel, UINT32* pu32KeyCode);
INT32 DrvADC_VoltageDetection(UINT32 u32Channel);
UINT32 keymap(UINT16 u16AinCode);



int IsPenDown(void);
int adc_read(unsigned char mode, unsigned short *x, unsigned short *y);

#ifdef __cplusplus
}
#endif

#endif /* __W55FA92_TOUCH_H__ */

















