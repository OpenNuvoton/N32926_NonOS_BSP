/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2008 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
#ifdef ECOS
    #include "drv_api.h"
    #include "diag.h"
    #include "wbtypes.h"
    #include "wbio.h"
    #define IRQ_SD   16
#else
    #include "wblib.h"
    #define IRQ_SD   IRQ_SIC
#endif

#include "w55fa92_reg.h"
#include "w55fa92_sic.h"

#include "fmi.h"
#include "nvtfat.h"

#ifdef ECOS
cyg_interrupt   fmi_interrupt;
cyg_handle_t    fmi_interrupt_handle;
#endif

// global variable
UINT32 _fmi_uFMIReferenceClock;
BOOL volatile _fmi_bIsSDDataReady=FALSE, _fmi_bIsSMDataReady=FALSE;
BOOL volatile _fmi_bIsSMPRegionDetect=FALSE;

typedef void (*fmi_pvFunPtr)();   /* function pointer */
void (*fmiSD0RemoveFun)() = NULL;
void (*fmiSD0InsertFun)() = NULL;

extern PDISK_T *pDisk_SD0;
extern PDISK_T *pDisk_SD1;
extern PDISK_T *pDisk_SD2;

#ifdef _SIC_USE_INT_
#ifdef ECOS
static cyg_uint32 fmiIntHandler(cyg_vector_t vector, cyg_addrword_t data)
#else
VOID fmiIntHandler()
#endif
{
    unsigned int volatile isr;
    unsigned int volatile ier;

    // FMI data abort interrupt
    if (inpw(REG_FMIISR) & FMI_DAT_IF)
    {
        /* fmiResetAllEngine() */
        outpw(REG_FMICR, inpw(REG_FMICR) | FMI_SWRST);
        outpw(REG_FMIISR, FMI_DAT_IF);
    }

    //----- SD interrupt status
    isr = inpw(REG_SDISR);
    if (isr & SDISR_BLKD_IF)        // block down
    {
        _fmi_bIsSDDataReady = TRUE;
        outpw(REG_SDISR, SDISR_BLKD_IF);
    }

#ifndef __SD0_NO_CARD_DETECT
    if (isr & SDISR_CD_IF)  // port 0 card detect
    {
    //----- SD interrupt status
        // it is work to delay 50 times for SD_CLK = 200KHz
        {
            volatile int i;         // delay 30 fail, 50 OK
            for (i=0; i<50; i++);  // delay to make sure got updated value from REG_SDISR.
            isr = inpw(REG_SDISR);
        }

        if (isr & SDISR_CD_Card)
        {
            pSD0->bIsCardInsert = FALSE;    // SDISR_CD_Card = 1 means card remove for GPIO mode
            if (fmiSD0RemoveFun != NULL)
                (*fmiSD0RemoveFun)(pDisk_SD0);
        }
        else
        {
            pSD0->bIsCardInsert = TRUE;     // SDISR_CD_Card = 0 means card insert for GPIO mode
            if (fmiSD0InsertFun != NULL)
                (*fmiSD0InsertFun)();
        }
        outpw(REG_SDISR, SDISR_CD_IF);
    }
#endif

    // CRC error interrupt
    if (isr & SDISR_CRC_IF)
    {
        if (!(isr & SDISR_CRC_16))
        {
            // handle CRC error
        }
        else if (!(isr & SDISR_CRC_7))
        {
            extern UINT32 _fmi_uR3_CMD;
            if (! _fmi_uR3_CMD)
            {
                // handle CRC error
            }
        }
        outpw(REG_SDISR, SDISR_CRC_IF);     // clear interrupt flag
    }

    //----- SM interrupt status
    ier = inpw(REG_SMIER);
    isr = inpw(REG_SMISR);
    if ((ier & SMIER_DMA_IE) && (isr & SMISR_DMA_IF))
    {
        _fmi_bIsSMDataReady = TRUE;
        outpw(REG_SMISR, SMISR_DMA_IF);
    }

    if ((ier & SMIER_PROT_REGION_WR_IE) && (isr & SMISR_PROT_REGION_WR_IF))
    {
        _fmi_bIsSMPRegionDetect = TRUE;
        outpw(REG_SMISR, SMISR_PROT_REGION_WR_IF);
    }

#ifdef ECOS
    cyg_drv_interrupt_acknowledge(IRQ_SD);
    return CYG_ISR_HANDLED;
#endif
}
#endif  //_SIC_USE_INT_


VOID fmiSetCallBack(UINT32 uCard, PVOID pvRemove, PVOID pvInsert)
{
    switch (uCard)
    {
        case FMI_SD_CARD:
            fmiSD0RemoveFun = (fmi_pvFunPtr)pvRemove;
            fmiSD0InsertFun = (fmi_pvFunPtr)pvInsert;
            break;
    }
}


VOID fmiInitDevice()
{
    // Enable SD Card Host Controller operation and driving clock.
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) | HCLK3_CKE | SIC_CKE | SD_CKE | NAND_CKE);

#ifdef _SIC_USE_INT_
    /* Install ISR */
#ifdef ECOS
    cyg_drv_interrupt_create(IRQ_SD, 10, 0, fmiIntHandler, NULL,
                                &fmi_interrupt_handle, &fmi_interrupt);
    cyg_drv_interrupt_attach(fmi_interrupt_handle);
#else
    sysInstallISR(IRQ_LEVEL_1, IRQ_SD, (PVOID)fmiIntHandler);
#endif

#ifndef ECOS
    /* enable CPSR I bit */
    sysSetLocalInterrupt(ENABLE_IRQ);
#endif
#endif  //_SIC_USE_INT_

    // DMAC Initial
    outpw(REG_DMACCSR, DMAC_SWRST | DMAC_EN);
    outpw(REG_DMACCSR, DMAC_EN);

#ifdef _SIC_USE_INT_
#ifdef ECOS
    cyg_drv_interrupt_unmask(IRQ_SD);
#else
    sysEnableInterrupt(IRQ_SD);
#endif
#endif  //_SIC_USE_INT_

#ifdef OPT_NAND_CARD_DETECT_GPD14
    outpw(REG_GPDFUN, inpw(REG_GPDFUN) & ~MF_GPD14);                // set GPD14 as GPIO pin
    outpw(REG_GPIOD_PUEN, inpw(REG_GPIOD_PUEN) & ~(0x00004000));    // set GPD14 pull-down resister to DISABLE mode
    outpw(REG_GPIOD_OMD, inpw(REG_GPIOD_OMD) & ~(0x00004000));      // set GPD14 to INPUT mode
    sysDelay(1);  // delay 1 ticks to wait for GPD14 stable
#endif

    outpw(REG_FMICR, FMI_SWRST);        // reset FMI engine
    while(inpw(REG_FMICR)&FMI_SWRST);
}


VOID fmiSetFMIReferenceClock(UINT32 uClock)
{
    _fmi_uFMIReferenceClock = uClock;   // kHz
}
