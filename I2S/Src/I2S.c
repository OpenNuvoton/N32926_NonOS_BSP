/****************************************************************
 *                                                              *
 * Copyright (c) Winbond Electronics Corp. All rights reserved. *
 *                                                              *
 ****************************************************************/

#include "wblib.h"
#include "w55fa92_i2s.h"
#include "w55fa92_reg.h"

//#define OPT_FPGA_DEBUG

PFN_DRVI2S_CB_FUNC 	*g_pfnRecordCallBack;
PFN_DRVI2S_CB_FUNC 	*g_pfnPlayCallBack;	
PFN_DRVI2S_CB_FUNC 	*g_pfnDownCounterCallBack;
PFN_DRVI2S_CB_FUNC 	*g_pfnDataZeroCallBack;	
PFN_DRVI2S_CB_FUNC 	*g_pfnPauseCallBack;	


UINT32	volatile g_DrvI2S_u32PlayBufAddr;			
UINT32	volatile g_DrvI2S_u32PlayBufSize;
UINT32	volatile g_DrvI2S_u32RecordBufAddr;			
UINT32	volatile g_DrvI2S_u32RecordBufSize;
UINT32	volatile g_DrvI2S_u32SampleRate = 0x00;

static VOID DrvI2S_SetDataFormat (E_DRVI2S_FORMAT  eInterface);

#define OPT_GCR_2010429
#define E_SUCCESS	0
/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_GetVersion                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      None																		                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None 						                                                                       */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Get driver version 				                                                                   */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

UINT32 DrvI2S_GetVersion(void)
{
	return (DRVI2S_MAJOR_NUM << 16) | (DRVI2S_MINOR_NUM << 8) | DRVI2S_BUILD_NUM;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_IntHandler                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      None																		                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None 						                                                                       */
/*                                                                                                         */
/* Description:                                                                                            */
/*      I2S Interrupt Service Routine                                                                      */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

static VOID DrvI2S_IntHandler(void)
{

	if (inp32(REG_I2S_ACTL_CON) &P_DMA_IRQ_EN)
	{
		if (inp32(REG_I2S_ACTL_PSR)&P_DMA_RIA_IRQ)
		{
			outp32(REG_I2S_ACTL_PSR, P_DMA_RIA_IRQ);
			outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | P_DMA_IRQ);
			g_pfnPlayCallBack((UINT8 *)(g_DrvI2S_u32PlayBufAddr+g_DrvI2S_u32PlayBufSize/2), g_DrvI2S_u32PlayBufSize/2);		
		
		}
	}		

	if (inp32(REG_I2S_ACTL_CON) &R_DMA_IRQ_EN)
	{
		if (inp32(REG_I2S_ACTL_RSR)&R_DMA_RIA_IRQ)
		{
			outp32(REG_I2S_ACTL_RSR, R_DMA_RIA_IRQ);
			outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | R_DMA_IRQ);
			g_pfnRecordCallBack((UINT8 *)(g_DrvI2S_u32RecordBufAddr+g_DrvI2S_u32RecordBufSize/2), g_DrvI2S_u32RecordBufSize/2);					
		}
	}		

	if (inp32(REG_I2S_ACTL_CON) &IRQ_DMA_CNTER_EN)
	{
		if (inp32(REG_I2S_ACTL_PSR)&DMA_CNTER_IRQ)
		{
			outp32(REG_I2S_ACTL_PSR, DMA_CNTER_IRQ);
			outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | P_DMA_IRQ);
			g_pfnDownCounterCallBack((UINT8 *)(g_DrvI2S_u32PlayBufAddr+g_DrvI2S_u32PlayBufSize/2), g_DrvI2S_u32PlayBufSize/2);		
		
		}
	}		

	if (inp32(REG_I2S_ACTL_CON) &IRQ_DMA_DATA_ZERO_EN)
	{
		if (inp32(REG_I2S_ACTL_PSR)&DMA_DATA_ZERO_IRQ)
		{
			outp32(REG_I2S_ACTL_PSR, DMA_DATA_ZERO_IRQ);
			outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | P_DMA_IRQ);
			g_pfnDataZeroCallBack((UINT8 *)(g_DrvI2S_u32RecordBufAddr+g_DrvI2S_u32RecordBufSize/2), g_DrvI2S_u32RecordBufSize/2);					
		}
	}		

