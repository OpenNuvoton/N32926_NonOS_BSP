/***************************************************************************
 *                                                                         									     *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              					     *
 *                                                                         									     *
 ***************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "wblib.h"
#include "demo.h"
#include "w55fa92_gpio.h"

__align(32) UINT8 u32Array[1024*1024];



/* For SPU disable DAC off */
static void delay(UINT32 kk)
{
	volatile UINT32 ii, jj;
	
	for(ii=0; ii < kk; ii++)
	{
		for(jj=0; jj < 0x10; jj++);	
	}
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



/*--------------------------------------------------------------------------------------------------------*
 *                                                                                                        					     *
 * 																		     *
 *	Wake up source														     *
 * 	KPI_WE, ADC_WE, UHC_WE, UDC_WE, UART_WE, SDH_WE, RTC_WE, GPIO_WE		     *	
 *	2. Default priority	  													     *
 *---------------------------------------------------------------------------------------------------------*/
 #define REG_DLLMODE_R	0xb0003058	/* Not fix in B version. still read in 3058 */

void Demo_PowerDownWakeUp(void)
{
	/* 					*/
	
	UINT32 u32Idx;
	UINT32 reg_AHBCLK, reg_APBCLK;
	UINT32 u32MPllOutHz,u32UPllOutHz,u32APllOutHz,u32ExtFreq,u32Div,u32clock;	
	
	reg_AHBCLK = inp32(REG_AHBCLK);
	reg_APBCLK = inp32(REG_APBCLK);
	u32ExtFreq = sysGetExternalClock();    	/* Hz unit */	

	u32UPllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);			
	sysprintf("eSYS_UPLL %d\n",u32UPllOutHz);
	u32APllOutHz = sysGetPLLOutputHz(eSYS_APLL, u32ExtFreq);		
	sysprintf("eSYS_APLL %d\n",u32APllOutHz);	
	u32MPllOutHz = sysGetPLLOutputHz(eSYS_MPLL, u32ExtFreq);	
	sysprintf("eSYS_MPLL %d\n",u32MPllOutHz);	
		
#if 0	
	{
		PUINT8 pu8Buf, pu8Tmp;	
		pu8Buf = u32Array;	
		sysprintf("Allocate memory address =0x%x\n", pu8Buf);
		pu8Tmp = pu8Buf;
		for(u32Idx=0; u32Idx<(1024*1024);u32Idx=u32Idx+1)
			*pu8Tmp++= (UINT8)((u32Idx>>8) + u32Idx);
	}	
#endif 
	
#if 0		
	gpio_setportpull(GPIO_PORTA, 0x01, 0x01);		/*Set GPIOA-0 to pull high 		*/	
	gpio_setportdir(GPIO_PORTA, 0x01, 0x00);		/*Correct	Set GPIOA-0 as input port		*/			
	//gpio_setportdir(GPIO_PORTA, 0x01, 0x01);		/*Wrong Set GPIOA-0 as input port		*/
	gpio_setsrcgrp(GPIO_PORTA, 0x01, 0x00);		/*Group GPIOA-0 to EXT_GPIO0	*/
	gpio_setintmode(GPIO_PORTA, 0x01, 0x01, 0x01);	/*Rising/Falling				 	*/
#else
	gpio_setportpull(GPIO_PORTB, 0x01, 0x01);		/*Set GPIOB-0 to pull high 		*/	
	gpio_setportdir(GPIO_PORTB, 0x01, 0x00);		/*Correct	Set GPIOB-0 as input port		*/			
	//gpio_setportdir(GPIO_PORTA, 0x01, 0x01);		/*Wrong Set GPIOB-0 as input port		*/
	gpio_setsrcgrp(GPIO_PORTB, 0x01, 0x00);		/*Group GPIOB-0 to EXT_GPIO0	*/
	gpio_setintmode(GPIO_PORTB, 0x01, 0x01, 0x01);	/*Rising/Falling				 	*/
#endif	
	
	outp32(REG_IRQTGSRC0, 0xFFFFFFFF);
	outp32(REG_IRQLHSEL, 0x11);
	/* Set gpio wake up source */
	
	sysprintf("Enter power down, GPIO Int status 0x%x\n", inp32(REG_IRQTGSRC0));

///////////////////////////////////////
#if 1
	outpw(REG_AHBCLK, 0xFFFFFFFF);
	outpw(REG_AHBCLK, 0xFFFFFFFF);
	
	DBG_PRINTF("Disable USB Transceiver\n");
	/* UPLL */
	if((u32UPllOutHz != 0) && ((u32UPllOutHz % 48000000) == 0)){
		u32clock = u32UPllOutHz / 48000000;
		u32Div = 0x18 | ((u32clock - 1)<<8); 
		sysprintf("USBH2.0 Clock source is UPLL\n");
	}
	else if((u32APllOutHz != 0) && ((u32APllOutHz % 48000000) == 0)){
		u32clock = u32APllOutHz / 48000000;
		u32Div = 0x10 | ((u32clock - 1)<<8);
		sysprintf("USBH2.0 Clock source is APLL\n"); 
	}
	else if((u32MPllOutHz != 0) && ((u32MPllOutHz % 48000000) == 0)){
		u32clock = u32MPllOutHz / 48000000;
		u32Div = 0x8 | ((u32clock - 1)<<8); 
		sysprintf("USBH2.0 Clock source is MPLL\n");
	}	 		
	outp32(REG_CLKDIV6, u32Div);	
		

	outp32(REG_CLKDIV2, inp32(REG_CLKDIV2) & ~(U20PHY_SS|U20PHY_N));	
	outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) | H20PHY_CKE);		//USBH 20 clock
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | USBH_CKE);					//USB Host transceiver disable. 
	DBG_PRINTF("REG_CLKDIV2 = 0x%d\n", inp32(REG_CLKDIV2));
	DBG_PRINTF("REG_CLKDIV6 = 0x%d\n", inp32(REG_CLKDIV6));
	outp32(0xb1009200, 0x08000000);
	outp32(REG_USBPCR0, inp32(REG_USBPCR0)&~BIT8); 
	DBG_PRINTF("REG_USBPCR = 0x%d\n", inp32(REG_USBPCR0));
	
	DBG_PRINTF("Disable Audio ADC and Touch ADC and LVR\n");
	outp32(REG_APBCLK, inp32(REG_APBCLK) | (ADC_CKE | TOUCH_CKE));			

