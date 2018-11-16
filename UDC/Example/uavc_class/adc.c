/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/

/****************************************************************************
* FILENAME
*   adc.c
*
* VERSION
*   1.0
*
* DESCRIPTION
*   ADC sample application using ADC library
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*
* HISTORY
*
* REMARK
*   None
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wblib.h"
#include "W55FA92_AudioRec.h"
#include "W55FA92_edma.h"
//#include "nvtfat.h"
#include "DrvEDMA.h"
#include "usbd.h"
#include "videoclass.h"

INT32 InitializeUAC(UINT32 u32SampleRate);
void StartUAC(void);
void StopUAC(void);
void Send_AudioOneMSPacket(PUINT32 pu32Address, PUINT32 pu32Length);

#define TIMES	1

#define UAC_MUTE_ADDR		0

#define E_AUD_BUF  64*TIMES  // 64 bytes
#define TRAN_SAMPLE  16*TIMES   // 64/2 = 32 bytes, for half, 32 bytes = 2 x 16, 16 samples   

#define IN_DATA_BUF_NUM  128*TIMES


#define IGNORE_NO	300  // 1 seconds data igore, = 2 x 16 x 1000


//static volatile INT8 g_i8PcmReady = FALSE;
__align(32) INT16 g_pi16SampleBuf[E_AUD_BUF/2];		/* Keep max 16K sample rate */
__align(32) INT16 g_pi16AudioBuf[E_AUD_BUF/2*4];

volatile UINT8 bPlaying = FALSE;
volatile UINT32 g_u32IgnoreNo;

volatile INT16  s_i16RecOutPos;
volatile INT16  s_i16RecInPos;   
volatile INT16  s_i16RecInSample;

UINT32 g_u32RecorderByte;
UINT32 g_u32SampleCount;

// recorder
__align(4) INT32 g_i32RecorderAttr[5] = {
0,        // mute off
FU_VOLUME_CUR,    //2560,     // current volume,
FU_VOLUME_MIN,      //-32768,   // min
FU_VOLUME_MAX,     // 32768,    // max
1 	  // res 	
};	

volatile BOOL bIsAudioSampleDone = FALSE;
INT32 i32Count = 0;
UINT32 g_u32EdmaChannel;

void AudioRecordSampleDone(void)
{
	i32Count = i32Count+1;
	bIsAudioSampleDone = TRUE;
}	

#if 0
static void pfnRecordCallback(void)
{
//	g_i8PcmReady = TRUE;
}
#endif

