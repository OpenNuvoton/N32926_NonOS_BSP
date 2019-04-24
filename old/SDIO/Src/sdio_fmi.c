/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2013 by Nuvoton Technology Corporation                              */
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
#include "w55fa92_sdio.h"

#include "sdio_fmi.h"
#include "nvtfat.h"

#ifdef ECOS
cyg_interrupt  /* dmac_interrupt, */ fmi_interrupt;
cyg_handle_t   /* dmac_interrupt_handle, */ fmi_interrupt_handle;
#endif

// global variable
UINT32 _fmiSDIO_uFMIReferenceClock;
BOOL volatile _fmiSDIO_bIsSDDataReady=FALSE, _fmiSDIO_bIsSMDataReady=FALSE;
BOOL volatile _fmiSDIO_bIsSMPRegionDetect=FALSE;

typedef void (*fmi_pvFunPtr)();   /* function pointer */
void (*fmiSDIO0RemoveFun)() = NULL;
void (*fmiSDIO0InsertFun)() = NULL;
void (*fmiSDIO1RemoveFun)() = NULL;
void (*fmiSDIO1InsertFun)() = NULL;

extern PDISK_T *pDisk_SDIO0;
extern PDISK_T *pDisk_SDIO1;

#ifdef _SDIO_USE_INT_
#ifdef ECOS
static cyg_uint32 sdiofmiIntHandler(cyg_vector_t vector, cyg_addrword_t data)
#else
VOID sdiofmiIntHandler()
#endif
{
    unsigned int volatile isr;

    // FMI data abort interrupt
    if (inpw(REG_SDIOFMIISR) & FMI_DAT_IF)
    {
        /* fmiResetAllEngine() */
        outpw(REG_SDIOFMICR, inpw(REG_SDIOFMICR) | FMI_SWRST);
        outpw(REG_SDIOFMIISR, FMI_DAT_IF);
    }

    //----- SD interrupt status
    isr = inpw(REG_SDIOISR);
    if (isr & SDISR_BLKD_IF)        // block down
    {
        _fmiSDIO_bIsSDDataReady = TRUE;
        outpw(REG_SDIOISR, SDISR_BLKD_IF);
    }

    if (isr & SDISR_CD_IF)  // port 0 card detect
    {
    //----- SD interrupt status
        // it is work to delay 50 times for SD_CLK = 200KHz
        {
            volatile int i;         // delay 30 fail, 50 OK
            for (i=0; i<50; i++);   // delay to make sure got updated value from REG_SDIOISR.
            isr = inpw(REG_SDIOISR);
        }

        if (isr & SDISR_CD_Card)
        {
            pSDIO0->bIsCardInsert = FALSE;    // SDISR_CD_Card = 1 means card remove for GPIO mode
            if (fmiSDIO0RemoveFun != NULL)
                (*fmiSDIO0RemoveFun)(pDisk_SDIO0);
        }
        else
        {
            pSDIO0->bIsCardInsert = TRUE;     // SDISR_CD_Card = 0 means card insert for GPIO mode
            if (fmiSDIO0InsertFun != NULL)
                (*fmiSDIO0InsertFun)();
        }
        outpw(REG_SDIOISR, SDISR_CD_IF);
    }


    if (isr & SDISR_CD1_IF)  // port 1 card detect
    {
    //----- SD interrupt status
        // it is work to delay 50 times for SD_CLK = 200KHz
        {
            volatile int i;         // delay 30 fail, 50 OK
            for (i=0; i<50; i++);   // delay to make sure got updated value from REG_SDIOISR.
            isr = inpw(REG_SDIOISR);
        }

        if (isr & SDISR_CD1_Card)
        {
            pSDIO1->bIsCardInsert = FALSE;    // SDISR_CD1_Card = 1 means card remove for GPIO mode
            if (fmiSDIO1RemoveFun != NULL)
                (*fmiSDIO1RemoveFun)(pDisk_SDIO1);
        }
        else
        {
            pSDIO1->bIsCardInsert = TRUE;     // SDISR_CD1_Card = 0 means card insert for GPIO mode
            if (fmiSDIO1InsertFun != NULL)
                (*fmiSDIO1InsertFun)();
        }
        outpw(REG_SDIOISR, SDISR_CD1_IF);
    }

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
        outpw(REG_SDIOISR, SDISR_CRC_IF);     // clear interrupt flag
    }

#ifdef ECOS
    cyg_drv_interrupt_acknowledge(IRQ_SDIO);
    return CYG_ISR_HANDLED;
#endif
}
#endif  //_SDIO_USE_INT_


VOID fmiSDIOSetCallBack(UINT32 uCard, PVOID pvRemove, PVOID pvInsert)
{
    switch (uCard)
    {
        case FMI_SDIO_CARD:
            fmiSDIO0RemoveFun = (fmi_pvFunPtr)pvRemove;
            fmiSDIO0InsertFun = (fmi_pvFunPtr)pvInsert;
            break;
        case FMI_SDIO1_CARD:
            fmiSDIO1RemoveFun = (fmi_pvFunPtr)pvRemove;
            fmiSDIO1InsertFun = (fmi_pvFunPtr)pvInsert;
            break;
    }
}


VOID fmiSDIOInitDevice()
{
    // Enable SD Card Host Controller operation and driving clock.
    outpw(REG_AHBCLK, inpw(REG_AHBCLK) | SDIO_CKE);

#ifdef _SDIO_USE_INT_
    /* Install ISR */
#ifdef ECOS
    cyg_drv_interrupt_create(IRQ_SDIO, 10, 0, sdiofmiIntHandler, NULL,
                                &fmi_interrupt_handle, &fmi_interrupt);
    cyg_drv_interrupt_attach(fmi_interrupt_handle);
#else
    sysInstallISR(IRQ_LEVEL_1, IRQ_SDIO, (PVOID)sdiofmiIntHandler);
#endif

#ifndef ECOS
    /* enable CPSR I bit */
    sysSetLocalInterrupt(ENABLE_IRQ);
#endif
#endif  //_SDIO_USE_INT_

    // DMAC Initial
    outpw(REG_SDIODMACCSR, DMAC_SWRST | DMAC_EN);
    outpw(REG_SDIODMACCSR, DMAC_EN);

#ifdef _SDIO_USE_INT_
#ifdef ECOS
    cyg_drv_interrupt_unmask(IRQ_SDIO);
#else
    sysEnableInterrupt(IRQ_SDIO);
#endif
#endif  //_SDIO_USE_INT_

    outpw(REG_SDIOFMICR, FMI_SWRST);        // reset FMI engine
    while(inpw(REG_SDIOFMICR)&FMI_SWRST);
}


VOID fmiSDIOSetFMIReferenceClock(UINT32 uClock)
{
    _fmiSDIO_uFMIReferenceClock = uClock;   // kHz
}
