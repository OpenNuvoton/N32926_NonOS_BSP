/***************************************************************************
 * Copyright (c) 2013 Nuvoton Technology. All rights reserved.
 *
 * FILENAME
 *     main.c
 * DESCRIPTION
 *     The main file for SDIO demo code.
 **************************************************************************/
 #include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "wbio.h"
#include "wblib.h"
#include "wbtypes.h"

#include "w55fa92_reg.h"
#include "w55fa92_sdio.h"
#include "nvtfat.h"
#include "sdio_fmi.h"

/*-----------------------------------------------------------------------------
 * For system configuration
 *---------------------------------------------------------------------------*/

// Define DBG_PRINTF to sysprintf to show more information about testing
#define DBG_PRINTF    sysprintf
//#define DBG_PRINTF(...)

#define OK      TRUE
#define FAIL    FALSE


/*-----------------------------------------------------------------------------
 * For global variables
 *---------------------------------------------------------------------------*/
#define SECTOR_SIZE         512
#define MAX_SECTOR_COUNT    512
#define BUF_SIZE        (SECTOR_SIZE * MAX_SECTOR_COUNT)
__align (32) UINT8 g_ram0[BUF_SIZE];
__align (32) UINT8 g_ram1[BUF_SIZE];

extern FMI_SDIO_INFO_T *pSDIO0, *pSDIO1; // define in SDIO driver


/*-----------------------------------------------------------------------------
 * show data by hex format
 *---------------------------------------------------------------------------*/
void show_hex_data(unsigned char *ptr, unsigned int length)
{
    unsigned int line_len = 8;
    unsigned int i;

    for (i=0; i<length; i++)
    {
        if (i % line_len == 0)
            DBG_PRINTF("        ");
        DBG_PRINTF("0x%02x ", *(ptr+i));
        if (i % line_len == line_len-1)
            DBG_PRINTF("\n");
    }
    if (i % line_len != 0)
        DBG_PRINTF("\n");
}


/*-----------------------------------------------------------------------------
 * ISR of Card detect interrupt for card insert
 *---------------------------------------------------------------------------*/
void isr_card_insert()
{
    UINT32 result;
    sysprintf("--- ISR: card inserted on SDIO port 0 ---\n\n");
    result = sdioSdOpen0();
    if (result < FMI_ERR_ID)
    {
        sysprintf("    Detect card on port %d.\n", 0);
        fmiSDIO_Show_info(0);
    }
    else if (result == FMISDIO_NO_SD_CARD)
    {
        sysprintf("WARNING: Don't detect card on port %d !\n", 0);
    }
    else
    {
        sysprintf("WARNING: Fail to initial SDIO card %d, result = 0x%x !\n", 0, result);
    }
    return;
}



/*-----------------------------------------------------------------------------
 * ISR of Card detect interrupt for card insert
 *---------------------------------------------------------------------------*/
void isr_card1_insert()
{
    UINT32 result;
    sysprintf("--- ISR: card inserted on SDIO port 1 ---\n\n");
    result = sdioSdOpen1();
    if (result < FMI_ERR_ID)
    {
        sysprintf("    Detect card on port %d.\n", 0);
        fmiSDIO_Show_info(1);
    }
    else if (result == FMISDIO_NO_SD_CARD)
    {
        sysprintf("WARNING: Don't detect card on port %d !\n", 1);
    }
    else
    {
        sysprintf("WARNING: Fail to initial SDIO card %d, result = 0x%x !\n", 1, result);
    }
    return;
}


/*-----------------------------------------------------------------------------
 * ISR of Card detect interrupt for card remove
 *---------------------------------------------------------------------------*/
void isr_card_remove()
{
    sysprintf("--- ISR: card removed on SDIO port 0 ---\n\n");
    sdioSdClose0();
    return;
}


/*-----------------------------------------------------------------------------
 * ISR of Card detect interrupt for card remove
 *---------------------------------------------------------------------------*/
void isr_card1_remove()
{
    sysprintf("--- ISR: card removed on SDIO port 1 ---\n\n");
    sdioSdClose1();
    return;
}


/*-----------------------------------------------------------------------------
 * To do test for SD card access. Write, read, and compare data on random sectors.
 *---------------------------------------------------------------------------*/
