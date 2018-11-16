/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.  *
 *                                                              *
 ****************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include "wblib.h"
#include "string.h"
#include "wbio.h"
#include "w55fa92_reg.h"
#include "w55fa92_spu.h"
#include "spu.h"

//#define DBG_PRINTF(...)
//#define DBG_PRINTF printf

volatile S_CHANNEL_CTRL g_sChannelSettings;
//volatile UINT32 g_ua32UserData[6];

PFN_DRVSPU_CB_FUNC	*g_pfnUserEventCallBack[32];
PFN_DRVSPU_CB_FUNC	*g_pfnSilentEventCallBack[32];
PFN_DRVSPU_CB_FUNC	*g_pfnLoopStartEventCallBack[32];
PFN_DRVSPU_CB_FUNC	*g_pfnEndEventCallBack[32];
PFN_DRVSPU_CB_FUNC	*g_pfnEndAddressEventCallBack[32];
PFN_DRVSPU_CB_FUNC	*g_pfnThresholdAddressEventCallBack[32];
 
#define	E_SUCCESS	0

static void delay(UINT32 kk)
{
	UINT32 ii, jj;
	
	for(ii=0; ii < kk; ii++)
	{
		for(jj=0; jj < 0x10; jj++);	
	}
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function: spuNormalizeVolume                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      inputVolume																		                   */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      noramlizedVolume						                                                           */
/*                                                                                                         */
/* Description:                                                                                            */
/*      SPU volume level input is normalized to 32 levels                                                  */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
const int N_table[32] = {128,114,101,90,80,71,64,57,50,45,40,36,32,28,25,22,20,18,16,14,12,11,10,9,8,7,6,5,4,3,2,0};

const int sp_Digi_table[11] = {113,107,101,92,87,80,71,62,51,36,0};
const int sp_Ana_table[11] = {4,4,4,4,4,4,4,4,4,4,4};
const int ear_Digi_table[11] = {110,100,89,76,66,55,44,33,22,11,0};
const int ear_Ana_table[11] = {4,4,4,4,4,4,4,4,4,4,4};


int spuNormalizeVolume(int inputVolume)
{
	inputVolume = (inputVolume & 0xff) * 32  / 100;
	inputVolume = 32 - inputVolume;
	
	if (inputVolume >= 31)	
		return N_table[31];			
	
	return N_table[inputVolume];
}		

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvSPU_IntHandler                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      None																		                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None 						                                                                       */
/*                                                                                                         */
/* Description:                                                                                            */
/*      SPU Interrupt Service Routine                                                                      */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

void DrvSPU_IntHandler(void)
{
	UINT8 ii;
	UINT32 u32Channel, u32InterruptFlag;	
	
	u32Channel = 1;
	
	for (ii=0; ii<32; ii++)
	{
		if (inp32(REG_SPU_CH_IRQ) & u32Channel)
		{
			while(inp32(REG_SPU_CH_CTRL) & CH_FN);
			
			// load previous channel settings		
			outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (ii << 24));		
			outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
			while(inp32(REG_SPU_CH_CTRL) & CH_FN);
				
			u32InterruptFlag = inp32(REG_SPU_CH_EVENT);
			outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT));				

	#if 1
			if (u32InterruptFlag & DRVSPU_USER_INT)
			{
				if ( inp32(REG_SPU_CH_EVENT) & EV_USR_EN)
					g_pfnUserEventCallBack[ii]((UINT8*)REG_SPU_CTRL);
			}				
	
			if (u32InterruptFlag & DRVSPU_SILENT_INT)
			{
				if ( inp32(REG_SPU_CH_EVENT) & EV_SLN_EN)
					g_pfnSilentEventCallBack[ii]((UINT8*)REG_SPU_CTRL);
			}				

			if (u32InterruptFlag & DRVSPU_LOOPSTART_INT)
			{
				if ( inp32(REG_SPU_CH_EVENT) & EV_LP_EN)
					g_pfnLoopStartEventCallBack[ii]((UINT8*)REG_SPU_CTRL);
			}				

			if (u32InterruptFlag & DRVSPU_END_INT)
			{
				if ( inp32(REG_SPU_CH_EVENT) & EV_END_EN)
					g_pfnEndEventCallBack[ii]((UINT8*)REG_SPU_CTRL);
			}				

			if (u32InterruptFlag & DRVSPU_ENDADDRESS_INT)				
			{
				if ( inp32(REG_SPU_CH_EVENT) & END_EN)
					g_pfnEndAddressEventCallBack[ii]((UINT8*)((UINT32)_pucPlayAudioBuff + HALF_FRAG_SIZE));
			}				
	
			if (u32InterruptFlag & DRVSPU_THADDRESS_INT)				
			{
				if ( inp32(REG_SPU_CH_EVENT) & TH_EN)
					g_pfnThresholdAddressEventCallBack[ii]((UINT8*)((UINT32)_pucPlayAudioBuff));
			}				

			outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) & ~DRVSPU_UPDATE_ALL_PARTIALS);		
			outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | (DRVSPU_UPDATE_IRQ_PARTIAL + DRVSPU_UPDATE_PARTIAL_SETTINGS));				
			while(inp32(REG_SPU_CH_CTRL) & CH_FN);
				
	#else										
			DrvSPU_UploadChannelSettings(&g_sChannelSettings);	// Will be added 20090715
			if (u32InterruptFlag & DRVSPU_USER_INT)
				g_pfnUserEventCallBack[ii]((S_CHANNEL_CTRL*)&g_sChannelSettings);
	
			if (u32InterruptFlag & DRVSPU_SILENT_INT)
				g_pfnSilentEventCallBack[ii]((S_CHANNEL_CTRL*)&g_sChannelSettings);

			if (u32InterruptFlag == DRVSPU_LOOPSTART_INT)
				g_pfnLoopStartEventCallBack[ii]((S_CHANNEL_CTRL*)&g_sChannelSettings);

			if (u32InterruptFlag == DRVSPU_END_INT)
				g_pfnEndEventCallBack[ii]((S_CHANNEL_CTRL*)&g_sChannelSettings);

			if (u32InterruptFlag == DRVSPU_ENDADDRESS_INT)
				g_pfnEndAddressEventCallBack[ii]((S_CHANNEL_CTRL*)&g_sChannelSettings);
	
			if (u32InterruptFlag == DRVSPU_THADDRESS_INT)
				g_pfnThresholdAddressEventCallBack[ii]((S_CHANNEL_CTRL*)&g_sChannelSettings);
	#endif				
		}
	
		u32Channel <<= 1; 
	}

	outp32(REG_SPU_CH_IRQ, inp32(REG_SPU_CH_IRQ));
}	