#if 1	
	if (inp32(REG_I2S_ACTL_CON) &P_PAUSE_IRQ_EN)
	{
		if (inp32(REG_I2S_ACTL_PSR)&P_PAUSE_IRQ)
		{
			outp32(REG_I2S_ACTL_PSR, P_PAUSE_IRQ);
			outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | P_DMA_IRQ);
			g_pfnPauseCallBack((UINT8 *)(g_DrvI2S_u32RecordBufAddr+g_DrvI2S_u32RecordBufSize/2), g_DrvI2S_u32RecordBufSize/2);					
		}
	}
#endif			
	
	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON));		// clear IRQ flag
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_Open 		                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      None																		                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      int						                                                                       */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Open I2S pin and engine clock 		                                                               */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

int DrvI2S_Open(
	void 
)
{

// 	check if there is share pins IP to be enabled
//	if YES then return ERROR

//#define OPT_I2S_FROM_GPG
#ifdef OPT_I2S_FROM_GPG
	// enable I2S pin function
#define REG_SHRPIN_TVDAC	(GCR_BA+0xF4)	// R/W	Share Pin With TV DAC
	#define SMTVDACAEN		BIT31	

	outp32(REG_SHRPIN_TVDAC, inp32(REG_SHRPIN_TVDAC)&~SMTVDACAEN);	// TV shared pins to be digital ones
	outp32(REG_SHRPIN_TOUCH, inp32(REG_SHRPIN_TOUCH)&~SAR_AHS_AEN);	// TV shared pins to be digital ones	
	outp32(REG_GPGFUN0, (inp32(REG_GPGFUN0)&~(MF_GPB5+MF_GPB4+MF_GPB3+MF_GPB2))|0x00333300);	// GPG0[5:2] to be I2S signals
	outp32(REG_GPGFUN1, (inp32(REG_GPGFUN1)&~(MF_GPB9))|0x0020);	// GPG1[9] to be I2S signals	
	outp32(REG_MISFUN, inp32(REG_MISFUN) & (~0x01));				// I2S interface for I2S, but not SPU
	
#else
	// enable I2S pin function
	outp32(REG_GPBFUN0, (inp32(REG_GPBFUN0)&~(MF_GPB6+MF_GPB5+MF_GPB4+MF_GPB3+MF_GPB2))|0x01111100);	// GPB0[6:2] to be I2S signals
	outp32(REG_MISFUN, inp32(REG_MISFUN) & (~0x01));				// I2S interface for I2S, but not SPU
#endif	

	// enable I2S engine clock 
//#ifdef OPT_FPGA_DEBUG
#if 0
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | SD_CKE);
//	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | SIC_CKE | SD_CKE);			// enable SD engine clock 		
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | ADO_CKE | I2S_CKE);			// enable I2S engine clock 
#else	
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | ADO_CKE | I2S_CKE | HCLK4_CKE);	// enable I2S engine clock 
#endif

	// reset I2S engine 
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) | I2S_RST);
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) & ~I2S_RST);	
	outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) | ACTL_RESET_);		// Reset whole audio controller
	outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) & ~ACTL_RESET_);
	outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) | I2S_RESET);			// Reset I2S function block
	outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) & ~I2S_RESET);

	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) & ~R_DMA_IRQ_SEL);	
	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | (0x01 <<14));	// threshold address is the half of DMA buffer for recording 
//	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | (0x02 <<14));	// threshold address is the quarter of DMA buffer for recording 	
//	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | (0x03 <<14));	// threshold address is the eighth of DMA buffer for recording 		
	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) & ~P_DMA_IRQ_SEL);		
	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | (0x01 <<12));	// threshold address is the half of DMA buffer for playing
//	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | (0x02 <<12));	// threshold address is the quarter of DMA buffer for playing
//	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | (0x03 <<12));	// threshold address is the eighth of DMA buffer for playing	

	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | I2S_EN);					// enable I2S interface
	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) & ~FIFO_TH);	// FIFO Threshold 8 level
//	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) |  FIFO_TH);	// FIFO Threshold 4 level	
	