unsigned int sd_access_test(int sdport)
{
    UINT32 ii, sectorIndex;
    UINT32 result;
    UINT32 u32SecCnt;
    UINT8  *ptr_g_ram0, *ptr_g_ram1;
    SDIO_DISK_DATA_T info;

    ptr_g_ram0 = (UINT8 *)((UINT32)g_ram0 | 0x80000000);    // non-cache
    ptr_g_ram1 = (UINT8 *)((UINT32)g_ram1 | 0x80000000);    // non-cache

    for(ii=0; ii<BUF_SIZE; ii++)
    {
        ptr_g_ram0[ii] = rand() & 0xFF;
    }

#if 1
    // get information about SD card
    switch (sdport)
    {
        case 0: fmiGet_SDIO_info(pSDIO0, &info);    break;
        case 1: fmiGet_SDIO_info(pSDIO1, &info);    break;
        default:
            sysprintf("ERROR: sd_access_test(): invalid SDIO port %d !!\n", sdport);
            return FAIL;
    }
#endif

//    while(1)
    {
        sectorIndex = rand() % info.totalSectorN;
        u32SecCnt   = rand() % MAX_SECTOR_COUNT;
        if (u32SecCnt == 0)
            u32SecCnt = 1;
        if (sectorIndex + u32SecCnt > info.totalSectorN)
            sectorIndex = info.totalSectorN - u32SecCnt;

        switch (sdport)
        {
            case 0: result = sdioSdWrite0(sectorIndex, u32SecCnt, (UINT32)ptr_g_ram0);   break;
            case 1: result = sdioSdWrite1(sectorIndex, u32SecCnt, (UINT32)ptr_g_ram0);   break;
        }
        DBG_PRINTF("    Write g_ram0 to SD card, result = 0x%x\n", result);
        show_hex_data(ptr_g_ram0, 16);

        memset(ptr_g_ram1, 0x5a, u32SecCnt * SECTOR_SIZE);
        switch (sdport)
        {
            case 0: result = sdioSdRead0(sectorIndex, u32SecCnt, (UINT32)ptr_g_ram1);    break;
            case 1: result = sdioSdRead1(sectorIndex, u32SecCnt, (UINT32)ptr_g_ram1);    break;
        }
        DBG_PRINTF("    Read g_ram1 to SD card, result = 0x%x\n", result);
        show_hex_data(ptr_g_ram1, 16);

        if(memcmp(ptr_g_ram0, ptr_g_ram1, u32SecCnt * SECTOR_SIZE) == 0)
        {
            result = OK;
            sysprintf("    Data compare OK at sector %d, sector count = %d\n", sectorIndex, u32SecCnt);
        }
        else
        {
            result = FAIL;
            sysprintf("    ERROR: Data compare ERROR at sector %d, sector count = %d\n", sectorIndex, u32SecCnt);
        }
    }
    return result;
}


/*-----------------------------------------------------------------------------
 * Initial UART1.
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
    int result, i, pass, fail;
    int target_port = 1;

    init_UART();
    init_timer();

    sysprintf("\n=====> W55FA92 Non-OS SDIO Driver Sampe Code [tick %d] <=====\n", sysGetTicks(0));

    //sysprintf("REG_CLKDIV4 = 0x%08X\n", inp32(REG_CLKDIV4));    // default HCLK234 should be 0
    //outp32(REG_CLKDIV4, 0x00000310);                            // set HCLK234 to 1
    //sysprintf("REG_CLKDIV4 = 0x%08X\n", inp32(REG_CLKDIV4));

    srand(time(NULL));

    //--- Initial system clock
#ifdef OPT_FPGA_DEBUG
    sdioIoctl(SDIO_SET_CLOCK, 27000, 0, 0);               // clock from FPGA clock in
#else
    sdioIoctl(SDIO_SET_CLOCK, sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock())/1000, 0, 0);    // clock from PLL
#endif

    //--- Enable AHB clock for SDIO, interrupt ISR, DMA, and FMI engineer
    sdioOpen();

    //--- Initial callback function for card detection interrupt
    //sdioIoctl(SDIO_SET_CALLBACK, FMI_SDIO_CARD,  (INT32)isr_card_remove,  (INT32)isr_card_insert);
    //sdioIoctl(SDIO_SET_CALLBACK, FMI_SDIO1_CARD, (INT32)isr_card1_remove, (INT32)isr_card1_insert);

    //--- Initial SD card on target SDIO port
    switch (target_port)
    {
        case 0: result = sdioSdOpen0();    break;
        case 1: result = sdioSdOpen1();    break;
    }
    if (result < 0)
    {
        sysprintf("ERROR: Open SD card on SDIO port %d fail. Return = 0x%x.\n", target_port, result);
    }
    else
    {
        sysprintf("Detect SD card on SDIO port %d with %d sectors.\n", target_port, result);

        DBG_PRINTF("Do basic SDIO port %d access test ...\n", target_port);

        i = 0;
        pass = 0;
        fail = 0;
        while(i<10)
        {
            i++;
            DBG_PRINTF("=== Loop %d ...\n", i);
            result = sd_access_test(target_port);
            if (result == OK)
            {
                DBG_PRINTF("SDIO access test is SUCCESSFUL!!!\n");
                pass++;
            }
            else
            {
                DBG_PRINTF("SDIO access test is FAIL!!!\n");
                fail++;
            }
            DBG_PRINTF("=== Pass %d, Fail %d\n", pass, fail);
        }
        switch (target_port)
        {
            case 0: sdioSdClose0();    break;
            case 1: sdioSdClose1();    break;
        }
    }

    sysprintf("\n===== THE END ===== [tick %d]\n", sysGetTicks(0));
    return OK;
}