//volatile BOOL bIsBufferDone=0;
void edmaCallback(UINT32 u32WrapStatus)
{
//	UINT32 u32Period, u32Attack, u32Recovery, u32Hold;
    INT16 *pi16srcADC;
    UINT32 i;

    g_u32IgnoreNo ++;	
	if(u32WrapStatus==256)
	{
//		bIsBufferDone = 1;
//		sysprintf("I %d\n\n", bIsBufferDone);
		if ( bPlaying == TRUE )	
		{
/*			if ( g_i32RecorderAttr[UAC_MUTE_ADDR] == 1 ) // mute on
			{
				for (i= 0; i< TRAN_SAMPLE; i++)
	            {
					g_pi16AudioBuf[s_i16RecInPos++] = 0 ;
                }

			}
			else
			*/
			{
/*		        if ( g_i32RecorderAttr[1] == 0 ) // volume = 0;
				{
					for (i= 0; i< TRAN_SAMPLE; i++)
		            {
						g_pi16AudioBuf[s_i16RecInPos++]= 0;
					}	
				}
				else
				*/
				{
			//        pi16srcADC = (INT16 *)((UINT32)g_pi16SampleBuf+E_AUD_BUF/2);
                    pi16srcADC = (INT16 *)(((UINT32)g_pi16SampleBuf+E_AUD_BUF/2) | 0x80000000);  			        
//               sysprintf("Buf 1 \n");
					for (i= 0; i< TRAN_SAMPLE; i++)
		            {
						g_pi16AudioBuf[s_i16RecInPos++]= *pi16srcADC++;
					}	
				}
			}			
 			if (s_i16RecInPos ==  IN_DATA_BUF_NUM)
   			{
				s_i16RecInPos = 0;
			}		
			s_i16RecInSample += TRAN_SAMPLE;
        }
	}	
	else if(u32WrapStatus==1024)
	{
		
//		bIsBufferDone = 2;		
//		sysprintf("I %d\n\n", bIsBufferDone);
		if ( bPlaying == TRUE )	
		{
/*			if ( g_i32RecorderAttr[UAC_MUTE_ADDR] == 1 ) // mute on
			{
				for (i= 0; i< TRAN_SAMPLE; i++)
	            {
					g_pi16AudioBuf[s_i16RecInPos++] = 0 ;
                }

			}
			else
			*/
			{
/*		        if ( g_i32RecorderAttr[1] == 0 ) // volume = 0;
				{
					for (i= 0; i< TRAN_SAMPLE; i++)
		            {
						g_pi16AudioBuf[s_i16RecInPos++]= 0;
					}	
				}
				else
				*/
				{
//			        pi16srcADC = (INT16 *)((UINT32)g_pi16SampleBuf);
                    pi16srcADC = (INT16 *)(((UINT32)g_pi16SampleBuf) | 0x80000000) ;
//             sysprintf("Buf 0 \n");                    
					for (i= 0; i< TRAN_SAMPLE; i++)
		            {
						g_pi16AudioBuf[s_i16RecInPos++]= *pi16srcADC++;
					}	
				}
			}			
 			if (s_i16RecInPos ==  IN_DATA_BUF_NUM)
   			{
				s_i16RecInPos = 0;
			}		
			s_i16RecInSample += TRAN_SAMPLE;
		}
	}
//	if ( g_u32IgnoreNo >= IGNORE_NO )
	if ( g_u32IgnoreNo == IGNORE_NO )
	{
       bPlaying = TRUE; 
//  sysprintf("ISR Change %x\n", bPlaying);       
    }       

}
/*
	The EDMA will move audio data to start address = g_pi16SampleBuf[0] with range u32Length. 
	The EDMA will issue interrupt in data reach to half of u32Length and u32Length. 	
	Then repeat to filled audio data to  g_pi16SampleBuf[0].
	Programmer must write audio data ASAP. 
*/
#define CLIENT_ADC_NAME 
int edma_channel=0;
int initEDMA(UINT32* u32EDMAChanel)
{
	int i;

    i32Count = 0;
	EDMA_Init();	
#if 1
	i = PDMA_FindandRequest(CLIENT_ADC_NAME); //w55fa95_edma_request
#else
	for (i = 4; i >=1; i--)
	{
		if (!EDMA_Request(i, CLIENT_ADC_NAME))
			break;
	}
#endif

//	if(i == -ENODEV)
//		return -ENODEV;

	edma_channel = i;
	EDMA_SetAPB(edma_channel,			//int channel, 
						eDRVEDMA_ADC,			//E_DRVEDMA_APB_DEVICE eDevice, 
						eDRVEDMA_READ_APB,		//E_DRVEDMA_APB_RW eRWAPB, 
						eDRVEDMA_WIDTH_32BITS);	//E_DRVEDMA_TRANSFER_WIDTH eTransferWidth	

	EDMA_SetupHandlers(edma_channel, 		//int channel
						eDRVEDMA_WAR, 			//int interrupt,	
						edmaCallback, 				//void (*irq_handler) (void *),
						NULL);					//void *data

	EDMA_SetWrapINTType(edma_channel , 
								eDRVEDMA_WRAPAROUND_EMPTY | 
								eDRVEDMA_WRAPAROUND_HALF);	//int channel, WR int type

	EDMA_SetDirection(edma_channel , eDRVEDMA_DIRECTION_FIXED, eDRVEDMA_DIRECTION_WRAPAROUND);


	EDMA_SetupSingle(edma_channel,		// int channel, 
								0xB800E010,//0xB800E010,		// unsigned int src_addr,  (ADC data port physical address) 
								(UINT32)g_pi16SampleBuf | 0x80000000, //phaddrrecord,		// unsigned int dest_addr,				
								E_AUD_BUF);	// unsigned int dma_length /* Lenth equal 2 half buffer */

	*u32EDMAChanel = i;
	return 0;
}
void releaseEDMA(UINT32 u32EdmaChannel)
{
	if(edma_channel!=0)
	{
		EDMA_Free(u32EdmaChannel);
		edma_channel = 0;
	}		
}



//	u32SampleRate = 16000;	
INT32 InitializeUAC(UINT32 u32SampleRate)
{	
	
	PFN_AUR_CALLBACK 	pfnOldCallback;
//	INT32 i;
	
     
#if 1
	DrvAUR_Open(eAUR_MONO_MIC_IN, TRUE);
	DrvAUR_InstallCallback(AudioRecordSampleDone, &pfnOldCallback);	
	DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)0x22, 0x1E);			/* Pregain */
	DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)0x23, 0x0E);			/* No any amplifier */
	DrvAUR_DisableInt();
	DrvAUR_SetSampleRate((E_AUR_SPS)u32SampleRate);
	DrvAUR_AutoGainTiming(1,1,1);
	DrvAUR_AutoGainCtrl(TRUE, TRUE, eAUR_OTL_N12P6);						
	DrvAUR_SetDataOrder(eAUR_ORDER_MONO_16BITS);
	
//	outp32(REG_AR_CON, inp32(REG_AR_CON)|AR_EDMA);			
//	sysDelay(30);		/* Delay 300ms for OTL stable */		
	
#endif
#if 0
    for (i=0; i<128; i++) 				
        g_pi16AudioBuf[i] = i;     					
