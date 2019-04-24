/***************************************************************************
 *                                                                         									     *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              					     *
 *                                                                         									     *
 ***************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "wblib.h"
#include "demo.h"

/*--------------------------------------------------------------------------------------------------------*
 *                                                                                                        					     *
 * Function-AIC_TEST_MODE() set  interrupt flags to drive AIC generation interrupt as expected       *
 *	1. Interrupt Priority														     *
 *	2. Default priority	  													     *
 *---------------------------------------------------------------------------------------------------------*/
#define _DRVAIC_TEST_MODE(bIsTestMode)			(outpw(REG_AIC_TEST, bIsTestMode))  		/* debug AIC_IPER*/
#define _DRVAIC_CLEAR_INT(u32IntChannels)			(outpw(REG_AIC_SCCR, u32IntChannels)) 	/* clear all int events */
#define _DRVAIC_CLEAR_H_INT(u32IntChannels)		(outpw(REG_AIC_SCCRH, u32IntChannels)) 	/* clear all int events */
#define _DRVAIC_ENABLE_INT(u32IntChannels)		(outpw(REG_AIC_MECR, u32IntChannels))   	/* turn on mask bit */
#define _DRVAIC_ENABLE_H_INT(u32IntChannels)		(outpw(REG_AIC_MECRH, u32IntChannels))   	/* turn on mask bit */
#define _DRVAIC_DISABLE_INT(u32IntChannels)		(outpw(REG_AIC_MDCR, u32IntChannels))   	/* turn on mask bit */
#define _DRVAIC_DISABLE_H_INT(u32IntChannels)		(outpw(REG_AIC_MDCRH, u32IntChannels))   	/* turn on mask bit */
#define _DRVAIC_TRIGGER_INT(u32IntChannels)		(outpw(REG_AIC_SSCR, u32IntChannels))   	/* trigger interrupt */
#define _DRVAIC_TRIGGER_H_INT(u32IntChannels)		(outpw(REG_AIC_SSCRH, u32IntChannels))   	/* trigger interrupt */
#define _DRVAIC_GETINTERRUPT()					(inpw(REG_AIC_IPER) >> 2)	
static VOID AIC_TEST_MODE(void)
{
	//_DRVAIC_TEST_MODE(0x01);		/* debug AIC_IPER*/
	_DRVAIC_CLEAR_INT(0xFFFFFFFE);		/* clear all int events */
	_DRVAIC_CLEAR_H_INT(0xFFFF);
	_DRVAIC_ENABLE_INT(0xFFFFFFFE);	/* turn on mask bit */
	_DRVAIC_ENABLE_H_INT(0xFFFF);
	_DRVAIC_CLEAR_INT(0xFFFFFFFE);		/* clear all int events */
	_DRVAIC_CLEAR_H_INT(0xFFFF);
	_DRVAIC_TRIGGER_INT(0xFFFFFFFE);	/* trigger all interrupt channel */	
	_DRVAIC_TRIGGER_H_INT(0xFFFF);
}

UINT32 IntPriority=0;
UINT8 Int[48]={0};

