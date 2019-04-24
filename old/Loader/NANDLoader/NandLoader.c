/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2013 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
#include <string.h>
#include "wblib.h"
#include "turbowriter.h"
#include "w55fa92_vpost.h"
#include "w55fa92_reg.h"

// define DATE CODE and show it when running to make maintaining easy.
#define DATE_CODE   "20181017"

/* global variable */
typedef struct nand_info
{
    unsigned int startBlock;
    unsigned int endBlock;
    unsigned int fileLen;
    unsigned int executeAddr;
} NVT_NAND_INFO_T;

extern ERRCODE DrvSPU_Open(void);
extern VOID spuDacPLLEnable (void);
extern VOID spuDacPrechargeEnable (void);

#ifdef __USER_DEFINE_FUNC
    extern VOID user_define_func(void);
#endif

/*-----------------------------------------------------------------------------
 * For RTC feature
 *---------------------------------------------------------------------------*/
#define RTC_DELAY       500000

VOID RTC_Check(void)
{
    UINT32 volatile i;

    i =0;
    while((inp32(REG_FLAG) & RTC_REG_FLAG) != RTC_REG_FLAG)
    {
        i++;
        if(i > RTC_DELAY)
        {
            //sysprintf("Time out\n");
            break;
        }
    }
}


// 2014/4/11, NVTLoader will write a 16 bytes signature pattern in image file offset
//      between 0x40 to 0x9F. (0x55AA55AA, 0xAA55AA55, 0x4F56554E, 0x2E4E4F54)
// NandLoader believe the image is NVTLoader if it found one pattern within this range.
INT isNVTLoader(NVT_NAND_INFO_T *image)
{
    int i;

    for (i=0x40; i<=0x90; i+=0x10)
    {
        //sysprintf("--> check NVTLoader signature = 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
        //    *(unsigned int *)(image->executeAddr+i),   *(unsigned int *)(image->executeAddr+i+4),
        //    *(unsigned int *)(image->executeAddr+i+8), *(unsigned int *)(image->executeAddr+i+12));
        if ((*(unsigned int *)(image->executeAddr+i)    == 0x55AA55AA) &&
            (*(unsigned int *)(image->executeAddr+i+4)  == 0xAA55AA55) &&
            (*(unsigned int *)(image->executeAddr+i+8)  == 0x4F56554E) &&
            (*(unsigned int *)(image->executeAddr+i+12) == 0x2E4E4F54))
        {
            return 1;   // image is NVTLoader
        }
    }
    return 0;   // image is not NVTLoader
}


INT MoveData(NVT_NAND_INFO_T *image, BOOL IsExecute)
{
    unsigned int page_count, block_count, curBlock, addr;
    int volatile i, j;
    void    (*fw_func)(void);

    sysprintf("Load file length %d, execute address 0x%x\n", image->fileLen, image->executeAddr);

    page_count = image->fileLen / pSM0->nPageSize;
    if ((image->fileLen % pSM0->nPageSize) != 0)
        page_count++;

    block_count = page_count / pSM0->uPagePerBlock;

    curBlock = image->startBlock;
    addr = image->executeAddr;
    j=0;
    while(1)
    {
        if (j >= block_count)
            break;

        if (CheckBadBlockMark(curBlock) == Successful)
        {
            for (i=0; i<pSM0->uPagePerBlock; i++)
            {
                sicSMpread(0, curBlock, i, (UINT8 *)addr);
                addr += pSM0->nPageSize;
            }
            j++;
        }
        curBlock++;
    }

    if ((page_count % pSM0->uPagePerBlock) != 0)
    {
        page_count = page_count - block_count * pSM0->uPagePerBlock;
_read_:
        if (CheckBadBlockMark(curBlock) == Successful)
        {
            for (i=0; i<page_count; i++)
            {
                sicSMpread(0, curBlock, i, (UINT8 *)addr);
                addr += pSM0->nPageSize;
            }
        }
        else
        {
            curBlock++;
            goto _read_;
        }
    }

    if (IsExecute == TRUE)
    {
        // disable NAND control pin used
        outpw(REG_GPDFUN0, inpw(REG_GPDFUN0) & (~0xFFF00000));   // disable NRE/RB0/RB1 pins
        outpw(REG_GPDFUN1, inpw(REG_GPDFUN1) & (~0x0000000F));   // disable NWR pins
        outpw(REG_GPEFUN1, inpw(REG_GPEFUN1) & (~0x000FFFFF));   // disable CS0/ALE/CLE/ND3/CS1 pins

        sysprintf("Nand Boot Loader exit. Jump to execute address 0x%x ...\n", image->executeAddr);

        // 2014/4/11, keep enable cache for NVTLoader only to speed up the system booting.
        if (! isNVTLoader(image))
        {
            // Disable cache. Use assembly code to avoid be optimized by compile option -O2.
            extern void sys_flush_and_clean_dcache(void);
            volatile int temp;

            sys_flush_and_clean_dcache();
            __asm
            {
                /*----- flush I, D cache & write buffer -----*/
                MOV temp, 0x0
                MCR p15, 0, temp, c7, c5, 0 /* flush I cache */
                MCR p15, 0, temp, c7, c6, 0 /* flush D cache */
                MCR p15, 0, temp, c7, c10,4 /* drain write buffer */

                /*----- disable Protection Unit -----*/
                MRC p15, 0, temp, c1, c0, 0     /* read Control register */
                BIC temp, temp, 0x01
                BIC temp, temp, 0x1000
                BIC temp, temp, 0x4
                MCR p15, 0, temp, c1, c0, 0     /* write Control register */
            }
        }

        fw_func = (void(*)(void))(image->executeAddr);
        fw_func();
    }
    return 0;
}


