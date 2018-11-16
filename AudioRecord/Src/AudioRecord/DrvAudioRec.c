/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.  *
 *                                                              *
 ****************************************************************/

#include <stdio.h>
#include <string.h>
#include "w55fa92_reg.h"
#include "w55fa92_AudioRec.h"
#include "wblib.h"
#define REAL_CHIP

#define 	REG_ADC_H20		0x20
	#define 	HPF_EN			BIT3
	#define 	STEREO_ADC		BIT2
	#define 	OSR				BIT1
	#define 	ADCEN			BIT0
#define 	REG_ADC_H21		0x21
	#define 	PDBIAS_L			BIT6
	#define 	PDPGAL_L			BIT5
	#define 	PDPGAR_L			BIT4
	#define 	PDL_L			BIT3
	#define 	PDR_L			BIT2
	#define 	ADC_EN_SEL		NVTBIT(1, 0)
	
#define 	REG_ADC_H22		0x22
	#define 	ADC_VOL_BOOST	BIT4
	#define 	ADC_VOLL		NVTBIT(3, 0)
	
#define 	REG_ADC_H23		0x23
	#define 	ADC_VOL_OD		BIT4
	#define 	ADC_VOLR		NVTBIT(3, 0)	
	
#define 	REG_ADC_H24		0x24
	#define 	RESADJ			NVTBIT(2, 0)

#define 	REG_ADC_H25		0x25
	#define 	BYPASSPLL		BIT7
	#define 	PDPLL			BIT6
	#define 	SRS				NVTBIT(4, 0)

#define 	REG_ADC_H26		0x26
	#define 	ADCSWRESET		BIT1
	
#define 	REG_ADC_H29		0x29
	#define 	MICIN_SEL		BIT7
	
INT32 DrvAUR_AutoClampingGain(UINT32 u32MaxGain, UINT32 u32MinGain)
{
	outp32(REG_AR_AGC1, (inp32(REG_AR_AGC1) & ~(MAXGAIN|MINGAIN))  | 
						(((u32MaxGain<<20)&MAXGAIN) | ((u32MinGain<<16)&MINGAIN)) );
	return Successful;
}
INT32 DrvAUR_AutoGainTiming(UINT32 u32Attack, UINT32 u32Recovery, UINT32 u32Hold)
{
	outp32(REG_AR_AGC1, (inp32(REG_AR_AGC1) & ~RECOVERY)  | 
						(u32Recovery& RECOVERY) );
						
	outp32(REG_AR_AGC2, (inp32(REG_AR_AGC2) & ~(ATTACK | HOLD))  | 
						(((u32Attack<<16) & ATTACK) |  (u32Hold & HOLD)) );	
	return Successful;
}
INT32 DrvAUR_NoiseGatCtrl(BOOL bIsEnable, UINT32 u32Gain, UINT32 u32Level)
{
	outp32(REG_AR_NG, (inp32(REG_AR_NG) & ~(NG_GAIN | NG_LEVEL))  | 
						( ((u32Gain<<8)&NG_GAIN) | (u32Level&NG_LEVEL)) );
	outp32(REG_AR_NG, (inp32(REG_AR_NG) & ~NG_EN) | 	
						((bIsEnable<<31)&NG_EN) );			
	return Successful;
}
INT32 DrvAUR_NoiseGateTiming(UINT32 u32DelayTime, UINT32 u32InTime, UINT32 u32OutTime)
{
	outp32(REG_AR_NG, (inp32(REG_AR_NG) & ~(IN_NG_TIME | OUT_NG_TIME))  | 
						( ((u32InTime<<20)&IN_NG_TIME) | ((u32OutTime<<16) &OUT_NG_TIME)) );
	outp32(REG_AR_NG, (inp32(REG_AR_NG) & ~DLYTIME) | 	
						((u32DelayTime<<24)&DLYTIME) );	
	return Successful;
}

INT32 DrvAUR_AudioI2cRead(E_AUR_ADC_ADDR u32Addr, UINT8* p8Data)
{
	UINT32 u32Ctrl = 0;
	/* 2013/0603	*/
	BOOL bIsEnableAgc = FALSE;
	bIsEnableAgc = inp32(REG_AR_AGC1)&AGC_EN;
	if(bIsEnableAgc ==  AGC_EN){/* Disable AGC Before I2C Write*/
		outp32(REG_AR_AGC1, inp32(REG_AR_AGC1) & ~AGC_EN);
	}
	/* 2013/0603	*/
	
	do
	{		
	}while((inp32(REG_SDADC_CTL)&AR_BUSY) != 0);
	
	u32Addr = (E_AUR_ADC_ADDR)(u32Addr & 0xFF);
	u32Ctrl = (u32Ctrl | (0x800000 | (u32Addr<<8)));
	u32Ctrl = u32Ctrl | 0x20000000;
	outp32(REG_SDADC_CTL, u32Ctrl);
	do
	{		
	}while((inp32(REG_SDADC_CTL)&AR_BUSY) != 0);
	*p8Data = inp32(REG_SDADC_CTL)&0xFF;
	
	/* 2013/0603 */
	do
	{		
	}while((inp32(REG_SDADC_CTL)&AR_BUSY) != 0);		
	if(bIsEnableAgc ==  AGC_EN){/* Enable AGC need wait I2C free */
		outp32(REG_AR_AGC1, inp32(REG_AR_AGC1) | AGC_EN);
	}
	/* 2013/0603 */
	
	return Successful;
}