#if 0		//FA95		
	outp32 (REG_ADC_CON, inp32(REG_ADC_CON) & ~(ADC_CON_ADC_EN | AUDADC_EN)); //ADC touch and ADC audio disable
	outp32 (REG_AUDIO_CON, inp32(REG_AUDIO_CON) & ~AUDIO_VOL_EN);
#else	//FA92
	outp32(REG_APBCLK, inp32(REG_APBCLK) | (ADC_CKE|TOUCH_CKE));	
#endif
	

	outp32(REG_POR_LVRD, inp32(REG_POR_LVRD) | 0x10);					//Disable POR


	//sysDelay(1);
	//outp32(REG_APBCLK, inp32(REG_APBCLK) & ~(ADC_CKE|TOUCH_CKE));
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~USBH_CKE);					//USB Host transceiver disable. 
	
	DBG_PRINTF("Disable SPU and ADO\n");																												
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | (SPU_CKE | ADO_CKE));		//DAC VDD33 power down 															
	//outp32(REG_SPU_DAC_VOL, inp32(REG_SPU_DAC_VOL) | ANA_PD);		//DAC SPU HPVDD33		//DAC SPU VDD33
	DBG_PRINTF("DAC register Index 0x5 = 0x%x\n", DrvSPU_ReadDACReg(0x05));
	DBG_PRINTF("DAC register Index 0x7 = 0x%x\n", DrvSPU_ReadDACReg(0x07));
	DrvSPU_WriteDACReg(0x05, 0xFF);
	DrvSPU_WriteDACReg(0x07, 0x00);
	DBG_PRINTF("DAC register Index 0x5 = 0x%x\n", DrvSPU_ReadDACReg(0x05));
	DBG_PRINTF("DAC register Index 0x7 = 0x%x\n", DrvSPU_ReadDACReg(0x07));
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~(SPU_CKE | ADO_CKE));															
														

	DBG_PRINTF("Disable USB phy\n");														
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | USBD_CKE);				//USB phy disable
	outp32(PHY_CTL, inp32(PHY_CTL)&~Phy_suspend);
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~USBD_CKE);
	DBG_PRINTF("Disable TV DAC \n");
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | VPOST_CKE);				//TV DAC
	outp32(REG_LCM_TVCtl, inp32(REG_LCM_TVCtl) | TVCtl_Tvdac);
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) & ~VPOST_CKE);
	
	

	//V
	//outp32(REG_APBCLK, inp32(REG_APBCLK) | (ADC_CKE|TOUCH_CKE));
	//outp32(REG_TP_CTL1, inp32(REG_TP_CTL1) | (PD_Power|PD_BUF|LOW_SPEED));						
	//outp32(REG_DLLMODE,  (inp32(REG_DLLMODE_R)&~0x8) | 0x10);	// Disable DLL of FA92	


	/* change  SD card pin function */
	outp32(REG_GPAFUN0, 0x0);	
	//outpw(REG_GPAFUN1, 0x0);	
	
	outp32(REG_GPBFUN0, 0x0);	
	outp32(REG_GPBFUN1, 0x0);	
	
	outp32(REG_GPCFUN0, 0x0);	
	outp32(REG_GPCFUN1, 0x0);	
	
	outp32(REG_GPEFUN0, 0x0);	
	outp32(REG_GPEFUN1, 0x0);			

	//Bon suggest
    	outp32(REG_GPIOG_PUEN, inp32(REG_GPIOG_PUEN)&~(BIT11|BIT12|BIT13|BIT14|BIT15));	
	
 	//outp32(REG_GPAFUN, (MF_GPA11 | MF_GPA10));
	DBG_PRINTF("GPIOA STATUS = 0x%x\n", inp32(REG_GPIOA_PIN));
	DBG_PRINTF("GPIOB STATUS = 0x%x\n", inp32(REG_GPIOB_PIN));
	DBG_PRINTF("GPIOC STATUS = 0x%x\n", inp32(REG_GPIOC_PIN));
	DBG_PRINTF("GPIOD STATUS = 0x%x\n", inp32(REG_GPIOD_PIN));
	DBG_PRINTF("GPIOE STATUS = 0x%x\n", inp32(REG_GPIOE_PIN));
	DBG_PRINTF("GPIOG STATUS = 0x%x\n", inp32(REG_GPIOG_PIN));
	DBG_PRINTF("GPIOH STATUS = 0x%x\n", inp32(REG_GPIOH_PIN));

	outp32(REG_GPIOA_OMD, 0x0);
	outp32(REG_GPIOB_OMD, 0x0);
	outp32(REG_GPIOC_OMD, 0x0);
	outp32(REG_GPIOD_OMD, 0x0);
	outp32(REG_GPIOE_OMD, 0x0);
	outp32(REG_GPIOG_OMD, 0x0);
	outp32(REG_GPIOH_OMD, 0x0);
	outp32(REG_GPIOA_PUEN, 0x3FF);
	outp32(REG_GPIOB_PUEN, 0xFFFF);
	outp32(REG_GPIOC_PUEN, 0xFFFF);
	outp32(REG_GPIOD_PUEN, 0xFFFF);
	outp32(REG_GPIOE_PUEN, 0x0FFF);	
	

	outp32(REG_GPIOG_PUEN, ~0xF85C);		//Pull up is inverse !	1.634~1.682mA

	//outp32(REG_GPIOH_PUEN, 0x0);		//Don't set GPIOH. R_FB will consume some power.
		
	outp32(REG_APBCLK, inp32(REG_APBCLK) | (ADC_CKE|TOUCH_CKE));
	outp32(REG_TP_CTL1, inp32(REG_TP_CTL1) | (PD_Power|PD_BUF|LOW_SPEED));											
	//outp32(REG_SDOPM, inp32(REG_SDOPM) & ~AUTOPDN);
	
