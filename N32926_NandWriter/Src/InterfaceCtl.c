/**************************************************************************//**
 * @file     InterfaceCtl.c
 * @brief    Source code for NandWriter interface control.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "W55FA92_GPIO.h"
#include "writer.h"
#include "InterfaceCtl.h"

#define TIMER1_TICKS_PER_SECOND     (100)

#if 1
    #define dbgprintf sysprintf
#else
    #define dbgprintf(...)
#endif

void Interface_LED_Y_ISR()
{
    unsigned short val;
    unsigned int ticks;

    if (Ini_Writer.TIMEOUT_SEC > 0)
    {
        /* Power off NAND flash and turn ON LED Red if timeout. */
        ticks = sysGetTicks(TIMER1);
        //sysprintf("--> Interface_LED_Y_ISR(): TIMER1 ticks = %d\n", ticks);
        if (ticks > (TIMER1_TICKS_PER_SECOND * Ini_Writer.TIMEOUT_SEC))
        {
            sysprintf("*** ERROR: NAND Writer timeout (> %d second) error !! Stop !!\n", Ini_Writer.TIMEOUT_SEC);
            Interface_NAND_Power_OFF();
            Interface_LED_Y_Stop();
            Interface_LED_R_ON();   // NAND Writer FAIL
            while(1);
        }
    }
    
    /* Toggle PB0 for LED Yellow */
    gpio_readport(GPIO_PORTB, &val);
    //sysprintf("--> Interface_LED_Y_ISR(): val = %d\n", val);
    if (val & BIT0)
        gpio_setportval(GPIO_PORTB, BIT0, 0);
    else
        gpio_setportval(GPIO_PORTB, BIT0, BIT0);
}

void Interface_Init(void)
{
    // PB0 for LED Yellow
    gpio_setportval(GPIO_PORTB, BIT0, LED_OFF << 0);
    gpio_setportdir(GPIO_PORTB, BIT0, BIT0);	/* Set GPB0 output mode */

    // PB1 for LED Green
    gpio_setportval(GPIO_PORTB, BIT1, LED_OFF << 1);
    gpio_setportdir(GPIO_PORTB, BIT1, BIT1);	/* Set GPB1 output mode */

    // PB2 for LED Red
    gpio_setportval(GPIO_PORTB, BIT2, LED_OFF << 2);
    gpio_setportdir(GPIO_PORTB, BIT2, BIT2);	/* Set GPB2 output mode */

    // PB3 for NAND flash power
    gpio_setportpull(GPIO_PORTB, BIT3, 0);      /* Disable GPB3 pull-high */
    gpio_setportval(GPIO_PORTB, BIT3, 1 << 3);  /* Output HIGH to power off */
    gpio_setportdir(GPIO_PORTB, BIT3, BIT3);	/* Set GPB3 output mode */

    // PB5 for Start button input
    gpio_setportpull(GPIO_PORTB, BIT5, 0);      /* Disable GPB5 pull-high */
    gpio_setportdir(GPIO_PORTB, BIT5, 0);       /* Set GPB5 input mode */
}

void Interface_LED_Y_Toggle()
{
    /* Initial TIMER1 to toggle LED Yellow */
    sysSetTimerReferenceClock(TIMER1, sysGetExternalClock());
    sysStartTimer(TIMER1, TIMER1_TICKS_PER_SECOND, PERIODIC_MODE);
    /* call ISR per 0.5 second */
    sysSetTimerEvent(TIMER1, TIMER1_TICKS_PER_SECOND/2, (PVOID)Interface_LED_Y_ISR);
    sysSetLocalInterrupt(ENABLE_IRQ);
}

void Interface_LED_Y_Stop()
{
    sysStopTimer(TIMER1);
    gpio_setportval(GPIO_PORTB, BIT0, LED_OFF << 0);
}

void Interface_LED_G_ON()
{
    gpio_setportval(GPIO_PORTB, BIT1, LED_ON << 1);
}

void Interface_LED_G_OFF()
{
    gpio_setportval(GPIO_PORTB, BIT1, LED_OFF << 1);
}

void Interface_LED_R_ON()
{
    gpio_setportval(GPIO_PORTB, BIT2, LED_ON << 2);
}

void Interface_LED_R_OFF()
{
    gpio_setportval(GPIO_PORTB, BIT2, LED_OFF << 2);
}

void Interface_NAND_Power_ON()
{
    gpio_setportval(GPIO_PORTB, BIT3, 0 << 3);
}

void Interface_NAND_Power_OFF()
{
    gpio_setportval(GPIO_PORTB, BIT3, 1 << 3);

    /* Set PD5/PD6/PD7/PD8/PE9 to GPIO input mode and disable internal pull-high */
    /*      to make sure NAND flash has no power. */
    outpw(REG_GPDFUN0, (inpw(REG_GPDFUN0) & (~0xFFF00000)) | 0x00000000);   // Set GPD5/6/7 to GPIO mode
    outpw(REG_GPDFUN1, (inpw(REG_GPDFUN1) & (~0x0000000F)) | 0x00000000);   // Set GPD8 to GPIO mode
    outpw(REG_GPEFUN1, (inpw(REG_GPEFUN1) & (~0x000000F0)) | 0x00000000);   // set GPE9 to GPIO mode

    gpio_setportpull(GPIO_PORTD, BIT5, 0);      /* Disable GPD5 pull-high */
    gpio_setportdir (GPIO_PORTD, BIT5, 0);      /* Set GPD5 input mode */
    gpio_setportpull(GPIO_PORTD, BIT6, 0);      /* Disable GPD6 pull-high */
    gpio_setportdir (GPIO_PORTD, BIT6, 0);      /* Set GPD6 input mode */
    gpio_setportpull(GPIO_PORTD, BIT7, 0);      /* Disable GPD7 pull-high */
    gpio_setportdir (GPIO_PORTD, BIT7, 0);      /* Set GPD7 input mode */
    gpio_setportpull(GPIO_PORTD, BIT8, 0);      /* Disable GPD8 pull-high */
    gpio_setportdir (GPIO_PORTD, BIT8, 0);      /* Set GPD8 input mode */
    gpio_setportpull(GPIO_PORTE, BIT9, 0);      /* Disable GPE9 pull-high */
    gpio_setportdir (GPIO_PORTE, BIT9, 0);      /* Set GPE9 input mode */
}

void Interface_Wait_Start()
{
#if 1
    /* Don't support START button */
#else
    unsigned short val;

    /* Wait PB5 become LOW to start NAND Writer */
    sysprintf("Press START button to start NAND Writer ...\n");
    do
    {
        gpio_readport(GPIO_PORTB, &val);
    } while (val & BIT5);
#endif
}

