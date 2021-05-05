/**************************************************************************//**
 * @file     main.c
 * @brief    The main file for SIC/SD demo code
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "wbio.h"
#include "wblib.h"
#include "wbtypes.h"

#include "W55FA92_reg.h"
#include "W55FA92_SIC.h"

/*-----------------------------------------------------------------------------
 * For system configuration
 *---------------------------------------------------------------------------*/
// Define DBG_PRINTF to sysprintf to show more information about testing
#define DBG_PRINTF    sysprintf
//#define DBG_PRINTF(...)

VOID fmiSD_Show_info(int sdport);

/*-----------------------------------------------------------------------------
 * For global variables
 *---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
 * Function of Card detect for card insert
 *---------------------------------------------------------------------------*/
void func_card_insert()
{
    UINT32 result;
    sysprintf("--- func_card_insert(): card inserted on SD port 0 ---\n\n");
    result = sicSdOpen0();
    if (result < FMI_ERR_ID)
    {
        sysprintf("    Detect card on port %d.\n", 0);
        fmiSD_Show_info(0);
        sysprintf("\n");
    }
    else if (result == FMI_NO_SD_CARD)
    {
        sysprintf("WARNING: Don't detect card on port %d !\n", 0);
    }
    else
    {
        sysprintf("WARNING: Fail to initial SD/MMC card %d, result = 0x%x !\n", 0, result);
    }
    return;
}


/*-----------------------------------------------------------------------------
 * Function of Card detect for card remove
 *---------------------------------------------------------------------------*/
void func_card_remove()
{
    sysprintf("--- func_card_remove: card removed on SD port 0 ---\n\n");
    sicSdClose0();
    return;
}


/*-----------------------------------------------------------------------------
 * Initial UART.
 *---------------------------------------------------------------------------*/
void init_UART()
{
    UINT32 u32ExtFreq;
    WB_UART_T uart;

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
}


/*-----------------------------------------------------------------------------
 * Initial Timer0 interrupt for system tick.
 *---------------------------------------------------------------------------*/
void init_timer()
{
    sysSetTimerReferenceClock(TIMER0, sysGetExternalClock());   // External Crystal
    sysStartTimer(TIMER0, 100, PERIODIC_MODE);                  // 100 ticks/per sec ==> 1tick/10ms
    sysSetLocalInterrupt(ENABLE_IRQ);
}


/*-----------------------------------------------------------------------------*/

int main(void)
{
    UINT32 card_current_status = FALSE, card_pre_status;

    init_UART();

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

    init_timer();

    sysprintf("\n=====> W55FA92 Non-OS SIC/SD Driver Sample Code [tick %d] <=====\n", sysGetTicks(0));

    //--- Initial system clock
    sicIoctl(SIC_SET_CLOCK, sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock())/1000, 0, 0);    // clock from PLL

    //--- Enable AHB clock for SIC/SD/NAND, interrupt ISR, DMA, and FMI engineer
    sicOpen();

    //--- Enable SD card detection feature
    sysprintf("SD0 card detect feature is DISABLED by default.\n");
    sysprintf("The card status will always be INSERTED if card detect feature disabled.\n");
    sysprintf("Now, enable SD0 card detect feature !\n");
    sysprintf("\n");
    // Second parameter: TRUE to enable, FALSE to disable card detection feature.
    sicIoctl(SIC_SET_CARD_DETECT, TRUE, 0, 0);  // MUST call sicIoctl() BEFORE sicSdOpen0()

    //--- Initial SD card on port 0
    sicSdOpen0();

    sysprintf("Please insert or remove SD card to trigger SD card detect interrupt ...\n");

    card_pre_status = 0xFF;
    while(1)
    {
        //--- Got SD card insert/remove status
        sicIoctl(SIC_GET_CARD_STATUS, (INT32)&card_current_status, 0, 0);

        if (card_current_status != card_pre_status)
        {
            sysprintf("SD0 card status change to %s\n", card_current_status ? "INSERTED" : "REMOVED");
            if (card_current_status == TRUE)
                func_card_insert();
            else
                func_card_remove();

            card_pre_status = card_current_status;
        }
    }
}