VOID Int_Channel_0(void)
{
	Int[0]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<0);
}
VOID Int_Channel_1(void)
{
	IntPriority = IntPriority+1;
	Int[1]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<1);
}
VOID Int_Channel_2(void)
{
	IntPriority = IntPriority+1;
	Int[2]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<2);
}
VOID Int_Channel_3(void)
{
	IntPriority = IntPriority+1;
	Int[3]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<3);
}
VOID Int_Channel_4(void)
{
	IntPriority = IntPriority+1;
	Int[4]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<4);
}
VOID Int_Channel_5(void)
{
	IntPriority = IntPriority+1;
	Int[5]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<5);
}
VOID Int_Channel_6(void)
{
	IntPriority = IntPriority+1;
	Int[6]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<6);
}
VOID Int_Channel_7(void)
{
	IntPriority = IntPriority+1;
	Int[7]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<7);
}
VOID Int_Channel_8(void)
{
	IntPriority = IntPriority+1;
	Int[8]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<8);
}
VOID Int_Channel_9(void)
{
	IntPriority = IntPriority+1;
	Int[9]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<9);
}
VOID Int_Channel_10(void)
{
	IntPriority = IntPriority+1;
	Int[10]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<10);
}
VOID Int_Channel_11(void)
{
	IntPriority = IntPriority+1;
	Int[11]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<11);
}
VOID Int_Channel_12(void)
{
	IntPriority = IntPriority+1;
	Int[12]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<12);
}
VOID Int_Channel_13(void)
{
	IntPriority = IntPriority+1;
	Int[13]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<13);
}
VOID Int_Channel_14(void)
{
	IntPriority = IntPriority+1;
	Int[14]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<14);
}
VOID Int_Channel_15(void)
{
	IntPriority = IntPriority+1;
	Int[15]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<15);
}
VOID Int_Channel_16(void)
{
	IntPriority = IntPriority+1;
	Int[16]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<16);
}
VOID Int_Channel_17(void)
{
	IntPriority = IntPriority+1;
	Int[17]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<17);
}
VOID Int_Channel_18(void)
{
	IntPriority = IntPriority+1;
	Int[18]= IntPriority;
	_DRVAIC_CLEAR_INT(1<<18);
}
VOID Int_Channel_19(void)
{
	IntPriority = IntPriority+1;
	Int[19]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<19);
}
VOID Int_Channel_20(void)
{
	IntPriority = IntPriority+1;
	Int[20]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<20);
}
VOID Int_Channel_21(void)
{
	IntPriority = IntPriority+1;
	Int[21]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<21);
}
VOID Int_Channel_22(void)
{
	IntPriority = IntPriority+1;
	Int[22]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<22);
}
VOID Int_Channel_23(void)
{
	IntPriority = IntPriority+1;
	Int[23]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<23);
}
VOID Int_Channel_24(void)
{
	IntPriority = IntPriority+1;
	Int[24]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<24);	
}
VOID Int_Channel_25(void)
{
	IntPriority = IntPriority+1;
	Int[25]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<25);
}
VOID Int_Channel_26(void)
{
	IntPriority = IntPriority+1;
	Int[26]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<26);
}
VOID Int_Channel_27(void)
{
	IntPriority = IntPriority+1;
	Int[27]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<27);
}
VOID Int_Channel_28(void)
{
	IntPriority = IntPriority+1;
	Int[28]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<28);
}
VOID Int_Channel_29(void)
{
	IntPriority = IntPriority+1;
	Int[29]=IntPriority;
	_DRVAIC_CLEAR_INT(1<<29);
}
VOID Int_Channel_30(void)
{
	IntPriority = IntPriority+1;
	Int[30]=IntPriority;
	_DRVAIC_CLEAR_INT(0x40000000);
}
VOID Int_Channel_31(void)
{
	IntPriority = IntPriority+1;
	Int[31]=IntPriority;
	_DRVAIC_CLEAR_INT(0x80000000);
}
VOID Int_Channel_32(void)
{
	IntPriority = IntPriority+1;
	Int[32]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<0);
}
VOID Int_Channel_33(void)
{
	IntPriority = IntPriority+1;
	Int[33]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<1);
}
VOID Int_Channel_34(void)
{
	IntPriority = IntPriority+1;
	Int[34]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<2);
}
VOID Int_Channel_35(void)
{
	IntPriority = IntPriority+1;
	Int[35]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<3);
}
VOID Int_Channel_36(void)
{
	IntPriority = IntPriority+1;
	Int[36]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<4);
}
VOID Int_Channel_37(void)
{
	IntPriority = IntPriority+1;
	Int[37]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<5);
}
VOID Int_Channel_38(void)
{
	IntPriority = IntPriority+1;
	Int[38]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<6);
}
VOID Int_Channel_39(void)
{
	IntPriority = IntPriority+1;
	Int[39]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<7);
}
VOID Int_Channel_40(void)
{
	IntPriority = IntPriority+1;
	Int[40]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<8);
}
VOID Int_Channel_41(void)
{
	IntPriority = IntPriority+1;
	Int[41]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<9);
}
VOID Int_Channel_42(void)
{
	IntPriority = IntPriority+1;
	Int[42]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<10);
}
VOID Int_Channel_43(void)
{
	IntPriority = IntPriority+1;
	Int[43]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<11);
}
VOID Int_Channel_44(void)
{
	IntPriority = IntPriority+1;
	Int[44]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<12);
}
VOID Int_Channel_45(void)
{
	IntPriority = IntPriority+1;
	Int[45]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<13);
}
VOID Int_Channel_46(void)
{
	IntPriority = IntPriority+1;
	Int[46]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<14);
}
VOID Int_Channel_47(void)
{
	IntPriority = IntPriority+1;
	Int[47]=IntPriority;
	_DRVAIC_CLEAR_H_INT(1<<15);
}