UINT8 image_buffer[8192];
unsigned char *imagebuf;
unsigned int *pImageList;

#define __DDR2__
#define E_CLKSKEW   0x00888800

void initClock(void)
{
    UINT32 u32ExtFreq;
    UINT32 reg_tmp;

    u32ExtFreq = sysGetExternalClock();     // Hz unit
    if(u32ExtFreq==12000000)
    {
        outp32(REG_SDREF, 0x805A);
    }
    else
    {
        outp32(REG_SDREF, 0x80C0);
    }

#ifdef __UPLL_NOT_SET__
    // 2012/3/7 Don't change anything that include
    //      System clock, clock skew, REG_DQSODS and REG_SDTIME.
    //      System clock will follow IBR setting.
    sysprintf("NAND Loader DONOT set anything and follow IBR setting !!\n");
#endif  // __UPLL_NOT_SET__

#ifdef __UPLL_192__
    //--- support UPLL 192MHz, MPLL 288MHz, APLL 432MHz
    outp32(REG_CKDQSDS, E_CLKSKEW);
    #ifdef __DDR2__
        outp32(REG_SDTIME, 0x2A38F726);     // REG_SDTIME for N32926 288MHz SDRAM clock
        outp32(REG_SDMR, 0x00000432);
        outp32(REG_MISC_SSEL, 0x00000155);  // set MISC_SSEL to Reduced Strength to improve EMI
    #endif

    // initial DRAM clock BEFORE inital system clock since we change it from low (216MHz by IBR) to high (288MHz)
    sysSetDramClock(eSYS_MPLL, 288000000, 288000000);   // change from 216MHz (IBR) to 288MHz

    // initial system clock
    sysSetSystemClock(eSYS_UPLL,
                    192000000,      // Specified the APLL/UPLL clock, unit Hz
                    192000000);     // Specified the system clock, unit Hz
    sysSetCPUClock (192000000);     // Unit Hz
    sysSetAPBClock ( 48000000);     // Unit Hz

    // set APLL to 432MHz to support TVout (need 27MHz)
    sysSetPllClock(eSYS_APLL, 432000000);
#endif  // __UPLL_192__

#ifdef __UPLL_240__
    //--- support UPLL 240MHz, MPLL 360MHz, APLL 432MHz
    outp32(REG_CKDQSDS, E_CLKSKEW);
    #ifdef __DDR2__
      #ifdef __N32923__
        outp32(REG_SDTIME, 0x32BEC84A);     // REG_SDTIME for N32923 360MHz SDRAM clock
      #else // for N32926
        outp32(REG_SDTIME, 0x2ABF394A);     // REG_SDTIME for N32926 360MHz SDRAM clock
      #endif
        outp32(REG_SDMR, 0x00000432);
        outp32(REG_MISC_SSEL, 0x00000155);  // set MISC_SSEL to Reduced Strength to improve EMI
    #endif

    // initial DRAM clock BEFORE inital system clock since we change it from low (216MHz by IBR) to high (360MHz)
    sysSetDramClock(eSYS_MPLL, 360000000, 360000000);   // change from 216MHz (IBR) to 360MHz,

    // initial system clock
    sysSetSystemClock(eSYS_UPLL,
                    240000000,      // Specified the APLL/UPLL clock, unit Hz
                    240000000);     // Specified the system clock, unit Hz
    sysSetCPUClock (240000000);     // Unit Hz
    sysSetAPBClock ( 60000000);     // Unit Hz

    // set APLL to 432MHz to support TVout (need 27MHz)
    sysSetPllClock(eSYS_APLL, 432000000);
#endif  // __UPLL_240__

    // always set HCLK234 to 0
    reg_tmp = inp32(REG_CLKDIV4) | CHG_APB;     // MUST set CHG_APB to HIGH when configure CLKDIV4
    outp32(REG_CLKDIV4, reg_tmp & (~HCLK234_N));
}


