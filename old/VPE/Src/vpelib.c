/***************************************************************************
 *                                                                         *
 * Copyright (c) 2009 Nuvoton Technology. All rights reserved.             *
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     vpelib.c
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     The header file of w55fa95 vpe library.
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *     None
 *
 *
 * REMARK
 *     None
 **************************************************************************/

#include "w55fa92_reg.h"
#include "wblib.h"
#include "w55fa92_vpe.h"

/* 
	
*/
PFN_VPE_CALLBACK (vpeIntHandlerTable)[6]={0};
void vpeInIntHandler(void)
{
	UINT32 u32VpeInt;
	u32VpeInt = inp32(REG_VPE_INTS)&0x3F;
	/*
	if((u32VpeInt&LL_INTS)==LL_INTS)
	{
		sysprintf("Page Misss interrupt\n");
		while(1);
	}
	*/
	if( (u32VpeInt&VP_INTS) == VP_INTS)
	{//VPE complete
		if(vpeIntHandlerTable[0]!=0)	
			vpeIntHandlerTable[0]();			/* Clear Interrupt */
		outp32(REG_VPE_INTS, (u32VpeInt & ~(TA_INTS | DE_INTS | MB_INTS | PG_MISS | PF_INTS)));			
		return;
	}	
	if( (u32VpeInt&PF_INTS) == PF_INTS)
	{//Page Fault
		outp32(REG_VPE_INTS, (u32VpeInt & ~(TA_INTS | DE_INTS | MB_INTS | PG_MISS | VP_INTS)));	/* Clear Interrupt */	
		if(vpeIntHandlerTable[1]!=0)	
			vpeIntHandlerTable[1]();					
		return;	
	}	
	if( (u32VpeInt&PG_MISS) == PG_MISS)
	{//Page Missing
		outp32(REG_VPE_INTS, (u32VpeInt & ~(TA_INTS | DE_INTS | MB_INTS | PF_INTS |VP_INTS)));	/* Clear Interrupt */
		if(vpeIntHandlerTable[2]!=0)	
			vpeIntHandlerTable[2]();			
		return;
	}	
	if( (u32VpeInt&MB_INTS) == MB_INTS)
	{//MB complete, Invalid due to JPEG OTF removed 
		if(vpeIntHandlerTable[3]!=0)	
			vpeIntHandlerTable[3]();			
		outp32(REG_VPE_INTS, (u32VpeInt & ~(TA_INTS | DE_INTS | PG_MISS | PF_INTS |VP_INTS)));	/* Clear Interrupt */	
		outp32(REG_VPE_TG, inp32(REG_VPE_TG)&~0x01);
	}
	if( (u32VpeInt&DE_INTS) == DE_INTS)
	{//Decode error,  Invalid due to JPEG OTF removed 
		if(vpeIntHandlerTable[4]!=0)	
			vpeIntHandlerTable[4]();			/* Clear Interrupt */
		outp32(REG_VPE_INTS, (u32VpeInt & ~(TA_INTS | MB_INTS| PG_MISS | PF_INTS |VP_INTS)));		
	}
	if( (u32VpeInt&TA_INTS) == TA_INTS)
	{//DMA abort
		if(vpeIntHandlerTable[5]!=0)	
			vpeIntHandlerTable[5]();			/* Clear Interrupt */
		outp32(REG_VPE_INTS, (u32VpeInt & ~(DE_INTS | MB_INTS| PG_MISS | PF_INTS |VP_INTS)));		
	}
}
/*-----------------------------------------------------------------------------------------------------------
*	The Function open VPE driver. 
*	And if specified PLL not meet some costraint, the funtion will search the near frequency and 
*	not over the specified frequency
*	
*	1. Enable clock 
*	2. Set correct clock
*	3. Set multiple pin function
*	3. Reset IP
*	 
*	Return: 
*		Error code or Successful                                                                                                        
-----------------------------------------------------------------------------------------------------------*/