//==========================================================================
//==========================================================================
// SPU open
ERRCODE
DrvSPU_Open(void)
{
	UINT8 ii;

	// enable SPU engine clock 
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | ADO_CKE | SPU_CKE | HCLK4_CKE);			// enable SPU engine clock 

	// disable SPU engine 
//	outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) & ~SPU_EN);
	outp32(REG_SPU_CTRL, 0x00);
	
	// given FIFO size = 4
	outp32(REG_SPU_CTRL, 0x04000000);		

	// reset SPU engine 
//	outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) | SPU_EN);
	outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) & ~SPU_SWRST);	
	
	outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) | SPU_SWRST);
	outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) & ~SPU_SWRST);	

	// enable I2S interface 
	outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) | SPU_I2S_EN);
	
	// disable all channels
	outp32(REG_SPU_CH_EN, 0x00);		
	
	for (ii=0; ii<32; ii++)
	{
		DrvSPU_ClearInt((E_DRVSPU_CHANNEL)ii, DRVSPU_ALL_INT);
		DrvSPU_DisableInt((E_DRVSPU_CHANNEL)ii, DRVSPU_ALL_INT);
	}
	
	return E_SUCCESS;
}

// SPU close 
void DrvSPU_Close(void)
{
	// reset SPU engine 
//	outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) | SPU_EN);
	outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) & ~SPU_SWRST);	
	
	outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) | SPU_SWRST);
	outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) & ~SPU_SWRST);	

	// disable SPU engine 
	outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) & ~SPU_EN);
	
	// disable SPU engine clock 
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~ADO_CKE & ~SPU_CKE);		// disable SPU engine clock 
}

ERRCODE
DrvSPU_ChannelOpen(
	E_DRVSPU_CHANNEL eChannel
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		outp32(REG_SPU_CH_EN, inp32(REG_SPU_CH_EN) | (0x0001 << eChannel));
		return E_SUCCESS;
	}
	else		
		return -1;	   	
}

BOOL
DrvSPU_IsChannelEnabled(
	E_DRVSPU_CHANNEL eChannel
)
{
	UINT32 u32Channel;
	
	u32Channel = 1;
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		u32Channel <<= eChannel;	
		if (inp32(REG_SPU_CH_EN ) & u32Channel)
			return TRUE;
		else 
			return FALSE;
	}
	else		
		return FALSE;
}

ERRCODE 
DrvSPU_ChannelClose(
	E_DRVSPU_CHANNEL eChannel
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		outp32(REG_SPU_CH_EN, inp32(REG_SPU_CH_EN) & ~(0x0001 << eChannel));
		return E_SUCCESS;
	}		
	else		
		return -1;	   	
}