int main()
{
    NVT_NAND_INFO_T image;
    int volatile count, i;
#ifdef __UPLL_NOT_SET__
    UINT32 u32PllHz, u32SysHz, u32CpuHz, u32Hclk1Hz, u32ApbHz, /*u32APllHz,*/ u32DramHz;
#endif

    /* Clear Boot Code Header in SRAM to avoid booting fail issue */
    outp32(0xFF000000, 0);

    //--- enable cache to speed up booting
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);

    sysprintf("W55FA92 Nand Boot Loader entry (%s).\n", DATE_CODE);

    //--- initial SPU
    DrvSPU_Open();
    spuDacPLLEnable();

    /* PLL clock setting */
    initClock();

    spuDacPrechargeEnable();

    sysprintf("System clock = %dHz\nDRAM clock = %dHz\nREG_SDTIME = 0x%08X\n",
               sysGetSystemClock(), sysGetDramClock(), inp32(REG_SDTIME));

#ifdef __UPLL_NOT_SET__
    //u32APllHz = sysGetPLLOutputHz(eSYS_APLL, sysGetExternalClock());
    u32PllHz = sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock());
    u32SysHz = sysGetSystemClock();
    u32CpuHz = sysGetCPUClock();
    u32Hclk1Hz = sysGetHCLK1Clock();
    u32ApbHz = sysGetAPBClock();
    u32DramHz = sysGetDramClock();
    //ysprintf("APLL Clock = %d\n", u32APllHz);
    sysprintf("UPLL Clock = %d\n", u32PllHz);
    sysprintf("System Clock = %d\n", u32SysHz);
    sysprintf("CPU Clock = %d\n", u32CpuHz);
    sysprintf("DRAM Clock = %d\n", u32DramHz);
    sysprintf("HCLK1 Clock = %d\n", u32Hclk1Hz);
    sysprintf("APB Clock = %d\n", u32ApbHz);
    sysprintf("REG_CLKDIV4=0x%08X, HCLK234=0x%X\n", inp32(REG_CLKDIV4), (inp32(REG_CLKDIV4) & HCLK234_N)>>4);