#endif        
    g_u32SampleCount = u32SampleRate/1000;
    g_u32RecorderByte = g_u32SampleCount << 1;   //	
    return 0;
}

void StartUAC(void)
{	
#if 1	
	outp32(REG_AR_CON, inp32(REG_AR_CON)|AR_EDMA);			
	sysDelay(30);		/* Delay 300ms for OTL stable */		
	
	initEDMA(&g_u32EdmaChannel);
	
	//Combine with EDMA, only mode 0 can be set. 
	DrvAUR_StartRecord(eAUR_MODE_1); 
	DrvEDMA_CHEnablelTransfer((E_DRVEDMA_CHANNEL_INDEX)g_u32EdmaChannel);
#endif	
	
    g_u32IgnoreNo = 0;
	bPlaying = FALSE;
	s_i16RecOutPos = s_i16RecInPos = 0;   
	s_i16RecInSample = 0;
//  sysprintf("Start UAC %x\n", bPlaying);	
}

void StopUAC(void)
{
#if 1
	DrvAUR_StopRecord();
	releaseEDMA(g_u32EdmaChannel);
#endif	
	bPlaying = FALSE;
    g_u32IgnoreNo = 0;
	s_i16RecOutPos = s_i16RecInPos = 0; 
	s_i16RecInSample = 0;	
//  sysprintf("Stop UAC %x\n", bPlaying);	
}

/* Send data to the interrupt of Isochronous In */	  
void Send_AudioOneMSPacket(PUINT32 pu32Address, PUINT32 pu32Length)
{
//	PUINT32 pu32Buf;
	int volatile i;
//	sysprintf("ISR audioOneSP\n");
#if 1    

	if ( s_i16RecInSample >= (g_u32SampleCount << 1) )
	{
	   if (s_i16RecOutPos + g_u32RecorderByte > IN_DATA_BUF_NUM )
	   {
		   // 1X 
//    	   UAC_SendOneAudioPacket((BYTE *)&s_ai16RecBuf[s_i16RecOutPos], g_u32RecorderByte);
//           uvcdSDRAM_USB_Transfer(EP_C, &g_pi16AudioBuf[s_i16RecOutPos] , g_u32RecorderByte);
		*pu32Address = (UINT32)&g_pi16AudioBuf[s_i16RecOutPos];
		*pu32Length	= g_u32RecorderByte;
       	   s_i16RecOutPos += g_u32SampleCount;
	       s_i16RecInSample -= g_u32SampleCount;
	   }
	   else
	   {
		   // 2X 
//     	   UAC_SendOneAudioPacket((BYTE *)&s_ai16RecBuf[s_i16RecOutPos], (g_u32RecorderByte << 1));
//           uvcdSDRAM_USB_Transfer(EP_C, &g_pi16AudioBuf[s_i16RecOutPos] , g_u32RecorderByte << 1);
		*pu32Address = (UINT32)&g_pi16AudioBuf[s_i16RecOutPos];
		*pu32Length	= g_u32RecorderByte << 1;
       	   s_i16RecOutPos += g_u32RecorderByte;
	       s_i16RecInSample -= g_u32RecorderByte;
	   }

 	}
    else if ( s_i16RecInSample >= g_u32SampleCount )
	{   
//       UAC_SendOneAudioPacket((BYTE *)&s_ai16RecBuf[s_i16RecOutPos], g_u32RecorderByte);
//       uvcdSDRAM_USB_Transfer(EP_C, &g_pi16AudioBuf[s_i16RecOutPos] , g_u32RecorderByte);
		*pu32Address = (UINT32)&g_pi16AudioBuf[s_i16RecOutPos];
		*pu32Length	= g_u32RecorderByte;
       s_i16RecOutPos += g_u32SampleCount;
	   s_i16RecInSample -= g_u32SampleCount;
    }
    else
    {
//     UAC_SendOneAudioPacket(0, 0);  // send 0 data.
//       uvcdSDRAM_USB_Transfer(EP_C, NULL, 0);
        *pu32Length	= 0;     
    }
	if (s_i16RecOutPos ==  IN_DATA_BUF_NUM)
   	{
		s_i16RecOutPos = 0;
	}
#else	
  //  uvcdSDRAM_USB_Transfer(EP_C, &g_pi16AudioBuf[s_i16RecOutPos] , g_u32RecorderByte);
        if ( s_i16RecInSample >= g_u32SampleCount )
        {
		*pu32Address = (UINT32)&g_pi16AudioBuf[s_i16RecOutPos];
		*pu32Length	= g_u32RecorderByte;
		s_i16RecOutPos += g_u32SampleCount;
		if (s_i16RecOutPos ==  IN_DATA_BUF_NUM)
   		{
		s_i16RecOutPos = 0;
		}		
	}
	else
	{
	        *pu32Length	= 0;    
	}

#endif    		
}  
 