ERRCODE 
DrvSPU_EnableInt(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32InterruptFlag,
	PFN_DRVSPU_CB_FUNC* pfnCallBack
)
{

	if ( (eChannel >=eDRVSPU_CHANNEL_0) && (eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// set new channel settings for previous channel settings						
		if (u32InterruptFlag & DRVSPU_USER_INT)
		{
			outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) | EV_USR_EN);		
			g_pfnUserEventCallBack[eChannel] = pfnCallBack;										
		}
		else if (u32InterruptFlag & DRVSPU_SILENT_INT)
		{
			outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) | EV_SLN_EN);				
			g_pfnSilentEventCallBack[eChannel] = pfnCallBack;													
		}
		else if (u32InterruptFlag & DRVSPU_LOOPSTART_INT)
		{
			outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) | EV_LP_EN);						
			g_pfnLoopStartEventCallBack[eChannel] = pfnCallBack;													
		}
		else if (u32InterruptFlag & DRVSPU_END_INT)
		{
			outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) | EV_END_EN);						
			g_pfnEndEventCallBack[eChannel] = pfnCallBack;													
		}

		else if (u32InterruptFlag & DRVSPU_ENDADDRESS_INT)
		{
			outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) | END_EN);						
			g_pfnEndAddressEventCallBack[eChannel] = pfnCallBack;													
		}

		else if (u32InterruptFlag & DRVSPU_THADDRESS_INT)
		{
			outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) | TH_EN);														
			g_pfnThresholdAddressEventCallBack[eChannel] = pfnCallBack;													
		}

		outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~AT_CLR_EN);
		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) & ~DRVSPU_UPDATE_ALL_PARTIALS);		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | (DRVSPU_UPDATE_IRQ_PARTIAL + DRVSPU_UPDATE_PARTIAL_SETTINGS));				
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);		
		
		sysInstallISR(IRQ_LEVEL_7, IRQ_SPU, (PVOID)DrvSPU_IntHandler);			
		//DrvAIC_InstallISR(eDRVAIC_INT_LEVEL1, eDRVAIC_INT_SPU, (PVOID)DrvSPU_IntHandler, 0);			
		
		return E_SUCCESS;
	}
	 
	else
		return -1;	 

}

BOOL
DrvSPU_IsIntEnabled(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32InterruptFlag 
)
{
	UINT32 u32Flag;

	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		switch (u32InterruptFlag)
		{
			case DRVSPU_USER_INT:
				u32Flag = inp32(REG_SPU_CH_EVENT) & EV_USR_EN;					
				break;

			case DRVSPU_SILENT_INT:
				u32Flag = inp32(REG_SPU_CH_EVENT) & EV_SLN_EN;
				break;

			case DRVSPU_LOOPSTART_INT:
				u32Flag = inp32(REG_SPU_CH_EVENT) & EV_LP_EN;
				break;

			case DRVSPU_END_INT:
				u32Flag = inp32(REG_SPU_CH_EVENT) & EV_END_EN;						
				break;

			case DRVSPU_ENDADDRESS_INT:
				u32Flag = inp32(REG_SPU_CH_EVENT) & END_EN;						
				break;

			case DRVSPU_THADDRESS_INT:
				u32Flag = inp32(REG_SPU_CH_EVENT) & TH_EN;														
				break;

			default:
				u32Flag = 0;			
				break;
		}

		if (u32Flag) return TRUE;
		else return FALSE;
	}
	 
	else
		return FALSE;
}

ERRCODE 
DrvSPU_DisableInt(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32InterruptFlag 
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// set new channel settings for previous channel settings						
		if (u32InterruptFlag & DRVSPU_USER_INT)
		{
			outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~EV_USR_EN);		
		}
		if (u32InterruptFlag & DRVSPU_SILENT_INT)
		{
			outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~EV_SLN_EN);				
		}
		if (u32InterruptFlag & DRVSPU_LOOPSTART_INT)
		{
			outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~EV_LP_EN);						
		}
		if (u32InterruptFlag & DRVSPU_END_INT)
		{
			outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~EV_END_EN);						
		}
		if (u32InterruptFlag & DRVSPU_ENDADDRESS_INT)
		{
			outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~END_EN);						
		}
		if (u32InterruptFlag & DRVSPU_THADDRESS_INT)
		{
			outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~TH_EN);
		}
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) & ~DRVSPU_UPDATE_ALL_PARTIALS);		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | (DRVSPU_UPDATE_IRQ_PARTIAL + DRVSPU_UPDATE_PARTIAL_SETTINGS));				
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
	
		return E_SUCCESS;
	}
	 
	else
		return -1;	   
}

ERRCODE 
DrvSPU_ClearInt(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32InterruptFlag 
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// set new channel settings for previous channel settings						
		if (u32InterruptFlag & DRVSPU_USER_INT)
		{
			outp32(REG_SPU_CH_EVENT, (inp32(REG_SPU_CH_EVENT) & ~0x3F00) | EV_USR_FG);		
		}
		if (u32InterruptFlag & DRVSPU_SILENT_INT)
		{
			outp32(REG_SPU_CH_EVENT, (inp32(REG_SPU_CH_EVENT) & ~0x3F00) | EV_SLN_FG);				
		}
		if (u32InterruptFlag & DRVSPU_LOOPSTART_INT)
		{
			outp32(REG_SPU_CH_EVENT, (inp32(REG_SPU_CH_EVENT) & ~0x3F00) | EV_LP_FG);						
		}
		if (u32InterruptFlag & DRVSPU_END_INT)
		{
			outp32(REG_SPU_CH_EVENT, (inp32(REG_SPU_CH_EVENT) & ~0x3F00) | EV_END_FG);						
		}
		if (u32InterruptFlag & DRVSPU_ENDADDRESS_INT)
		{
			outp32(REG_SPU_CH_EVENT, (inp32(REG_SPU_CH_EVENT) & ~0x3F00) | END_FG);						
		}
		if (u32InterruptFlag & DRVSPU_THADDRESS_INT)
		{
			outp32(REG_SPU_CH_EVENT, (inp32(REG_SPU_CH_EVENT) & ~0x3F00) | TH_FG);														
		}
		
		outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~AT_CLR_EN);			// clear Auto Clear Enable bit
		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) & ~DRVSPU_UPDATE_ALL_PARTIALS);		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | (DRVSPU_UPDATE_IRQ_PARTIAL + DRVSPU_UPDATE_PARTIAL_SETTINGS));				
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);		
		
		return E_SUCCESS;
	}
	else
		return -1;	   
}