int DemoAPI_AIC(void)
{
	UINT32 u32Idx;
	UINT32 u32IntTypeLevel;
	
	DBG_PRINTF("+--------------------------------------------------------------------------------+\n");
	DBG_PRINTF("|                                 AIC Sample Code                                |\n");
	DBG_PRINTF("+--------------------------------------------------------------------------------+\n");
	DBG_PRINTF("\n");
	
	for(u32IntTypeLevel=1; u32IntTypeLevel<=7; u32IntTypeLevel=u32IntTypeLevel+1)
	{	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)0, (PVOID)Int_Channel_0);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)1, (PVOID)Int_Channel_1);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)2, (PVOID)Int_Channel_2);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)3, (PVOID)Int_Channel_3);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)4, (PVOID)Int_Channel_4);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)5, (PVOID)Int_Channel_5);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)6, (PVOID)Int_Channel_6);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)7, (PVOID)Int_Channel_7);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)8, (PVOID)Int_Channel_8);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)9, (PVOID)Int_Channel_9);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)10, (PVOID)Int_Channel_10);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)11, (PVOID)Int_Channel_11);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)12, (PVOID)Int_Channel_12);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)13, (PVOID)Int_Channel_13);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)14, (PVOID)Int_Channel_14);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)15, (PVOID)Int_Channel_15);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)16, (PVOID)Int_Channel_16);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)17, (PVOID)Int_Channel_17);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)18, (PVOID)Int_Channel_18);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)19, (PVOID)Int_Channel_19);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)20, (PVOID)Int_Channel_20);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)21, (PVOID)Int_Channel_21);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)22, (PVOID)Int_Channel_22);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)23, (PVOID)Int_Channel_23);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)24, (PVOID)Int_Channel_24);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)25, (PVOID)Int_Channel_25);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)26, (PVOID)Int_Channel_26);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)27, (PVOID)Int_Channel_27);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)28, (PVOID)Int_Channel_28);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)29, (PVOID)Int_Channel_29);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)30, (PVOID)Int_Channel_30);
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)31, (PVOID)Int_Channel_31);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)32, (PVOID)Int_Channel_32);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)33, (PVOID)Int_Channel_33);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)34, (PVOID)Int_Channel_34);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)35, (PVOID)Int_Channel_35);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)36, (PVOID)Int_Channel_36);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)37, (PVOID)Int_Channel_37);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)38, (PVOID)Int_Channel_38);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)39, (PVOID)Int_Channel_39);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)40, (PVOID)Int_Channel_40);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)41, (PVOID)Int_Channel_41);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)42, (PVOID)Int_Channel_42);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)43, (PVOID)Int_Channel_43);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)44, (PVOID)Int_Channel_44);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)45, (PVOID)Int_Channel_45);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)46, (PVOID)Int_Channel_46);	
		sysInstallISR(u32IntTypeLevel, (INT_SOURCE_E)47, (PVOID)Int_Channel_47);	
		sysSetLocalInterrupt(DISABLE_FIQ_IRQ);		
				
		for(u32Idx=0;u32Idx<48;u32Idx=u32Idx+1)
		{
		    /* Only Rising edge and falling edge can be teset */		
			sysEnableInterrupt((INT_SOURCE_E)u32Idx);
			if(u32IntTypeLevel%2)
				sysSetInterruptType((INT_SOURCE_E)u32Idx, 2);//Falling
			else
				sysSetInterruptType((INT_SOURCE_E)u32Idx, 3);//Rising		
		}		
		
		
		AIC_TEST_MODE();
		
				
		sysSetLocalInterrupt(ENABLE_FIQ_IRQ);					
	  	
		for(u32Idx=0;u32Idx<48;u32Idx=u32Idx+1)
		{
			if(Int[u32Idx]!=u32Idx)
			{
				sysSetLocalInterrupt(DISABLE_FIQ_IRQ);  
				DBG_PRINTF("AIC test mode is fail\n");
			}
			else
			{
			    DBG_PRINTF("Interrupt Source %d test ok!\n", u32Idx);
			}	
		}
		IntPriority = 0;
	}
	sysSetLocalInterrupt(DISABLE_FIQ_IRQ);			

	DBG_PRINTF("AIC sample finished!\n");
	
	return 0;
}






