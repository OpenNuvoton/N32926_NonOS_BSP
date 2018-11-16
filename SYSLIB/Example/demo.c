/***************************************************************************
 *                                                                         									     *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              					     *
 *                                                                         									     *
 ***************************************************************************/

#include <stdio.h>
#include "wblib.h"
#include "demo.h"
#include "w55fa92_gpio.h"
//#pragma import (__use_no_semihosting_swi)

extern int DemoAPI_AIC(void);
extern void DemoAPI_UART(void);
extern void DemoAPI_Timer0(void);
extern void DemoAPI_Timer1(void);
extern void DemoAPI_WDT(void);
extern void DemoAPI_Cache(BOOL bIsCacheOn);
extern void DemoAPI_CLK(void);
extern void DemoAPI_CLKRandom(void);
extern void DemoAPI_Timer2(void);
extern void DemoAPI_Timer3(void);
extern void DemoAPI_ChangeMPLL(void);
extern void DemoAPI_ChangeSystemClock(void);
extern void DemoAPI_ChangeMPLL_FromOtherPLL(void);
extern int DemoAPI_HUART(void);
extern void DemoAPI_Timer0_SetLocalTime(void);
extern void DemoAPI_ChangeMemoryClock(void);

unsigned long initMMU(void);

int main()
{
	//unsigned int volatile i;
	WB_UART_T uart;
	UINT32 u32Item, u32ExtFreq;
	UINT32 u32PllOutKHz;
	

	
	u32ExtFreq = sysGetExternalClock();
	uart.uart_no = WB_UART_1; 
	uart.uiFreq = u32ExtFreq;	//use APB clock
    	uart.uiBaudrate = 115200;
    	uart.uiDataBits = WB_DATA_BITS_8;
    	uart.uiStopBits = WB_STOP_BITS_1;
    	uart.uiParity = WB_PARITY_NONE;
    	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    	sysInitializeUART(&uart);
    	sysSetLocalInterrupt(ENABLE_FIQ_IRQ);	
   
    	u32PllOutKHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
	DBG_PRINTF("Power on UPLL out frequency %d Khz\n", u32PllOutKHz);	
	u32PllOutKHz = sysGetPLLOutputHz(eSYS_MPLL, u32ExtFreq);    	
	DBG_PRINTF("Power on MPLL out frequency %d Khz\n", u32PllOutKHz);
	
	u32PllOutKHz = sysGetAPBClock();    	
	DBG_PRINTF("APB %d Khz\n", u32PllOutKHz);
#if 0 //Slower down clock, start from system clock then memory clock
	sysSetSystemClock(eSYS_EXT, 12000000, 12000000);
	sysSetDramClock(eSYS_EXT, 12000000, 12000000);						
#else//Speed up clock, start from memory clock then system clock	
#if 1
	sysSetDramClock(eSYS_MPLL, 360000000, 360000000);
	sysSetSystemClock(eSYS_UPLL, 		//E_SYS_SRC_CLK eSrcClk,
					240000000,		//UINT32 u32PllKHz,
					240000000);		//UINT32 u32SysKHz,
#endif
#endif					

	do
	{    	
		DBG_PRINTF("================================================================\n");
		DBG_PRINTF("						System library demo code									  	\n");
		DBG_PRINTF(" [1] UART demo 																  	\n");
		DBG_PRINTF(" [2] Timer0 demo 																\n");
		DBG_PRINTF(" [3] Timer1 demo 																\n");
		DBG_PRINTF(" [4] Timer2 demo 																\n");
		DBG_PRINTF(" [5] Timer3 demo 																\n");
		DBG_PRINTF(" [6] Watch dog																	\n");		
		DBG_PRINTF(" [7] Cache demo disable															\n");
		DBG_PRINTF(" [8] Cache demo enable													          	\n");
		DBG_PRINTF(" [9] AIC demo 																  	\n");				
		DBG_PRINTF(" [A] Clock switch 																\n");
		DBG_PRINTF(" [B] Clock switch random															\n");
		DBG_PRINTF(" [C] Change MPLL source															\n");
		DBG_PRINTF(" [D] Demo High Speec UART 														\n");
		DBG_PRINTF(" [E] Demo to set local time for FAT													\n");
		DBG_PRINTF(" [F] Power down then wake up by GPIO-B0 level change									\n");
		DBG_PRINTF(" [G] Change MPLL clock															\n");
		DBG_PRINTF("================================================================\n");
	
		DBG_PRINTF("REG_CLKDIV0 = 0x%x\n", inp32(REG_CLKDIV0));
		DBG_PRINTF("REG_CLKDIV3 = 0x%x\n", inp32(REG_CLKDIV3));	
		DBG_PRINTF("DRAM frequency %d Khz\n", sysGetDramClock());
		DBG_PRINTF("SYS frequency %d Khz\n", sysGetSystemClock());
	
		//outp32(0xb0000084, 0x03);
		//outp32(0xb0000230, 0x85);
		u32Item = sysGetChar();
		switch(u32Item)
		{
			case '1': 	DemoAPI_UART();		break; 	//OK-sysprintf
	    		case '2': 	DemoAPI_Timer0();		break;
	    		case '3': 	DemoAPI_Timer1();		break;	
	    		case '4': 	DemoAPI_Timer2();		break;
	    		case '5': 	DemoAPI_Timer3();		break;	
	    		case '6':	DemoAPI_WDT();		break;
	    		case '7':	DemoAPI_Cache(FALSE);	break;	
	    		case '8':	DemoAPI_Cache(TRUE);	break;	
	    		case '9': 	DemoAPI_AIC(); 		break;	    		    	    	
	    		case 'A':	
	    				sysprintf("Remember MCLK's clock need > HCLK1, HCLK3, HCLK4\n");
	    				sysprintf("And all of IP can't have memory require duration switching MPLL clock\n");
	    				sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/64);	
	    				DemoAPI_ChangeMPLL();	break;
	    				
	    		case 'B':		    	
	    				sysprintf("And all of IP can't have memory require duration switching MPLL clock\n");
	    				sysprintf("Remember MCLK's clock need > HCLK1, HCLK3, HCLK4\n");
	    				sysSetDramClock(eSYS_MPLL, 360000000, 360000000);	
	    				sysprintf("Set MCLK's clock to 360000000/2\n");	    				
	    				DemoAPI_ChangeSystemClock();	break;	
	    				
	    		case 'C':		    	
	    				sysprintf("And all of IP can't have memory require duration switching MPLL clock\n");
	    				sysprintf("Remember MCLK's clock need > HCLK1, HCLK3, HCLK4\n");
	    				sysSetDramClock(eSYS_MPLL, 360000000, 360000000);	
	    				sysprintf("Set MCLK's clock to 360000000/2\n");	    				
	    				DemoAPI_ChangeMPLL_FromOtherPLL();	break;		    
	    					
	    		case 'D':	DemoAPI_HUART();		break;
	    		case 'E':	DemoAPI_Timer0_SetLocalTime();	break;
	    	
	    		case 'F':	
	    				sysprintf("Register GPAB Int = 0x%x\n", inp32(REG_IRQTGSRC0));
			    		sysprintf("Register GPCD Int= 0x%x\n", inp32(REG_IRQTGSRC1));
			    		sysprintf("Register GPEF Int = 0x%x\n", inp32(REG_IRQTGSRC2));
			    		sysprintf("Register MISSR = 0x%x\n", inp32(REG_MISSR));
	    				Demo_PowerDownWakeUp();		break;					
	    				
	    		  case 'G':	
	    			DemoAPI_ChangeMemoryClock();	    					    						
				break;		
	    		case 'Q':	break;
	    		case 'q':	break;
		}
	}while((u32Item!= 'q') || (u32Item!= 'Q'));	
    	return 0;
} /* end main */