ERRCODE 
DrvSPU_PollInt(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32InterruptFlag 
)
{

	BOOL bStatus;
	
	bStatus = TRUE;
	
	if ( (eChannel >=eDRVSPU_CHANNEL_0) && (eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// set new channel settings for previous channel settings						
		if (u32InterruptFlag == DRVSPU_USER_INT)
		{
			bStatus = (inp32(REG_SPU_CH_EVENT) & EV_USR_FG) ? TRUE : FALSE;		
		}
		else if (u32InterruptFlag == DRVSPU_SILENT_INT)
		{
			bStatus = (inp32(REG_SPU_CH_EVENT) & EV_SLN_FG) ? TRUE : FALSE;				
		}
		else if (u32InterruptFlag == DRVSPU_LOOPSTART_INT)
		{
			bStatus = (inp32(REG_SPU_CH_EVENT) & EV_LP_FG) ? TRUE : FALSE;						
		}
		else if (u32InterruptFlag == DRVSPU_END_INT)
		{
			bStatus = (inp32(REG_SPU_CH_EVENT) & EV_END_FG) ? TRUE : FALSE;						
		}
		else if (u32InterruptFlag == DRVSPU_ENDADDRESS_INT)
		{
			bStatus = (inp32(REG_SPU_CH_EVENT) & END_FG) ? TRUE : FALSE;						
		}
		else if (u32InterruptFlag == DRVSPU_THADDRESS_INT)
		{
			bStatus = (inp32(REG_SPU_CH_EVENT) & TH_FG) ? TRUE : FALSE;						
		}
		else 
			return E_DRVSPU_WRONG_INTERRUPT;	   		
		
		return bStatus;
	}
	 
	else
		return FALSE;

//return FALSE;		
}

ERRCODE 
DrvSPU_SetPauseAddress_PCM16(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32Address
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		outp32(REG_SPU_PA_ADDR, u32Address);		

		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_UPDATE_ALL_SETTINGS);				
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);		
		
		return E_SUCCESS;
	}
	else
		return -1;	   
}

ERRCODE
DrvSPU_GetPauseAddress_PCM16(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32* pu32Address		
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		*pu32Address = inp32(REG_SPU_PA_ADDR);
		return E_SUCCESS;		
	}
	else
		return -1;
}



ERRCODE 
DrvSPU_SetBaseAddress(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32Address
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		outp32(REG_SPU_S_ADDR, u32Address);		

		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_UPDATE_ALL_SETTINGS);				
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);		
		
		return E_SUCCESS;
	}
	else
		return -1;	   
}

ERRCODE
DrvSPU_GetBaseAddress(
	E_DRVSPU_CHANNEL eChannel,
	UINT32* pu32Address	
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		*pu32Address = inp32(REG_SPU_S_ADDR);
		return E_SUCCESS;		
	}
	else
		return -1;
}

ERRCODE 
DrvSPU_SetThresholdAddress(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32Address
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		outp32(REG_SPU_M_ADDR, u32Address);		

		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_UPDATE_ALL_SETTINGS);				
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);		
		
		return E_SUCCESS;
	}
	 
	else
		return -1;	   
}

ERRCODE
DrvSPU_GetThresholdAddress(
	E_DRVSPU_CHANNEL eChannel,
	UINT32* pu32Address	
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		*pu32Address = inp32(REG_SPU_M_ADDR);
		return E_SUCCESS;
	}
	else
		return -1;
}

ERRCODE 
DrvSPU_SetToneAmp(
	E_DRVSPU_CHANNEL eChannel,
	UINT32 u32Amp
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		outp32(REG_SPU_TONE_AMP, u32Amp);		

		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_UPDATE_ALL_SETTINGS);				
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);		
		
		return E_SUCCESS;
	}
	 
	else
		return -1;	   
}

ERRCODE 
DrvSPU_SetTonePulse(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32Pulse
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		outp32(REG_SPU_TONE_PULSE, u32Pulse);		

		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_UPDATE_ALL_SETTINGS);				
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);		
		
		return E_SUCCESS;
	}
	 
	else
		return -1;	   
}

ERRCODE 
DrvSPU_SetEndAddress(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32Address
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		outp32(REG_SPU_E_ADDR, u32Address);		

		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_UPDATE_ALL_SETTINGS);				
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);		
		
		return E_SUCCESS;
	}
	 
	else
		return -1;	   
}

ERRCODE
DrvSPU_GetEndAddress(
	E_DRVSPU_CHANNEL eChannel,
	UINT32* pu32Address		
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		*pu32Address = inp32(REG_SPU_E_ADDR);
		return E_SUCCESS;
	}
	else
		return -1;
}

