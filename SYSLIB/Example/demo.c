/**************************************************************************//**
 * @file     demo.c
 * @brief    Sample code to demostrate the APIs of SYS library
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include <stdio.h>
#include "wblib.h"
#include "demo.h"
#include "W55FA92_GPIO.h"
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

int main()
{
    WB_UART_T uart;
    UINT32 u32Item, u32ExtFreq;
    UINT32 u32PllOutHz;

    u32ExtFreq = sysGetExternalClock();
    uart.uart_no = WB_UART_1;
    uart.uiFreq = u32ExtFreq;   //use APB clock
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    sysInitializeUART(&uart);
    sysSetLocalInterrupt(ENABLE_FIQ_IRQ);

    u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
    DBG_PRINTF("Power on UPLL out frequency %d Khz\n", u32PllOutHz);
    u32PllOutHz = sysGetPLLOutputHz(eSYS_MPLL, u32ExtFreq);
    DBG_PRINTF("Power on MPLL out frequency %d Khz\n", u32PllOutHz);

    u32PllOutHz = sysGetAPBClock();
    DBG_PRINTF("APB %d Hz\n", u32PllOutHz);

    /**********************************************************************************************
     * Clock Constraints:
     * (a) If Memory Clock > System Clock, the source clock of Memory and System can come from
     *     different clock source. Suggestion MPLL for Memory Clock, UPLL for System Clock
     * (b) For Memory Clock = System Clock, the source clock of Memory and System must come from
     *     same clock source
     *********************************************************************************************/
#if 0
    /**********************************************************************************************
     * Slower down system and memory clock procedures:
     * If current working clock fast than desired working clock, Please follow the procedure below
     * 1. Change System Clock first
     * 2. Then change Memory Clock
     *
     * Following example shows the Memory Clock = System Clock case. User can specify different
     * Memory Clock and System Clock depends on DRAM bandwidth or power consumption requirement.
     *********************************************************************************************/
    sysSetSystemClock(eSYS_EXT, 12000000, 12000000);
    sysSetDramClock(eSYS_EXT, 12000000, 12000000);
#else
    /**********************************************************************************************
     * Speed up system and memory clock procedures:
     * If current working clock slower than desired working clock, Please follow the procedure below
     * 1. Change Memory Clock first
     * 2. Then change System Clock
     *
     * Following example shows to speed up clock case. User can specify different
     * Memory Clock and System Clock depends on DRAM bandwidth or power consumption requirement.
     *********************************************************************************************/
    sysSetDramClock(eSYS_MPLL, 360000000, 360000000);
    sysSetSystemClock(eSYS_UPLL,            //E_SYS_SRC_CLK eSrcClk,
                      240000000,            //UINT32 u32PllHz,
                      240000000);           //UINT32 u32SysHz,
    sysSetCPUClock(240000000/2);
#endif

    do
    {
        DBG_PRINTF("================================================================\n");
        DBG_PRINTF("                     System library demo code                   \n");
        DBG_PRINTF(" [1] UART demo                                                  \n");
        DBG_PRINTF(" [2] Timer0 demo                                                \n");
        DBG_PRINTF(" [3] Timer1 demo                                                \n");
        DBG_PRINTF(" [4] Timer2 demo                                                \n");
        DBG_PRINTF(" [5] Timer3 demo                                                \n");
        DBG_PRINTF(" [6] Watch dog                                                  \n");
        DBG_PRINTF(" [7] Cache demo disable                                         \n");
        DBG_PRINTF(" [8] Cache demo enable                                          \n");
        DBG_PRINTF(" [9] AIC demo                                                   \n");
        DBG_PRINTF(" [A] Clock switch                                               \n");
        DBG_PRINTF(" [B] Clock switch random                                        \n");
        DBG_PRINTF(" [C] Change MPLL source                                         \n");
        DBG_PRINTF(" [D] Demo High Speec UART                                       \n");
        DBG_PRINTF(" [E] Demo to set local time for FAT                             \n");
        DBG_PRINTF(" [F] Power down then wake up by GPIO-B0 level change            \n");
        DBG_PRINTF(" [G] Change MPLL clock                                          \n");
        DBG_PRINTF("================================================================\n");

        DBG_PRINTF("REG_CLKDIV0 = 0x%x\n", inp32(REG_CLKDIV0));
        DBG_PRINTF("REG_CLKDIV3 = 0x%x\n", inp32(REG_CLKDIV3));
        DBG_PRINTF("DRAM frequency %d Hz\n", sysGetDramClock());
        DBG_PRINTF("SYS frequency %d Hz\n", sysGetSystemClock());

        //outp32(0xb0000084, 0x03);
        //outp32(0xb0000230, 0x85);
        u32Item = sysGetChar();
        switch(u32Item)
        {
        case '1':
            DemoAPI_UART();
            break;  //OK-sysprintf
        case '2':
            DemoAPI_Timer0();
            break;
        case '3':
            DemoAPI_Timer1();
            break;
        case '4':
            DemoAPI_Timer2();
            break;
        case '5':
            DemoAPI_Timer3();
            break;
        case '6':
            DemoAPI_WDT();
            break;
        case '7':
            DemoAPI_Cache(FALSE);
            break;
        case '8':
            DemoAPI_Cache(TRUE);
            break;
        case '9':
            DemoAPI_AIC();
            break;
        case 'A':
            sysprintf("Remember MCLK's clock need > HCLK1, HCLK3, HCLK4\n");
            sysprintf("And all of IP can't have memory require duration switching MPLL clock\n");
            sysSetSystemClock(eSYS_UPLL, 192000000, 192000000/64);
            DemoAPI_ChangeMPLL();
            break;

        case 'B':
            sysprintf("And all of IP can't have memory require duration switching MPLL clock\n");
            sysprintf("Remember MCLK's clock need > HCLK1, HCLK3, HCLK4\n");
            sysSetDramClock(eSYS_MPLL, 360000000, 360000000);
            sysprintf("Set MCLK's clock to 360000000/2\n");
            DemoAPI_ChangeSystemClock();
            break;

        case 'C':
            sysprintf("And all of IP can't have memory require duration switching MPLL clock\n");
            sysprintf("Remember MCLK's clock need > HCLK1, HCLK3, HCLK4\n");
            sysSetDramClock(eSYS_MPLL, 360000000, 360000000);
            sysprintf("Set MCLK's clock to 360000000/2\n");
            DemoAPI_ChangeMPLL_FromOtherPLL();
            break;

        case 'D':
            DemoAPI_HUART();
            break;
        case 'E':
            DemoAPI_Timer0_SetLocalTime();
            break;

        case 'F':
            sysprintf("Register GPAB Int = 0x%x\n", inp32(REG_IRQTGSRC0));
            sysprintf("Register GPCD Int= 0x%x\n", inp32(REG_IRQTGSRC1));
            sysprintf("Register GPEF Int = 0x%x\n", inp32(REG_IRQTGSRC2));
            sysprintf("Register MISSR = 0x%x\n", inp32(REG_MISSR));
            Demo_PowerDownWakeUp();
            break;

        case 'G':
            DemoAPI_ChangeMemoryClock();
            break;
        case 'Q':
            break;
        case 'q':
            break;
        }
    }
    while((u32Item!= 'q') || (u32Item!= 'Q'));
    return 0;
} /* end main */