#endif

    outp32(REG_APBCLK, inp32(REG_APBCLK) | RTC_CKE);     // enable RTC clock since FA92 IBR disable it.
    RTC_Check();    // waiting for RTC regiesters ready for access

    // RTC H/W Power Off Function Configuration
    outp32(PWRON, (inp32(PWRON) & ~PCLR_TIME) | 0x00060005);   // Press Power Key during 6 sec to Power off (0x'6'0005)
    RTC_Check();
    outp32(RIIR, 0x4);
    RTC_Check();
    sysprintf("Enable RTC power off feature to %d seconds.\n", (inp32(PWRON) & PCLR_TIME) >> 16);
    outp32(REG_APBCLK, inp32(REG_APBCLK) & ~RTC_CKE);   // disable RTC clock to save power

    // 2013/9/26, 2014/3/26, enable External RESET Debounce feature
    //      with debounce counter 0x0FFF (4096 * 83.3ns = 341.1968us)
    outp32(REG_EXTRST_DEBOUNCE, inp32(REG_EXTRST_DEBOUNCE) & (~EXTRST_DEBOUNCE));   // MUST disable debounce before set counter to 0
    outp32(REG_DEBOUNCE_CNTR, inp32(REG_DEBOUNCE_CNTR) & (~DEBOUNCE_CNTR));
    outp32(REG_DEBOUNCE_CNTR, inp32(REG_DEBOUNCE_CNTR) | 0x0FFF);
    outp32(REG_EXTRST_DEBOUNCE, inp32(REG_EXTRST_DEBOUNCE) | EXTRST_DEBOUNCE);      // enable debounce after set counter

    // 2013/9/26, suspend USBH to save power. MUST enable clock first, and then suspend it.
    outp32(REG_AHBCLK, inp32(REG_AHBCLK) | HCLK3_CKE);
    outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) | OHCI_CKE | H20PHY_CKE);        // enable USB Host clock
    outp32(REG_USBPCR0, inp32(REG_USBPCR0) & (~BIT8));                      // suspend USB Host PHY 0
    for (i=0; i<2000; i++);     // MUST wait suspend be completed before disable USB Host clock.
    outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) & (~(OHCI_CKE | H20PHY_CKE)));   // disable USB Host clock

    // 2013/10/1, suspend USBD to save power. MUST enable clock first, and then suspend it.
    outp32(REG_AHBCLK, inp32(REG_AHBCLK) | HCLK3_CKE | USBD_CKE);   // enable USB Device clock
    outp32(PHY_CTL, inp32(PHY_CTL) & (~Phy_suspend));           // suspend USB Device PHY
    for (i=0; i<2000; i++);     // wait suspend be completed before disable USB Device clock.
    outp32(REG_AHBCLK, inp32(REG_AHBCLK) & (~USBD_CKE));        // disable USB Device clock

    imagebuf = (UINT8 *)((UINT32)image_buffer | 0x80000000);
    pImageList=((unsigned int *)(((unsigned int)image_buffer)|0x80000000));

    /* Initial DMAC and NAND interface */
    fmiInitDevice();
    sicSMInit();
    memset((char *)&image, 0, sizeof(NVT_NAND_INFO_T));

    /* read physical block 0~3 - image information */
    for (i=0; i<4; i++)
    {
        if (!sicSMpread(0, i, pSM0->uPagePerBlock-2, imagebuf))
        {
            if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
            {
                sysprintf("Get image information from block 0x%x ..\n", i);
                break;
            }
        }
    }

    if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
    {
        count = *(pImageList+1);

        /* load logo first */
        pImageList = pImageList+4;
        for (i=0; i<count; i++)
        {
            if (((*(pImageList) >> 16) & 0xffff) == 4)  // logo
            {
                image.startBlock = *(pImageList + 1) & 0xffff;
                image.endBlock = (*(pImageList + 1) & 0xffff0000) >> 16;
                image.executeAddr = *(pImageList + 2);
                image.fileLen = *(pImageList + 3);
                MoveData(&image, FALSE);
                break;
            }
            /* pointer to next image */
            pImageList = pImageList+12;
        }

        pImageList=((unsigned int*)(((unsigned int)image_buffer)|0x80000000));
        memset((char *)&image, 0, sizeof(NVT_NAND_INFO_T));

#ifdef __USER_DEFINE_FUNC
        //--- call user define function before jump to next application.
        user_define_func();
#endif

        /* load execution file */
        pImageList = pImageList+4;
        for (i=0; i<count; i++)
        {
            if (((*(pImageList) >> 16) & 0xffff) == 1)  // execute
            {
                image.startBlock = *(pImageList + 1) & 0xffff;
                image.endBlock = (*(pImageList + 1) & 0xffff0000) >> 16;
                image.executeAddr = *(pImageList + 2);
                // sysprintf("executing address = 0x%x\n", image.executeAddr);
                image.fileLen = *(pImageList + 3);
                MoveData(&image, TRUE);
                break;
            }
            /* pointer to next image */
            pImageList = pImageList+12;
        }
        while(1);
    }
    return 0;
}