//	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) |  I2S_BITS_16_24);	// 24-bit (Must BCLK_SEL[1:0]=00, and WS_SEL=1, 48ws is selected)
	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) & ~I2S_BITS_16_24);	// 16-bit 	

	return E_SUCCESS;
	
}	

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_Close 		                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      None																		                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None						                                                                       */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Open I2S pin and engine clock 		                                                               */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

VOID DrvI2S_Close(void)
{
#if 1
//	outp32(PINFUN, inp32(PINFUN) & ~(I2SEN0 | I2SEN1 | I2SEN2));		// disable IP I/O pins
//	outp32(CLKMAN, inp32(CLKMAN) & ~AUD_EN);							// disable I2S engine
	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) & ~I2S_EN);						// disable I2S interface	
#endif	
}	

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_EnableInt                                                                              */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      u32InterruptFlag - [in], set corresponding interrupt flag					                       */
/*      callback - callback funciton                                                                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None 						                                                                       */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Enable I2S interrupt and setup callback function.		                                           */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

VOID DrvI2S_EnableInt(
	UINT32	u32InterruptFlag, PFN_DRVI2S_CB_FUNC* pfnCallBack 
)
{

	if(u32InterruptFlag & DRVI2S_IRQ_RECORD)
	{
		outp32(REG_I2S_ACTL_RSR, inp32(REG_I2S_ACTL_RSR));
		
		outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | R_DMA_IRQ_EN);
		g_pfnRecordCallBack = pfnCallBack;							
	}
	else if(u32InterruptFlag & DRVI2S_IRQ_PLAYBACK)
	{
		outp32(REG_I2S_ACTL_PSR, inp32(REG_I2S_ACTL_PSR));	
		outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | P_DMA_IRQ_EN);
		g_pfnPlayCallBack = pfnCallBack;			
	}
	else if(u32InterruptFlag & DRVI2S_IRQ_DMA_COUNTER)
	{
		outp32(REG_I2S_ACTL_PSR, inp32(REG_I2S_ACTL_PSR));		
//		outp32(REG_I2S_ACTL_CON, P_DMA_IRQ);		
		outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | IRQ_DMA_CNTER_EN);
		outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) | DMA_CNTER_EN);
		g_pfnDownCounterCallBack = pfnCallBack;					
	}
	else if(u32InterruptFlag & DRVI2S_IRQ_DMA_DATA_ZERO)
	{
		outp32(REG_I2S_ACTL_PSR, inp32(REG_I2S_ACTL_PSR));		
//		outp32(REG_I2S_ACTL_CON, P_DMA_IRQ);		
		outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | IRQ_DMA_DATA_ZERO_EN);
		outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) | DMA_DATA_ZERO_EN);
		g_pfnDataZeroCallBack = pfnCallBack;					
	}

	else if(u32InterruptFlag & DRVI2S_IRQ_PAUSE)
	{
		outp32(REG_I2S_ACTL_PSR, inp32(REG_I2S_ACTL_PSR));		
//		outp32(REG_I2S_ACTL_CON, P_DMA_IRQ);				
		outp32(REG_I2S_ACTL_PSR, P_PAUSE_IRQ);		
//		outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | P_DMA_IRQ_EN);	
		outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | P_PAUSE_IRQ_EN);
		g_pfnPauseCallBack = pfnCallBack;					
	}

//	DrvAIC_InstallISR(eDRVAIC_INT_LEVEL1, eDRVAIC_INT_I2S, (PVOID)DrvI2S_IntHandler, 0);	
	sysInstallISR(IRQ_LEVEL_7, IRQ_I2S, (PVOID)DrvI2S_IntHandler);	
//  DrvAIC_EnableInt(INT_I2S);		
	
}	

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_DisableInt                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      u32InterruptFlag - [in], set corresponding interrupt flag					                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None 						                                                                       */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Disable I2S interupt.		                                                                       */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

