/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2013 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "wblib.h"
#include "w55fa92_sdio.h"

INT fmiSDIO_Read_in(FMI_SDIO_INFO_T *pSDIO, UINT32 uSector, UINT32 uBufcnt, UINT32 uDAddr);
INT fmiSDIO_Write_in(FMI_SDIO_INFO_T *pSDIO, UINT32 uSector, UINT32 uBufcnt, UINT32 uSAddr);

/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sdioOpen                                                                         */
/*                                                                                   */
/* Parameters:                                                                       */
/*   None                                                                            */
/*                                                                                   */
/* Returns:                                                                          */
/*   None.                                                                           */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function reset the DMAC and SDIO interface and enable interrupt.            */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
static short _fmi_init_flag = FALSE;
void sdioOpen(void)
{
    if (!_fmi_init_flag)
    {
        fmiSDIOInitDevice();
        _fmi_init_flag = TRUE;
    }
}

/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sdioClose                                                                         */
/*                                                                                   */
/* Parameters:                                                                       */
/*   None                                                                            */
/*                                                                                   */
/* Returns:                                                                          */
/*   None.                                                                           */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function close the DMAC and SDIO engine clock                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
void sdioClose(void)
{
    outpw(REG_AHBCLK, inpw(REG_AHBCLK) & (~SDIO_CKE));
    sysDisableInterrupt(IRQ_SDIO);
    _fmi_init_flag = FALSE;
}

/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sdioIoctl                                                                       */
/*                                                                                   */
/* Parameters:                                                                       */
/*   sdioFeature1   SDIO_SET_CLOCK, SDIO_SET_CALLBACK,                               */
/*                  SDIO_GET_CARD_STATUS, SDIO0_GET_CARD_STATUS, SDIO1_GET_CARD_STATUS */
/*                  SDIO_SET_CARD_DETECT, SDIO0_SET_CARD_DETECT, SDIO1_SET_CARD_DETECT */
/*   sdioArg0       Depend on feature setting.                                       */
/*   sdioArg1       Depend on feature setting.                                       */
/*   sdioArg2       Depend on feature setting.                                       */
/*                                                                                   */
/* Returns:                                                                          */
/*   None.                                                                           */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function set the FMI engine clock. Card insert/remove callback             */
/*  SDIO_SET_CLOCK : sdioArg0 used to set clock by KHz                               */
/*                                                                                   */
/*  SDIO_SET_CALLBACK : sdioArg0 used to select card type (FMISDIO_SD_CARD0)         */
/*                    : sdioArg1 remove function pointer                             */
/*                    : sdioArg2 insert function pointer                             */
/*                                                                                   */
/*  SDIO_GET_CARD_STATUS                                                             */
/*  SDIO0_GET_CARD_STATUS : sicArg0 got SDIO0 card status. TRUE is inserted,         */
/*                        FALSE is removed.                                          */
/*                                                                                   */
/*  SDIO1_GET_CARD_STATUS : sicArg0 got SDIO1 card status. TRUE is inserted,         */
/*                        FALSE is removed.                                          */
/*                                                                                   */
/*  SDIO_SET_CARD_DETECT                                                             */
/*  SDIO0_SET_CARD_DETECT : sicArg0 used to enable/disable SDIO0 card detect feature */
/*                        TRUE  is enable card detect feature                        */
/*                        FALSE is disable card detect feature                       */
/*                                                                                   */
/*  SDIO1_SET_CARD_DETECT : sicArg0 used to enable/disable SDIO1 card detect feature */
/*                        TRUE  is enable card detect feature                        */
/*                        FALSE is disable card detect feature                       */
/*-----------------------------------------------------------------------------------*/
VOID sdioIoctl(INT32 sdioFeature, INT32 sdioArg0, INT32 sdioArg1, INT32 sdioArg2)
{
    switch(sdioFeature)
    {
        case SDIO_SET_CLOCK:
            fmiSDIOSetFMIReferenceClock(sdioArg0);
            break;

        case SDIO_SET_CALLBACK:
            fmiSDIOSetCallBack(sdioArg0, (PVOID)sdioArg1, (PVOID)sdioArg2);
            break;

        case SDIO_GET_CARD_STATUS:
        case SDIO0_GET_CARD_STATUS:
            if (fmiSDIO_CardStatus(pSDIO0) == FMISDIO_NO_SD_CARD)
                *(UINT32 *)sdioArg0 = FALSE;    // card removed
            else
                *(UINT32 *)sdioArg0 = TRUE;     // card inserted
            break;

        case SDIO1_GET_CARD_STATUS:
            if (fmiSDIO_CardStatus(pSDIO1) == FMISDIO_NO_SD_CARD)
                *(UINT32 *)sdioArg0 = FALSE;    // card removed
            else
                *(UINT32 *)sdioArg0 = TRUE;     // card inserted
            break;

        case SDIO_SET_CARD_DETECT:
        case SDIO0_SET_CARD_DETECT:
            g_SDIO0_card_detect = sdioArg0;
            break;

        case SDIO1_SET_CARD_DETECT:
            g_SDIO1_card_detect = sdioArg0;
            break;
    }
}

