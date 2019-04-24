

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

/*
static void pfnRecordCallback(void)
{
	g_i8PcmReady = TRUE;
}
*/
volatile BOOL bIsBufferDone=0;
UINT32 u32Count = 0;
void edmaCallback(UINT32 u32WrapStatus)
{
	//UINT32 u32Period, u32Attack, u32Recovery, u32Hold;
	if(u32WrapStatus==256)
	{
		bIsBufferDone = 1;
		//sysprintf("1 %d\n\n", bIsBufferDone);
	}	
	else if(u32WrapStatus==1024)
	{
		
		bIsBufferDone = 2;		
		//sysprintf("2 %d\n\n", bIsBufferDone);
	}
	u32Count = u32Count+1;
	//sysprintf(" ints = %d,  0x%x\n", u32Count, u32WrapStatus);
#if 0	
	/* AGC response speed */
	ADC_GetAutoGainTiming(&u32Period, &u32Attack, &u32Recovery, &u32Hold);
	if(u32Period<128)
	{		
		u32Period = u32Period+16;
		ADC_SetAutoGainTiming(u32Period, u32Attack, u32Recovery, u32Hold);		
	}
#endif 	
}


#define CLIENT_ADC_NAME 
int edma_channel=0;
int initEDMA(UINT32* u32EDMAChanel)
{
	int i;
	u32Count = 0;
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
								(UINT32)g_pi32AudSampleBuf, //phaddrrecord,		// unsigned int dest_addr,				
								E_AUD_BUF);	// unsigned int dma_length /* Lenth equal 2 half buffer */

	*u32EDMAChanel = i;
	return Successful;
}
void releaseEDMA(UINT32 u32EdmaChannel)
{
	if(edma_channel!=0)
	{
		EDMA_Free(u32EdmaChannel);
		edma_channel = 0;
	}		
}