VOID DrvI2S_DisableInt(
	UINT32	u32InterruptFlag
)
{

	if(u32InterruptFlag & DRVI2S_IRQ_RECORD)
	{
		outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) & ~R_DMA_IRQ_EN);
	}
	else if(u32InterruptFlag & DRVI2S_IRQ_PLAYBACK)
	{
		outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) & ~P_DMA_IRQ_EN);
	}
	else if(u32InterruptFlag & DRVI2S_IRQ_DMA_COUNTER)
	{
		outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) & ~IRQ_DMA_CNTER_EN);
		outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) & ~DMA_CNTER_EN);
	}
	else if(u32InterruptFlag & DRVI2S_IRQ_DMA_DATA_ZERO)
	{
		outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) & ~IRQ_DMA_DATA_ZERO_EN);	
		outp32(REG_I2S_ACTL_RESET, (inp32(REG_I2S_ACTL_RESET) & ~DMA_DATA_ZERO_EN));
	}
}	

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_PollInt                                                                                */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      u32InterruptFlag - [in], get corresponding interrupt flag					                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      The relative interrupt flag.                                                                       */
/*                                                                                                         */
/* Description:                                                                                            */
/*      To get the I2S interrupt flag.                                                                     */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

UINT32 DrvI2S_PollInt(
	UINT32	u32InterruptFlag
)	
{
	return (inp32(REG_I2S_ACTL_CON) & u32InterruptFlag);
}	

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_ClearInt                                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      u32InterruptFlag - [in], clear corresponding interrupt flag					                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None                                                                                               */
/*                                                                                                         */
/* Description:                                                                                            */
/*      To clear the I2S interrupt flag.                                                                   */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

VOID DrvI2S_ClearInt(
	UINT32	u32InterruptFlag
)
{
	if(u32InterruptFlag & DRVI2S_IRQ_RECORD)
	{
		outp32(REG_I2S_ACTL_RSR, inp32(REG_I2S_ACTL_RSR));
		outp32(REG_I2S_ACTL_CON, R_DMA_IRQ);
	}
	else if(u32InterruptFlag & DRVI2S_IRQ_PLAYBACK)
	{
		outp32(REG_I2S_ACTL_PSR, inp32(REG_I2S_ACTL_PSR));	
		outp32(REG_I2S_ACTL_CON, P_DMA_IRQ);
		
	}
	else if(u32InterruptFlag & DRVI2S_IRQ_DMA_COUNTER)
	{
		outp32(REG_I2S_ACTL_PSR, inp32(REG_I2S_ACTL_PSR));		
		outp32(REG_I2S_ACTL_CON, P_DMA_IRQ);
	}
	else if(u32InterruptFlag & DRVI2S_IRQ_DMA_DATA_ZERO)
	{
		outp32(REG_I2S_ACTL_PSR, inp32(REG_I2S_ACTL_PSR));		
		outp32(REG_I2S_ACTL_CON, P_DMA_IRQ);
	}
}	

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_SetDestBase                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      bType - [in], select DrvI2S_PLAY or DrvI2S_RECORD							                       */
/*      u32Address - [in], set buffer base address									                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None                                                                                               */
/*                                                                                                         */
/* Description:                                                                                            */
/*      To set buffer base address					                                                       */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

VOID DrvI2S_SetDestBase(
	BOOL	bType, 			
	UINT32	u32Address
)
{
	if(bType == DRVI2S_PLAY)
	{
		outp32(REG_I2S_ACTL_PDSTB, u32Address);
		g_DrvI2S_u32PlayBufAddr = u32Address;
	}		
	else
	{
		outp32(REG_I2S_ACTL_RDSTB, u32Address);
		g_DrvI2S_u32RecordBufAddr = u32Address;		
	}		
}	

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_GetDestBase                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      bType - [in], select DrvI2S_PLAY or DrvI2S_RECORD							                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      base address                                                                                       */
/*                                                                                                         */
/* Description:                                                                                            */
/*      get buffer base address setting		                                                               */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

UINT32 DrvI2S_GetDestBase(
	BOOL	bType				
)
{
	if(bType == DRVI2S_PLAY)
		return inp32(REG_I2S_ACTL_PDSTB);
	else
		return inp32(REG_I2S_ACTL_RDSTB);
}	

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_SetDestLength                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      bType - [in], select DrvI2S_PLAY or DrvI2S_RECORD							                       */
/*      u32Address - [in], set buffer length 										                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None                                                                                               */
/*                                                                                                         */
/* Description:                                                                                            */
/*      To set buffer legnth 				                                                               */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

