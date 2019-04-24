/***************************************************************************
 *                                                                         									     *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              					     *
 *                                                                         									     *
 ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include "wblib.h"
#include "W55FA92_AudioRec.h"
#include "W55FA92_edma.h"
#include "nvtfat.h"
#include "DrvEDMA.h"

#include "AurExample.h"

typedef struct tagOtl
{
	UINT32 u32Otl;
	char *pString;
}S_OTL;



INT32 Emu_AudioRecordAnalogMIC_EDMA_Sample16Bit_AGC(E_AUR_MIC_SEL eMicType, UINT32 u32SamplingRate, UINT32 u32RecTime)
{
	char szAsciiName[256];
	char szAppendName[256];
	INT32 hFile;
	INT32 i32Idx;
	INT32 i32FileLength, u32MaxSupport;	
	UINT32 i32ErrCode = Successful, u32Length, u32EdmaChannel, u32SampleRate;
	PFN_AUR_CALLBACK pfnOldCallback;
///	UINT32 u32DstBuf;  
	
	UINT32 aArraySampleRate[]= {							
							eAUR_SPS_48000, 						
							eAUR_SPS_44100, eAUR_SPS_32000, eAUR_SPS_24000, eAUR_SPS_22050,
							eAUR_SPS_16000, eAUR_SPS_12000, eAUR_SPS_11025, eAUR_SPS_8000, 
							eAUR_SPS_96000, 
							eAUR_SPS_192000					
						};	
	u32MaxSupport = sizeof(aArraySampleRate)/sizeof(aArraySampleRate[0]);
	for(i32Idx=0; i32Idx<u32MaxSupport ; i32Idx=i32Idx+1)
		if(u32SamplingRate==aArraySampleRate[i32Idx])
			break;
	if( i32Idx == u32MaxSupport){
		sysprintf("Not in support list\n")	;
		return 0;	
	}
	
	DrvAUR_Open(eMicType, TRUE);
	DrvAUR_InstallCallback(AudioRecordSampleDone, &pfnOldCallback);	
	if(eMicType == eAUR_MONO_MIC_IN){
		DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)0x22, 0x1E);			/* Pregain */
		DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)0x23, 0x0E);			/* No any amplifier */
		DrvAUR_DisableInt();
		DrvAUR_SetSampleRate((E_AUR_SPS)aArraySampleRate[i32Idx]);
		DrvAUR_AutoGainTiming(1,1,1);		
		DrvAUR_AutoGainCtrl(TRUE, TRUE, eAUR_OTL_N12P6);						
	}else if((eMicType == eAUR_MONO_DIGITAL_MIC_IN) || (eMicType == eAUR_STEREO_DIGITAL_MIC_IN)){
		DrvAUR_SetDigiMicGain(TRUE, eAUR_DIGI_MIC_GAIN_P19P2);
		DrvAUR_DisableInt();
		DrvAUR_SetSampleRate((E_AUR_SPS)aArraySampleRate[i32Idx]);
	}
	DrvAUR_SetDataOrder(eAUR_ORDER_MONO_16BITS);	
	szAppendName[0]=0;
    	szAsciiName[0] = 0;
	sprintf(szAppendName, "C:\\AudioRecordSampleRate_%d.pcm", aArraySampleRate[i32Idx]);
	strcat(szAsciiName, szAppendName);
	sysprintf("Save file-%s .....", szAsciiName);
#if 0		
	hFile = AudioOpenFile(szAsciiName);
#else	
	u32SampleRate = aArraySampleRate[i32Idx];
	hFile = AudioWriteFileHead(szAsciiName,							
							u32SampleRate*2*u32RecTime,		
							u32SampleRate);
#endif	
	outp32(REG_AR_CON, inp32(REG_AR_CON)|AR_EDMA);			
	sysDelay(30);		/* Delay 300ms for OTL stable */		
	initEDMA(&u32EdmaChannel);
	
	//Combine with EDMA, only mode 1 can be set. 
	DrvAUR_StartRecord(eAUR_MODE_1); 

	u32Length = 0;
	bIsBufferDone = 0;
	
///	u32DstBuf = (UINT32)g_u32DstBuf | 0x80000000;
	DrvEDMA_CHEnablelTransfer((E_DRVEDMA_CHANNEL_INDEX)u32EdmaChannel);
	i32FileLength = u32SamplingRate*2*u32RecTime;
	do
	{
		if(bIsBufferDone==1)
		{			
			AudioWriteFileData(hFile,
							(UINT16*)(((UINT32)g_pi32AudSampleBuf+E_AUD_BUF/2) |E_NONCACHE_BIT),
							E_AUD_BUF/2);															
			u32Length = u32Length+E_AUD_BUF/2;		
			bIsBufferDone = 0;		
		}
		else if(bIsBufferDone==2)
		{		
			AudioWriteFileData(hFile,
							(PUINT16)((UINT32)(&g_pi32AudSampleBuf) | E_NONCACHE_BIT),
							E_AUD_BUF/2);															
			u32Length = u32Length+E_AUD_BUF/2;	
			bIsBufferDone = 0;			
		}				
	}while(u32Length<i32FileLength);
	
	DrvAUR_StopRecord();
	
	releaseEDMA(u32EdmaChannel);
	sysprintf("EDMA release \n ");				
	AudioWriteFileClose(hFile);						   	    		    		  
	
	DrvAUR_Close();

	sysprintf("Done......\n");
	return i32ErrCode;
}