static INT vpeOpenCount = 0;
ERRCODE vpeOpen(void)
{

//	UINT32 u32PllFreq;
//	UINT32 u32VpeDiv;
/* VPE without engine clock. It is always same as HCLK */	
//	sysGetExternalClock();
//	u32PllFreq = sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock());
//	u32VpeDiv = u32PllFreq / WorkingFreq;  	
//	outp32(REG_CLKDIV5, u32VpeDiv-1);
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | HCLK3_CKE | GVE_CKE | VPE_CKE);
	if(vpeOpenCount>0)
	{
		return ERR_VPE_OPEN;
	}
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) | VPE_RST);
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) & ~VPE_RST);	
	outp32(REG_VPE_CMD, inp32(REG_VPE_CMD) | BUSRT | BIT13 | BIT12);	//Burst write- Dual buffer //Lost Block 
	
	vpeOpenCount = vpeOpenCount+1;
	
	sysInstallISR(IRQ_LEVEL_1, 
						IRQ_VPE, 
						(PVOID)vpeInIntHandler);						
	sysEnableInterrupt(IRQ_VPE);		
	return Successful;  
}
/*-----------------------------------------------------------------------------------------------------------
*	The Function close VPE driver. 
*	And if specified PLL not meet some costraint, the funtion will search the near frequency and 
*	not over the specified frequency
*	
*	 
*	Return: 
*		Error code or Successful                                                                                                        
-----------------------------------------------------------------------------------------------------------*/
ERRCODE vpeClose(void)
{
	if(vpeOpenCount==1)
		vpeOpenCount = vpeOpenCount-1;			
	else	
		return ERR_VPE_CLOSE;		
	return Successful;  
}

/*-----------------------------------------------------------------------------------------------------------
*	The Function install call back function
*   
*	
*	 
*	Return: 
*		Error code or Successful                                                                                                        
-----------------------------------------------------------------------------------------------------------*/
ERRCODE 
vpeInstallCallback(
	E_VPE_INT_TYPE eIntType, 
	PFN_VPE_CALLBACK pfnCallback,
	PFN_VPE_CALLBACK* pfnOldCallback
	)
{
	if(eIntType == VPE_INT_COMP)
	{//VPE complete
		*pfnOldCallback = vpeIntHandlerTable[0];
		vpeIntHandlerTable[0] = (PFN_VPE_CALLBACK)(pfnCallback);
	}	
	else if(eIntType == VPE_INT_PAGE_FAULT)
	{//Page fault
		*pfnOldCallback = vpeIntHandlerTable[1];
		vpeIntHandlerTable[1] = (PFN_VPE_CALLBACK)(pfnCallback);
	}
	else if(eIntType == VPE_INT_PAGE_MISS)
	{
		*pfnOldCallback = vpeIntHandlerTable[2];
		vpeIntHandlerTable[2] = (PFN_VPE_CALLBACK)(pfnCallback);
	}	
	else if(eIntType == VPE_INT_MB_COMP)
	{
		*pfnOldCallback = vpeIntHandlerTable[3];
		vpeIntHandlerTable[3] = (PFN_VPE_CALLBACK)(pfnCallback);
	}	
	else if(eIntType == VPE_INT_MB_ERR)
	{
		*pfnOldCallback = vpeIntHandlerTable[4];
		vpeIntHandlerTable[4] = (PFN_VPE_CALLBACK)(pfnCallback);
	}	
	else if(eIntType == VPE_INT_DMA_ERR)
	{
		*pfnOldCallback = vpeIntHandlerTable[5];
		vpeIntHandlerTable[5] = (PFN_VPE_CALLBACK)(pfnCallback);
	}	
	else
		return E_VPE_INVALID_INT;			
	return Successful;	
}

ERRCODE 
vpeEnableInt(
	E_VPE_INT_TYPE eIntType
	)
{
	UINT32 u32IntEnable = inp32(REG_VPE_CMD);
	
	if(eIntType>VPE_INT_MB_COMP)
			return E_VPE_INVALID_INT; 
	
	outp32(REG_VPE_CMD, u32IntEnable |(1<<eIntType));	
	return Successful;
}	
ERRCODE 
vpeDisableInt(
	E_VPE_INT_TYPE eIntType
	)
{
	UINT32 u32IntEnable = inp32(REG_VPE_CMD);
	
	if(eIntType>VPE_INT_MB_COMP)
			return E_VPE_INVALID_INT; 
	
	outp32(REG_VPE_CMD, u32IntEnable& ~(1<<eIntType));	
	return Successful;
}

