/**************************************************************************//**
 * @file     main_vpe.c
 * @brief    Demonstrate VPE functions
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "wblib.h"
#include "W55FA92_VPE.h"
#include "W55FA92_VPOST.h"

#define LCD_WIDTH   320
#define LCD_HEIGHT  240
#define VPE_SOURCE_PATTERN_WIDTH    468
#define VPE_SOURCE_PATTERN_HEIGHT   88

extern unsigned char g_au8VpeSrcPattern[];

#if defined (__GNUC__)
static UINT8 s_VpostFrameBufferPool[LCD_WIDTH*LCD_HEIGHT*4] __attribute__((aligned (256)));
#else
static __align(256) UINT8 s_VpostFrameBufferPool[LCD_WIDTH*LCD_HEIGHT*4];
#endif

void init(void)
{
    WB_UART_T uart;
    UINT32 u32ExtFreq;
    LCDFORMATEX lcdFormat;

    /* Init UART */
    u32ExtFreq = sysGetExternalClock();
    sysUartPort(1);
    uart.uiFreq = u32ExtFreq;   //use APB clock
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    uart.uart_no = WB_UART_1;
    sysInitializeUART(&uart);

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
#endif

    memset(s_VpostFrameBufferPool, 0x00, (LCD_WIDTH * LCD_HEIGHT * 4));
    lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGBx888;
    lcdFormat.nScreenWidth = LCD_WIDTH;
    lcdFormat.nScreenHeight = LCD_HEIGHT;
    vpostLCMInit(&lcdFormat, (UINT32*)s_VpostFrameBufferPool);

    sysSetLocalInterrupt(ENABLE_FIQ_IRQ);
}

void EnableCache(void)
{
    if (! sysGetCacheState ())
    {
        sysInvalidCache ();
///     sysEnableCache (CACHE_WRITE_THROUGH);
        sysEnableCache (CACHE_WRITE_BACK);
        sysFlushCache (I_D_CACHE);
    }
}

void vpe_demo(void)
{
    INT32 i32OpNormal, i, j, k;
    UINT32 u32Y, u32U, u32V;

    vpeOpen();

    u32Y = ((UINT32)g_au8VpeSrcPattern) | BIT31;
    u32U = u32Y+VPE_SOURCE_PATTERN_WIDTH*VPE_SOURCE_PATTERN_HEIGHT;     /* Planar YUV420 */
    u32V = u32U+VPE_SOURCE_PATTERN_WIDTH*VPE_SOURCE_PATTERN_HEIGHT/4;

    while(1)
    {
        for (i32OpNormal = 0; i32OpNormal < 6; i32OpNormal++)
        {
            if ((i32OpNormal == 1) || (i32OpNormal == 2))
                continue;

            if (j == 0)
            {
                j = 1;
                k = LCD_WIDTH;
            }
            else
            {
                j = 0;
                k = 200;
            }

            vpeIoctl(VPE_IOCTL_HOST_OP,
                     VPE_HOST_FRAME_TURBO,
                     (E_VPE_CMD_OP)i32OpNormal,
                     0);

            vpeIoctl(VPE_IOCTL_SET_SRCBUF_ADDR,
                     u32Y,
                     u32U,
                     u32V);

            vpeIoctl(VPE_IOCTL_SET_DSTBUF_ADDR,
                     (((UINT32)s_VpostFrameBufferPool) | BIT31),
                     0,
                     0);

            vpeIoctl(VPE_IOCTL_SET_FMT,
                     VPE_SRC_PLANAR_YUV420, /* Src Format */
                     VPE_DST_PACKET_RGB888, /* Dst Format */
                     0);

            vpeIoctl(VPE_IOCTL_SET_SRC_OFFSET,
                     (UINT32)0, /* Src Left offset */
                     (UINT32)0, /* Src right offset */
                     0);

            vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
                     (UINT32)((LCD_WIDTH - k) / 2),   /* Dst Left offset */
                     (UINT32)((LCD_WIDTH - k) / 2),   /* Dst right offset */
                     0);

            vpeIoctl(VPE_IOCTL_SET_SRC_DIMENSION,
                     VPE_SOURCE_PATTERN_WIDTH,
                     VPE_SOURCE_PATTERN_HEIGHT,
                     0);

            vpeIoctl(VPE_IOCTL_SET_DST_DIMENSION,   /* Scale Down */
                     k,
                     230,
                     0);

            vpeIoctl(VPE_IOCTL_SET_FILTER,
                     VPE_SCALE_BILINEAR,
                     0,
                     0);

            vpeIoctl(VPE_IOCTL_TRIGGER,
                     0,
                     0,
                     0);
            do
            {
                ERRCODE errcode;
                //TRUE==>Not complete, FALSE==>Complete
                errcode = vpeIoctl(VPE_IOCTL_CHECK_TRIGGER,
                                   0,
                                   0,
                                   0);
                if(errcode==0)
                    break;
            }
            while(1);

            for (i = 0; i < 0x800000; i ++);
        }
    }
}

INT32 main(void)
{
    EnableCache();
    init();

    sysprintf("================================================================\n");
    sysprintf("						VPE demo\n");
    sysprintf("================================================================\n");

    vpe_demo();

    return 0;
}
