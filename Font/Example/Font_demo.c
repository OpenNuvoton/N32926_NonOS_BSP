/**************************************************************************//**
 * @file     Font_demo.c
 * @brief    Draw font and border on panel
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "Font.h"
#include "W55FA92_VPOST.h"
#include "Font_demo.h"

extern S_DEMO_FONT s_sDemo_Font;
int font_x=0, font_y=16;
UINT32 u32SkipX;


#if 1
#define dbgprintf sysprintf
#else
#define dbgprintf(...)
#endif



#if defined (__GNUC__)
char pstrDisp[26][32] __attribute__((aligned(32))) =
{
#else
__align(32) char pstrDisp[26][32] =
{
#endif
    {"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"},
    {"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"},
    {"CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"},
    {"DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"},
    {"EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"},
    {"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"},
    {"GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG"},
    {"HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH"},
    {"IIIIIIIIIIIIIIIIIIIIIIIIIIIIIII"},
    {"JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ"},
    {"KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK"},
    {"LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"},
    {"MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM"},
    {"NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"},
    {"OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"},
    {"PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP"},
    {"QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ"},
    {"RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR"},
    {"SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS"},
    {"TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT"},
    {"UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU"},
    {"VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV"},
    {"WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"},
    {"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"},
    {"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY"},
    {"ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ"}
};





/**********************************/
int main()
{
    DateTime_T ltime;
    WB_UART_T uart;
    UINT32 u32ExtFreq;
    int j, i;
    UINT32 wait_ticks, no, dispno, dispcolor;

    //--- Reset SIC engine to make sure it under normal status.
    outp32(REG_AHBCLK, inp32(REG_AHBCLK) | (SIC_CKE | NAND_CKE | SD_CKE));
    outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) | SIC_RST);     // SIC engine reset is avtive
    outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) & ~SIC_RST);    // SIC engine reset is no active. Reset completed.

    /* enable cache */
    sysDisableCache();
    sysInvalidCache();
    sysEnableCache(CACHE_WRITE_BACK);

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
                      240000000,            //UINT32 u32PllKHz,
                      240000000);           //UINT32 u32SysKHz,
    sysSetCPUClock(240000000/2);
#endif
    
    sysprintf("\nDRAM frequency %d Hz\n", sysGetDramClock());
    sysprintf("SYS frequency %d Hz\n", sysGetSystemClock());

    /* configure Timer0 for FMI library */
    sysSetTimerReferenceClock(TIMER0, 12000000);
    sysStartTimer(TIMER0, 100, PERIODIC_MODE);

    ltime.year = 2011;
    ltime.mon  = 10;
    ltime.day  = 31;
    ltime.hour = 8;
    ltime.min  = 40;
    ltime.sec  = 0;
    sysSetLocalTime(ltime);

    Draw_Init();
    font_y = g_Font_Height;

    i= 0;
    while (1)
    {
        no=i%26;
        dispno = i % 3;
        if (dispno == 0 )
            dispcolor = COLOR_RGB16_RED;
        else if ( dispno == 1 )
            dispcolor = COLOR_RGB16_GREEN;
        else
            dispcolor = COLOR_RGB16_BLUE;
        DemoFont_ChangeFontColor(&s_sDemo_Font, dispcolor);
        for (j= 0; j<8; j++)
        {
            DemoFont_PaintA(&s_sDemo_Font,
                            font_x,
                            font_y+ j*g_Font_Height,
                            pstrDisp[no]);
        }
        wait_ticks = sysGetTicks(TIMER0);
        while( 1 )
        {
            if((sysGetTicks(TIMER0) - wait_ticks) > 200)
                break;
        }
        DemoFont_ChangeFontColor(&s_sDemo_Font, COLOR_RGB16_BLACK);
        for (j= 0; j<8; j++)
        {
            DemoFont_PaintA(&s_sDemo_Font,
                            font_x,
                            font_y+ j*g_Font_Height,
                            pstrDisp[no]);
        }
        i++;
    }

}