VOID DrvI2S_SetDestLength(
	BOOL	bType, 			
	UINT32	u32Length
)
{
	if(bType == DRVI2S_PLAY)
	{
		outp32(REG_I2S_ACTL_PDST_LENGTH, u32Length);
		g_DrvI2S_u32PlayBufSize = u32Length;		
	}		
	else
	{
		outp32(REG_I2S_ACTL_RDST_LENGTH, u32Length);
		g_DrvI2S_u32RecordBufSize = u32Length;				
	}		
}	

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_GetDestLength                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      bType - [in], select DrvI2S_PLAY or DrvI2S_RECORD							                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      buffer length                                                                                      */
/*                                                                                                         */
/* Description:                                                                                            */
/*      get buffer length setting		  		                                                           */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

UINT32 DrvI2S_GetDestLength(
	BOOL	bType				
)
{
	if(bType == DRVI2S_PLAY)
		return inp32(REG_I2S_ACTL_PDST_LENGTH);
	else
		return inp32(REG_I2S_ACTL_RDST_LENGTH);
}	

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_GetDestLength                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      bType - [in], select DrvI2S_PLAY or DrvI2S_RECORD							                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      buffer current address                                                                             */
/*                                                                                                         */
/* Description:                                                                                            */
/*      get current record/play DMA address 	                                                           */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

UINT32 DrvI2S_GetCurrDest(		
	BOOL	bType				
)
{
	if(bType == DRVI2S_PLAY)
		return inp32(REG_I2S_ACTL_PDSTC);
	else
		return inp32(REG_I2S_ACTL_RDSTC);
}	


/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_StartPlay                           	                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      S_DRVI2S_PLAY* psPlayStruct													                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None					                                                                           */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Start playing audio data 				                                                           */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

VOID DrvI2S_StartPlay (
	S_DRVI2S_PLAY* psPlayStruct
)
{

	DrvI2S_SetDestBase(DRVI2S_PLAY, psPlayStruct->u32BufferAddr);
	DrvI2S_SetDestLength(DRVI2S_PLAY, psPlayStruct->u32BufferLength);
		
	outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) & ~(eDRVI2S_PLAY_STEREO));
    switch (psPlayStruct->eChannel)
    {
        case eDRVI2S_PLAY_MONO:
    		outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) | eDRVI2S_PLAY_MONO);
    		break;
    		
        case eDRVI2S_PLAY_STEREO:
        default:
    		outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) | eDRVI2S_PLAY_STEREO);	
    		break;
    }

	/* set play sampling rate and data format */
	DrvI2S_SetSampleRate(psPlayStruct->eSampleRate);
	DrvI2S_SetDataFormat(psPlayStruct->eFormat);	
		
	/* call back to fill DMA play buffer */
//	g_pfnPlayCallBack = pfnCallBack;	
//	g_pfnPlayCallBack((UINT8 *)g_DrvI2S_u32PlayBufAddr, g_DrvI2S_u32PlayBufSize/2);
//	g_pfnPlayCallBack((UINT8 *)(g_DrvI2S_u32PlayBufAddr+g_DrvI2S_u32PlayBufSize/2), g_DrvI2S_u32PlayBufSize/2);		
	
	/* enable I2S & disable AC-link and start playing */
	outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) | I2S_PLAY);						// Enable I2S path
	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | I2S_EN);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_StopPlay                            	                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      None 																		                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None 			                                                                                   */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Stop playing 					 		                                                           */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

VOID DrvI2S_StopPlay (
	void
)
{
	outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) & ~I2S_PLAY);						// Enable I2S path
	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) & ~I2S_EN);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_StartRecord                           	                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      S_DRVI2S_RECORD* psRecordStruct											                       	   */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None					                                                                           */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Start recording audio data 				                                                           */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

VOID DrvI2S_StartRecord (
	S_DRVI2S_RECORD* psRecordStruct
)
{

	DrvI2S_SetDestBase(DRVI2S_RECORD, psRecordStruct->u32BufferAddr);
	DrvI2S_SetDestLength(DRVI2S_RECORD, psRecordStruct->u32BufferLength);

	outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) & ~eDRVI2S_RECORD_STEREO);
	switch (psRecordStruct->eChannel)
    {
        case eDRVI2S_RECORD_LEFT:
    		outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) | eDRVI2S_RECORD_LEFT);	        
    		break;
    		
        case eDRVI2S_RECORD_RIGHT:
            outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) | eDRVI2S_RECORD_RIGHT);            		
    		break;
    
        default:
    		outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) | eDRVI2S_RECORD_STEREO);	    
    		break;
    }

	/* set play sampling rate and data format */
	DrvI2S_SetSampleRate(psRecordStruct->eSampleRate);
	DrvI2S_SetDataFormat(psRecordStruct->eFormat);	

	/* setup AIC I2S interrupt */			