ERRCODE
DrvSPU_GetCurrentAddress(
	E_DRVSPU_CHANNEL eChannel,
	UINT32* pu32Address	
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		* pu32Address = inp32(REG_SPU_CUR_ADDR);
		return E_SUCCESS;
	}
	else 
		return -1;
}

ERRCODE
DrvSPU_GetLoopStartAddress(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32* pu32Address	
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		* pu32Address = inp32(REG_SPU_LP_ADDR);
		return E_SUCCESS;		
	}
	else return -1;	   
}

#ifdef OPT_DIRECT_SET_DFA
//#if 1
	ERRCODE 
	DrvSPU_SetDFA(
		E_DRVSPU_CHANNEL eChannel, 
		UINT16 u16DFA
	)
	{
		if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
		{
			// wait to finish previous channel settings
			while(inp32(REG_SPU_CH_CTRL) & CH_FN);
			
			// load previous channel settings		
			outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
			outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
			while(inp32(REG_SPU_CH_CTRL) & CH_FN);
			
			outp32(REG_SPU_CH_PAR_2, u16DFA);		
	
			outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) & ~DRVSPU_UPDATE_ALL_PARTIALS);		
			outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | (DRVSPU_UPDATE_DFA_PARTIAL + DRVSPU_UPDATE_PARTIAL_SETTINGS));				
	
			while(inp32(REG_SPU_CH_CTRL) & CH_FN);		
			
			return E_SUCCESS;
		}
		 
		else
			return -1;	   
	}

#else
	ERRCODE 
	DrvSPU_SetDFA(
		E_DRVSPU_CHANNEL eChannel, 
		UINT16 u16SrcSampleRate, 
		UINT16 u16OutputSampleRate
	)
	{
		UINT32 u32DFA;
		
	//	u32DFA = (u16SrcSampleRate/u16OutputSampleRate) * 1024;
		u32DFA = (u16SrcSampleRate*1024)/u16OutputSampleRate;
	
		if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
		{
			// wait to finish previous channel settings
			while(inp32(REG_SPU_CH_CTRL) & CH_FN);
			
			// load previous channel settings		
			outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
			outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
			while(inp32(REG_SPU_CH_CTRL) & CH_FN);
	
			outp32(REG_SPU_CH_PAR_2, u32DFA);		
	
			outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) & ~DRVSPU_UPDATE_ALL_PARTIALS);		
			outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | (DRVSPU_UPDATE_DFA_PARTIAL + DRVSPU_UPDATE_PARTIAL_SETTINGS));				
	
			while(inp32(REG_SPU_CH_CTRL) & CH_FN);		
			
			return E_SUCCESS;
		}
		 
		else
			return -1;	   
	}
#endif	

ERRCODE
DrvSPU_GetDFA(
	E_DRVSPU_CHANNEL eChannel,
	UINT16* pu16DFA		
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		* pu16DFA = inp32(REG_SPU_CH_PAR_2) & 0x1FFF;
		return E_SUCCESS;		
	}
	else
		return -1;
}

// MSB 8-bit = left channel; LSB 8-bit = right channel
ERRCODE 
DrvSPU_SetPAN(
	E_DRVSPU_CHANNEL eChannel, 
	UINT16 u16PAN	
)
{
	UINT32 u32PAN;
	
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		u32PAN = u16PAN;
		u32PAN <<= 8;			
		u32PAN &= (PAN_L + PAN_R);
		outp32(REG_SPU_CH_PAR_1, (inp32(REG_SPU_CH_PAR_1) & (~(PAN_L+PAN_R))) | u32PAN);		

		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) & ~DRVSPU_UPDATE_ALL_PARTIALS);		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | (DRVSPU_UPDATE_PAN_PARTIAL + DRVSPU_UPDATE_PARTIAL_SETTINGS));				

		while(inp32(REG_SPU_CH_CTRL) & CH_FN);		
		
		return E_SUCCESS;
	}
	 
	else
		return -1;	   
}

ERRCODE
DrvSPU_GetPAN(
	E_DRVSPU_CHANNEL eChannel,
	UINT16* pu16PAN		
)
{
	UINT32 u32PAN;
	
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		u32PAN = inp32(REG_SPU_CH_PAR_1);
		u32PAN >>= 8;
		* pu16PAN = u32PAN & 0xFFFF;
		return E_SUCCESS;		
	}
	else
		return -1;
}


ERRCODE 
DrvSPU_SetSrcType(
	E_DRVSPU_CHANNEL eChannel, 
	E_DRVSPU_FORMAT eDataFormat
)
{

	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		outp32(REG_SPU_CH_PAR_1, (inp32(REG_SPU_CH_PAR_1) & ~SRC_TYPE) | eDataFormat);		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_UPDATE_ALL_SETTINGS );				

		while(inp32(REG_SPU_CH_CTRL) & CH_FN);		
		
		return E_SUCCESS;
	}
	 
	else
		return -1;	   
}

ERRCODE
DrvSPU_GetSrcType(
	E_DRVSPU_CHANNEL eChannel,
	UINT16* pu16SrcType	
)
{
	UINT8 u8DataFormat;
	
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		u8DataFormat = inp32(REG_SPU_CH_PAR_1);
		*pu16SrcType = u8DataFormat & 0x07;
		return E_SUCCESS;		
	}
	else
		return -1;
}