INT32 DrvAUR_AudioI2cWrite(E_AUR_ADC_ADDR u32Addr, UINT32 u32Data)
{
	UINT32 u32Ctrl = 0;
	
	/* 2013/0603	*/
	BOOL bIsEnableAgc = FALSE;
	bIsEnableAgc = inp32(REG_AR_AGC1)&AGC_EN;
	if(bIsEnableAgc ==  AGC_EN){/* Disable AGC Before I2C Write*/
		outp32(REG_AR_AGC1, inp32(REG_AR_AGC1) & ~AGC_EN);
	}
	/* 2013/0603	*/
	
	do
	{		
	}while((inp32(REG_SDADC_CTL)&AR_BUSY) != 0);
	
	if(u32Addr == REG_ADC_H22)
	{//Left
		if(u32Data&BIT4)
			outp32(REG_SDADC_AGBIT, inp32(REG_SDADC_AGBIT) | 0x10);
		else
			outp32(REG_SDADC_AGBIT, inp32(REG_SDADC_AGBIT) & ~0x10);	
	}
	if(u32Addr==REG_ADC_H23)
	{//Right
		if(u32Data&BIT4)
			outp32(REG_SDADC_AGBIT, inp32(REG_SDADC_AGBIT) | 0x1);
		else
			outp32(REG_SDADC_AGBIT, inp32(REG_SDADC_AGBIT) & ~0x1);	
	}
	u32Addr = (E_AUR_ADC_ADDR)(u32Addr & 0xFF);
	u32Data = u32Data & 0xFF;
	
	u32Ctrl = u32Ctrl | (0x810000 | (u32Addr<<8)  | u32Data) ;
	u32Ctrl = u32Ctrl | 0x20000000;
	outp32(REG_SDADC_CTL, u32Ctrl);		
	
	/* 2013/0603 */
	do
	{		
	}while((inp32(REG_SDADC_CTL)&AR_BUSY) != 0);		
	if(bIsEnableAgc ==  AGC_EN){/* Enable AGC need wait I2C free */
		outp32(REG_AR_AGC1, inp32(REG_AR_AGC1) | AGC_EN);
	}
	/* 2013/0603 */
	
	return Successful;
}

VOID DrvAUR_StartRecord(E_AUR_MODE eMode)
{
	UINT32 u32Reg = inp32(REG_AR_CON) & ~INT_MOD; 
	u32Reg = u32Reg | ((eMode <<2)&INT_MOD);
	outp32(REG_AR_CON, u32Reg);
}
VOID DrvAUR_StopRecord(void)
{
	UINT8 u8Data;
	outp32(REG_AR_CON , inp32(REG_AR_CON)& ~AR_EDMA);
	DrvAUR_AudioI2cRead((E_AUR_ADC_ADDR)REG_ADC_H20, &u8Data); 
	u8Data = u8Data & ~ADCEN;
	DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H20, u8Data);
}

