/**************************************************************************//**
 * @file     demo.c
 * @brief    Demostrate keypad, touch pannel, voltage detection and wake up system by touch event
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include "wblib.h"
#include "W55FA92_ADC.h"
#include "demo.h"

/*
    1. There are max 3 AIN. They are SA-AHS(Channel 1), SA-AIN2(Channel 2) and SA-SEN(channel 3)
    2. SA-SEN is used in 5 wire touch panel. So there are 2 chanel for AIN for 5-wire touch panel
*/
int main(VOID)
{
    //unsigned int volatile i;
    WB_UART_T uart;
    UINT32 u32Item, u32ExtFreq;

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
    DBG_PRINTF("DRAM frequency %d Hz\n", sysGetDramClock());
    DBG_PRINTF("SYS frequency %d Hz\n", sysGetSystemClock());
		
    DrvADC_Open();

    do
    {
        DBG_PRINTF("================================================================\n");
        DBG_PRINTF("						Touch library demo code									  	\n");
        DBG_PRINTF(" [1] KeyPad demo 																\n");
        DBG_PRINTF(" [2] Voltage detection demo 														\n");
        DBG_PRINTF(" [3] Touch panel demo 															\n");
        DBG_PRINTF(" [4] Integration demo 																\n");
        DBG_PRINTF(" [5] Touch wakeup																\n");
        DBG_PRINTF(" [6] Key wakeup		 															\n");
        DBG_PRINTF(" [7] Pen up/down detect														\n");
        DBG_PRINTF("================================================================\n");

        u32Item = sysGetChar();
        //u32Item = '3';
        switch(u32Item)
        {

        case '1':
            DBG_PRINTF("Please input test channel AIN1, AIN2 or AIN3 (DEV:Default AIN2, HMI: Change to GPIO IP)\n");
            u32Item = sysGetChar();
            if( (u32Item>'0')||(u32Item<'4') )
                KeyPad((u32Item-0x30));
            else
                DBG_PRINTF("Input Wrong Channel\n");
            break;
        case '2':
            DBG_PRINTF("Please input test channel AIN1, AIN2 or AIN3 (DEV:Default AIN1, HMI: Don't Support)\n");
            u32Item = sysGetChar();
            if( (u32Item>'0')||(u32Item<'4') )
                VoltageDetection((u32Item-0x30));
            else
                DBG_PRINTF("Input Wrong Channel\n");
            break;
        case '3':
            Raw_TouchPanel();
            break;
        case '4':
            Integration();
            break;
        case '5':
            Raw_TouchPanel();
            DrvADC_Wakeup(eADC_WAKEUP_TOUCH);
            sysPowerDown(WE_ADC);
            break;
        case '6':
            KeyPad(2);
            DrvADC_Wakeup(eADC_WAKEUP_KEY);
            sysPowerDown(WE_ADC);
            break;
        case '7':
            Polling_Processed_TouchPanel();
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
