/***************************************************************************
 * Copyright (c) 2013 Nuvoton Technology. All rights reserved.
 *
 * FILENAME
 *     main.c
 * DESCRIPTION
 *     The main file for SIC/NAND demo code.
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "wbio.h"
#include "wblib.h"
#include "wbtypes.h"

#include "w55fa92_reg.h"
#include "w55fa92_sic.h"
#include "fmi.h"

/*-----------------------------------------------------------------------------
 * For system configuration
 *---------------------------------------------------------------------------*/
// Define DBG_PRINTF to sysprintf to show more information about testing.
#define DBG_PRINTF    sysprintf
//#define DBG_PRINTF(...)

#define OK      TRUE
#define FAIL    FALSE


/*-----------------------------------------------------------------------------
 * For global variables
 *---------------------------------------------------------------------------*/
NDISK_T *ptMassNDisk;
NDISK_T MassNDisk;

// Define number and size for data buffer
#define SECTOR_SIZE     512
#define BUF_SIZE    (SECTOR_SIZE*16*16*2)
__align (32) UINT8 g_ram0[BUF_SIZE];
__align (32) UINT8 g_ram1[BUF_SIZE];


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
 * To do test for NAND access. Write, read, and compare data.
 *---------------------------------------------------------------------------*/
int nand_access_test(int nand_port)
{
    int ii;
    UINT32  result;
    UINT32  block, page;
    UINT8   *ptr_g_ram0, *ptr_g_ram1;

#ifdef CACHE_ON
    ptr_g_ram0 = g_ram0;
    ptr_g_ram1 = g_ram1;
#else
    ptr_g_ram0 = (UINT8 *)((UINT32)g_ram0 | 0x80000000);    // non-cache
    ptr_g_ram1 = (UINT8 *)((UINT32)g_ram1 | 0x80000000);    // non-cache
#endif

    //--- initial random data, select random block index and page index
    for(ii=0; ii<ptMassNDisk->nPageSize; ii++)
    {
        ptr_g_ram0[ii] = rand() & 0xFF;
    }
    // select random block except first 4 blocks.
    //      First 4 blocks is booting block and control by IBR. Don't touch it.
    block = (rand() % (ptMassNDisk->nBlockPerZone - 4)) + 4;
    page  = rand() % ptMassNDisk->nPagePerBlock;

    //--- do write and read back test
    switch (nand_port)
    {
        case 0: result = nand_block_erase0(block);  break;
        case 1: result = nand_block_erase1(block);  break;
    }
    if (result != 0)
    {
        // Erase block fail. Could be bad block. Ignore it.
        DBG_PRINTF("    Ignore since nand_block_erase0() fail, block = %d, page = %d, result = 0x%x\n", block, page, result);
        return OK;
    }

    switch (nand_port)
    {
        case 0: result = nandpwrite0(block, page, ptr_g_ram0);  break;
        case 1: result = nandpwrite1(block, page, ptr_g_ram0);  break;
    }
    DBG_PRINTF("    Write g_ram0 to NAND, result = 0x%x\n", result);
    show_hex_data(ptr_g_ram0, 16);

    memset(ptr_g_ram1, 0x5a, BUF_SIZE);
    switch (nand_port)
    {
        case 0: result = nandpread0(block, page, ptr_g_ram1);   break;
        case 1: result = nandpread1(block, page, ptr_g_ram1);   break;
    }
    DBG_PRINTF("    Read NAND to g_ram1,  result = 0x%x\n", result);
    show_hex_data(ptr_g_ram1, 16);

    //--- compare data
    if(memcmp(ptr_g_ram0, ptr_g_ram1, ptMassNDisk->nPageSize) == 0)
    {
        DBG_PRINTF("    Data compare OK at block %d page %d\n", block, page);
        return OK;
    }
    else
    {
        DBG_PRINTF("    ERROR: Data compare ERROR at block %d page %d\n", block, page);
        return FAIL;
    }
}


/*-----------------------------------------------------------------------------
 * Initial UART0.
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
    sysSetTimerReferenceClock(TIMER0, sysGetExternalClock()/1000);   // External Crystal
    sysStartTimer(TIMER0, 100, PERIODIC_MODE);                  // 100 ticks/per sec ==> 1tick/10ms
    sysSetLocalInterrupt(ENABLE_IRQ);
}


/*-----------------------------------------------------------------------------*/
int main(void)
{
    int result, i, pass, fail;
    int target_port = 0;

    init_UART();
    init_timer();

    sysprintf("\n=====> W55FA92 Non-OS SIC/NAND Driver Sample Code [tick %d] <=====\n", sysGetTicks(0));

    //sysprintf("REG_CLKDIV4 = 0x%08X\n", inp32(REG_CLKDIV4));    // default HCLK234 should be 0
    //outp32(REG_CLKDIV4, 0x00000310);                            // set HCLK234 to 1
    //sysprintf("REG_CLKDIV4 = 0x%08X\n", inp32(REG_CLKDIV4));

    //--- enable cache feature
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);

    srand(time(NULL));

    //--- initial SIC/NAND driver for target_port
    sicOpen();
    ptMassNDisk = (NDISK_T*)&MassNDisk;

    switch (target_port)
    {
        case 0: result = nandInit0((NDISK_T *)ptMassNDisk);   break;
        case 1: result = nandInit1((NDISK_T *)ptMassNDisk);   break;
        default: sysprintf("ERRROR: Wrong NAND port number %d\n", target_port); break;
    }
    if (result)
    {
        sysprintf("ERROR: NAND port %d initial fail !!\n", target_port);
    }
    else
    {
        //--- do basic NAND access test
        DBG_PRINTF("Do basic NAND access test on port %d ...\n", target_port);
        i = 0;
        pass = 0;
        fail = 0;
        while(1)
        {
            i++;
            DBG_PRINTF("=== Loop %d ...\n", i);
            result = nand_access_test(target_port);
            if (result == OK)
            {
                DBG_PRINTF("NAND access test is SUCCESSFUL!!!\n");
                pass++;
            }
            else
            {
                DBG_PRINTF("NAND access test is FAIL!!!\n");
                fail++;
            }
            DBG_PRINTF("=== Pass %d, Fail %d\n", pass, fail);
        }
    }

    sysprintf("\n===== THE END ===== [tick %d]\n", sysGetTicks(0));
    return OK;
}