ERRCODE 
DrvSPU_SetChannelVolume(
	E_DRVSPU_CHANNEL eChannel, 
	UINT8 u8Volume
)
{
	UINT32 u32PAN;
	
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		u32PAN = u8Volume;
		u32PAN <<= 24;
		outp32(REG_SPU_CH_PAR_1, (inp32(REG_SPU_CH_PAR_1) & 0x00FFFFFF) | u32PAN);		

		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) & ~DRVSPU_UPDATE_ALL_PARTIALS);		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | (DRVSPU_UPDATE_VOL_PARTIAL + DRVSPU_UPDATE_PARTIAL_SETTINGS));				

		while(inp32(REG_SPU_CH_CTRL) & CH_FN);		
		
		return E_SUCCESS;
	}
	 
	else
		return -1;	   
}

ERRCODE
DrvSPU_GetChannelVolume(
	E_DRVSPU_CHANNEL eChannel,
	UINT8* pu8Volume
)
{
	UINT32 u32PAN;
	
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		u32PAN = inp32(REG_SPU_CH_PAR_1);
		u32PAN >>= 24;
		*pu8Volume = u32PAN & 0xFF;
		return 0;
	}
	else
		return -1;
}

void DrvSPU_EqOpen(
	E_DRVSPU_EQ_BAND eEqBand,
	E_DRVSPU_EQ_GAIN eEqGain		
)
{
	switch (eEqBand)
	{
		case eDRVSPU_EQBAND_DC:
			outp32(REG_SPU_EQGain1, (inp32(REG_SPU_EQGain1) & (~Gaindc)) | eEqGain <<16);
			break;
	
		case eDRVSPU_EQBAND_1:
			outp32(REG_SPU_EQGain0, (inp32(REG_SPU_EQGain0) & (~Gain01)) | eEqGain);
			break;
	
		case eDRVSPU_EQBAND_2:
			outp32(REG_SPU_EQGain0, (inp32(REG_SPU_EQGain0) & (~Gain02)) | eEqGain <<4);
			break;

		case eDRVSPU_EQBAND_3:
			outp32(REG_SPU_EQGain0, (inp32(REG_SPU_EQGain0) & (~Gain03)) | eEqGain <<8);
			break;

		case eDRVSPU_EQBAND_4:
			outp32(REG_SPU_EQGain0, (inp32(REG_SPU_EQGain0) & (~Gain04)) | eEqGain <<12);
			break;

		case eDRVSPU_EQBAND_5:
			outp32(REG_SPU_EQGain0, (inp32(REG_SPU_EQGain0) & (~Gain05)) | eEqGain <<16);
			break;

		case eDRVSPU_EQBAND_6:
			outp32(REG_SPU_EQGain0, (inp32(REG_SPU_EQGain0) & (~Gain06)) | eEqGain <<20);
			break;

		case eDRVSPU_EQBAND_7:
			outp32(REG_SPU_EQGain0, (inp32(REG_SPU_EQGain0) & (~Gain07)) | eEqGain <<24);
			break;

		case eDRVSPU_EQBAND_8:
			outp32(REG_SPU_EQGain0, (inp32(REG_SPU_EQGain0) & (~Gain08)) | eEqGain <<28);
			break;

		case eDRVSPU_EQBAND_9:
			outp32(REG_SPU_EQGain1, (inp32(REG_SPU_EQGain1) & (~Gain09)) | eEqGain);
			break;

		default:
		case eDRVSPU_EQBAND_10:
			outp32(REG_SPU_EQGain1, (inp32(REG_SPU_EQGain1) & (~Gain10)) | eEqGain <<4);
			break;
	}
	
	outp32(REG_SPU_DAC_PAR, inp32(REG_SPU_DAC_PAR) | EQU_EN | ZERO_EN);
}

void DrvSPU_EqClose(void)
{
	outp32(REG_SPU_DAC_PAR, inp32(REG_SPU_DAC_PAR) & (~EQU_EN) & (~ZERO_EN));
}

void DrvSPU_SetVolume(
	UINT16 u16Volume	// MSB: left channel; LSB right channel
)	
{
	UINT8 ucVol;
	
	ucVol = u16Volume & 0xFF;
	ucVol = spuNormalizeVolume(ucVol);
	DrvSPU_WriteDACReg(0x0A, ucVol);
	
	ucVol = (u16Volume>>8) & 0xFF;
	ucVol = spuNormalizeVolume(ucVol);	
	DrvSPU_WriteDACReg(0x09, ucVol);
}

void 
DrvSPU_GetVolume(UINT16* pu16Volume)
{
	UINT16 u16Vol =0, u16Data;

	u16Data = DrvSPU_ReadDACReg(0x09);	
	u16Vol |= u16Data<<8;

	u16Data = DrvSPU_ReadDACReg(0x0A);	
	u16Vol |= u16Data;

	*pu16Volume = u16Vol;
}	

void DrvSPU_StartPlay(void)
{
	outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) | SPU_EN);
}

