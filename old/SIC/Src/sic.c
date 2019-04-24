/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2008 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
/****************************************************************************
 *
 * FILENAME
 *     sic.c
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     This file contains SIC library APIs.
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *     None
 *
 * HISTORY
  *     10/11/07      Create Ver 1.0
 *
 * REMARK
 *     None
 **************************************************************************/
/* Header files */
#include <stdio.h>
#include <string.h>

#include "wblib.h"
#include "w55fa92_sic.h"
#include "fmi.h"

/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sicOpen                                                                         */
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
/*   This function reset the DMAC and SIC interface and enable interrupt.            */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
static short _fmi_init_flag = FALSE;
void sicOpen(void)
{
    if (!_fmi_init_flag)
    {
        fmiInitDevice();
        _fmi_init_flag = TRUE;
    }
}

/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sicClose                                                                         */
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
/*   This function close the DMAC and SIC engine clock                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
void sicClose(void)
{
    outpw(REG_AHBCLK, inpw(REG_AHBCLK) & (~(SD_CKE | SIC_CKE)));
    sysDisableInterrupt(IRQ_SIC);
    _fmi_init_flag = FALSE;
}

/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sicIoctl                                                                        */
/*                                                                                   */
/* Parameters:                                                                       */
/*   sicFeature     SIC_SET_CLOCK, SIC_SET_CALLBACK, SIC_GET_CARD_STATUS.            */
/*   sicArg0        Depend on feature setting.                                       */
/*   sicArg1        Depend on feature setting.                                       */
/*   sicArg2        Depend on feature setting.                                       */
/*                                                                                   */
/* Returns:                                                                          */
/*   None.                                                                           */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function set the FMI engine clock. Card insert/remove callback             */
/*  SIC_SET_CLOCK : sicArg0 used to set clock by KHz                                 */
/*                                                                                   */
/*  SIC_SET_CALLBACK : sicArg0 used to select card type (FMI_SD_CARD0)               */
/*  SIC_SET_CALLBACK : sicArg1 remove function pointer                               */
/*  SIC_SET_CALLBACK : sicArg2 insert function pointer                               */
/*                                                                                   */
/*  SIC_GET_CARD_STATUS : sicArg0 got SD0 card status. TRUE is inserted,             */
/*                        FALSE is removed.                                          */
/*                                                                                   */
/*  SIC_SET_CARD_DETECT : sicArg0 used to enable/disable SD0 card detect feature     */
/*                        TRUE  is enable card detect feature                        */
/*                        FALSE is disable card detect feature                       */
/*-----------------------------------------------------------------------------------*/
VOID sicIoctl(INT32 sicFeature, INT32 sicArg0, INT32 sicArg1, INT32 sicArg2)
{
    switch(sicFeature)
    {
        case SIC_SET_CLOCK:
            fmiSetFMIReferenceClock(sicArg0);
            break;
        case SIC_SET_CALLBACK:
            fmiSetCallBack(sicArg0, (PVOID)sicArg1, (PVOID)sicArg2);
            break;
        case SIC_GET_CARD_STATUS:
            if (fmiSD_CardStatus() == FMI_NO_SD_CARD)
                *(UINT32 *)sicArg0 = FALSE;     // card removed
            else
                *(UINT32 *)sicArg0 = TRUE;      // card inserted
            break;
        case SIC_SET_CARD_DETECT:
            g_SD0_card_detect = sicArg0;
            break;
    }
}

/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sicSdRead                                                                       */
/*                                                                                   */
/* Parameters:                                                                       */
/*   sicSdPortNo    Select SD port 0 or port 1.                                      */
/*   sdSectorNo     Sector No. to get the data from.                                 */
/*   sdSectorCount  Sector count of this access.                                     */
/*   sdTargetAddr   The address which data upload to SDRAM.                          */
/*                                                                                   */
/* Returns:                                                                          */
/*   0                  Success                                                      */
/*   FMI_TIMEOUT        Access timeout                                               */
/*   FMI_NO_SD_CARD     Card removed                                                 */
/*   FMI_SD_CRC7_ERROR  Command/Response error                                       */
/*   FMI_SD_CRC16_ERROR Data transfer error                                          */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function used to get the data from SD card                                 */
/*-----------------------------------------------------------------------------------*/
INT sicSdRead(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr)
{
    return sicSdRead0(sdSectorNo, sdSectorCount, sdTargetAddr);
}

INT sicSdRead0(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr)
{
    if (fmiSD_CardSel(0))
        return FMI_NO_SD_CARD;

    outpw(REG_FMICR, FMI_SD_EN);
    return fmiSD_Read_in(pSD0, sdSectorNo, sdSectorCount, sdTargetAddr);
}

INT sicSdRead1(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr)
{
    if (fmiSD_CardSel(1))
        return FMI_NO_SD_CARD;

    outpw(REG_FMICR, FMI_SD_EN);
    return fmiSD_Read_in(pSD1, sdSectorNo, sdSectorCount, sdTargetAddr);
}

INT sicSdRead2(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr)
{
    if (fmiSD_CardSel(2))
        return FMI_NO_SD_CARD;

    outpw(REG_FMICR, FMI_SD_EN);
    return fmiSD_Read_in(pSD2, sdSectorNo, sdSectorCount, sdTargetAddr);
}

/*-----------------------------------------------------------------------------
 * Read data from SD card by DMA with Scatter-Gather feature.
 *---------------------------------------------------------------------------*/