INT32 DrvAUR_SetSampleRate(E_AUR_SPS eSampleRate)
{
	UINT8 u8I2cData;
	INT32 i32ErrCode = 0;
	/* Set Sample Rate */
	do
	{
		i32ErrCode = DrvAUR_AudioI2cRead((E_AUR_ADC_ADDR)REG_ADC_H25, &u8I2cData);
	}while(i32ErrCode != Successful);
	sysDelay(1);
	u8I2cData = u8I2cData &  (~SRS);
	
	switch(eSampleRate)
	{
		case eAUR_SPS_48000:
			u8I2cData = u8I2cData | 0x0;
			break;
		case eAUR_SPS_44100:
			u8I2cData = u8I2cData | 0x8;
			break;
		case eAUR_SPS_32000:
			u8I2cData = u8I2cData | 0x10;
			break;	
		case eAUR_SPS_24000:
			u8I2cData = u8I2cData | 0x01;
			break;
		case eAUR_SPS_22050:
			u8I2cData = u8I2cData | 0x09;
			break;	
		case eAUR_SPS_16000:
			u8I2cData = u8I2cData | 0x11;
			break;
		case eAUR_SPS_12000:
			u8I2cData = u8I2cData | 0x02;
			break;
		case eAUR_SPS_11025:
			u8I2cData = u8I2cData | 0x0A;
			break;
		case eAUR_SPS_8000:
			u8I2cData = u8I2cData | 0x12;
			break;
		case eAUR_SPS_96000:
			u8I2cData = u8I2cData | 0x04;
			break;
		case eAUR_SPS_192000:
			u8I2cData = u8I2cData | 0x04;
			break;
	}

	do
	{
		i32ErrCode = DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H25, u8I2cData);
	}while(i32ErrCode != Successful);
	
	/* Set OSR */
	do
	{
		DrvAUR_AudioI2cRead((E_AUR_ADC_ADDR)REG_ADC_H20, &u8I2cData);
	}while(i32ErrCode != Successful);
	sysprintf("Readback H20 = 0x%x\n", u8I2cData);
	if(eSampleRate==eAUR_SPS_192000)
	{
		u8I2cData = u8I2cData | OSR;
		sysprintf("Will Write H20 = 0x%x\n", u8I2cData);
		do
		{
			i32ErrCode = DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H20, u8I2cData);
		}while(i32ErrCode != Successful);
		
		DrvAUR_AudioI2cRead((E_AUR_ADC_ADDR)REG_ADC_H20, &u8I2cData);
		sysprintf("H20 = 0x%x\n", u8I2cData);
		
	}
	else
	{
		u8I2cData = u8I2cData & (~OSR);
		sysprintf("Will Write H20 = 0x%x\n", u8I2cData);
		do
		{
			i32ErrCode = DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H20, u8I2cData);
		}while(i32ErrCode != Successful);	
	}	
	return Successful;
}
VOID DrvAUR_SetDataOrder(E_AUR_ORDER eOrder)
{
	UINT32 u32RegData = (inp32(REG_AR_DIGIM) & ~SMPL_MODE) |  (eOrder<<8);
	outp32(REG_AR_DIGIM, u32RegData);
}

VOID DrvAUR_SetDigiMicGain(BOOL bIsEnable, E_AUR_DIGI_MIC_GAIN eDigiGain)
{//Digital MIC only
	UINT32 u32RegData = inp32(REG_AR_DIGIM) & ~ (DIGIM_EN|DIGIM_LV);
	outp32(REG_AR_DIGIM, u32RegData | (((bIsEnable<<7) |DIGIM_EN) | (eDigiGain&DIGIM_LV)) );
}
INT32 DrvAUR_AutoGainCtrl(BOOL bIsEnable, BOOL bIsChangeStep, E_AUR_AGC_LEVEL eLevel)
{
	outp32(REG_AR_AGC1, (inp32(REG_AR_AGC1) & ~(GSTEP | OTL)) | 
						(((bIsChangeStep<<28)&GSTEP) | ((eLevel<<24) & OTL)) );	
	outp32(REG_AR_AGC1, (inp32(REG_AR_AGC1) & ~AGC_EN) | 	
						((bIsEnable << 31) & AGC_EN)	);						
	return Successful;
}

static PFN_AUR_CALLBACK g_psADCCallBack;

INT32 
DrvAUR_InstallCallback(	
	PFN_AUR_CALLBACK pfnCallback,
	PFN_AUR_CALLBACK* pfnOldCallback
	)
{
 
	*pfnOldCallback = g_psADCCallBack;
    	g_psADCCallBack = pfnCallback; 	    		
    	return Successful;
}

static void 
AurIntHandler(void)
{
	if(g_psADCCallBack !=0)
	        g_psADCCallBack();  
	outp32(REG_AR_CON, (inp32(REG_AR_CON) | AR_INT));	/* Write one clear */
}

