#ifndef __AUREXAMPLE_H_
#define __AUREXAMPLE_H_

#include "W55fa92_AudioRec.h"

#define __ENABLE_CACHE__
#ifdef __ENABLE_CACHE__	
#define E_NONCACHE_BIT	0x80000000	
#else
#define E_NONCACHE_BIT	0x00000000
#endif 

#define E_MODE1
				
extern INT32 g_pi32AudSampleBuf[];		/* Reserved 20s * 4 * 192000 */ 
void AudioRecordSampleDone(void);
extern volatile BOOL bIsAudioSampleDone;

INT32 Emu_AudioRecordAnalogMIC_EDMA_Sample16Bit_AGC(E_AUR_MIC_SEL eMicType, UINT32 u32SamplingRate, UINT32 u32RecTime);


extern INT32 g_pi32AudSampleBuf[];
extern UINT32 g_u32DstBuf[];

INT32 WriteFile(char* szAsciiName, PUINT16 pu16BufAddr, UINT32 u32Length);
INT32 AudioOpenFile(char* szAsciiName);
INT32 AudioWriteFileData(INT hFile, UINT16* pu16BufAddr, UINT32 u32Length);
INT32 AudioWriteFileClose(INT32 hFile);
INT32 AudioWriteFileHead(char* szAsciiName, UINT32 u32Length,	UINT32 u32SampleRate);

void edmaCallback(UINT32 u32WrapStatus);
int initEDMA(UINT32* u32EdmaChannel);
void releaseEDMA(UINT32 u32FreeChannel);
extern volatile BOOL bIsBufferDone;	
//#define E_AUD_BUF 		(48000*4*10)			
#define E_AUD_BUF 		(16000)	/* 16000*4 = 64KB */
					
//#define DBG_PRINTF(...)	

#define DBG_PRINTF sysprintf				
#endif /*	__AUREXAMPLE_H_ */				