#endif 	
	
	outp32(REG_SHRPIN_TOUCH, 0x0);
	//outp32(REG_SHRPIN_AUDIO, 0x0); //enable it cause current rising 300uA ~ 400uA
	outp32(REG_PWRCON, (inp32(REG_PWRCON) & ~(0xFFFF00)) | (0xFF<<8)); /* Wake up time about 40ms */
	sysPowerDown(WE_GPIO);
	
	outp32(REG_AHBCLK, reg_AHBCLK);
	outp32(REG_APBCLK, reg_APBCLK);
	
	sysprintf("Exit power down\n");		
#if 0	
	pu8Tmp = pu8Buf;
	for(u32Idx=0; u32Idx<(1024*1024);u32Idx=u32Idx+1)
	{
		if( *pu8Tmp !=  (UINT8)((u32Idx>>8) + u32Idx))
		{
			sysprintf("!!!!!!!!!!!!!!!Data is noconsisient after power down\n");
			sysprintf("0x%x, 0x%x, 0x%x)\n",u32Idx, *pu8Tmp, (UINT8)((u32Idx>>8) + u32Idx) );
			free(pu8Buf);	
			return;
		}	
		pu8Tmp++;
	}
	sysprintf("Data is consisient\n");
#endif	
	//free(pu8Buf);	
}