/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sdioSdRead                                                                       */
/*                                                                                   */
/* Parameters:                                                                       */
/*   sdioSdPortNo   Select SD port 0 or port 1.                                      */
/*   sdSectorNo     Sector No. to get the data from.                                 */
/*   sdSectorCount  Sector count of this access.                                     */
/*   sdTargetAddr   The address which data upload to SDRAM.                          */
/*                                                                                   */
/* Returns:                                                                          */
/*   0                  Success                                                      */
/*   FMISDIO_TIMEOUT        Access timeout                                               */
/*   FMISDIO_NO_SD_CARD     Card removed                                                 */
/*   FMISDIO_SD_CRC7_ERROR  Command/Response error                                       */
/*   FMISDIO_SD_CRC16_ERROR Data transfer error                                          */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function used to get the data from SD card                                 */
/*-----------------------------------------------------------------------------------*/
INT sdioSdRead(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr)
{
    return sdioSdRead0(sdSectorNo, sdSectorCount, sdTargetAddr);
}

INT sdioSdRead0(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr)
{
    if (fmiSDIO_CardSel(0))
        return FMISDIO_NO_SD_CARD;

    outpw(REG_SDIOFMICR, FMI_SD_EN);
    return fmiSDIO_Read_in(pSDIO0, sdSectorNo, sdSectorCount, sdTargetAddr);
}

INT sdioSdRead1(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr)
{
    if (fmiSDIO_CardSel(1))
        return FMISDIO_NO_SD_CARD;

    outpw(REG_SDIOFMICR, FMI_SD_EN);
    return fmiSDIO_Read_in(pSDIO1, sdSectorNo, sdSectorCount, sdTargetAddr);
}


/*-----------------------------------------------------------------------------
 * Read data from SD card by DMA with Scatter-Gather feature.
 *---------------------------------------------------------------------------*/
INT sdioSdRead_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder)
{
    return sdioSdRead0_SG(sdSectorNo, sdSectorCount, sdTargetAddr, outOfOrder);
}

INT sdioSdRead0_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder)
{
    INT result;

    outpw(REG_SDIODMACCSR, inpw(REG_SDIODMACCSR) | SG_EN);  // enable Scatter Gather feature
    if (outOfOrder)
        result = sdioSdRead0(sdSectorNo, sdSectorCount, sdTargetAddr | PAD_ORDER);   // out of order
    else
        result = sdioSdRead0(sdSectorNo, sdSectorCount, sdTargetAddr & ~PAD_ORDER);  // in order
    outpw(REG_SDIODMACCSR, inpw(REG_SDIODMACCSR) & ~SG_EN); // disable Scatter Gather feature
    return result;
}