void DrvSPU_StopPlay(void)
{
	outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) & ~SPU_EN);
}

UINT32  
DrvSPU_SetSampleRate (
	E_DRVSPU_SAMPLING eSampleRate
)
{
	UINT32 u32RealSampleRate;		
//	UINT32 u32ClockIn, u32Divider, u32ExtClock, u32EngineClockKHz;

	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | ADO_CKE | SPU_CKE);			// enable SPU engine clock 
//	outp32(REG_CLKDIV1, inp32(REG_CLKDIV1) & ~(ADO_N1 | ADO_S));	

//	#define OPT_MCLK_FROM_UPLL
	#ifdef OPT_MCLK_FROM_UPLL

		u32ExtClock = sysGetExternalClock();
		u32EngineClockKHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtClock);		// CLK_IN = 12 MHz
//		u32EngineClockKHz /= 1000;

		outp32(REG_SPU_DAC_PAR, inp32(REG_SPU_DAC_PAR) & ~PLLSELMOD);
		u32Divider = u32EngineClockKHz / (256*eSampleRate);
		u32Divider --;

		outp32(REG_CLKDIV1, inp32(REG_CLKDIV1) & ~(ADO_N1 | ADO_S | ADO_N0));	// engine clcok fixed to 12 MHz		
//		outp32(REG_CLKDIV1, inp32(REG_CLKDIV1) | (0x03 << 19) );				// SPU clock from UPLL			
		outp32(REG_CLKDIV1, inp32(REG_CLKDIV1) | (u32Divider << 24));
    	outp32(REG_CLKDIV1, inp32(REG_CLKDIV1) | (0x03 << 19) );				// SPU clock from UPLL					
		u32RealSampleRate = eSampleRate;

	#else

		outp32(REG_SPU_DAC_PAR, inp32(REG_SPU_DAC_PAR) | PLLSELMOD);
		outp32(REG_CLKDIV1, inp32(REG_CLKDIV1) & ~(ADO_N1 | ADO_S | ADO_N0));	// engine clcok fixed to 12 MHz

//  return;
		switch(eSampleRate)
		{
			case  eDRVSPU_FREQ_48000: 
				
				DrvSPU_WriteDACReg(0x02, 0x00);			

				DrvSPU_WriteDACReg(0x0B, 0x14);
				
				DrvSPU_WriteDACReg(0x0C, 0x55);	

				u32RealSampleRate = eDRVSPU_FREQ_48000;
				
			        break;
			case  eDRVSPU_FREQ_44100: 
				
			       DrvSPU_WriteDACReg(0x02, 0x00);		       

				DrvSPU_WriteDACReg(0x0B, 0x10);
				
				DrvSPU_WriteDACReg(0x0C, 0x3F);	

				u32RealSampleRate = eDRVSPU_FREQ_44100;
				
			        break;	    
			case  eDRVSPU_FREQ_32000: 
				
				DrvSPU_WriteDACReg(0x02, 0x04);			

				DrvSPU_WriteDACReg(0x0B, 0x14);			

				DrvSPU_WriteDACReg(0x0C, 0x55);	

				u32RealSampleRate = eDRVSPU_FREQ_32000;
			        						
			        break;	
			case  eDRVSPU_FREQ_24000: 

				DrvSPU_WriteDACReg(0x02, 0x01);			

				DrvSPU_WriteDACReg(0x0B, 0x14);			

				DrvSPU_WriteDACReg(0x0C, 0x55);	

				u32RealSampleRate = eDRVSPU_FREQ_24000;
			        
			        break;	
			case  eDRVSPU_FREQ_22050: 

				DrvSPU_WriteDACReg(0x02, 0x01);			

				DrvSPU_WriteDACReg(0x0B, 0x10);			

				DrvSPU_WriteDACReg(0x0C, 0x3F);	

				u32RealSampleRate = eDRVSPU_FREQ_22050;
			        
			        break;	
			case  eDRVSPU_FREQ_16000: 

				DrvSPU_WriteDACReg(0x02, 0x05);			

				DrvSPU_WriteDACReg(0x0B, 0x14);			

				DrvSPU_WriteDACReg(0x0C, 0x55);	

				u32RealSampleRate = eDRVSPU_FREQ_16000;
			       
			        break;	
			case  eDRVSPU_FREQ_12000: 

				DrvSPU_WriteDACReg(0x02, 0x02);			

				DrvSPU_WriteDACReg(0x0B, 0x14);			

				DrvSPU_WriteDACReg(0x0C, 0x55);	

				u32RealSampleRate = eDRVSPU_FREQ_12000;
			       
			        break;	
			case  eDRVSPU_FREQ_11025: 

				DrvSPU_WriteDACReg(0x02, 0x02);			

				DrvSPU_WriteDACReg(0x0B, 0x10);			

				DrvSPU_WriteDACReg(0x0C, 0x3F);	

				u32RealSampleRate = eDRVSPU_FREQ_11025;
			       
			        break;
			case  eDRVSPU_FREQ_8000: 

				DrvSPU_WriteDACReg(0x02, 0x06);			

				DrvSPU_WriteDACReg(0x0B, 0x14);			

				DrvSPU_WriteDACReg(0x0C, 0x55);	

				u32RealSampleRate = eDRVSPU_FREQ_11025;
			       
			        break;	
			default:	
				u32RealSampleRate = 0;
				break;		        
		}
	#endif		

	return u32RealSampleRate;
}