//	outp32(REG_I2S_ACTL_CON, (inp32(REG_I2S_ACTL_CON) &~R_DMA_IRQ_SEL) |  (0x01 <<14));
	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | R_DMA_IRQ);

	
	/* enable I2S to start reccording */
	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) | I2S_EN);
	outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) | I2S_RECORD);						// Enable I2S path	
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_StopRecording                         	                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      None 																		                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None 			                                                                                   */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Stop recording 					 		                                                           */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

VOID DrvI2S_StopRecord (
	void
)
{
	outp32(REG_I2S_ACTL_RESET, inp32(REG_I2S_ACTL_RESET) & (~I2S_RECORD));						// Enable I2S path
	outp32(REG_I2S_ACTL_CON, inp32(REG_I2S_ACTL_CON) & ~I2S_EN);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_SetApllClock                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      clock - [in], APLL clock        		 						                                   */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      Success (0) or Fail (-1)					                                                       */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Set APLL clock              															           */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
int DrvI2S_SetApllClock(unsigned int clock)
{
	int ret = 0;
	
	/* external clock is forced to 12 MHz */ 
	int w55fa92_external_clock = 12000000;

	if (w55fa92_external_clock == 12000000) {
		if (clock == 430080) {
			// for SPU/I2S 16/48KHz * 256 and ADC 16KHz * 16 * 80, TD = 0.0186%
			outp32(REG_APLLCON, 0x09EB);
		}
		else if (clock == 344064) {
			// for SPU/I2S 32/64/96/192KHz * 256, TD = 0.0186%
			outp32(REG_APLLCON, 0x09D6);
		}
		else if (clock == 254016) {
			// for SPU/I2S 22.05KHz * 256 and ADC 11.025KHz * 16 * 80, TD = 0.0063%
			outp32(REG_APLLCON, 0x0B7F);
		}
		else if (clock == 225792) {
			// for SPU/I2S 44.1/88.2/176.4KHz * 256, TD = 0.0850%
			outp32(REG_APLLCON, 0x0ADE);
		}
		else if (clock == 208896) {
			// for I2S, TD = 0.0498%
			outp32(REG_APLLCON, 0x11E8);
		}
		else if (clock == 123000) {
			// for I2S, TD = 0.0498%
			outp32(REG_APLLCON, 0x1952);
		}
		else if (clock == 169344) {
			// for I2S, TD = 0.0921%
			outp32(REG_APLLCON, 0x1271);
		}
		else if (clock == 147456) {
			// for I2S, TD = 0.0977%
			outp32(REG_APLLCON, 0x12FB);
		}
		else if (clock == 135000) {
			// for TV 27MHz, TD = 0%, 8KHz TD = 0.1243%, 11.025KHz TD = 0.3508%
			outp32(REG_APLLCON, 0x195A);
		}
		else {
		//	printf("%s does not support %dKHz APLL clock!\n", __FUNCTION__, clock);
			ret = -1;
		}
	}
	else {
		// printf("%s does not support %d.%dMHz xtal clock!\n", __FUNCTION__, print_mhz(w55fa92_external_clock));
		ret = -1;
	}
	return ret;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_SetSampleRate                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      eSampleRate - [in], sampling rate selection 		 						                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      Success (0) or Fail (-1)					                                                       */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Select sampling rate 															                   */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

INT DrvI2S_SetSampleRate(
	E_DRVI2S_SAMPLING  eSampleRate)
{
	UINT32 u32MCLK;
	UINT32 uData, ApllFreq;
	UINT32 u32Divider0, u32Divider1, ii;
	INT ret = 0;

	g_DrvI2S_u32SampleRate = eSampleRate;
	
	/* MCLK = 256*WS */
	switch (eSampleRate)
	{
		case eDRVI2S_FREQ_8000:						// 8 KHz
			ApllFreq = 123000;			
			u32MCLK = 12288/6;									
			break;
		case eDRVI2S_FREQ_11025:					// 11.025 KHz
			ApllFreq = 169344;
			u32MCLK = 16934/6;							
			break;
		case eDRVI2S_FREQ_12000:
			ApllFreq = 123000;			
			u32MCLK = 12288/4;
			break;
		case eDRVI2S_FREQ_16000:					// 16 KHz
			ApllFreq = 123000;			
			u32MCLK = 12288/3;
			break;
		case eDRVI2S_FREQ_22050:					// 22.05 KHz
			ApllFreq = 169344;		
			u32MCLK = 16934/3;
			break;
		case eDRVI2S_FREQ_24000:					// 24 KHz
			ApllFreq = 123000;			
			u32MCLK = 12288/2;
			break;
		case eDRVI2S_FREQ_32000:			
			ApllFreq = 147456;
			u32MCLK = 16384/2;									
			break;
		case eDRVI2S_FREQ_44100:					// 44.1 KHz
			ApllFreq = 169344;		
			u32MCLK = 16934*2/3;
			break;
		case eDRVI2S_FREQ_48000:					// 48 KHz
		default:				
			ApllFreq = 123000;
			u32MCLK = 12288;
			break;
		case eDRVI2S_FREQ_64000:					//64KHz
			ApllFreq = 147456;
			u32MCLK = 16384;
			break;
		case eDRVI2S_FREQ_88200:					//88.2KHz
			ApllFreq = 135475;		
			u32MCLK = 16934*4/3;
			break;
		case eDRVI2S_FREQ_96000:					//96KHz
			ApllFreq = 147456;
			u32MCLK = 12288*2;
			break;
	}
	if (DrvI2S_SetApllClock(ApllFreq))
		return -1;
	
	uData = inp32(REG_I2S_ACTL_I2SCON) & 0x08;	
	outp32(REG_I2S_ACTL_I2SCON, uData);			
	u32Divider1 = ApllFreq / u32MCLK;
	
	for (ii=8; ii>1; ii--)
	{
		if (!(u32Divider1%ii))
			break;
	}			
	u32Divider0 = ii;
	u32Divider1 = u32Divider1 / ii;
		
	u32Divider0 --;				
	u32Divider1	--;			

	outp32(REG_CLKDIV7, (inp32(REG_CLKDIV7) & (~I2S_S)) | (0x02 << 11) );	// SPU clock from APLL	
	outp32(REG_CLKDIV7, (inp32(REG_CLKDIV7) & (~I2S_N0)) | (u32Divider0<<8));			
	outp32(REG_CLKDIV7, (inp32(REG_CLKDIV7) & (~I2S_N1)) | (u32Divider1<<16));		
	return ret;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_GetSampleRate                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      eSampleRate - [in], sampling rate selection 		 						                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      Sample rate 			                                                                           */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Select sampling rate 															                   */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

UINT32 DrvI2S_GetSampleRate(
	void
)
{
	return g_DrvI2S_u32SampleRate;
}	

/*---------------------------------------------------------------------------------------------------------*/
/* Function: DrvI2S_SetDataFormat                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*      eInterface - [in], I2S/MSB-justified format selection 						                       */
/*                                                                                                         */
/* Returns:                                                                                                */
/*      None					                                                                           */
/*                                                                                                         */
/* Description:                                                                                            */
/*      Select interface data format 													                   */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

static VOID DrvI2S_SetDataFormat (
	E_DRVI2S_FORMAT  eInterface)
{
	UINT32 uBuf, uData;
	UINT32 uFormatSel;

	switch(eInterface){
		case eDRVI2S_I2S:
			uFormatSel = eDRVI2S_I2S;
			break;
			
		case eDRVI2S_MSB_JUSTIFIED:
		default:
			uFormatSel = eDRVI2S_MSB_JUSTIFIED;
			break;
	}
	
	uData = (uFormatSel << 3);
	uBuf = inp32(REG_I2S_ACTL_I2SCON);
	uBuf &= ~0x18L;					// reserve PreScaler, BCLK_SEL, FS_SEL and MCLK_SEL settings
	uBuf |= uData;
	outp32(REG_I2S_ACTL_I2SCON,uBuf);
}
