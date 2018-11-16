/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved. *
 *                                                              *
 ****************************************************************/
 
#ifndef __W55FA92_AUDIOREC_H__
#define __W55FA92_AUDIOREC_H__

#include "wblib.h"
// #include header file
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
// E_VIDEOIN_INVALID_COLOR_MODE				Invalid color mode
#define AR_ERROR_CODE						0xB800E000
#define E_AR_BUSY  						(0xB800E000 | 0x01)

typedef void (*PFN_AUR_CALLBACK)(void);


typedef enum{
	eAUR_MODE_0 = 0,
	eAUR_MODE_1,
	eAUR_MODE_2,
	eAUR_MODE_3
}E_AUR_MODE;


typedef enum{ 
	eAUR_CLAMP_GAIN_0 = 0,         
	eAUR_CLAMP_GAIN_P1P6,                                             
	eAUR_CLAMP_GAIN_P3P2,
	eAUR_CLAMP_GAIN_P4P8,
	eAUR_CLAMP_GAIN_P6P4,
	eAUR_CLAMP_GAIN_P8,
	eAUR_CLAMP_GAIN_P9P6,
	eAUR_CLAMP_GAIN_P11P2,
	eAUR_CLAMP_GAIN_P12P8,
	eAUR_CLAMP_GAIN_P14P4,
	eAUR_CLAMP_GAIN_P16,
	eAUR_CLAMP_GAIN_P17P6,
	eAUR_CLAMP_GAIN_P19P2,
	eAUR_CLAMP_GAIN_P20P8,
	eAUR_CLAMP_GAIN_P22P4	
}E_AUR_MAX_GAIN, E_AUR_MIN_GAIN;

typedef enum
{
	eAUR_SPS_48000 = 48000,
	eAUR_SPS_44100 = 44100,
	eAUR_SPS_32000 = 32000,	
	eAUR_SPS_24000 = 24000,
	eAUR_SPS_22050 = 22050,
	eAUR_SPS_16000 = 16000,
	eAUR_SPS_12000 = 12000,
	eAUR_SPS_11025 = 11025,
	eAUR_SPS_8000 = 8000,
	eAUR_SPS_96000 = 96000,
	eAUR_SPS_192000 = 192000		
}E_AUR_SPS;

typedef enum
{
	eAUR_ORDER_MONO_32BITS =0,
	eAUR_ORDER_MONO_16BITS,
	eAUR_ORDER_STEREO_16BITS,
	eAUR_ORDER_MONO_24BITS
}E_AUR_ORDER;

typedef enum
{
	eAUR_DIGI_MIC_GAIN_P0 =0,
	eAUR_DIGI_MIC_GAIN_P1P6,
	eAUR_DIGI_MIC_GAIN_P3P2,
	eAUR_DIGI_MIC_GAIN_P4P8,
	eAUR_DIGI_MIC_GAIN_P6P4,
	eAUR_DIGI_MIC_GAIN_P8,
	eAUR_DIGI_MIC_GAIN_P9P6,
	eAUR_DIGI_MIC_GAIN_P11P2,
	eAUR_DIGI_MIC_GAIN_P12P8,
	eAUR_DIGI_MIC_GAIN_P14P4,
	eAUR_DIGI_MIC_GAIN_P16,
	eAUR_DIGI_MIC_GAIN_P17P6,
	eAUR_DIGI_MIC_GAIN_P19P2,
	eAUR_DIGI_MIC_GAIN_P20P8,
	eAUR_DIGI_MIC_GAIN_P22P4,
	eAUR_DIGI_MIC_GAIN_P24
}E_AUR_DIGI_MIC_GAIN;

typedef enum
{
	eAUR_MONO_LINE_IN = 0,
	eAUR_MONO_MIC_IN,
	eAUR_MONO_DIGITAL_MIC_IN,
	eAUR_STEREO_DIGITAL_MIC_IN,
	
	eAUR_STEREO_LINE_IN	/* Test Only */	
}E_AUR_MIC_SEL;

typedef enum
{
	eAUR_OTL_N3 = 0,                                             
	eAUR_OTL_N4P6,
	eAUR_OTL_N6P2,
	eAUR_OTL_N7P8,
	eAUR_OTL_N9P4,
	eAUR_OTL_N11,	
	eAUR_OTL_N12P6,
	eAUR_OTL_N14P2,
	eAUR_OTL_N15P8,
	eAUR_OTL_N17P4,
	eAUR_OTL_N19,
	eAUR_OTL_N20P6,
	eAUR_OTL_N22P2,
	eAUR_OTL_N23P8,
	eAUR_OTL_N25P4
}E_AUR_AGC_LEVEL;	

typedef enum
{
	REG_ADC_H20 = 0x20,                                             
	REG_ADC_H21 = 0x21,
	REG_ADC_H22  = 0x22,
	REG_ADC_H23  = 0x23,
	REG_ADC_H24  = 0x24,
	REG_ADC_H25  = 0x25,
	REG_ADC_H26  = 0x26,
	REG_ADC_H29 = 0x29
}E_AUR_ADC_ADDR;	


/* API  */
INT32 DrvAUR_AutoClampingGain(UINT32 u32MaxGain, UINT32 u32MinGain);
INT32 DrvAUR_AutoGainTiming(UINT32 u32Attack, UINT32 u32Recovery, UINT32 u32Hold);
//INT32 DrvAUR_NoiseGatCtrl(BOOL bIsEnable, UINT32 u32Gain, UINT32 u32Level);
//INT32 DrvAUR_NoiseGateTiming(UINT32 u32DelayTime, UINT32 u32InTime, UINT32 u32OutTime);
INT32 DrvAUR_AudioI2cRead(E_AUR_ADC_ADDR eAddr, UINT8* p8Data);
INT32 DrvAUR_AudioI2cWrite(E_AUR_ADC_ADDR eAddr, UINT32 u32Data);
INT32 DrvAUR_SetSampleRate(E_AUR_SPS eSampleRate);
VOID DrvAUR_SetDigiMicGain(BOOL bIsEnable, E_AUR_DIGI_MIC_GAIN eDigiGain);
INT32 DrvAUR_AutoGainCtrl(BOOL bIsEnable, BOOL bIsChangeStep, E_AUR_AGC_LEVEL eLevel);	
VOID DrvAUR_StartRecord(E_AUR_MODE eMode);
VOID DrvAUR_StopRecord(void);
INT32 DrvAUR_Open(E_AUR_MIC_SEL eMIC, BOOL bIsCoworkEDMA);
void DrvAUR_EnableInt(void);
void DrvAUR_DisableInt(void);
INT32 DrvAUR_InstallCallback(PFN_AUR_CALLBACK pfnCallback, PFN_AUR_CALLBACK* pfnOldCallback);
VOID DrvAUR_SetDataOrder(E_AUR_ORDER eOrder);
INT32 DrvAUR_Close(void);


#ifdef __cplusplus
}
#endif

#endif

