INT32 DrvAUR_Open(E_AUR_MIC_SEL eMIC, BOOL bIsCoworkEDMA)
{
	
	UINT8 u8RegDac;
	outp32(REG_APBCLK, inp32(REG_APBCLK) | ADC_CKE);
#ifdef  REAL_CHIP
	outp32(REG_CLKDIV3, (inp32(REG_CLKDIV3) & ~(ADC_N1 | ADC_S| ADC_N0)) );				/* Fed to ADC clock need 12MHz=External clock */
#else	
	outp32(REG_CLKDIV3, (inp32(REG_CLKDIV3) & ~(ADC_N1 | ADC_S| ADC_N0)) | (1<<24) );		/* FPGA: Divider need to be 1 at least for I2C clock*/
#endif 		
	outp32(REG_APBIPRST, ADCRST);
	outp32(REG_APBIPRST, 0);	
	
	outp32(REG_AR_CON, inp32(REG_AR_CON) & ~AR_RST);
	outp32(REG_AR_CON, inp32(REG_AR_CON) | AR_RST);		/* Hardware reset ADC codec (Only for real chip)*/
	
	
	//outp32(REG_SDADC_CTL, ((inp32(REG_SDADC_CTL) & ~SCK_DIV) | (0x3F<<24)) );
	DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H25, 0x00);
	DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H26, 0x02); 
	DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H26, 0x00); /* Reset AD Filter and I2S parts except I2C block */
	DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H26, 0x02);
	
	DrvAUR_AudioI2cRead((E_AUR_ADC_ADDR)REG_ADC_H20, &u8RegDac);
	if ((eMIC == eAUR_STEREO_DIGITAL_MIC_IN) || (eMIC == eAUR_STEREO_LINE_IN))
		u8RegDac = u8RegDac | (HPF_EN  | ADCEN | STEREO_ADC);
	else
		u8RegDac = (u8RegDac | (HPF_EN  | ADCEN)) & ~STEREO_ADC;
		
	if( (eMIC == eAUR_MONO_DIGITAL_MIC_IN) || (eMIC == eAUR_STEREO_DIGITAL_MIC_IN) )		
		outp32(REG_GPDFUN1, (inp32(REG_GPDFUN1)&~(MF_GPD14 | MF_GPD12))|0x04040000);	
		
		
	sysprintf("Reg 20 will write value = 0x%x\n", u8RegDac);	
	DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H20, u8RegDac);	
	//DrvAUR_AudioI2cWrite(REG_ADC_H29, 0x80);		/* Analog MIC */
	{
		UINT8 udata;	
		DrvAUR_AudioI2cRead((E_AUR_ADC_ADDR)REG_ADC_H20, &udata);
		sysprintf("read back Reg 20 bw written value = 0x%x\n\n\n", udata);	
	}
	if(bIsCoworkEDMA)
		outp32(REG_AR_CON , inp32(REG_AR_CON)| AR_EDMA);
	else
		outp32(REG_AR_CON , inp32(REG_AR_CON)& ~AR_EDMA);	
	
	switch(eMIC)
	{
		case eAUR_STEREO_LINE_IN:
		case	eAUR_MONO_LINE_IN:
			DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H29, 0x0);		/* Analog Line in */
			u8RegDac = (UINT8) (~(PDBIAS_L | PDPGAL_L | PDPGAR_L | PDL_L | PDR_L | 3));	
			DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H21, u8RegDac);
			break;
		case eAUR_MONO_MIC_IN:
			DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H29, 0x0);		/* Analog MIC */
			u8RegDac = (UINT8)(~(PDBIAS_L | PDPGAL_L | PDPGAR_L | PDL_L | PDR_L | ADC_EN_SEL)  | 0x02);	
			DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H21, u8RegDac);
			break;			
		case eAUR_MONO_DIGITAL_MIC_IN:
			DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H29, 0x80);			/* Digital MIC */
			u8RegDac = (UINT8)(~(PDBIAS_L | PDPGAL_L | PDPGAR_L | PDL_L | PDR_L | ADC_EN_SEL)  | 0x02);	
			DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H21, u8RegDac);			
			break;			
		case 	eAUR_STEREO_DIGITAL_MIC_IN:
			DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H29, 0x80);			/* Digital MIC */
			u8RegDac = (UINT8)(~(PDBIAS_L | PDPGAL_L | PDPGAR_L | PDL_L | PDR_L | ADC_EN_SEL) | 0x02);	
			DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H21, u8RegDac);			
			break;	
	
				
	}	
	

	
	sysInstallISR(IRQ_LEVEL_1, 
					IRQ_AUDIO, 
					(PVOID)AurIntHandler);
	sysEnableInterrupt(IRQ_AUDIO);		

	return Successful;		
}
void DrvAUR_EnableInt(void)
{
	outp32(REG_AR_CON, (inp32(REG_AR_CON) | AR_INT_EN) );
}	
void DrvAUR_DisableInt(void)
{
	outp32(REG_AR_CON, (inp32(REG_AR_CON) & ~AR_INT_EN) );
}	

INT DrvAUR_Close(void)
{	
	UINT8 u8RegDac;

	/* Power down VBIAS, IBIAS, Left/Right Channel PGA, Left/Right Channel SDM  */
	u8RegDac = (UINT8)(PDBIAS_L | PDPGAL_L | PDPGAR_L | PDL_L | PDR_L );	
	DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)REG_ADC_H21, u8RegDac);
		
	outp32(REG_AR_CON, inp32(REG_AR_CON) & ~AR_RST);
    	outp32(REG_AR_CON, inp32(REG_AR_CON) | AR_RST);    

	outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~ADC_CKE);
	return Successful;
}
