/**************************************************************************//**
 * @file     ROT_main.c
 * @brief    Demonstrate rotation image on panel
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "wblib.h"
#include "W55FA92_ROT.h"
#include "W55FA92_SIC.h"
#include "NVTFAT.h"
#include "ROT_demo.h"

#if defined(__GNUC__)
UINT8 u8FrameBuffer0[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2] __attribute__((aligned (32)));
UINT8 u8FrameBuffer1[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2] __attribute__((aligned (32)));
#else
__align(32) UINT8 u8FrameBuffer0[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2];
__align(32) UINT8 u8FrameBuffer1[OPT_LCM_WIDTH*OPT_LCM_HEIGHT*2];
#endif

volatile UINT32 bIsBuffer0Dirty=0;  //0 Means ready for ROT use
volatile UINT32 bIsBuffer1Dirty=1;  //1 means for TV show
volatile UINT32 VpostUseBuf = 1;        //default 1 due to bIsBuffer1Dirty=1.

static BOOL bIsRotDoneFlag =0;
void rotDoneHandler(void)
{
    bIsRotDoneFlag =1;
}
void rotClearDoneFlag(void)
{
    bIsRotDoneFlag = 0;
}
BOOL rotGetDoneFlag(void)
{
    return bIsRotDoneFlag;
}
static BOOL bIsRotAbortFlag =0;
void rotAbortHandler(void)
{
    bIsRotAbortFlag =1;
}
void rotClearAbortFlag(void)
{
    bIsRotAbortFlag = 0;
}
BOOL rotGetAbortFlag(void)
{
    return bIsRotAbortFlag;
}
static BOOL bIsRotOverFlag =0;
void rotOverHandler(void)
{
    bIsRotOverFlag =1;
}
void rotClearOverFlag(void)
{
    bIsRotOverFlag = 0;
}
BOOL rotGetOverFlag(void)
{
    return bIsRotOverFlag;
}

int main(void)
{
    WB_UART_T uart;

    UINT32 u32ExtFreq, u32PllOutHz;

#ifdef __ENABLE_CACHE__
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
#else
    sysDisableCache();
#endif
	
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
	
    InitVPOST(u8FrameBuffer0);
    u32ExtFreq = sysGetExternalClock();
    u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);

    sysSetTimerReferenceClock (TIMER0, u32ExtFreq);
    sysStartTimer(TIMER0, 100, PERIODIC_MODE);

    sysUartPort(1);
    uart.uiFreq = u32ExtFreq;
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    uart.uart_no = WB_UART_1;
    sysInitializeUART(&uart);
    sysSetLocalInterrupt(ENABLE_FIQ_IRQ);

    fsInitFileSystem();
    sicIoctl(SIC_SET_CLOCK, u32PllOutHz/1000, 0, 0);
    sicOpen();
    if (sicSdOpen0()<=0)
    {
        sysprintf("Error in initializing SD card !! \n");
        while(1);
    }
    fsAssignDriveNumber('C', DISK_TYPE_SD_MMC, 0, 1);

    //Create_File_Test();
    DBG_PRINTF("PLL out frequency %d Khz\n", u32PllOutHz);
    DBG_PRINTF("Please prepare source pattern in SD card 0. Detail refer readme.txt \n");
    DBG_PRINTF("Start Conversion\n");
    while(1)
        Emu_DestinationLineOffsetFineTune(u8FrameBuffer0, u8FrameBuffer1);
}