INT sdioSdRead1_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder)
{
    INT result;

    outpw(REG_SDIODMACCSR, inpw(REG_SDIODMACCSR) | SG_EN);  // enable Scatter Gather feature
    if (outOfOrder)
        result = sdioSdRead1(sdSectorNo, sdSectorCount, sdTargetAddr | PAD_ORDER);   // out of order
    else
        result = sdioSdRead1(sdSectorNo, sdSectorCount, sdTargetAddr & ~PAD_ORDER);  // in order
    outpw(REG_SDIODMACCSR, inpw(REG_SDIODMACCSR) & ~SG_EN); // disable Scatter Gather feature
    return result;
}


/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sdioSdWrite                                                                      */
/*                                                                                   */
/* Parameters:                                                                       */
/*   sdioSdPortNo   Select SD port 0 or port 1.                                      */
/*   sdSectorNo     Sector No. to get the data from.                                 */
/*   sdSectorCount  Sector count of this access.                                     */
/*   sdSourcetAddr  The address which data download data from SDRAM.                 */
/*                                                                                   */
/* Returns:                                                                          */
/*   0                  Success                                                      */
/*   FMISDIO_TIMEOUT        Access timeout                                               */
/*   FMISDIO_NO_SD_CARD     Card removed                                                 */
/*   FMISDIO_SD_CRC7_ERROR  Command/Response error                                       */
/*   FMISDIO_SD_CRC_ERROR   Data transfer error                                          */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function used to write the data to SD card                                 */
/*-----------------------------------------------------------------------------------*/
INT sdioSdWrite(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr)
{
    return sdioSdWrite0(sdSectorNo, sdSectorCount, sdSourceAddr);
}

INT sdioSdWrite0(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr)
{
    if (fmiSDIO_CardSel(0))
        return FMISDIO_NO_SD_CARD;

    outpw(REG_SDIOFMICR, FMI_SD_EN);
    return fmiSDIO_Write_in(pSDIO0, sdSectorNo, sdSectorCount, sdSourceAddr);
}

INT sdioSdWrite1(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr)
{
    if (fmiSDIO_CardSel(1))
        return FMISDIO_NO_SD_CARD;

    outpw(REG_SDIOFMICR, FMI_SD_EN);
    return fmiSDIO_Write_in(pSDIO1, sdSectorNo, sdSectorCount, sdSourceAddr);
}


/*-----------------------------------------------------------------------------
 * Write data to SD card by DMA with Scatter-Gather feature.
 *---------------------------------------------------------------------------*/
INT sdioSdWrite_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder)
{
    return sdioSdWrite0_SG(sdSectorNo, sdSectorCount, sdTargetAddr, outOfOrder);
}

INT sdioSdWrite0_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder)
{
    INT result;

    outpw(REG_SDIODMACCSR, inpw(REG_SDIODMACCSR) | SG_EN);  // enable Scatter Gather feature
    if (outOfOrder)
        result = sdioSdWrite0(sdSectorNo, sdSectorCount, sdTargetAddr | PAD_ORDER);   // out of order
    else
        result = sdioSdWrite0(sdSectorNo, sdSectorCount, sdTargetAddr & ~PAD_ORDER);  // in order
    outpw(REG_SDIODMACCSR, inpw(REG_SDIODMACCSR) & ~SG_EN); // disable Scatter Gather feature
    return result;
}

INT sdioSdWrite1_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder)
{
    INT result;

    outpw(REG_SDIODMACCSR, inpw(REG_SDIODMACCSR) | SG_EN);  // enable Scatter Gather feature
    if (outOfOrder)
        result = sdioSdWrite1(sdSectorNo, sdSectorCount, sdTargetAddr | PAD_ORDER);   // out of order
    else
        result = sdioSdWrite1(sdSectorNo, sdSectorCount, sdTargetAddr & ~PAD_ORDER);  // in order
    outpw(REG_SDIODMACCSR, inpw(REG_SDIODMACCSR) & ~SG_EN); // disable Scatter Gather feature
    return result;
}