INT sicSdRead_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder)
{
    return sicSdRead0_SG(sdSectorNo, sdSectorCount, sdTargetAddr, outOfOrder);
}

INT sicSdRead0_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder)
{
    INT result;

    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | SG_EN);  // enable Scatter Gather feature
    if (outOfOrder)
        result = sicSdRead0(sdSectorNo, sdSectorCount, sdTargetAddr | PAD_ORDER);   // out of order
    else
        result = sicSdRead0(sdSectorNo, sdSectorCount, sdTargetAddr & ~PAD_ORDER);  // in order
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) & ~SG_EN); // disable Scatter Gather feature
    return result;
}

INT sicSdRead1_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder)
{
    INT result;

    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | SG_EN);  // enable Scatter Gather feature
    if (outOfOrder)
        result = sicSdRead1(sdSectorNo, sdSectorCount, sdTargetAddr | PAD_ORDER);   // out of order
    else
        result = sicSdRead1(sdSectorNo, sdSectorCount, sdTargetAddr & ~PAD_ORDER);  // in order
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) & ~SG_EN); // disable Scatter Gather feature
    return result;
}

INT sicSdRead2_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder)
{
    INT result;

    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | SG_EN);  // enable Scatter Gather feature
    if (outOfOrder)
        result = sicSdRead2(sdSectorNo, sdSectorCount, sdTargetAddr | PAD_ORDER);   // out of order
    else
        result = sicSdRead2(sdSectorNo, sdSectorCount, sdTargetAddr & ~PAD_ORDER);  // in order
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) & ~SG_EN); // disable Scatter Gather feature
    return result;
}


/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sicSdWrite                                                                      */
/*                                                                                   */
/* Parameters:                                                                       */
/*   sicSdPortNo    Select SD port 0 or port 1.                                      */
/*   sdSectorNo     Sector No. to get the data from.                                 */
/*   sdSectorCount  Sector count of this access.                                     */
/*   sdSourcetAddr  The address which data download data from SDRAM.                 */
/*                                                                                   */
/* Returns:                                                                          */
/*   0                  Success                                                      */
/*   FMI_TIMEOUT        Access timeout                                               */
/*   FMI_NO_SD_CARD     Card removed                                                 */
/*   FMI_SD_CRC7_ERROR  Command/Response error                                       */
/*   FMI_SD_CRC_ERROR   Data transfer error                                          */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function used to write the data to SD card                                 */
/*-----------------------------------------------------------------------------------*/
INT sicSdWrite(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr)
{
    return sicSdWrite0(sdSectorNo, sdSectorCount, sdSourceAddr);
}

INT sicSdWrite0(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr)
{
    if (fmiSD_CardSel(0))
        return FMI_NO_SD_CARD;

    outpw(REG_FMICR, FMI_SD_EN);
    return fmiSD_Write_in(pSD0, sdSectorNo, sdSectorCount, sdSourceAddr);
}

INT sicSdWrite1(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr)
{
    if (fmiSD_CardSel(1))
        return FMI_NO_SD_CARD;

    outpw(REG_FMICR, FMI_SD_EN);
    return fmiSD_Write_in(pSD1, sdSectorNo, sdSectorCount, sdSourceAddr);
}

INT sicSdWrite2(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr)
{
    if (fmiSD_CardSel(2))
        return FMI_NO_SD_CARD;

    outpw(REG_FMICR, FMI_SD_EN);
    return fmiSD_Write_in(pSD2, sdSectorNo, sdSectorCount, sdSourceAddr);
}


/*-----------------------------------------------------------------------------
 * Write data to SD card by DMA with Scatter-Gather feature.
 *---------------------------------------------------------------------------*/
INT sicSdWrite_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder)
{
    return sicSdWrite0_SG(sdSectorNo, sdSectorCount, sdTargetAddr, outOfOrder);
}

INT sicSdWrite0_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder)
{
    INT result;

    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | SG_EN);  // enable Scatter Gather feature
    if (outOfOrder)
        result = sicSdWrite0(sdSectorNo, sdSectorCount, sdTargetAddr | PAD_ORDER);   // out of order
    else
        result = sicSdWrite0(sdSectorNo, sdSectorCount, sdTargetAddr & ~PAD_ORDER);  // in order
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) & ~SG_EN); // disable Scatter Gather feature
    return result;
}

INT sicSdWrite1_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder)
{
    INT result;

    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | SG_EN);  // enable Scatter Gather feature
    if (outOfOrder)
        result = sicSdWrite1(sdSectorNo, sdSectorCount, sdTargetAddr | PAD_ORDER);   // out of order
    else
        result = sicSdWrite1(sdSectorNo, sdSectorCount, sdTargetAddr & ~PAD_ORDER);  // in order
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) & ~SG_EN); // disable Scatter Gather feature
    return result;
}

INT sicSdWrite2_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder)
{
    INT result;

    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | SG_EN);  // enable Scatter Gather feature
    if (outOfOrder)
        result = sicSdWrite2(sdSectorNo, sdSectorCount, sdTargetAddr | PAD_ORDER);   // out of order
    else
        result = sicSdWrite2(sdSectorNo, sdSectorCount, sdTargetAddr & ~PAD_ORDER);  // in order
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) & ~SG_EN); // disable Scatter Gather feature
    return result;
}