ERRCODE
DrvSPU_UploadChannelContents (
	E_DRVSPU_CHANNEL eChannel
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		// wait to finish previous channel settings
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		
		// load previous channel settings		
		outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));		
		outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
		while(inp32(REG_SPU_CH_CTRL) & CH_FN);
		return E_SUCCESS;
	}
	else
		return -1;	   	
}


ERRCODE 
DrvSPU_ChannelCtrl(
	S_CHANNEL_CTRL *psChannelCtrl
)
{
	volatile UINT32 u32Channel; 
	
	u32Channel = psChannelCtrl->u32ChannelIndex;
	
	if ( ((INT)u32Channel >=eDRVSPU_CHANNEL_0) && ((INT)u32Channel <=eDRVSPU_CHANNEL_31) )
	{
		DrvSPU_SetChannelVolume((E_DRVSPU_CHANNEL)u32Channel, psChannelCtrl->u8ChannelVolume);	
		DrvSPU_SetPAN((E_DRVSPU_CHANNEL)u32Channel, psChannelCtrl->u16PAN);	
		DrvSPU_SetSrcType((E_DRVSPU_CHANNEL)u32Channel, (E_DRVSPU_FORMAT)psChannelCtrl->u8DataFormat);
		
#ifdef OPT_DIRECT_SET_DFA		
//		DrvSPU_SetDFA(u32Channel, psChannelCtrl->u16DFA);		
#else		
//		DrvSPU_SetDFA(u32Channel, psChannelCtrl->u16SrcSampleRate, psChannelCtrl->u16OutputSampleRate);				
#endif		
		DrvSPU_SetBaseAddress((E_DRVSPU_CHANNEL)u32Channel, psChannelCtrl->u32SrcBaseAddr);		
		DrvSPU_SetThresholdAddress((E_DRVSPU_CHANNEL)u32Channel, psChannelCtrl->u32SrcThresholdAddr);
		DrvSPU_SetEndAddress((E_DRVSPU_CHANNEL)u32Channel, psChannelCtrl->u32SrcEndAddr);
		
		return E_SUCCESS;
	}
	else
		return -1;	   	
}

ERRCODE
DrvSPU_ChannelPause (
	E_DRVSPU_CHANNEL eChannel
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		outp32(REG_SPU_CH_PAUSE, inp32(REG_SPU_CH_PAUSE) | (0x0001 << eChannel));
		return E_SUCCESS;
	}
	else		
		return -1;	   	
}

ERRCODE
DrvSPU_ChannelResume (
	E_DRVSPU_CHANNEL eChannel
)
{
	if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
	{
		outp32(REG_SPU_CH_PAUSE, inp32(REG_SPU_CH_PAUSE) & ~(0x0001 << eChannel));
		return E_SUCCESS;
	}
	else		
		return -1;	   	
}


UINT8
DrvSPU_ReadDACReg (
	UINT8 DACRegIndex
)
{
	UINT32 u32Reg = 0x30800000;		// clock divider = 0x30, ID = 0x80
	UINT8 u8Ret;

#if 0
	u32Reg |= DACRegIndex << 8;
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);
	outp32(REG_SPU_DAC_CTRL, u32Reg);
	delay(10);
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);
	outp32(REG_GPIOA_DOUT, inp32(REG_GPIOA_DOUT) & ~BIT4);			
	delay(50);
	outp32(REG_GPIOA_DOUT, inp32(REG_GPIOA_DOUT) | BIT4);					
	u8Ret = inp32(REG_SPU_DAC_CTRL) & 0xFF;
#else	
	u32Reg |= DACRegIndex << 8;
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);
	outp32(REG_SPU_DAC_CTRL, u32Reg);
	delay(20);
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);
	delay(200);
	u8Ret = inp32(REG_SPU_DAC_CTRL) & 0xFF;
#endif	
	
	return u8Ret;
}

VOID DrvSPU_WriteDACReg (
	UINT8 DACRegIndex, 
	UINT8 DACRegData
)
{
	UINT32 u32Reg = 0x30810000;		// clock divider = 0x30, ID = 0x80

#if 0
	u32Reg |= DACRegIndex << 8;
	u32Reg |= DACRegData;
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);
	outp32(REG_SPU_DAC_CTRL, u32Reg);	
	outp32(REG_GPIOA_DOUT, inp32(REG_GPIOA_DOUT) | BIT4);		
	delay(10);
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);	
	outp32(REG_GPIOA_DOUT, inp32(REG_GPIOA_DOUT) & ~BIT4);			
	delay(100);	
	outp32(REG_GPIOA_DOUT, inp32(REG_GPIOA_DOUT) | BIT4);				
#else
	u32Reg |= DACRegIndex << 8;
	u32Reg |= DACRegData;
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);
	outp32(REG_SPU_DAC_CTRL, u32Reg);	
	delay(20);
	while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);	
	delay(200);	
#endif	
}
