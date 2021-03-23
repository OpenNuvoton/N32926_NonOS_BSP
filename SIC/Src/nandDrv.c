/**************************************************************************//**
 * @file     nandDrv.c
 * @version  V3.00
 * @brief    N3292x series SIC driver source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "wbio.h"
#include "wblib.h"

#include "W55FA92_reg.h"
#include "W55FA92_SIC.h"
#include "fmi.h"
#include "W55FA92_GNAND.h"

//--- Should define these compile flags from ADS target.
// __OPT_SW_WP_GPA0             : use GPA0 as software Write Protect pin for CS0; 1 is normal; 0 is protected
// __OPT_NAND_CARD_DETECT_GPD14 : use GPD14 as NAND card detect pin for CS1; 1 is removed; 0 is inserted

// define DATE CODE and show it when running to make maintaining easy.
#define NAND_DATE_CODE  FMI_DATE_CODE

/*-----------------------------------------------------------------------------
 * Define message display level
 *---------------------------------------------------------------------------*/
// show large messages for debug
//#define DBG_PRINTF  sysprintf
#define DBG_PRINTF(...)
// show improtant information for normal operation
#define INF_PRINTF  sysprintf
// show error message
#define ERR_PRINTF  sysprintf

#define NAND_RESERVED_BLOCK     10
#define OPT_TWO_RB_PINS
#define OPT_FIRST_4BLOCKS_ECC4

#define OPT_SUPPORT_H27UAG8T2A


/*-----------------------------------------------------------------------------
 * Define some constants for BCH
 *---------------------------------------------------------------------------*/
// define the total padding bytes for 512/1024 data segment
#define BCH_PADDING_LEN_512     32
#define BCH_PADDING_LEN_1024    64
// define the BCH parity code lenght for 512 bytes data pattern
#define BCH_PARITY_LEN_T4       8
#define BCH_PARITY_LEN_T8       15
#define BCH_PARITY_LEN_T12      23
#define BCH_PARITY_LEN_T15      29
// define the BCH parity code lenght for 1024 bytes data pattern
#define BCH_PARITY_LEN_T24      45


#if defined (OPT_SW_WP) || defined (__OPT_SW_WP_GPA0)
#define SW_WP_DELAY_LOOP        3000
#endif

BOOL volatile _fmi_bIsNandFirstAccess = TRUE;
extern BOOL volatile _fmi_bIsSMDataReady;
INT fmiSMCheckBootHeader(INT chipSel, FMI_SM_INFO_T *pSM);
static int _nand_init0 = 0, _nand_init1 = 0;

#if defined (__GNUC__)
    UCHAR _fmi_ucSMBuffer[8192] __attribute__((aligned (4096)));
#else
    __align(4096) UCHAR _fmi_ucSMBuffer[8192];
#endif


UINT8 *_fmi_pSMBuffer;


// return 0 for R/B timeout
// return 1 for Ready
INT fmiSMCheckRB(FMI_SM_INFO_T *pSM)
{
#if 0   // no timer in it
    UINT32 ii;
    for (ii=0; ii<100; ii++);

    while(1)
    {
        if(pSM == pSM0)
        {
            if (inpw(REG_SMISR) & SMISR_RB0_IF)
            {
                while(! (inpw(REG_SMISR) & SMISR_RB0) );
                outpw(REG_SMISR, SMISR_RB0_IF);
                return 1;
            }
        }
        else
        {
            if (inpw(REG_SMISR) & SMISR_RB1_IF)
            {
                while(! (inpw(REG_SMISR) & SMISR_RB1) );
                outpw(REG_SMISR, SMISR_RB1_IF);
                return 1;
            }
        }
    }
    return 0;

#else
    unsigned int volatile tick;
    tick = sysGetTicks(TIMER0);

    while(1)
    {
        if(pSM == pSM0)
        {
            if (inpw(REG_SMISR) & SMISR_RB0_IF)
            {
                while(! (inpw(REG_SMISR) & SMISR_RB0) );
                outpw(REG_SMISR, SMISR_RB0_IF);
                return 1;
            }
        }
        else    // for pSM1 or Nand Card
        {
            if (inpw(REG_SMISR) & SMISR_RB1_IF)
            {
                while(! (inpw(REG_SMISR) & SMISR_RB1) );
                outpw(REG_SMISR, SMISR_RB1_IF);
                return 1;
            }
#ifdef __OPT_NAND_CARD_DETECT_GPD14
            if (nand_is_card_inserted() != 0)   // nand card removed
                return GNERR_NAND_NOT_FOUND;
#endif
        }
        if ((sysGetTicks(TIMER0) - tick) > 1000)
            break;
    }
    return 0;   // timeout error
#endif
}


INT fmiSMCheckStatus(FMI_SM_INFO_T *pSM)
{
    UINT32 status, ret;

    ret = 0;
    outpw(REG_SMCMD, 0x70);     // Status Read command for NAND flash
    status = inpw(REG_SMDATA);

    if (status & BIT0)          // BIT0: Chip status: 1:fail; 0:pass
    {
        ERR_PRINTF("ERROR: NAND device status: FAIL!!\n");
        ret = FMI_SM_STATUS_ERR;
    }

    if ((status & BIT7) == 0)   // BIT7: Write Protect: 1:unprotected; 0:protected
    {
        INF_PRINTF("WARNING: NAND device status: Write Protected!!\n");
        ret = FMI_SM_STATUS_ERR;
    }

    return ret;
}


// SM functions
INT fmiSM_Reset(FMI_SM_INFO_T *pSM)
{
#ifdef OPT_TWO_RB_PINS
    UINT32 volatile i;
    int result;

    if(pSM == pSM0)
        outpw(REG_SMISR, SMISR_RB0_IF);
    else
        outpw(REG_SMISR, SMISR_RB1_IF);

    outpw(REG_SMCMD, 0xff);
    for (i=100; i>0; i--);

    result = fmiSMCheckRB(pSM);
    if (result == GNERR_NAND_NOT_FOUND)
        return result;
    if (!result)
        return FMI_SM_RB_ERR;

    return 0;
#else
    UINT32 volatile i;
    outpw(REG_SMISR, SMISR_RB0_IF);
    outpw(REG_SMCMD, 0xff);
    for (i=100; i>0; i--);

    result = fmiSMCheckRB(pSM);
    if (result == GNERR_NAND_NOT_FOUND)
        return result;
    if (!result)
        return FMI_SM_RB_ERR;

    return 0;
#endif  // OPT_TWO_RB_PINS
}


/*-----------------------------------------------------------------------------
 * According to pSM parameters to Set SIC register to initial NAND sub-system
 *      for page size, RA size, BCH, and so on.
 *---------------------------------------------------------------------------*/
VOID fmiSM_Initial(FMI_SM_INFO_T *pSM)
{
    //--- Set registers for ECC enable/disable
    outpw(REG_SMCSR,  inpw(REG_SMCSR) | SMCR_ECC_EN);   // enable ECC
    if (pSM->bIsCheckECC)
        outpw(REG_SMCSR,  inpw(REG_SMCSR) | SMCR_ECC_CHK);  // enable ECC check
    else
        outpw(REG_SMCSR, inpw(REG_SMCSR) & ~SMCR_ECC_CHK);  // disable ECC check

#ifdef _SIC_USE_INT_
    if ((_nand_init0 == 0) && (_nand_init1 == 0))
        outpw(REG_SMIER, SMIER_DMA_IE);
#endif  //_SIC_USE_INT_

    //--- Set register to disable Mask ECC feature
    outpw(REG_SMREAREA_CTL, inpw(REG_SMREAREA_CTL) & ~SMRE_MECC);

    //--- Set registers that depend on page size. According to the sepc, the correct order is
    //--- 1. SMCR_BCH_TSEL  : to support T24, MUST set SMCR_BCH_TSEL before SMCR_PSIZE.
    //--- 2. SMCR_PSIZE     : set SMCR_PSIZE will auto change SMRE_REA128_EXT to default value.
    //--- 3. SMRE_REA128_EXT: to use non-default value, MUST set SMRE_REA128_EXT after SMCR_PSIZE.
    if (pSM->nPageSize == NAND_PAGE_8KB)
    {
        outpw(REG_SMCSR, inpw(REG_SMCSR) & ~SMCR_BCH_TSEL);
        if (pSM->bIsNandECC24 == TRUE)
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T24);
        else if (pSM->bIsNandECC15 == TRUE)
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T15);
        else if (pSM->bIsNandECC12 == TRUE)
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T12);
        else if (pSM->bIsNandECC8 == TRUE)
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T8);
        else
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T4);

        outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_8K));

        outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 376);  // Redundant area size
    }

    else if (pSM->nPageSize == NAND_PAGE_4KB)
    {
        if (pSM->bIsNandECC24 == TRUE)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T24);
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | PSIZE_4K);
    #ifdef OPT_SUPPORT_H27UAG8T2A
            if (pSM->bIsRA224)
                outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 224);  // Redundant area size
            else
                outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 216);  // Redundant area size
    #else
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 216);  // Redundant area size
    #endif
        }
        else if (pSM->bIsNandECC12 == TRUE)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T12);            // BCH_12 is selected
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | PSIZE_4K);
    #ifdef OPT_SUPPORT_H27UAG8T2A
            if (pSM->bIsRA224)
                outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 224);  // Redundant area size
            else
                outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 216);  // Redundant area size
    #else
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 216);  // Redundant area size
    #endif
        }
        else if (pSM->bIsNandECC8 == TRUE)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T8);             // BCH_8 is selected
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | PSIZE_4K);
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 128);  // Redundant area size
        }
        else
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T4);             // BCH_4 is selected
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | PSIZE_4K);
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 128);  // Redundant area size
        }
    }

    else if (pSM->nPageSize == NAND_PAGE_2KB)
    {
        if (pSM->bIsNandECC8 == TRUE)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T8);             // BCH_8 is selected
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | PSIZE_2K);
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 64);  // Redundant area size
        }
        else
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T4);             // BCH_4 is selected
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | PSIZE_2K);
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 64);  // Redundant area size
        }
    }

    else    // Page size should be 512 bytes
    {
        outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
        outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T4);                 // BCH_4 is selected
        outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | PSIZE_512);
        outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 16);  // Redundant area size
    }

    //--- config register for Region Protect
    if (pSM->uRegionProtect == 0)
        outpw(REG_SMCSR, inpw(REG_SMCSR) & ~SMCR_PROT_REGION_EN);   // disable Region Protect
    else
    {
        if (pSM->nPageSize == NAND_PAGE_512B)
        {
            // just has 8 bits for column address
            outpw(REG_SM_PROT_ADDR0, (pSM->uRegionProtect & 0x00FFFFFF) << 8);  // low  address (cycle 1~4) for protect region end address
            outpw(REG_SM_PROT_ADDR1, (pSM->uRegionProtect & 0xFF000000) >> 24); // high address (cycle 5) for protect region end address
        }
        else
        {
            // has 16 bits for column address
            outpw(REG_SM_PROT_ADDR0, (pSM->uRegionProtect & 0x0000FFFF) << 16); // low  address (cycle 1~4) for protect region end address
            outpw(REG_SM_PROT_ADDR1, (pSM->uRegionProtect & 0xFFFF0000) >> 16); // high address (cycle 5) for protect region end address
        }
        outpw(REG_SMISR, SMISR_PROT_REGION_WR_IF);                      // clear protect region detection flag
#ifdef _SIC_USE_INT_
        _fmi_bIsSMPRegionDetect = FALSE;                                // clear protect region detection flag for ISR
        outpw(REG_SMIER, inpw(REG_SMIER) | SMIER_PROT_REGION_WR_IE);    // enable interrupt for Region Protect
#else
        outpw(REG_SMIER, inpw(REG_SMIER) & ~SMIER_PROT_REGION_WR_IE);   // disable interrupt for Region Protect
#endif
        outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_PROT_REGION_EN);        // enable Region Protect
    }
}


/*-----------------------------------------------------------------------------
 * Read NAND chip ID from chip and then set pSM and NDISK by chip ID.
 *---------------------------------------------------------------------------*/
INT fmiSM_ReadID(FMI_SM_INFO_T *pSM, NDISK_T *NDISK_info)
{
    UINT32 tempID[5];

    fmiSM_Reset(pSM);
    outpw(REG_SMCMD, 0x90);     // read ID command
    outpw(REG_SMADDR, EOA_SM);  // address 0x00

#ifdef OPT_SUPPORT_H27UAG8T2A
// 2011/12/13 support Hynix H27UAG8T2A that with larger redundancy area size 224.
//      fmiSM_Initial() will set Redundancy Area size to 224 for 4K page NAND if pSM->bIsRA224 is TRUE.
//      Else, the default Redundancy Area size for other 4K page NAND is 216.
    pSM->bIsRA224 = 0;
#endif

    tempID[0] = inpw(REG_SMDATA);
    tempID[1] = inpw(REG_SMDATA);
    tempID[2] = inpw(REG_SMDATA);
    tempID[3] = inpw(REG_SMDATA);
    tempID[4] = inpw(REG_SMDATA);

    NDISK_info->vendor_ID = tempID[0];
    NDISK_info->device_ID = tempID[1];

    if (((tempID[0] == 0xC2) && (tempID[1] == 0x79)) ||
        ((tempID[0] == 0xC2) && (tempID[1] == 0x76)))
        // Don't support ECC for NAND Interface ROM
        pSM->bIsCheckECC = FALSE;
    else
        pSM->bIsCheckECC = TRUE;

    pSM->bIsNandECC4 = FALSE;
    pSM->bIsNandECC8 = FALSE;
    pSM->bIsNandECC12 = FALSE;
    pSM->bIsNandECC15 = FALSE;
    pSM->bIsNandECC24 = FALSE;

    switch (tempID[1])
    {
        /* page size 512B */
        case 0x79:  // 128M
            pSM->uSectorPerFlash = 255744;
            pSM->uBlockPerFlash = 8191;
            pSM->uPagePerBlock = 32;
            pSM->uSectorPerBlock = 32;
            pSM->bIsMulticycle = TRUE;
            pSM->nPageSize = NAND_PAGE_512B;
            pSM->bIsNandECC4 = TRUE;
            pSM->bIsMLCNand = FALSE;

            NDISK_info->NAND_type = NAND_TYPE_SLC;
            NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_OUT_SEQ;
            NDISK_info->nZone = 1;              /* number of zones */
            NDISK_info->nBlockPerZone = 8192;   /* blocks per zone */
            NDISK_info->nPagePerBlock = 32;     /* pages per block */
            NDISK_info->nLBPerZone = 8000;      /* logical blocks per zone */
            NDISK_info->nPageSize = 512;
            break;

        case 0x76:  // 64M
        case 0x5A:  // 64M XtraROM
            pSM->uSectorPerFlash = 127872;
            pSM->uBlockPerFlash = 4095;
            pSM->uPagePerBlock = 32;
            pSM->uSectorPerBlock = 32;
            pSM->bIsMulticycle = TRUE;
            pSM->nPageSize = NAND_PAGE_512B;
            pSM->bIsNandECC4 = TRUE;
            pSM->bIsMLCNand = FALSE;

            NDISK_info->NAND_type = NAND_TYPE_SLC;
            NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_OUT_SEQ;
            NDISK_info->nZone = 1;              /* number of zones */
            NDISK_info->nBlockPerZone = 4096;   /* blocks per zone */
            NDISK_info->nPagePerBlock = 32;     /* pages per block */
            NDISK_info->nLBPerZone = 4000;      /* logical blocks per zone */
            NDISK_info->nPageSize = 512;
            break;

        case 0x75:  // 32M
            pSM->uSectorPerFlash = 63936;
            pSM->uBlockPerFlash = 2047;
            pSM->uPagePerBlock = 32;
            pSM->uSectorPerBlock = 32;
            pSM->bIsMulticycle = FALSE;
            pSM->nPageSize = NAND_PAGE_512B;
            pSM->bIsNandECC4 = TRUE;
            pSM->bIsMLCNand = FALSE;

            NDISK_info->NAND_type = NAND_TYPE_SLC;
            NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_OUT_SEQ;
            NDISK_info->nZone = 1;              /* number of zones */
            NDISK_info->nBlockPerZone = 2048;   /* blocks per zone */
            NDISK_info->nPagePerBlock = 32;     /* pages per block */
            NDISK_info->nLBPerZone = 2000;      /* logical blocks per zone */
            NDISK_info->nPageSize = 512;
            break;

        case 0x73:  // 16M
            pSM->uSectorPerFlash = 31968;   // max. sector no. = 999 * 32
            pSM->uBlockPerFlash = 1023;
            pSM->uPagePerBlock = 32;
            pSM->uSectorPerBlock = 32;
            pSM->bIsMulticycle = FALSE;
            pSM->nPageSize = NAND_PAGE_512B;
            pSM->bIsNandECC4 = TRUE;
            pSM->bIsMLCNand = FALSE;

            NDISK_info->NAND_type = NAND_TYPE_SLC;
            NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_OUT_SEQ;
            NDISK_info->nZone = 1;              /* number of zones */
            NDISK_info->nBlockPerZone = 1024;   /* blocks per zone */
            NDISK_info->nPagePerBlock = 32;     /* pages per block */
            NDISK_info->nLBPerZone = 1000;      /* logical blocks per zone */
            NDISK_info->nPageSize = 512;
            break;

        /* page size 2KB */
        case 0xf1:  // 128M
        case 0xd1:
            pSM->uBlockPerFlash = 1023;
            pSM->uPagePerBlock = 64;
            pSM->uSectorPerBlock = 256;
            pSM->uSectorPerFlash = 255744;
            pSM->bIsMulticycle = FALSE;
            pSM->nPageSize = NAND_PAGE_2KB;
            pSM->bIsNandECC8 = TRUE;
            pSM->bIsMLCNand = FALSE;

            NDISK_info->NAND_type = NAND_TYPE_SLC;
            NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_OUT_SEQ;
            NDISK_info->nZone = 1;              /* number of zones */
            NDISK_info->nBlockPerZone = 1024;   /* blocks per zone */
            NDISK_info->nPagePerBlock = 64;     /* pages per block */
            NDISK_info->nLBPerZone = 1000;      /* logical blocks per zone */
            NDISK_info->nPageSize = 2048;

            // 2013/10/22, support MXIC MX30LF1G08AA NAND flash
            // 2015/06/22, support MXIC MX30LF1G18AC NAND flash
            if ( ((tempID[0]==0xC2)&&(tempID[1]==0xF1)&&(tempID[2]==0x80)&&(tempID[3]==0x1D)) ||
                 ((tempID[0]==0xC2)&&(tempID[1]==0xF1)&&(tempID[2]==0x80)&&(tempID[3]==0x95)&&(tempID[4]==0x02)) )
            {
                // The first ID of this NAND is 0xC2 BUT it is NOT NAND ROM (read only)
                // So, we MUST modify the configuration of it
                //      1. change pSM->bIsCheckECC to TRUE to enable ECC feature;
                //      2. assign a fake vendor_ID to make NVTFAT can write data to this NAND disk.
                //         (GNAND will check vendor_ID and set disk to DISK_TYPE_READ_ONLY if it is 0xC2)
                pSM->bIsCheckECC = TRUE;
                NDISK_info->vendor_ID = 0xFF;   // fake vendor_ID
            }

            // 2014/10/16, support Winbond W29N01GV NAND flash
            // 2017/09/14, support Samsung K9F1G08U0B NAND flash
            // 2017/09/19, support Winbond W29N01HV NAND flash
            if (   ((tempID[0]==0xEF)&&(tempID[1]==0xF1)&&(tempID[2]==0x80)&&(tempID[3]==0x95))
                || ((tempID[0]==0xEC)&&(tempID[1]==0xF1)&&(tempID[2]==0x00)&&(tempID[3]==0x95))
                || ((tempID[0]==0xEF)&&(tempID[1]==0xF1)&&(tempID[2]==0x00)&&(tempID[3]==0x95))
               )
            {
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
            }
            break;

        case 0xda:  // 256M
            if ((tempID[3] & 0x33) == 0x11)
            {
                pSM->uBlockPerFlash = 2047;
                pSM->uPagePerBlock = 64;
                pSM->uSectorPerBlock = 256;
                pSM->bIsMLCNand = FALSE;

                NDISK_info->NAND_type = NAND_TYPE_SLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_OUT_SEQ;
                NDISK_info->nZone = 1;              /* number of zones */
                NDISK_info->nPagePerBlock = 64;     /* pages per block */
                NDISK_info->nBlockPerZone = 2048;   /* blocks per zone */
                NDISK_info->nLBPerZone = 2000;      /* logical blocks per zone */
            }
            else if ((tempID[3] & 0x33) == 0x21)
            {
                pSM->uBlockPerFlash = 1023;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 512;
                pSM->bIsMLCNand = TRUE;

                NDISK_info->NAND_type = NAND_TYPE_MLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                NDISK_info->nZone = 1;              /* number of zones */
                NDISK_info->nPagePerBlock = 128;    /* pages per block */
                NDISK_info->nBlockPerZone = 1024;   /* blocks per zone */
                NDISK_info->nLBPerZone = 1000;      /* logical blocks per zone */
            }
            pSM->uSectorPerFlash = 511488;
            pSM->bIsMulticycle = TRUE;
            pSM->nPageSize = NAND_PAGE_2KB;
            pSM->bIsNandECC8 = TRUE;

            // 2018/10/29, support MXIC MX30LF2G18AC NAND flash
            if ((tempID[0]==0xC2)&&(tempID[1]==0xDA)&&(tempID[2]==0x90)&&(tempID[3]==0x95)&&(tempID[4]==0x06))
            {
                // The first ID of this NAND is 0xC2 BUT it is NOT NAND ROM (read only)
                // So, we MUST modify the configuration of it
                //      1. change pSM->bIsCheckECC to TRUE to enable ECC feature;
                //      2. assign a fake vendor_ID to make NVTFAT can write data to this NAND disk.
                //         (GNAND will check vendor_ID and set disk to DISK_TYPE_READ_ONLY if it is 0xC2)
                pSM->bIsCheckECC = TRUE;
                NDISK_info->vendor_ID = 0xFF;   // fake vendor_ID
            }

            NDISK_info->nPageSize = 2048;
            break;

        case 0xdc:  // 512M
            // 2020/10/08, support Micron MT29F4G08ABAEA 512MB NAND flash
            if ((tempID[0]==0x2C)&&(tempID[2]==0x90)&&(tempID[3]==0xA6)&&(tempID[4]==0x54))
            {
                pSM->uBlockPerFlash  = 2047;        // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 64;
                pSM->nPageSize       = NAND_PAGE_4KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = FALSE;
                pSM->bIsMulticycle   = TRUE;
                pSM->bIsNandECC24    = TRUE;

                NDISK_info->NAND_type     = (pSM->bIsMLCNand ? NAND_TYPE_MLC : NAND_TYPE_SLC);
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                NDISK_info->nZone         = 1;      // number of zones
                NDISK_info->nBlockPerZone = pSM->uBlockPerFlash + 1;   // blocks per zone
                NDISK_info->nPagePerBlock = pSM->uPagePerBlock;
                NDISK_info->nPageSize     = pSM->nPageSize;
                NDISK_info->nLBPerZone    = 2000;   // logical blocks per zone

                pSM->uSectorPerFlash = pSM->uSectorPerBlock * NDISK_info->nLBPerZone / 1000 * 999;
                break;
            }

            // 2017/9/19, To support both Maker Founder MP4G08JAA
            //                        and Toshiba TC58NVG2S0HTA00 512MB NAND flash
            if ((tempID[0]==0x98)&&(tempID[2]==0x90)&&(tempID[3]==0x26)&&(tempID[4]==0x76))
            {
                pSM->uBlockPerFlash  = 2047;        // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 64;
                pSM->nPageSize       = NAND_PAGE_4KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = FALSE;
                pSM->bIsMulticycle   = TRUE;
                pSM->bIsNandECC8     = TRUE;

                NDISK_info->NAND_type     = (pSM->bIsMLCNand ? NAND_TYPE_MLC : NAND_TYPE_SLC);
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                NDISK_info->nZone         = 1;      // number of zones
                NDISK_info->nBlockPerZone = pSM->uBlockPerFlash + 1;   // blocks per zone
                NDISK_info->nPagePerBlock = pSM->uPagePerBlock;
                NDISK_info->nPageSize     = pSM->nPageSize;
                NDISK_info->nLBPerZone    = 2000;   // logical blocks per zone

                pSM->uSectorPerFlash = pSM->uSectorPerBlock * NDISK_info->nLBPerZone / 1000 * 999;
                break;
            }

            if ((tempID[3] & 0x33) == 0x11)
            {
                pSM->uBlockPerFlash = 4095;
                pSM->uPagePerBlock = 64;
                pSM->uSectorPerBlock = 256;
                pSM->bIsMLCNand = FALSE;

                NDISK_info->NAND_type = NAND_TYPE_SLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_OUT_SEQ;
                NDISK_info->nZone = 1;              /* number of zones */
                NDISK_info->nPagePerBlock = 64;     /* pages per block */
                NDISK_info->nBlockPerZone = 4096;   /* blocks per zone */
                NDISK_info->nLBPerZone = 4000;      /* logical blocks per zone */
            }
            else if ((tempID[3] & 0x33) == 0x21)
            {
                pSM->uBlockPerFlash = 2047;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 512;
                pSM->bIsMLCNand = TRUE;

                NDISK_info->NAND_type = NAND_TYPE_MLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                NDISK_info->nZone = 1;              /* number of zones */
                NDISK_info->nPagePerBlock = 128;    /* pages per block */
                NDISK_info->nBlockPerZone = 2048;   /* blocks per zone */
                NDISK_info->nLBPerZone = 2000;      /* logical blocks per zone */
            }
            pSM->uSectorPerFlash = 1022976;
            pSM->bIsMulticycle = TRUE;
            pSM->nPageSize = NAND_PAGE_2KB;
            pSM->bIsNandECC8 = TRUE;

            NDISK_info->nPageSize = 2048;

            // 2018/10/29, support MXIC MX30LF4G18AC NAND flash
            if ((tempID[0]==0xC2)&&(tempID[1]==0xDC)&&(tempID[2]==0x90)&&(tempID[3]==0x95)&&(tempID[4]==0x56))
            {
                // The first ID of this NAND is 0xC2 BUT it is NOT NAND ROM (read only)
                // So, we MUST modify the configuration of it
                //      1. change pSM->bIsCheckECC to TRUE to enable ECC feature;
                //      2. assign a fake vendor_ID to make NVTFAT can write data to this NAND disk.
                //         (GNAND will check vendor_ID and set disk to DISK_TYPE_READ_ONLY if it is 0xC2)
                pSM->bIsCheckECC = TRUE;
                NDISK_info->vendor_ID = 0xFF;   // fake vendor_ID
            }
            break;

        case 0xd3:
            // 2014/4/2, To support Samsung K9WAG08U1D 512MB NAND flash
            if ((tempID[0]==0xEC)&&(tempID[2]==0x51)&&(tempID[3]==0x95)&&(tempID[4]==0x58))
            {
                pSM->uBlockPerFlash  = 4095;        // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 64;
                pSM->nPageSize       = NAND_PAGE_2KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = FALSE;
                pSM->bIsMulticycle   = TRUE;
                pSM->bIsNandECC8     = TRUE;
                pSM->uSectorPerFlash = 1022976;

                NDISK_info->NAND_type     = NAND_TYPE_MLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                NDISK_info->nZone         = 1;      // number of zones
                NDISK_info->nBlockPerZone = pSM->uBlockPerFlash + 1;   // blocks per zone
                NDISK_info->nPagePerBlock = pSM->uPagePerBlock;
                NDISK_info->nPageSize     = pSM->nPageSize;
                NDISK_info->nLBPerZone    = 4000;   // logical blocks per zone
                break;
            }

            // 2016/9/29, support MXIC MX60LF8G18AC NAND flash
            if ((tempID[0]==0xC2)&&(tempID[1]==0xD3)&&(tempID[2]==0xD1)&&(tempID[3]==0x95)&&(tempID[4]==0x5A))
            {
                // The first ID of this NAND is 0xC2 BUT it is NOT NAND ROM (read only)
                // So, we MUST modify the configuration of it
                //      1. change pSM->bIsCheckECC to TRUE to enable ECC feature;
                //      2. assign a fake vendor_ID to make NVTFAT can write data to this NAND disk.
                //         (GNAND will check vendor_ID and set disk to DISK_TYPE_READ_ONLY if it is 0xC2)
                pSM->bIsCheckECC = TRUE;
                NDISK_info->vendor_ID = 0xFF;   // fake vendor_ID
            }

            if ((tempID[3] & 0x33) == 0x32)
            {
                pSM->uBlockPerFlash = 2047;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 1024;    /* 128x8 */
                pSM->nPageSize = NAND_PAGE_4KB;
                pSM->bIsMLCNand = TRUE;

                NDISK_info->NAND_type = NAND_TYPE_MLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                NDISK_info->nZone = 1;              /* number of zones */
                NDISK_info->nPagePerBlock = 128;    /* pages per block */
                NDISK_info->nPageSize = 4096;
                NDISK_info->nBlockPerZone = 2048;   /* blocks per zone */
                NDISK_info->nLBPerZone = 2000;      /* logical blocks per zone */
            }
            else if ((tempID[3] & 0x33) == 0x11)
            {
                pSM->uBlockPerFlash = 8191;
                pSM->uPagePerBlock = 64;
                pSM->uSectorPerBlock = 256;
                pSM->nPageSize = NAND_PAGE_2KB;
                pSM->bIsMLCNand = FALSE;

                NDISK_info->NAND_type = NAND_TYPE_SLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_OUT_SEQ;
                NDISK_info->nZone = 1;              /* number of zones */
                NDISK_info->nPagePerBlock = 64;     /* pages per block */
                NDISK_info->nPageSize = 2048;
                NDISK_info->nBlockPerZone = 8192;   /* blocks per zone */
                NDISK_info->nLBPerZone = 8000;      /* logical blocks per zone */
            }
            else if ((tempID[3] & 0x33) == 0x21)
            {
                pSM->uBlockPerFlash = 4095;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 512;
                pSM->nPageSize = NAND_PAGE_2KB;
                pSM->bIsMLCNand = TRUE;

                NDISK_info->NAND_type = NAND_TYPE_MLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                NDISK_info->nZone = 1;              /* number of zones */
                NDISK_info->nPagePerBlock = 128;    /* pages per block */
                NDISK_info->nPageSize = 2048;
                NDISK_info->nBlockPerZone = 4096;   /* blocks per zone */
                NDISK_info->nLBPerZone = 4000;      /* logical blocks per zone */
            }
            else if ((tempID[3] & 0x33) == 0x22)
            {
                pSM->uBlockPerFlash = 4095;
                pSM->uPagePerBlock = 64;
                pSM->uSectorPerBlock = 512; /* 64x8 */
                pSM->nPageSize = NAND_PAGE_4KB;
                pSM->bIsMLCNand = FALSE;

                NDISK_info->NAND_type = NAND_TYPE_SLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_OUT_SEQ;
                NDISK_info->nZone = 1;              /* number of zones */
                NDISK_info->nPagePerBlock = 64;     /* pages per block */
                NDISK_info->nPageSize = 4096;
                NDISK_info->nBlockPerZone = 4096;   /* blocks per zone */
                NDISK_info->nLBPerZone = 4000;      /* logical blocks per zone */
            }

            pSM->uSectorPerFlash = 2045952;
            pSM->bIsMulticycle = TRUE;
            pSM->bIsNandECC8 = TRUE;
            break;

        case 0xd5:  // 2048M

            // 2011/7/28, To support Hynix H27UAG8T2B NAND flash
            if ((tempID[0]==0xAD)&&(tempID[2]==0x94)&&(tempID[3]==0x9A))
            {
                pSM->uBlockPerFlash  = 1023;        // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 256;
                pSM->nPageSize       = NAND_PAGE_8KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = TRUE;
                pSM->bIsMulticycle   = TRUE;
                pSM->bIsNandECC24    = TRUE;

                NDISK_info->NAND_type     = NAND_TYPE_MLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                NDISK_info->nZone         = 1;      // number of zones
                NDISK_info->nBlockPerZone = pSM->uBlockPerFlash + 1;   // blocks per zone
                NDISK_info->nPagePerBlock = pSM->uPagePerBlock;
                NDISK_info->nPageSize     = pSM->nPageSize;
                // why nLBPerZone is not 1024 ? sicSMInit() had reserved %2 blocks for bad block base on this value.
                NDISK_info->nLBPerZone    = 1000;   // logical blocks per zone

                pSM->uSectorPerFlash = 4091904;
                break;
            }

            // 2011/7/28, To support Toshiba TC58NVG4D2FTA00 NAND flash
            if ((tempID[0]==0x98)&&(tempID[2]==0x94)&&(tempID[3]==0x32))
            {
                pSM->uBlockPerFlash  = 2075;        // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 128;
                pSM->nPageSize       = NAND_PAGE_8KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = TRUE;
                pSM->bIsMulticycle   = TRUE;
                pSM->bIsNandECC24    = TRUE;

                NDISK_info->NAND_type     = NAND_TYPE_MLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                NDISK_info->nZone         = 1;      // number of zones
                NDISK_info->nBlockPerZone = pSM->uBlockPerFlash + 1;   // blocks per zone
                NDISK_info->nPagePerBlock = pSM->uPagePerBlock;
                NDISK_info->nPageSize     = pSM->nPageSize;
                // why nLBPerZone is not 2076 ? sicSMInit() had reserved %2 blocks for bad block base on this value.
                NDISK_info->nLBPerZone    = 2000;   // logical blocks per zone

                pSM->uSectorPerFlash = 4091904;
                break;
            }

    #ifdef OPT_SUPPORT_H27UAG8T2A
            if ((tempID[0]==0xAD)&&(tempID[2] == 0x94)&&(tempID[3] == 0x25))
            {
                pSM->uBlockPerFlash = 4095;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 1024;    /* 128x8 */
                pSM->nPageSize = NAND_PAGE_4KB;
                pSM->bIsMLCNand = TRUE;

                NDISK_info->NAND_type = NAND_TYPE_MLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                NDISK_info->nZone = 1;              /* number of zones */
                NDISK_info->nPagePerBlock = 128;    /* pages per block */
                NDISK_info->nPageSize = 4096;
                NDISK_info->nBlockPerZone = 4096;   /* blocks per zone */
                NDISK_info->nLBPerZone = 4000;      /* logical blocks per zone */

                pSM->uSectorPerFlash = 4091904;
                pSM->bIsMulticycle = TRUE;
                pSM->bIsNandECC12 = TRUE;

                pSM->bIsRA224 = 1;
                break;
            }
            else
            {
                if ((tempID[3] & 0x33) == 0x32)
                {
                    pSM->uBlockPerFlash = 4095;
                    pSM->uPagePerBlock = 128;
                    pSM->uSectorPerBlock = 1024;    /* 128x8 */
                    pSM->nPageSize = NAND_PAGE_4KB;
                    pSM->bIsMLCNand = TRUE;

                    NDISK_info->NAND_type = NAND_TYPE_MLC;
                    NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                    NDISK_info->nZone = 1;              /* number of zones */
                    NDISK_info->nPagePerBlock = 128;    /* pages per block */
                    NDISK_info->nPageSize = 4096;
                    NDISK_info->nBlockPerZone = 4096;   /* blocks per zone */
                    NDISK_info->nLBPerZone = 4000;      /* logical blocks per zone */
                }
                else if ((tempID[3] & 0x33) == 0x11)
                {
                    pSM->uBlockPerFlash = 16383;
                    pSM->uPagePerBlock = 64;
                    pSM->uSectorPerBlock = 256;
                    pSM->nPageSize = NAND_PAGE_2KB;
                    pSM->bIsMLCNand = FALSE;

                    NDISK_info->NAND_type = NAND_TYPE_SLC;
                    NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_OUT_SEQ;
                    NDISK_info->nZone = 1;              /* number of zones */
                    NDISK_info->nPagePerBlock = 64;     /* pages per block */
                    NDISK_info->nPageSize = 2048;
                    NDISK_info->nBlockPerZone = 16384;  /* blocks per zone */
                    NDISK_info->nLBPerZone = 16000;     /* logical blocks per zone */
                }
                else if ((tempID[3] & 0x33) == 0x21)
                {
                    pSM->uBlockPerFlash = 8191;
                    pSM->uPagePerBlock = 128;
                    pSM->uSectorPerBlock = 512;
                    pSM->nPageSize = NAND_PAGE_2KB;
                    pSM->bIsMLCNand = TRUE;

                    NDISK_info->NAND_type = NAND_TYPE_MLC;
                    NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                    NDISK_info->nZone = 1;              /* number of zones */
                    NDISK_info->nPagePerBlock = 128;    /* pages per block */
                    NDISK_info->nPageSize = 2048;
                    NDISK_info->nBlockPerZone = 8192;   /* blocks per zone */
                    NDISK_info->nLBPerZone = 8000;      /* logical blocks per zone */
                }

                pSM->uSectorPerFlash = 4091904;
                pSM->bIsMulticycle = TRUE;
                pSM->bIsNandECC8 = TRUE;
                break;
            }
    #else
            if ((tempID[3] & 0x33) == 0x32)
            {
                pSM->uBlockPerFlash = 4095;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 1024;    /* 128x8 */
                pSM->nPageSize = NAND_PAGE_4KB;
                pSM->bIsMLCNand = TRUE;

                NDISK_info->NAND_type = NAND_TYPE_MLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                NDISK_info->nZone = 1;              /* number of zones */
                NDISK_info->nPagePerBlock = 128;    /* pages per block */
                NDISK_info->nPageSize = 4096;
                NDISK_info->nBlockPerZone = 4096;   /* blocks per zone */
                NDISK_info->nLBPerZone = 4000;      /* logical blocks per zone */
            }
            else if ((tempID[3] & 0x33) == 0x11)
            {
                pSM->uBlockPerFlash = 16383;
                pSM->uPagePerBlock = 64;
                pSM->uSectorPerBlock = 256;
                pSM->nPageSize = NAND_PAGE_2KB;
                pSM->bIsMLCNand = FALSE;

                NDISK_info->NAND_type = NAND_TYPE_SLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_OUT_SEQ;
                NDISK_info->nZone = 1;              /* number of zones */
                NDISK_info->nPagePerBlock = 64;     /* pages per block */
                NDISK_info->nPageSize = 2048;
                NDISK_info->nBlockPerZone = 16384;  /* blocks per zone */
                NDISK_info->nLBPerZone = 16000;     /* logical blocks per zone */
            }
            else if ((tempID[3] & 0x33) == 0x21)
            {
                pSM->uBlockPerFlash = 8191;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 512;
                pSM->nPageSize = NAND_PAGE_2KB;
                pSM->bIsMLCNand = TRUE;

                NDISK_info->NAND_type = NAND_TYPE_MLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                NDISK_info->nZone = 1;              /* number of zones */
                NDISK_info->nPagePerBlock = 128;    /* pages per block */
                NDISK_info->nPageSize = 2048;
                NDISK_info->nBlockPerZone = 8192;   /* blocks per zone */
                NDISK_info->nLBPerZone = 8000;      /* logical blocks per zone */
            }

            pSM->uSectorPerFlash = 4091904;
            pSM->bIsMulticycle = TRUE;
            pSM->bIsNandECC8 = TRUE;
            break;
    #endif
        default:
            // 2012/3/8, To support Micron MT29F16G08CBACA NAND flash
            if ((tempID[0]==0x2C)&&(tempID[1]==0x48)&&(tempID[2]==0x04)&&(tempID[3]==0x4A)&&(tempID[4]==0xA5))
            {
                pSM->uBlockPerFlash  = 2047;        // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 256;
                pSM->nPageSize       = NAND_PAGE_4KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = TRUE;
                pSM->bIsMulticycle   = TRUE;
                pSM->bIsNandECC24    = TRUE;

                NDISK_info->NAND_type     = NAND_TYPE_MLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                NDISK_info->nZone         = 1;      // number of zones
                NDISK_info->nBlockPerZone = pSM->uBlockPerFlash + 1;   // blocks per zone
                NDISK_info->nPagePerBlock = pSM->uPagePerBlock;
                NDISK_info->nPageSize     = pSM->nPageSize;
                // why nLBPerZone is not 1024 ? sicSMInit() had reserved %2 blocks for bad block base on this value.
                NDISK_info->nLBPerZone    = 2000;   // logical blocks per zone

                pSM->uSectorPerFlash = 4091904;
                break;
            }
            // 2012/3/27, To support Micron MT29F32G08CBACA NAND flash
            else if ((tempID[0]==0x2C)&&(tempID[1]==0x68)&&(tempID[2]==0x04)&&(tempID[3]==0x4A)&&(tempID[4]==0xA9))
            {
                pSM->uBlockPerFlash  = 4095;        // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 256;
                pSM->nPageSize       = NAND_PAGE_4KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = TRUE;
                pSM->bIsMulticycle   = TRUE;
                pSM->bIsNandECC24    = TRUE;

                NDISK_info->NAND_type     = NAND_TYPE_MLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_IN_SEQ;
                NDISK_info->nZone         = 1;      // number of zones
                NDISK_info->nBlockPerZone = pSM->uBlockPerFlash + 1;   // blocks per zone
                NDISK_info->nPagePerBlock = pSM->uPagePerBlock;
                NDISK_info->nPageSize     = pSM->nPageSize;
                // why nLBPerZone is not 1024 ? sicSMInit() had reserved %2 blocks for bad block base on this value.
                NDISK_info->nLBPerZone    = 4000;   // logical blocks per zone

                pSM->uSectorPerFlash = 8183808; // = pSM->uSectorPerBlock * NDISK_info->nLBPerZone / 1000 * 999
                break;
            }
            // 2013/9/25, To support MXIC MX30LF1208AA NAND flash
            else if ((tempID[0]==0xC2)&&(tempID[1]==0xF0)&&(tempID[2]==0x80)&&(tempID[3]==0x1D))
            {
                pSM->uBlockPerFlash  = 511;         // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 64;
                pSM->nPageSize       = NAND_PAGE_2KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = FALSE;
                pSM->bIsMulticycle   = FALSE;
                pSM->bIsNandECC8     = TRUE;

                NDISK_info->NAND_type     = NAND_TYPE_SLC;
                NDISK_info->write_page_in_seq = NAND_TYPE_PAGE_OUT_SEQ;
                NDISK_info->nZone         = 1;      // number of zones
                NDISK_info->nBlockPerZone = pSM->uBlockPerFlash + 1;    // blocks per zone
                NDISK_info->nPagePerBlock = pSM->uPagePerBlock;
                NDISK_info->nPageSize     = pSM->nPageSize;
                // why nLBPerZone is not 512 ? sicSMInit() had reserved %2 blocks for bad block base on this value.
                NDISK_info->nLBPerZone    = 500;    // logical blocks per zone

                pSM->uSectorPerFlash = pSM->uSectorPerBlock * NDISK_info->nLBPerZone / 1000 * 999;

                // The first ID of this NAND is 0xC2 BUT it is NOT NAND ROM (read only)
                // So, we MUST modify the configuration of it
                //      1. change pSM->bIsCheckECC to TRUE to enable ECC feature;
                //      2. assign a fake vendor_ID to make NVTFAT can write data to this NAND disk.
                //         (GNAND will check vendor_ID and set disk to DISK_TYPE_READ_ONLY if it is 0xC2)
                pSM->bIsCheckECC = TRUE;
                NDISK_info->vendor_ID = 0xFF;   // fake vendor_ID

                break;
            }
            ERR_PRINTF("ERROR: SM ID not support!! [%02x][%02x][%02x][%02x][%02x]\n", tempID[0], tempID[1], tempID[2], tempID[3], tempID[4]);
            return FMI_SM_ID_ERR;
    }

    INF_PRINTF("NAND: Found %s NAND, ID [%02x][%02x][%02x][%02x][%02x], page size %d, BCH T%d\n",
        pSM->bIsMLCNand ? "MLC" : "SLC",
        tempID[0], tempID[1], tempID[2], tempID[3], tempID[4],
        pSM->nPageSize,
        pSM->bIsNandECC4*4 + pSM->bIsNandECC8*8 + pSM->bIsNandECC12*12 + pSM->bIsNandECC15*15 + pSM->bIsNandECC24*24
        );
    return 0;
}


/*-----------------------------------------------------------------------------
 * 2011/8/1, To clear Ready/Busy flag in registers.
 *---------------------------------------------------------------------------*/
VOID fmiSMClearRBflag(FMI_SM_INFO_T *pSM)
{
    if (pSM == pSM0)
    {
        while(!(inpw(REG_SMISR) & SMISR_RB0));
        outpw(REG_SMISR, SMISR_RB0_IF);
    }
    else
    {
        while(!(inpw(REG_SMISR) & SMISR_RB1));
        outpw(REG_SMISR, SMISR_RB1_IF);
    }
}


/*-----------------------------------------------------------------------------
 * To issue command and address to NAND flash chip
 *  to order NAND flash chip to prepare the data at chip side and wait FMI to read actually.
 *  Support small page size 512 bytes only
 *---------------------------------------------------------------------------*/
INT fmiSM2BufferM(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT8 ucColAddr)
{
    int result;

    fmiSMClearRBflag(pSM);  // clear R/B flag

    outpw(REG_SMCMD, 0x00);             // read command
    outpw(REG_SMADDR, ucColAddr);       // CA0 - CA7
    outpw(REG_SMADDR, uSector & 0xff);  // PA0 - PA7
    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((uSector >> 8) & 0xff)|EOA_SM);      // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (uSector >> 8) & 0xff);               // PA8 - PA15
        outpw(REG_SMADDR, ((uSector >> 16) & 0xff)|EOA_SM);     // PA16 - PA17
    }

    result = fmiSMCheckRB(pSM);
    if (result == GNERR_NAND_NOT_FOUND)
        return result;
    if (!result)
        return FMI_SM_RB_ERR;

    return 0;
}


/*-----------------------------------------------------------------------------
 * To issue command and address to NAND flash chip
 *  to order NAND flash chip to prepare the RA data at chip side and wait FMI to read actually.
 *  Support small page size 512 bytes only
 *---------------------------------------------------------------------------*/
INT fmiSM2BufferM_RA(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT8 ucColAddr)
{
    int result;

    fmiSMClearRBflag(pSM);  // clear R/B flag

    outpw(REG_SMCMD, 0x50);                 // Read RA command for 512 page
    outpw(REG_SMADDR, ucColAddr);           // CA0 - CA7
    outpw(REG_SMADDR, uSector & 0xff);      // PA0 - PA7
    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((uSector >> 8) & 0xff)|EOA_SM);      // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (uSector >> 8) & 0xff);               // PA8 - PA15
        outpw(REG_SMADDR, ((uSector >> 16) & 0xff)|EOA_SM);     // PA16 - PA17
    }

    result = fmiSMCheckRB(pSM);
    if (result == GNERR_NAND_NOT_FOUND)
        return result;
    if (!result)
        return FMI_SM_RB_ERR;
    else
        return 0;
}


INT fmiSMECCErrCount(UINT32 ErrSt, BOOL bIsECC8)
{
    unsigned int volatile errCount;

    if ((ErrSt & 0x02) || (ErrSt & 0x200) || (ErrSt & 0x20000) || (ErrSt & 0x2000000))
    {
        ERR_PRINTF("ERROR: uncorrectable!![0x%x]\n", ErrSt);
        return FMI_SM_ECC_ERROR;
    }
    if (ErrSt & 0x01)
    {
        errCount = (ErrSt >> 2) & 0xf;
        DBG_PRINTF("Field %s have %d error!!\n", bIsECC8 ? "5" : "1", errCount);
    }
    if (ErrSt & 0x100)
    {
        errCount = (ErrSt >> 10) & 0xf;
        DBG_PRINTF("Field %s have %d error!!\n", bIsECC8 ? "6" : "2", errCount);
    }
    if (ErrSt & 0x10000)
    {
        errCount = (ErrSt >> 18) & 0xf;
        DBG_PRINTF("Field %s have %d error!!\n", bIsECC8 ? "7" : "3", errCount);
    }
    if (ErrSt & 0x1000000)
    {
        errCount = (ErrSt >> 26) & 0xf;
        DBG_PRINTF("Field %s have %d error!!\n", bIsECC8 ? "8" : "4", errCount);
    }
    return errCount;
}


static VOID fmiSM_CorrectData_BCH(UINT8 ucFieidIndex, UINT8 ucErrorCnt, UINT8* pDAddr)
{
    UINT32 uaData[24], uaAddr[24];
    UINT32 uaErrorData[4];
    UINT8  ii, jj;
    UINT32 uPageSize;
    UINT32 field_len, padding_len, parity_len;
    UINT32 total_field_num;
    UINT8  *smra_index;

    //--- assign some parameters for different BCH and page size
    switch (inpw(REG_SMCSR) & SMCR_BCH_TSEL)
    {
        case BCH_T24:
            field_len   = 1024;
            padding_len = BCH_PADDING_LEN_1024;
            parity_len  = BCH_PARITY_LEN_T24;
            break;
        case BCH_T15:
            field_len   = 512;
            padding_len = BCH_PADDING_LEN_512;
            parity_len  = BCH_PARITY_LEN_T15;
            break;
        case BCH_T12:
            field_len   = 512;
            padding_len = BCH_PADDING_LEN_512;
            parity_len  = BCH_PARITY_LEN_T12;
            break;
        case BCH_T8:
            field_len   = 512;
            padding_len = BCH_PADDING_LEN_512;
            parity_len  = BCH_PARITY_LEN_T8;
            break;
        case BCH_T4:
            field_len   = 512;
            padding_len = BCH_PADDING_LEN_512;
            parity_len  = BCH_PARITY_LEN_T4;
            break;
        default:
            ERR_PRINTF("ERROR: fmiSM_CorrectData_BCH(): invalid SMCR_BCH_TSEL = 0x%08X\n", (UINT32)(inpw(REG_SMCSR) & SMCR_BCH_TSEL));
            return;
    }

    uPageSize = inpw(REG_SMCSR) & SMCR_PSIZE;
    switch (uPageSize)
    {
        case PSIZE_8K:  total_field_num = 8192 / field_len; break;
        case PSIZE_4K:  total_field_num = 4096 / field_len; break;
        case PSIZE_2K:  total_field_num = 2048 / field_len; break;
        case PSIZE_512: total_field_num =  512 / field_len; break;
        default:
            ERR_PRINTF("ERROR: fmiSM_CorrectData_BCH(): invalid SMCR_PSIZE = 0x%08X\n", uPageSize);
            return;
    }

    //--- got valid BCH_ECC_DATAx and parse them to uaData[]
    // got the valid register number of BCH_ECC_DATAx since one register include 4 error bytes
    jj = ucErrorCnt/4;
    jj ++;
    if (jj > 6)
        jj = 6;     // there are 6 BCH_ECC_DATAx registers to support BCH T24

    for(ii=0; ii<jj; ii++)
    {
        uaErrorData[ii] = inpw(REG_BCH_ECC_DATA0 + ii*4);
    }

    for(ii=0; ii<jj; ii++)
    {
        uaData[ii*4+0] = uaErrorData[ii] & 0xff;
        uaData[ii*4+1] = (uaErrorData[ii]>>8) & 0xff;
        uaData[ii*4+2] = (uaErrorData[ii]>>16) & 0xff;
        uaData[ii*4+3] = (uaErrorData[ii]>>24) & 0xff;
    }

    //--- got valid REG_BCH_ECC_ADDRx and parse them to uaAddr[]
    // got the valid register number of REG_BCH_ECC_ADDRx since one register include 2 error addresses
    jj = ucErrorCnt/2;
    jj ++;
    if (jj > 12)
        jj = 12;    // there are 12 REG_BCH_ECC_ADDRx registers to support BCH T24

    for(ii=0; ii<jj; ii++)
    {
        uaAddr[ii*2+0] = inpw(REG_BCH_ECC_ADDR0 + ii*4) & 0x07ff;   // 11 bits for error address
        uaAddr[ii*2+1] = (inpw(REG_BCH_ECC_ADDR0 + ii*4)>>16) & 0x07ff;
    }

    //--- pointer to begin address of field that with data error
    pDAddr += (ucFieidIndex-1) * field_len;

    //--- correct each error bytes
    for(ii=0; ii<ucErrorCnt; ii++)
    {
        // for wrong data in field
        if (uaAddr[ii] < field_len)
        {
            //DBG_PRINTF("BCH error corrected for data: address 0x%08X, data [0x%02X] --> ",
            //    pDAddr+uaAddr[ii], *(pDAddr+uaAddr[ii]));

            *(pDAddr+uaAddr[ii]) ^= uaData[ii];

            //DBG_PRINTF("[0x%02X]\n", *(pDAddr+uaAddr[ii]));
        }
        // for wrong first-3-bytes in redundancy area
        else if (uaAddr[ii] < (field_len+3))
        {
            uaAddr[ii] -= field_len;
            uaAddr[ii] += (parity_len*(ucFieidIndex-1));    // field offset

            //DBG_PRINTF("BCH error corrected for 3 bytes: address 0x%08X, data [0x%02X] --> ",
            //    (UINT8 *)REG_SMRA_0+uaAddr[ii], *((UINT8 *)REG_SMRA_0+uaAddr[ii]));

            *((UINT8 *)REG_SMRA_0+uaAddr[ii]) ^= uaData[ii];

            //DBG_PRINTF("[0x%02X]\n", *((UINT8 *)REG_SMRA_0+uaAddr[ii]));
        }
        // for wrong parity code in redundancy area
        else
        {
            // BCH_ERR_ADDRx = [data in field] + [3 bytes] + [xx] + [parity code]
            //                                   |<--     padding bytes      -->|
            // The BCH_ERR_ADDRx for last parity code always = field size + padding size.
            // So, the first parity code = field size + padding size - parity code length.
            // For example, for BCH T12, the first parity code = 512 + 32 - 23 = 521.
            // That is, error byte address offset within field is
            uaAddr[ii] = uaAddr[ii] - (field_len + padding_len - parity_len);

            // smra_index point to the first parity code of first field in register SMRA0~n
            smra_index = (UINT8 *)
                         (REG_SMRA_0 + (inpw(REG_SMREAREA_CTL) & SMRE_REA128_EXT) - // bottom of all parity code -
                          (parity_len * total_field_num)                            // byte count of all parity code
                         );

            // final address = first parity code of first field +
            //                 offset of fields +
            //                 offset within field
            //DBG_PRINTF("BCH error corrected for parity: address 0x%08X, data [0x%02X] --> ",
            //    smra_index + (parity_len * (ucFieidIndex-1)) + uaAddr[ii],
            //    *((UINT8 *)smra_index + (parity_len * (ucFieidIndex-1)) + uaAddr[ii]));
            *((UINT8 *)smra_index + (parity_len * (ucFieidIndex-1)) + uaAddr[ii]) ^= uaData[ii];
            //DBG_PRINTF("[0x%02X]\n",
            //    *((UINT8 *)smra_index + (parity_len * (ucFieidIndex-1)) + uaAddr[ii]));
        }
    }   // end of for (ii<ucErrorCnt)
}


INT fmiSM_Read_512(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT32 uDAddr)
{
    int volatile ret=0;
    UINT32 uStatus;
    UINT32 uErrorCnt;
    volatile UINT32 uError = 0;

    outpw(REG_DMACSAR, uDAddr);
    ret = fmiSM2BufferM(pSM, uSector, 0);
    if (ret < 0)
        return ret;

#ifdef _SIC_USE_INT_
    _fmi_bIsSMDataReady = FALSE;
#endif  //_SIC_USE_INT_

    outpw(REG_SMISR, SMISR_DMA_IF);                 // clear DMA flag
    outpw(REG_SMISR, SMISR_ECC_FIELD_IF);           // clear ECC_FIELD flag
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DRD_EN);

#ifdef _SIC_USE_INT_
    while(!_fmi_bIsSMDataReady);
#else
    while(1)
    {
        if (!(inpw(REG_SMCSR) & SMCR_DRD_EN))
            break;
    }
#endif

    if (pSM->bIsCheckECC)
    {
        while(1)
        {
            if (inpw(REG_SMISR) & SMISR_ECC_FIELD_IF)
            {
                if (inpw(REG_SMCSR) & BCH_T4)   // BCH_ECC selected
                {
                    uStatus = inpw(REG_SM_ECC_ST0);
                    uStatus &= 0x3f;

                    if ((uStatus & 0x03)==0x01)         // correctable error in 1st field
                    {
                        // 2011/8/17, mask uErrorCnt since Fx_ECNT just has 5 valid bits
                        uErrorCnt = (uStatus >> 2) & 0x1F;
                        fmiSM_CorrectData_BCH(1, uErrorCnt, (UINT8*)uDAddr);
                        DBG_PRINTF("Warning: Field 1 have %d BCH error. Corrected!!\n", uErrorCnt);
                    }
                    else if (((uStatus & 0x03)==0x02) ||
                             ((uStatus & 0x03)==0x03))  // uncorrectable error or ECC error
                    {
                        ERR_PRINTF("ERROR: Field 1 encountered uncorrectable BCH error!!\n");
                        uError = 1;
                    }
                }
                else
                {
                    ERR_PRINTF("ERROR: Wrong BCH setting for page-512 NAND !!\n");
                    uError = 2;
                }
                outpw(REG_SMISR, SMISR_ECC_FIELD_IF);       // clear ECC_FLD_Error
            }

    #ifdef _SIC_USE_INT_
            if (_fmi_bIsSMDataReady)    // waiting for DMAC transfer finish. ISR will update _fmi_bIsSMDataReady.
            {
                if ( !(inpw(REG_SMISR) & SMISR_ECC_FIELD_IF) )
                    break;
            }
    #else
            if (inpw(REG_SMISR) & SMISR_DMA_IF)      // waiting for DMAC transfer finish. DMAC will update SMISR_DMA_IF.
            {
                if ( !(inpw(REG_SMISR) & SMISR_ECC_FIELD_IF) )
                    break;
            }
    #endif  //_SIC_USE_INT_
        }
    }
    else
        outpw(REG_SMISR, SMISR_ECC_FIELD_IF);

    outpw(REG_SMISR, SMISR_DMA_IF);                 // clear DMA flag
    if (uError)
        return -1;

    return 0;
}

/*-----------------------------------------------------------------------------
 * To issue command and address to NAND flash chip
 *  to order NAND flash chip to prepare the receive data at chip side and wait FMI to write actually.
 *  Support small page size 512 bytes only
 *---------------------------------------------------------------------------*/
VOID fmiBuffer2SMM(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT8 ucColAddr)
{
    // set the spare area configuration
    /* write byte 514, 515 as used page */
    outpw(REG_SMRA_0, 0x0000FFFF);
    outpw(REG_SMRA_1, 0xFFFFFFFF);

    // send command
    outpw(REG_SMCMD, 0x80);     // serial data input command
    outpw(REG_SMADDR, ucColAddr);       // CA0 - CA7
    outpw(REG_SMADDR, uSector & 0xff);  // PA0 - PA7
    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((uSector >> 8) & 0xff)|EOA_SM);  // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (uSector >> 8) & 0xff);           // PA8 - PA15
        outpw(REG_SMADDR, ((uSector >> 16) & 0xff)|EOA_SM); // PA16 - PA17
    }
}


INT fmiSM_Write_512(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT32 uSAddr)
{
    int result;

    fmiSMClearRBflag(pSM);  // clear R/B flag
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);

    outpw(REG_DMACSAR, uSAddr);
    fmiBuffer2SMM(pSM, uSector, 0);

#ifdef _SIC_USE_INT_
    _fmi_bIsSMDataReady = FALSE;
    _fmi_bIsSMPRegionDetect = FALSE;
#endif  // _SIC_USE_INT_

    outpw(REG_SMISR, SMISR_DMA_IF);                 // clear DMA flag
    outpw(REG_SMISR, SMISR_ECC_FIELD_IF);           // clear ECC_FIELD flag
    outpw(REG_SMISR, SMISR_PROT_REGION_WR_IF);      // clear Region Protect flag
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DWR_EN);

    while(1)
    {
#ifdef _SIC_USE_INT_
        if (_fmi_bIsSMDataReady)
#else
        if (inpw(REG_SMISR) & SMISR_DMA_IF)         // wait to finish DMAC transfer.
#endif  //_SIC_USE_INT_
            break;
    }

    outpw(REG_SMISR, SMISR_DMA_IF);     // clear DMA flag
    outpw(REG_SMCMD, 0x10);             // auto program command

    result = fmiSMCheckRB(pSM);
    if (result == GNERR_NAND_NOT_FOUND)
        return result;
    if (!result)
        return FMI_SM_RB_ERR;

    //--- check Region Protect result
#ifdef _SIC_USE_INT_
    if (_fmi_bIsSMPRegionDetect)
#else
    if (inpw(REG_SMISR) & SMISR_PROT_REGION_WR_IF)
#endif  //_SIC_USE_INT_
    {
        ERR_PRINTF("ERROR: fmiSM_Write_512(): region write protect detected!!\n");
        outpw(REG_SMISR, SMISR_PROT_REGION_WR_IF);      // clear Region Protect flag
        return FMI_SM_REGION_PROTECT_ERR;
    }

    if (fmiSMCheckStatus(pSM) != 0)
    {
        ERR_PRINTF("ERROR: fmiSM_Write_512(): write data error!!\n");
        return FMI_SM_STATE_ERROR;
    }
    return 0;
}




/*-----------------------------------------------------------------------------
 * 2011/7/28, To issue command and address to NAND flash chip
 *  to order NAND flash chip to prepare the data or RA data at chip side and wait FMI to read actually.
 *  Support large page size 2K / 4K / 8K.
 *  INPUT: ucColAddr = 0 means prepare data from begin of page;
 *                   = <page size> means prepare RA data from begin of spare area.
 *---------------------------------------------------------------------------*/
INT fmiSM2BufferM_large_page(FMI_SM_INFO_T *pSM, UINT32 uPage, UINT32 ucColAddr)
{
    int result;

    fmiSMClearRBflag(pSM);  // clear R/B flag

    outpw(REG_SMCMD, 0x00);     // read command
    outpw(REG_SMADDR, ucColAddr);                   // CA0 - CA7
    outpw(REG_SMADDR, (ucColAddr >> 8) & 0xFF);     // CA8 - CA11
    outpw(REG_SMADDR, uPage & 0xff);                // PA0 - PA7
    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((uPage >> 8) & 0xff)|EOA_SM);    // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (uPage >> 8) & 0xff);             // PA8 - PA15
        outpw(REG_SMADDR, ((uPage >> 16) & 0xff)|EOA_SM);   // PA16 - PA18
    }
    outpw(REG_SMCMD, 0x30);     // read command

    result = fmiSMCheckRB(pSM);
    if (result == GNERR_NAND_NOT_FOUND)
        return result;
    if (!result)
        return FMI_SM_RB_ERR;
    else
        return 0;
}


INT fmiSM_Read_RA(FMI_SM_INFO_T *pSM, UINT32 uPage, UINT32 ucColAddr)
{
    return fmiSM2BufferM_large_page(pSM, uPage, ucColAddr);
}


INT fmiSM_Read_RA_512(FMI_SM_INFO_T *pSM, UINT32 uPage, UINT32 uColumm)
{
    return fmiSM2BufferM_RA(pSM, uPage, uColumm);
}


/*-----------------------------------------------------------------------------
 * Really write data and parity code to 2K/4K/8K page size NAND flash by NAND commands.
 *---------------------------------------------------------------------------*/
INT fmiSM_Write_large_page(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT32 ucColAddr, UINT32 uSAddr)
{
    int result;

    outpw(REG_DMACSAR, uSAddr); // set DMA transfer starting address

    // set the spare area configuration
    /* write byte 2050, 2051 as used page */
    outpw(REG_SMRA_0, 0x0000FFFF);

    fmiSMClearRBflag(pSM);  // clear R/B flag

    // send command
    outpw(REG_SMCMD, 0x80);                     // serial data input command
    outpw(REG_SMADDR, ucColAddr);               // CA0 - CA7
    outpw(REG_SMADDR, (ucColAddr >> 8) & 0x3f); // CA8 - CA12
    outpw(REG_SMADDR, uSector & 0xff);          // PA0 - PA7
    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((uSector >> 8) & 0xff)|EOA_SM);  // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (uSector >> 8) & 0xff);           // PA8 - PA15
        outpw(REG_SMADDR, ((uSector >> 16) & 0xff)|EOA_SM); // PA16 - PA17
    }

#ifdef _SIC_USE_INT_
    _fmi_bIsSMDataReady = FALSE;
    _fmi_bIsSMPRegionDetect = FALSE;
#endif  // _SIC_USE_INT_

    outpw(REG_SMISR, SMISR_DMA_IF);                     // clear DMA flag
    outpw(REG_SMISR, SMISR_ECC_FIELD_IF);               // clear ECC_FIELD flag
    outpw(REG_SMISR, SMISR_PROT_REGION_WR_IF);          // clear Region Protect flag
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);    // auto write redundancy data to NAND after page data written
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DWR_EN);            // begin to write one page data to NAND flash

    while(1)
    {
#ifdef _SIC_USE_INT_
        if (_fmi_bIsSMDataReady)
#else
        if (inpw(REG_SMISR) & SMISR_DMA_IF)         // wait to finish DMAC transfer.
#endif  //_SIC_USE_INT_
            break;
    }

    outpw(REG_SMISR, SMISR_DMA_IF); // clear DMA flag
    outpw(REG_SMCMD, 0x10);         // auto program command

    result = fmiSMCheckRB(pSM);
    if (result == GNERR_NAND_NOT_FOUND)
        return result;
    if (!result)
        return FMI_SM_RB_ERR;

    //--- check Region Protect result
#ifdef _SIC_USE_INT_
    if (_fmi_bIsSMPRegionDetect)
#else
    if (inpw(REG_SMISR) & SMISR_PROT_REGION_WR_IF)
#endif  //_SIC_USE_INT_
    {
        ERR_PRINTF("ERROR: fmiSM_Write_large_page(): region write protect detected!!\n");
        outpw(REG_SMISR, SMISR_PROT_REGION_WR_IF);      // clear Region Protect flag
        return FMI_SM_REGION_PROTECT_ERR;
    }

    if (fmiSMCheckStatus(pSM) != 0)
    {
        ERR_PRINTF("ERROR: fmiSM_Write_large_page(): data error!!\n");
        return FMI_SM_STATE_ERROR;
    }
    return 0;
}

/*-----------------------------------------------------------------------------
 * 1. Write data from RAM address uSAddr to NAND but DON'T generate BCH parity code.
 * 2. Write BCH parity code from register REG_SMRA_x to NAND redundacny area.
 *---------------------------------------------------------------------------*/
INT fmiSM_Write_large_page_ALC(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT32 ucColAddr, UINT32 uSAddr)
{
    int result;

    outpw(REG_DMACSAR, uSAddr); // set DMA transfer starting address

    // set the spare area configuration
    /* write byte 2050, 2051 as used page */
    outpw(REG_SMRA_0, 0x0000FFFF);

    fmiSMClearRBflag(pSM);  // clear R/B flag

    // send command
    outpw(REG_SMCMD, 0x80);                     // serial data input command
    outpw(REG_SMADDR, ucColAddr);               // CA0 - CA7
    outpw(REG_SMADDR, (ucColAddr >> 8) & 0x3f); // CA8 - CA12
    outpw(REG_SMADDR, uSector & 0xff);          // PA0 - PA7
    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((uSector >> 8) & 0xff)|EOA_SM);  // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (uSector >> 8) & 0xff);           // PA8 - PA15
        outpw(REG_SMADDR, ((uSector >> 16) & 0xff)|EOA_SM); // PA16 - PA17
    }

#ifdef _SIC_USE_INT_
    _fmi_bIsSMDataReady = FALSE;
    _fmi_bIsSMPRegionDetect = FALSE;
#endif  // _SIC_USE_INT_

    outpw(REG_SMISR, SMISR_DMA_IF);                 // clear DMA flag
    outpw(REG_SMISR, SMISR_ECC_FIELD_IF);           // clear ECC_FIELD flag
    outpw(REG_SMISR, SMISR_PROT_REGION_WR_IF);          // clear Region Protect flag
    outpw(REG_SMCSR, inpw(REG_SMCSR) & ~SMCR_REDUN_AUTO_WEN);   // DON'T write redundancy data to NAND after page data written
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DWR_EN);            // begin to write one page data to NAND flash

    while(1)
    {
#ifdef _SIC_USE_INT_
        if (_fmi_bIsSMDataReady)
#else
        if (inpw(REG_SMISR) & SMISR_DMA_IF)         // wait to finish DMAC transfer.
#endif  //_SIC_USE_INT_
            break;
    }

    outpw(REG_SMISR, SMISR_DMA_IF);                 // clear DMA flag

    //--- NAND controller don't write RA since SMCR_REDUN_AUTO_WEN is 0.
    //      You can write RA by yourself here.
    {
        int i;
        UINT8 *u8pData;
        int spare_size;

        spare_size = inpw(REG_SMREAREA_CTL) & SMRE_REA128_EXT;
        u8pData = (UINT8 *)REG_SMRA_0;
        for (i=0; i<spare_size; i++)
            outpw(REG_SMDATA, *u8pData++);  // write register SMRAx to NAND
    }

    outpw(REG_SMCMD, 0x10);     // auto program command

    result = fmiSMCheckRB(pSM);
    if (result == GNERR_NAND_NOT_FOUND)
        return result;
    if (!result)
        return FMI_SM_RB_ERR;

    //--- check Region Protect result
#ifdef _SIC_USE_INT_
    if (_fmi_bIsSMPRegionDetect)
#else
    if (inpw(REG_SMISR) & SMISR_PROT_REGION_WR_IF)
#endif  //_SIC_USE_INT_
    {
        ERR_PRINTF("ERROR: fmiSM_Write_large_page_ALC(): region write protect detected!!\n");
        outpw(REG_SMISR, SMISR_PROT_REGION_WR_IF);      // clear Region Protect flag
        return FMI_SM_REGION_PROTECT_ERR;
    }

    if (fmiSMCheckStatus(pSM) != 0)
    {
        ERR_PRINTF("ERROR: fmiSM_Write_large_page_ALC(): write data error!!\n");
        return FMI_SM_STATE_ERROR;
    }
    return 0;
}


/*-----------------------------------------------------------------------------
 * 2011/7/28, To move data from NAND chip side to FMI by DMA,
 *  and then check the ECC error.
 *  Support page size 2K / 4K / 8K.
 *---------------------------------------------------------------------------*/
INT fmiSM_Read_move_data_ecc_check(FMI_SM_INFO_T *pSM, UINT32 uDAddr, UINT32 uPage)
{
    UINT32 uStatus;
    UINT32 uErrorCnt, ii, jj;
    volatile UINT32 uError = 0;
    UINT32 uLoop;

    //--- uLoop is the number of SM_ECC_STx should be check.
    //      One SM_ECC_STx include ECC status for 4 fields.
    //      Field size is 1024 bytes for BCH_T24 and 512 bytes for other BCH.
    //switch (pSM->nPageSize)
    switch (inpw(REG_SMCSR) & SMCR_PSIZE)
    {
        case PSIZE_2K:
            uLoop = 1;
            break;
        case PSIZE_4K:
            if (inpw(REG_SMCSR) & SMCR_BCH_TSEL == BCH_T24)
                uLoop = 1;
            else
                uLoop = 2;
            break;
        case PSIZE_8K:
            if (inpw(REG_SMCSR) & SMCR_BCH_TSEL == BCH_T24)
                uLoop = 2;
            else
                uLoop = 4;
            break;
        default:
            return -1;     // don't work for 512 bytes page
    }

#ifdef _SIC_USE_INT_
    _fmi_bIsSMDataReady = FALSE;
#endif  //_SIC_USE_INT_

    outpw(REG_DMACSAR, uDAddr);                         // set DMA transfer starting address
    outpw(REG_SMISR, SMISR_DMA_IF);                     // clear DMA flag
    outpw(REG_SMISR, SMISR_ECC_FIELD_IF);               // clear ECC_FIELD flag
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DRD_EN);    // begin to move data by DMA

    //--- waiting for DMA transfer stop since complete or ECC error
    // IF no ECC error, DMA transfer complete and make SMCR[DRD_EN]=0
    // IF ECC error, DMA transfer suspend     and make SMISR[ECC_FIELD_IF]=1 but keep keep SMCR[DRD_EN]=1
    //      If we clear SMISR[ECC_FIELD_IF] to 0, DMA transfer will resume.
    // So, we should keep wait if DMA not complete (SMCR[DRD_EN]=1) and no ERR error (SMISR[ECC_FIELD_IF]=0)
//#ifdef _SIC_USE_INT_
//  while((inpw(REG_SMCSR) & SMCR_DRD_EN) && (_fmi_bIsSMECCError==FALSE))
//        ;
//#else
    while((inpw(REG_SMCSR) & SMCR_DRD_EN) && ((inpw(REG_SMISR) & SMISR_ECC_FIELD_IF)==0))
        ;
//#endif

    //--- DMA transfer completed or suspend by ECC error, check and correct ECC error
    if ((pSM->bIsCheckECC) || (inpw(REG_SMCSR)&SMCR_ECC_CHK))
    {
        while(1)
        {
            if (inpw(REG_SMISR) & SMISR_ECC_FIELD_IF)
            {
                for (jj=0; jj<uLoop; jj++)
                {
                    uStatus = inpw(REG_SM_ECC_ST0+jj*4);
                    // DBG_PRINTF("REG_SM_ECC_ST%d = 0x%08x\n", jj, uStatus);
                    if (!uStatus)
                        continue;   // no error on this register for 4 fields
                    // ECC error !! Check 4 fields. Each field has 512 bytes data
                    for (ii=1; ii<5; ii++)
                    {
                        if (!(uStatus & ECCST_F1_STAT))     // no error for this field
                        {
                            uStatus >>= 8;  // next field
                            continue;
                        }

                        if ((uStatus & ECCST_F1_STAT)==0x01)  // correctable error in field (jj*4+ii)
                        {
                            // 2011/8/17, mask uErrorCnt since Fx_ECNT just has 5 valid bits
                            uErrorCnt = (uStatus >> 2) & 0x1F;
                            fmiSM_CorrectData_BCH(jj*4+ii, uErrorCnt, (UINT8*)uDAddr);
//                            DBG_PRINTF("Warning: Field %d have %d BCH error. Corrected!!\n", jj*4+ii, uErrorCnt);
                            ERR_PRINTF("Warning: Page %d Field %d have %d BCH error. Corrected!!\n", uPage, jj*4+ii, uErrorCnt);
                            break;
                        }
                        else if (((uStatus & ECCST_F1_STAT)==0x02) ||
                                 ((uStatus & ECCST_F1_STAT)==0x03)) // uncorrectable error or ECC error in 1st field
                        {
                            ERR_PRINTF("ERROR: Field %d encountered uncorrectable BCH error!!\n", jj*4+ii);
                            uError = 1;
                            break;
                        }
                        uStatus >>= 8;  // next field
                    }
                }
                outpw(REG_SMISR, SMISR_ECC_FIELD_IF);       // clear ECC_FIELD_IF to resume DMA transfer
            }

#ifdef _SIC_USE_INT_
            if (_fmi_bIsSMDataReady)
            {
                if ( !(inpw(REG_SMISR) & SMISR_ECC_FIELD_IF) )
                    break;
            }
#else
            if (inpw(REG_SMISR) & SMISR_DMA_IF)      // wait to finish DMAC transfer.
            {
                if ( !(inpw(REG_SMISR) & SMISR_ECC_FIELD_IF) )
                    break;
            }
#endif
        }   // end of while(1)
    }
    //--- Don't check ECC. Just wait the DMA finish.
    else
    {
        while(1)
        {
            outpw(REG_SMISR, SMISR_ECC_FIELD_IF);
#ifdef _SIC_USE_INT_
            if (_fmi_bIsSMDataReady)
            {
                break;
            }
#else
            if (inpw(REG_SMISR) & SMISR_DMA_IF)
            {
                outpw(REG_SMISR, SMISR_DMA_IF);                 // clear DMA flag
                break;
            }
#endif
        }   // end of while(1)
    }

    if (uError)
        return -1;
    else
        return 0;
}


/*-----------------------------------------------------------------------------
 * 2011/7/28, To support large page NAND flash read function.
 *  support 2K / 4K / 8K page.
 *---------------------------------------------------------------------------*/
INT fmiSM_Read_large_page(FMI_SM_INFO_T *pSM, UINT32 uPage, UINT32 uDAddr)
{
    INT result;

    result = fmiSM2BufferM_large_page(pSM, uPage, 0);
    if (result != 0)
        return result;  // fail for FMI_SM_RB_ERR
    result = fmiSM_Read_move_data_ecc_check(pSM, uDAddr, uPage);
    return result;
}


/*-----------------------------------------------------------------------------
 * 2012/3/2, to check if block is valid or not.
 * The block is GOOD only when all checked bytes are 0xFF as below:
 *      NAND type       check pages     check bytes in spare area
 *      ---------       --------------  -------------------------
 *      SLC 512         1st & 2nd       1st & 6th
 *      SLC 2K/4K/8K    1st & 2nd       1st
 *      MLC 2K/4K/8K    1st & last      1st
 * Return:
 *      0: valid block
 *      1: invalid block
 *---------------------------------------------------------------------------*/
INT fmiCheckInvalidBlock(FMI_SM_INFO_T *pSM, UINT32 BlockNo)
{
    int volatile status=0;
    unsigned int volatile sector;
    unsigned char volatile data512=0xff, data517=0xff, blockStatus=0xff;

    if (BlockNo == 0)
        return 0;

//    if (pSM->bIsMLCNand == TRUE)
//        sector = (BlockNo+1) * pSM->uPagePerBlock - 1;
//    else
//        sector = BlockNo * pSM->uPagePerBlock;

    //--- check first page ...
    sector = BlockNo * pSM->uPagePerBlock;  // first page
    if (pSM->nPageSize == NAND_PAGE_512B)
        status = fmiSM_Read_RA_512(pSM, sector, 0);
    else
        status = fmiSM_Read_RA(pSM, sector, pSM->nPageSize);

    if (status == GNERR_NAND_NOT_FOUND)
        return status;
    if (status < 0)
    {
        ERR_PRINTF("ERROR: fmiCheckInvalidBlock() read fail, for block %d, return 0x%x\n", BlockNo, status);
        return 1;
    }

    // for 512B page size NAND
    if (pSM->nPageSize == NAND_PAGE_512B)
    {
        data512 = inpw(REG_SMDATA) & 0xff;
        data517 = inpw(REG_SMDATA);
        data517 = inpw(REG_SMDATA);
        data517 = inpw(REG_SMDATA);
        data517 = inpw(REG_SMDATA);
        data517 = inpw(REG_SMDATA) & 0xff;
        if ((data512 == 0xFF) && (data517 == 0xFF)) // check byte 1 & 6
        {
            //--- first page PASS; check second page ...
            fmiSM_Reset(pSM);
            status = fmiSM_Read_RA_512(pSM, sector+1, 0);
            if (status == GNERR_NAND_NOT_FOUND)
                return status;
            if (status < 0)
            {
                ERR_PRINTF("ERROR: fmiCheckInvalidBlock() read fail, for block %d, return 0x%x\n", BlockNo, status);
                return 1;
            }
            data512 = inpw(REG_SMDATA) & 0xff;
            data517 = inpw(REG_SMDATA);
            data517 = inpw(REG_SMDATA);
            data517 = inpw(REG_SMDATA);
            data517 = inpw(REG_SMDATA);
            data517 = inpw(REG_SMDATA) & 0xff;
            if ((data512 != 0xFF) || (data517 != 0xFF)) // check byte 1 & 6
            {
                fmiSM_Reset(pSM);
                return 1;   // invalid block
            }
        }
        else
        {
            fmiSM_Reset(pSM);
            return 1;   // invalid block
        }
    }
    // for 2K/4K/8K page size NAND
    else
    {
        blockStatus = inpw(REG_SMDATA) & 0xff;
        if (blockStatus == 0xFF)    // check first byte
        {
            if (pSM->bIsMLCNand == TRUE)
                //--- first page PASS; check last page for MLC
                sector = (BlockNo+1) * pSM->uPagePerBlock - 1;  // last page
            else
                //--- first page PASS; check second page for SLC
                sector++;                                       // second page

            fmiSM_Reset(pSM);
            status = fmiSM_Read_RA(pSM, sector, pSM->nPageSize);
            if (status == GNERR_NAND_NOT_FOUND)
                return status;
            if (status < 0)
            {
                ERR_PRINTF("ERROR: fmiCheckInvalidBlock() read fail, for block %d, return 0x%x\n", BlockNo, status);
                return 1;
            }
            blockStatus = inpw(REG_SMDATA) & 0xff;
            if (blockStatus != 0xFF)    // check first byte
            {
                fmiSM_Reset(pSM);
                return 1;   // invalid block
            }
        }
        else
        {
            fmiSM_Reset(pSM);
            return 1;   // invalid block
        }
    }

    fmiSM_Reset(pSM);
    return 0;   // valid block
}


/*-----------------------------------------------------------------------------
 * Config registers of GPIO and NAND to activate the selected NAND chip.
 *---------------------------------------------------------------------------*/
static void sicSMselect(INT chipSel)
{
    if (chipSel == 0)
    {
        outpw(REG_GPDFUN0, (inpw(REG_GPDFUN0) & (~0xF0F00000)) | 0x20200000);   // enable NRE/RB0 pins
        outpw(REG_GPDFUN1, (inpw(REG_GPDFUN1) & (~0x0000000F)) | 0x00000002);   // enable NWR pins
        outpw(REG_GPEFUN1, (inpw(REG_GPEFUN1) & (~0x000FFF0F)) | 0x00022202);   // enable CS0/ALE/CLE/ND3 pins
        outpw(REG_SMCSR, inpw(REG_SMCSR) & ~SMCR_CS0);
        outpw(REG_SMCSR, inpw(REG_SMCSR) |  SMCR_CS1);
    }
    else if (chipSel == 1)
    {
        outpw(REG_GPDFUN0, (inpw(REG_GPDFUN0) & (~0xFF000000)) | 0x22000000);   // enable NRE/RB1 pins
        outpw(REG_GPDFUN1, (inpw(REG_GPDFUN1) & (~0x0000000F)) | 0x00000002);   // enable NWR pins
        outpw(REG_GPEFUN1, (inpw(REG_GPEFUN1) & (~0x000FFFF0)) | 0x00022220);   // enable CS1/ALE/CLE/ND3 pins
        outpw(REG_SMCSR, inpw(REG_SMCSR) & ~SMCR_CS1);
        outpw(REG_SMCSR, inpw(REG_SMCSR) |  SMCR_CS0);
    }

    //--- 2014/2/26, Reset NAND controller and DMAC to keep clean status for next access.
    // Reset DMAC engine and interrupt satus
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_SWRST | DMAC_EN);
    while(inpw(REG_DMACCSR) & DMAC_SWRST);
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_EN);
    outpw(REG_DMACISR, WEOT_IF | TABORT_IF);    // clear all interrupt flag

    // Reset FMI engine and interrupt status
    outpw(REG_FMICR, FMI_SWRST);
    while(inpw(REG_FMICR) & FMI_SWRST);
    outpw(REG_FMIISR, FMI_DAT_IF);              // clear all interrupt flag

    // Reset NAND engine and interrupt status
    outpw(REG_FMICR, FMI_SM_EN);
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_SM_SWRST);
    while(inpw(REG_SMCSR) & SDCR_SWRST);
    outpw(REG_SMISR, 0xFFFFFFFF);               // clear all interrupt flag
}


/*-----------------------------------------------------------------------------
 * To check if block is valid or not.
 * The checking rule is according to NAND flash factory default setting.
 * Please refer to NAND flash spec that the target board used.
 * Return:
 *      0: valid block
 *      1: invalid block
 *---------------------------------------------------------------------------*/
static INT fmiNormalCheckBlock(FMI_SM_INFO_T *pSM, UINT32 BlockNo)
{
    int volatile status=0;
    unsigned int volatile sector;
    unsigned char data, data517;

    _fmi_pSMBuffer = (UINT8 *)((UINT32)_fmi_ucSMBuffer | 0x80000000);

    /* MLC check the 2048 byte of last page per block */
    if (pSM->bIsMLCNand == TRUE)
    {
        if (pSM->nPageSize == NAND_PAGE_2KB)
        {
            sector = (BlockNo+1) * pSM->uPagePerBlock - 1;
            status = fmiSM_Read_RA(pSM, sector, pSM->nPageSize);
            if (status == GNERR_NAND_NOT_FOUND)
                return status;
            if (status < 0)
            {
                ERR_PRINTF("ERROR: fmiNormalCheckBlock() read fail, for block %d, status 0x%x\n", BlockNo, status);
                return 1;
            }
            data = inpw(REG_SMDATA) & 0xff;
            if (data != 0xFF)
                return 1;   // invalid block
        }
        else if (pSM->nPageSize == NAND_PAGE_4KB)
        {
            sector = (BlockNo+1) * pSM->uPagePerBlock - 1;
            status = fmiSM_Read_RA(pSM, sector, pSM->nPageSize);
            if (status == GNERR_NAND_NOT_FOUND)
                return status;
            if (status < 0)
            {
                ERR_PRINTF("ERROR: fmiNormalCheckBlock() read fail, for block %d, status 0x%x 0x%x\n", BlockNo, status);
                return 1;
            }
            data = inpw(REG_SMDATA) & 0xff;
            if (data != 0xFF)
                return 1;   // invalid block
        }
        // 2011/7/28, according to datasheet of Hynix H27UAG8T2B 8K page MLC NAND flash
        // "Any block where the 1st Byte in the spare area of either the 1st or the last page does not contain FFh is a Bad Block."
        else if (pSM->nPageSize == NAND_PAGE_8KB)
        {
            // check last page
            sector = (BlockNo+1) * pSM->uPagePerBlock - 1;
            status = fmiSM_Read_RA(pSM, sector, pSM->nPageSize);
            if (status == GNERR_NAND_NOT_FOUND)
                return status;
            if (status < 0)
            {
                ERR_PRINTF("ERROR: fmiNormalCheckBlock() read fail, for block %d last page, return 0x%x\n", BlockNo, status);
                return 1;   // invalid block
            }
            data = inpw(REG_SMDATA) & 0xff;
            if (data != 0xFF)
                return 1;   // invalid block

            // check 1st page
            sector = BlockNo * pSM->uPagePerBlock;
            status = fmiSM_Read_RA(pSM, sector, pSM->nPageSize);
            if (status == GNERR_NAND_NOT_FOUND)
                return status;
            if (status < 0)
            {
                ERR_PRINTF("ERROR: fmiNormalCheckBlock() read fail, for block %d first page, return 0x%x\n", BlockNo, status);
                return 1;   // invalid block
            }
            data = inpw(REG_SMDATA) & 0xff;
            if (data != 0xFF)
                return 1;   // invalid block
        }
    }
    /* SLC check the 2048 byte of 1st or 2nd page per block */
    else    // SLC
    {
        sector = BlockNo * pSM->uPagePerBlock;
        if (pSM->nPageSize == NAND_PAGE_4KB)
        {
            status = fmiSM_Read_RA(pSM, sector, 4096);
            if (status == GNERR_NAND_NOT_FOUND)
                return status;
            if (status < 0)
            {
                DBG_PRINTF("fmiNormalCheckBlock 0x%x\n", status);
                return 1;
            }
            data = inpw(REG_SMDATA) & 0xff;
            if (data == 0xFF)
            {
                status = fmiSM_Read_RA(pSM, sector+1, 4096);
                if (status == GNERR_NAND_NOT_FOUND)
                    return status;
                if (status < 0)
                {
                    DBG_PRINTF("fmiNormalCheckBlock 0x%x\n", status);
                    return 1;
                }
                data = inpw(REG_SMDATA) & 0xff;
                if (data != 0xFF)
                {
                    DBG_PRINTF("find bad block is conformed.\n");
                    return 1;   // invalid block
                }
            }
            else
            {
                DBG_PRINTF("find bad block is conformed.\n");
                return 1;   // invalid block
            }
        }
        else if (pSM->nPageSize == NAND_PAGE_2KB)
        {
            status = fmiSM_Read_RA(pSM, sector, 2048);
            if (status == GNERR_NAND_NOT_FOUND)
                return status;
            if (status < 0)
            {
                DBG_PRINTF("fmiNormalCheckBlock 0x%x\n", status);
                return 1;
            }
            data = inpw(REG_SMDATA) & 0xff;
            if (data == 0xFF)
            {
                status = fmiSM_Read_RA(pSM, sector+1, 2048);
                if (status == GNERR_NAND_NOT_FOUND)
                    return status;
                if (status < 0)
                {
                    DBG_PRINTF("fmiNormalCheckBlock 0x%x\n", status);
                    return 1;
                }
                data = inpw(REG_SMDATA) & 0xff;
                if (data != 0xFF)
                {
                    DBG_PRINTF("find bad block is conformed.\n");
                    return 1;   // invalid block
                }
            }
            else
            {
                    DBG_PRINTF("find bad block is conformed.\n");
                    return 1;   // invalid block
            }
        }
        else    /* page size 512B */
        {
            status = fmiSM_Read_RA_512(pSM, sector, 0);
            if (status < 0)
            {
                DBG_PRINTF("fmiNormalCheckBlock 0x%x\n", status);
                return 1;
            }
            data = inpw(REG_SMDATA) & 0xff;
            data517 = inpw(REG_SMDATA);
            data517 = inpw(REG_SMDATA);
            data517 = inpw(REG_SMDATA);
            data517 = inpw(REG_SMDATA);
            data517 = inpw(REG_SMDATA) & 0xff;
            if ((data == 0xFF) && (data517 == 0xFF))
            {
                fmiSM_Reset(pSM);
                status = fmiSM_Read_RA_512(pSM, sector+1, 0);
                if (status < 0)
                {
                    DBG_PRINTF("fmiNormalCheckBlock 0x%x\n", status);
                    return 1;
                }
                data = inpw(REG_SMDATA) & 0xff;
                data517 = inpw(REG_SMDATA);
                data517 = inpw(REG_SMDATA);
                data517 = inpw(REG_SMDATA);
                data517 = inpw(REG_SMDATA);
                data517 = inpw(REG_SMDATA) & 0xff;
                if ((data != 0xFF) || (data517 != 0xFF))
                {
                    fmiSM_Reset(pSM);
                    return 1;   // invalid block
                }
            }
            else
            {
                    DBG_PRINTF("find bad block is conformed.\n");
                    fmiSM_Reset(pSM);
                    return 1;   // invalid block
            }
            fmiSM_Reset(pSM);
        }
    }

    return 0;
}


/* function pointer */
FMI_SM_INFO_T *pSM0=0, *pSM1=0;

static INT sicSMInit(INT chipSel, NDISK_T *NDISK_info)
{
    int status=0, count;

    INF_PRINTF("Initial NAND NonOS Driver (%s) for NAND port %d\n", NAND_DATE_CODE, chipSel);

    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_EN);
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_SWRST);
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) & ~DMAC_SWRST);
    outpw(REG_FMICR, FMI_SM_EN);

    if ((_nand_init0 == 0) && (_nand_init1 == 0))
    {
        // enable SM
        outpw(REG_SMTCR, 0x20305);
        outpw(REG_SMCSR, (inpw(REG_SMCSR) & ~SMCR_PSIZE) | PSIZE_512);
        outpw(REG_SMCSR, inpw(REG_SMCSR) |  SMCR_ECC_3B_PROTECT);
        outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_ECC_CHK);

        /* init SM interface */
#ifdef _NAND_PAR_ALC_
        outpw(REG_SMCSR, inpw(REG_SMCSR) & ~SMCR_REDUN_AUTO_WEN);
        DBG_PRINTF("Parity only written to SM registers but not NAND !!\n");
#else
        outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);
        DBG_PRINTF("Parity written to SM registers and NAND !!\n");
#endif  // _NAND_PAR_ALC_

#ifdef __OPT_NAND_CARD_DETECT_GPD14
        // initial GPIO D14 to input mode for NAND card detect.
        outpw(REG_GPDFUN1, inpw(REG_GPDFUN1) & ~MF_GPD14);      // set GPD14 as GPIO pin
        outpw(REG_GPIOD_PUEN, inpw(REG_GPIOD_PUEN) & ~BIT14);   // disable GPD14 internal pull up resistor
        outpw(REG_GPIOD_OMD, inpw(REG_GPIOD_OMD) & ~BIT14);     // set GPD14 to INPUT mode
#endif
    }

    sicSMselect(chipSel);

    if (chipSel == 0)
    {
        if (_nand_init0)
            return 0;

        pSM0 = malloc(sizeof(FMI_SM_INFO_T));
        if (pSM0 == NULL)
            return FMI_NO_MEMORY;
        memset((char *)pSM0, 0, sizeof(FMI_SM_INFO_T));

        if ((status = fmiSM_ReadID(pSM0, NDISK_info)) < 0)
        {
            if (pSM0 != NULL)
            {
                free(pSM0);
                pSM0 = 0;
            }
            return status;
        }
        fmiSM_Initial(pSM0);

#ifdef OPT_SW_WP
        outpw(REG_GPAFUN0, inpw(REG_GPAFUN0) & ~MF_GPA7);       // port A7 low (WP)
        outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & ~0x0080);  // port A7 low (WP)
        outpw(REG_GPIOA_OMD, inpw(REG_GPIOA_OMD) | 0x0080);     // port A7 output
#endif

#ifdef __OPT_SW_WP_GPA0
        outpw(REG_GPAFUN0, inpw(REG_GPAFUN0) & ~MF_GPA0);       // set GPA0 as GPIO pin
        outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & ~0x0001);  // output 0 to GPA0; initial to Write Protected mode
        outpw(REG_GPIOA_OMD, inpw(REG_GPIOA_OMD) | 0x0001);     // set GPA0 to OUTPUT mode
#endif

        // check NAND boot header
        fmiSMCheckBootHeader(chipSel, pSM0);
        while(1)
        {
            status = fmiNormalCheckBlock(pSM0, pSM0->uLibStartBlock);
            if (status == GNERR_NAND_NOT_FOUND)
                return status;
            if (!status)
                break;
            else
            {
                DBG_PRINTF("invalid start block %d\n", pSM0->uLibStartBlock);
                pSM0->uLibStartBlock++;
                if (pSM0->uLibStartBlock > pSM0->uBlockPerFlash)
                {
                    ERR_PRINTF("ERROR: has no valid blocks to use. Initial CS0 NAND fail !\n");
                    return FMI_SM_INIT_ERROR;
                }
            }
        }
        if (pSM0->bIsCheckECC)
            if (pSM0->uLibStartBlock == 0)
                pSM0->uLibStartBlock++;
        NDISK_info->nStartBlock = pSM0->uLibStartBlock;     /* available start block */
        pSM0->uBlockPerFlash -= pSM0->uLibStartBlock;

        _nand_init0 = 1;
    }
    else if (chipSel == 1)
    {
        if (_nand_init1)
            return 0;

        pSM1 = malloc(sizeof(FMI_SM_INFO_T));
        if (pSM1 == NULL)
            return FMI_NO_MEMORY;
        memset((char *)pSM1, 0, sizeof(FMI_SM_INFO_T));

        if ((status = fmiSM_ReadID(pSM1, NDISK_info)) < 0)
        {
            if (pSM1 != NULL)
            {
                free(pSM1);
                pSM1 = 0;
            }
            return status;
        }
        fmiSM_Initial(pSM1);
#ifdef OPT_SW_WP
        outpw(REG_GPAFUN0, inpw(REG_GPAFUN0) & ~MF_GPA7);       // port A7 low (WP)
        outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & ~0x0080);  // port A7 low (WP)
        outpw(REG_GPIOA_OMD, inpw(REG_GPIOA_OMD) | 0x0080);     // port A7 output
#endif

        // check NAND boot header
        fmiSMCheckBootHeader(chipSel, pSM1);
        while(1)
        {
            status = fmiNormalCheckBlock(pSM1, pSM1->uLibStartBlock);
            if (status == GNERR_NAND_NOT_FOUND)
                return status;
            if (!status)
                break;
            else
            {
                DBG_PRINTF("invalid start block %d\n", pSM1->uLibStartBlock);
                pSM1->uLibStartBlock++;
                if (pSM1->uLibStartBlock > pSM1->uBlockPerFlash)
                {
                    ERR_PRINTF("ERROR: has no valid blocks to use. Initial CS1 NAND fail !\n");
                    return FMI_SM_INIT_ERROR;
                }
            }
        }
        if (pSM1->bIsCheckECC)
            if (pSM1->uLibStartBlock == 0)
                pSM1->uLibStartBlock++;
        NDISK_info->nStartBlock = pSM1->uLibStartBlock;     /* available start block */
        pSM1->uBlockPerFlash -= pSM1->uLibStartBlock;

        _nand_init1 = 1;
    }
    else
        return FMI_SM_INIT_ERROR;

    count = NDISK_info->nBlockPerZone * 2 / 100 + NAND_RESERVED_BLOCK;

    NDISK_info->nBlockPerZone = (NDISK_info->nBlockPerZone * NDISK_info->nZone - NDISK_info->nStartBlock) / NDISK_info->nZone;
    NDISK_info->nLBPerZone = NDISK_info->nBlockPerZone - count;
    NDISK_info->nNandNo = chipSel;

    // default to disable Region Protect by set block 0 and page 0
    sicSMRegionProtect(chipSel, 0, 0);

    return 0;
}


/*-----------------------------------------------------------------------------
 * Set BCH according to the input parametere inIBR.
 *      The BCH of IBR area is different from others.
 * INPUT:
 *      pSM: pointer to data stucture of CS0 or CS1
 *      inIBR: TRUE  to use BCH rule in IBR area
 *             FALSE to sue BCH rule in others
 * OUTPUT:
 *      None
 * RETURN:
 *      None.
 *---------------------------------------------------------------------------*/
void sicSMsetBCH(FMI_SM_INFO_T *pSM, int inIBR)
{
    volatile UINT32 u32PowerOn, powerOnPageSize, powerOnEcc;

    if (inIBR)
    {
        u32PowerOn = inp32(REG_CHIPCFG);
        // CHIPCFG[9:8] : 0=2KB, 1=4KB, 2=8KB, 3=Ignore power-on setting
        powerOnPageSize = (u32PowerOn & (NPAGE)) >> 8;
        // CHIPCFG[11,1] : 0=BCH12, 1=BCH24, 2=BCH15, 3=Ignore BCH
        powerOnEcc = ((u32PowerOn & (BIT11)) >> 10) | ((u32PowerOn & (BIT1)) >> 1);

//        DBG_PRINTF("sicSMsetBCH() set BCH to IBR rule\n");
        // page size 512B: BCH T4, Spare area size 16 bytes
        // page size 2KB:  BCH T4, Spare area size 64 bytes
        // page size 4KB:  BCH T8, Spare area size 128 bytes
        // page size 8K:   BCH T12, Spare area size 376 bytes
        if (pSM->nPageSize == NAND_PAGE_512B)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T4);
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_512));
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 16);
        }
        else if (pSM->nPageSize == NAND_PAGE_2KB)
        {
            if ((powerOnPageSize == 0) && (powerOnEcc == 0))
            {
                // For power-on setting enabled for both 2KB page size and ECC-12.
                outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
                outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T12);
                outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_2K));
                outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 100);
            }
            else
            {
                // Default setting is ECC-4 for 2KB page size.
                outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
                outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T4);
                outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_2K));
                outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 64);
            }
        }
        else if (pSM->nPageSize == NAND_PAGE_4KB)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T8);
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_4K));
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 128);
        }
        else if (pSM->nPageSize == NAND_PAGE_8KB)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T12);            // BCH_12 is selected
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_8K));
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 376);
        }
    }
    else    // not in IBR area
    {
//        DBG_PRINTF("sicSMsetBCH() set BCH to Normal rule\n");
        // page size 512B: BCH T4, Spare area size 16 bytes
        // page size 2KB:  BCH T8, Spare area size 64 bytes
        // page size 4KB:  BCH T8, Spare area size 128 bytes or
        //                 BCH T12, Spare area size 224 bytes or
        //                 BCH T24, Spare area size 216 bytes
        // page size 8K:   BCH T24, Spare area size 376 bytes
        if (pSM->nPageSize == NAND_PAGE_512B)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T4);
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_512));
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 16);
        }
        else if (pSM->nPageSize == NAND_PAGE_2KB)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T8);
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_2K));
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 64);
        }
        else if (pSM->nPageSize == NAND_PAGE_4KB)
        {
            if (pSM->bIsNandECC8 == TRUE)
            {
                outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
                outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T8);
                outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_4K));
                outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 128);
            }
            else if (pSM->bIsNandECC12 == TRUE) // for Hynix H27UAG8T2A
            {
                outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
                outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T12);
                outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_4K));
                outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 224);
            }
            else if (pSM->bIsNandECC24 == TRUE) // for Micron MT29F16G08CBACA and MT29F32G08CBACA
            {
                outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
                outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T24);
                outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_4K));
                outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 216);
            }
        }
        else if (pSM->nPageSize == NAND_PAGE_8KB)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) &  ~SMCR_BCH_TSEL);
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T24);
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_8K));
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 376);
        }
    }
}


INT sicSMpread(INT chipSel, INT PBA, INT page, UINT8 *buff)
{
    FMI_SM_INFO_T *pSM;
    int pageNo;
    int status=0;
    int i;
    char *ptr;
    int spareSize;

    sicSMselect(chipSel);
    if (chipSel == 0)
        pSM = pSM0;
    else
        pSM = pSM1;

    if ((page < 0) || (page > pSM->uPagePerBlock))
        ERR_PRINTF("Warning: sicSMpread() read block %d, page %d ...\n", PBA, page);

    // enable SM
    outpw(REG_FMICR, FMI_SM_EN);
    fmiSM_Initial(pSM);     //removed by mhuko

    PBA += pSM->uLibStartBlock;
    pageNo = PBA * pSM->uPagePerBlock + page;

    //sysprintf("sicSMpread(): uLibStartBlock=%d, pageNo=%d\n", pSM->uLibStartBlock, pageNo);

#ifdef OPT_FIRST_4BLOCKS_ECC4
    if (PBA < pSM->uIBRBlock)
        sicSMsetBCH(pSM, TRUE);
#endif

    //--- read redunancy area to register SMRAx
    spareSize = inpw(REG_SMREAREA_CTL) & SMRE_REA128_EXT;
    ptr = (char *)REG_SMRA_0;
    if (pSM->nPageSize == NAND_PAGE_512B)
    {
        fmiSM_Read_RA_512(pSM, pageNo, 0);
        for (i=0; i<spareSize; i++)
            *ptr++ = inpw(REG_SMDATA) & 0xff;
        status = fmiSM_Read_512(pSM, pageNo, (UINT32)buff);
    }
    else    // for non-512B page, 2K/4K/8K page
    {
        fmiSM_Read_RA(pSM, pageNo, pSM->nPageSize);
        for (i=0; i<spareSize; i++)
            *ptr++ = inpw(REG_SMDATA) & 0xff;                   // copy RA data from NAND to SMRA by SW
        // 2011/8/1, the new API fmiSM_Read_large_page() support 2K/4K/8K page.
        status = fmiSM_Read_large_page(pSM, pageNo, (UINT32)buff);
    }

    if (status)
    {
        ERR_PRINTF("ERROR: Read NAND page fail !!! CS %d, Block %d, page %d, pageNo %d\n", chipSel, PBA, page, pageNo);
    }

#ifdef OPT_FIRST_4BLOCKS_ECC4
    if (PBA < pSM->uIBRBlock)
        sicSMsetBCH(pSM, FALSE);
#endif

    return status;
}


INT sicSMpwrite(INT chipSel, INT PBA, INT page, UINT8 *buff)
{
    FMI_SM_INFO_T *pSM;
    int pageNo;
    int status=0;

    sicSMselect(chipSel);
    if (chipSel == 0)
        pSM = pSM0;
    else
        pSM = pSM1;

    // enable SM
    outpw(REG_FMICR, FMI_SM_EN);
    fmiSM_Initial(pSM);         //removed by mhuko

    PBA += pSM->uLibStartBlock;
    pageNo = PBA * pSM->uPagePerBlock + page;

#ifdef OPT_FIRST_4BLOCKS_ECC4
    if (PBA < pSM->uIBRBlock)
        sicSMsetBCH(pSM, TRUE);
#endif

    if (pSM->nPageSize == NAND_PAGE_512B)
        status = fmiSM_Write_512(pSM, pageNo, (UINT32)buff);
    else    // for non-512B page, 2K/4K/8K page
    {
#ifdef _NAND_PAR_ALC_
        status = fmiSM_Write_large_page_ALC(pSM, pageNo, 0, (UINT32)buff);
#else
        status = fmiSM_Write_large_page(pSM, pageNo, 0, (UINT32)buff);
#endif
    }

    if (status)
    {
        ERR_PRINTF("ERROR: Write NAND page fail !!! Block %d, page %d, pageNo %d\n", PBA, page, pageNo);
    }

#ifdef OPT_FIRST_4BLOCKS_ECC4
    if (PBA < pSM->uIBRBlock)
        sicSMsetBCH(pSM, FALSE);
#endif

    return status;
}


static INT sicSM_is_page_dirty(INT chipSel, INT PBA, INT page)
{
    int result;
    FMI_SM_INFO_T *pSM;
    int pageNo;
    UINT8 data0;
    //UINT8 data1;

    sicSMselect(chipSel);
    if (chipSel == 0)
        pSM = pSM0;
    else
        pSM = pSM1;

    // enable SM
    outpw(REG_FMICR, FMI_SM_EN);
    fmiSM_Initial(pSM);     //removed by mhuko

    PBA += pSM->uLibStartBlock;
    pageNo = PBA * pSM->uPagePerBlock + page;

    // if redundancy area is 0xFF 0xFF 0xFF 0xFF .... --> clean page
    // if redundancy area is 0xFF 0xFF 0x00 0x00 .... --> dirty page
    if (pSM->nPageSize == NAND_PAGE_512B)
        result = fmiSM_Read_RA_512(pSM, pageNo, 2);
    else
        result = fmiSM_Read_RA(pSM, pageNo, pSM->nPageSize+2);   // read (page size + 2) bytes to ignore them
    if (result == GNERR_NAND_NOT_FOUND)
        return result;

    data0 = inpw(REG_SMDATA);   // read 3rd bytes of redundancy area
    //data1 = inpw(REG_SMDATA);   // read 4th bytes of redundancy area

    if (pSM->nPageSize == NAND_PAGE_512B)
        fmiSM_Reset(pSM);

    //mhkuo
//  if ((data0 == 0) && (data1 == 0x00))
//  if ((data0 == 0) || (data1 == 0x00))
    if (data0 == 0x00)
        return 1;   // used page
    else if (data0 != 0xff)
        return 1;   // used page

    return 0;   // un-used page
}


static INT sicSM_is_valid_block(INT chipSel, INT PBA)
{
    int result;
    FMI_SM_INFO_T *pSM;

    sicSMselect(chipSel);
    if (chipSel == 0)
        pSM = pSM0;
    else
        pSM = pSM1;

    PBA += pSM->uLibStartBlock;

    // enable SM
    outpw(REG_FMICR, FMI_SM_EN);
    fmiSM_Initial(pSM);

    result = fmiCheckInvalidBlock(pSM, PBA);
    if (result == GNERR_NAND_NOT_FOUND)
        return result;
    if (result)
    {
        DBG_PRINTF("invalid block %d\n", PBA);
        return 0;
    }
    else
        return 1;   // valid block
}


#ifdef OPT_MARK_BAD_BLOCK_WHILE_ERASE_FAIL

static INT sicSMMarkBadBlock_WhileEraseFail(FMI_SM_INFO_T *pSM, UINT32 BlockNo)
{
    int result;
    UINT32 uSector, ucColAddr;

    /* check if MLC NAND */
    if (pSM->bIsMLCNand == TRUE)
    {
        uSector = (BlockNo+1) * pSM->uPagePerBlock - 1; // write last page
        ucColAddr = pSM->nPageSize;

        // send command
        outpw(REG_SMCMD, 0x80);     // serial data input command
        outpw(REG_SMADDR, ucColAddr);   // CA0 - CA7
        outpw(REG_SMADDR, (ucColAddr >> 8) & 0xff); // CA8 - CA11
        outpw(REG_SMADDR, uSector & 0xff);  // PA0 - PA7
        if (!pSM->bIsMulticycle)
            outpw(REG_SMADDR, ((uSector >> 8) & 0xff)|EOA_SM);      // PA8 - PA15
        else
        {
            outpw(REG_SMADDR, (uSector >> 8) & 0xff);       // PA8 - PA15
            outpw(REG_SMADDR, ((uSector >> 16) & 0xff)|EOA_SM);     // PA16 - PA17
        }
        outpw(REG_SMDATA, 0xf0);    // mark bad block (use 0xf0 instead of 0x00 to differ from Old (Factory) Bad Blcok Mark)
        outpw(REG_SMCMD, 0x10);

        result = fmiSMCheckRB(pSM);
        if (result == GNERR_NAND_NOT_FOUND)
            return result;
        if (!result)
            return FMI_SM_RB_ERR;

        fmiSM_Reset(pSM);
        return 0;
    }
    /* SLC check the 2048 byte of 1st or 2nd page per block */
    else    // SLC
    {
        uSector = BlockNo * pSM->uPagePerBlock;     // write lst page
        /*
        if (pSM->nPageSize == NAND_PAGE_2KB)
        {
            ucColAddr = 2048;       // write 2048th byte
        }
        else if (pSM->nPageSize == NAND_PAGE_4KB)
        {
            ucColAddr = 4096;       // write 4096th byte
        }
        else if (pSM->nPageSize == NAND_PAGE_512B)
        {
            ucColAddr = 0;          // write 4096th byte
            goto _mark_512;
        }
        */
        if (pSM->nPageSize == NAND_PAGE_512B)
        {
            ucColAddr = 0;          // write 4096th byte
            goto _mark_512;
        }
        else
            ucColAddr = pSM->nPageSize;

        // send command
        outpw(REG_SMCMD, 0x80);     // serial data input command
        outpw(REG_SMADDR, ucColAddr);   // CA0 - CA7
        outpw(REG_SMADDR, (ucColAddr >> 8) & 0xff); // CA8 - CA11
        outpw(REG_SMADDR, uSector & 0xff);  // PA0 - PA7
        if (!pSM->bIsMulticycle)
            outpw(REG_SMADDR, ((uSector >> 8) & 0xff)|EOA_SM);      // PA8 - PA15
        else
        {
            outpw(REG_SMADDR, (uSector >> 8) & 0xff);       // PA8 - PA15
            outpw(REG_SMADDR, ((uSector >> 16) & 0xff)|EOA_SM);     // PA16 - PA17
        }
        outpw(REG_SMDATA, 0xf0);    // mark bad block (use 0xf0 instead of 0x00 to differ from Old (Factory) Bad Blcok Mark)
        outpw(REG_SMCMD, 0x10);

        result = fmiSMCheckRB(pSM);
        if (result == GNERR_NAND_NOT_FOUND)
            return result;
        if (!result)
            return FMI_SM_RB_ERR;

        fmiSM_Reset(pSM);
        return 0;

_mark_512:

        outpw(REG_SMCMD, 0x50);     // point to redundant area
        outpw(REG_SMCMD, 0x80);     // serial data input command
        outpw(REG_SMADDR, ucColAddr);   // CA0 - CA7
        outpw(REG_SMADDR, uSector & 0xff);  // PA0 - PA7
        if (!pSM->bIsMulticycle)
            outpw(REG_SMADDR, ((uSector >> 8) & 0xff)|EOA_SM);      // PA8 - PA15
        else
        {
            outpw(REG_SMADDR, (uSector >> 8) & 0xff);       // PA8 - PA15
            outpw(REG_SMADDR, ((uSector >> 16) & 0xff)|EOA_SM);     // PA16 - PA17
        }

        outpw(REG_SMDATA, 0xf0);    // 512
        outpw(REG_SMDATA, 0xff);
        outpw(REG_SMDATA, 0xff);
        outpw(REG_SMDATA, 0xff);
        outpw(REG_SMDATA, 0xf0);    // 516
        outpw(REG_SMDATA, 0xf0);    // 517
        outpw(REG_SMCMD, 0x10);

        result = fmiSMCheckRB(pSM);
        if (result == GNERR_NAND_NOT_FOUND)
            return result;
        if (!result)
            return FMI_SM_RB_ERR;

        fmiSM_Reset(pSM);
        return 0;
    }
}

#endif      // OPT_MARK_BAD_BLOCK_WHILE_ERASE_FAIL

INT sicSMMarkBadBlock(FMI_SM_INFO_T *pSM, UINT32 BlockNo)
{
    int result;
    UINT32 sector, column;

    /* page 0 */
    sector = BlockNo * pSM->uPagePerBlock;
    column = pSM->nPageSize;

    // send command
    outpw(REG_SMCMD, 0x80);     // serial data input command
    outpw(REG_SMADDR, column);                  // CA0 - CA7
    outpw(REG_SMADDR, (column >> 8) & 0x3f);    // CA8 - CA12
    outpw(REG_SMADDR, sector & 0xff);           // PA0 - PA7
    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((sector >> 8) & 0xff)|EOA_SM);   // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (sector >> 8) & 0xff);            // PA8 - PA15
        outpw(REG_SMADDR, ((sector >> 16) & 0xff)|EOA_SM);  // PA16 - PA17
    }

    outpw(REG_SMDATA, 0xf0);    // 512
    outpw(REG_SMDATA, 0xff);
    outpw(REG_SMDATA, 0xff);
    outpw(REG_SMDATA, 0xff);
    outpw(REG_SMDATA, 0xf0);    // 516
    outpw(REG_SMDATA, 0xf0);    // 517
    outpw(REG_SMCMD, 0x10);

    result = fmiSMCheckRB(pSM);
    if (result == GNERR_NAND_NOT_FOUND)
        return result;
    if (!result)
        return FMI_SM_RB_ERR;

    fmiSM_Reset(pSM);

    /* page 1 */
    sector++;
    // send command
    outpw(REG_SMCMD, 0x80);     // serial data input command
    outpw(REG_SMADDR, column);                  // CA0 - CA7
    outpw(REG_SMADDR, (column >> 8) & 0x3f);    // CA8 - CA12
    outpw(REG_SMADDR, sector & 0xff);           // PA0 - PA7
    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((sector >> 8) & 0xff)|EOA_SM);   // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (sector >> 8) & 0xff);            // PA8 - PA15
        outpw(REG_SMADDR, ((sector >> 16) & 0xff)|EOA_SM);  // PA16 - PA17
    }

    outpw(REG_SMDATA, 0xf0);    // 512
    outpw(REG_SMDATA, 0xff);
    outpw(REG_SMDATA, 0xff);
    outpw(REG_SMDATA, 0xff);
    outpw(REG_SMDATA, 0xf0);    // 516
    outpw(REG_SMDATA, 0xf0);    // 517
    outpw(REG_SMCMD, 0x10);

    result = fmiSMCheckRB(pSM);
    if (result == GNERR_NAND_NOT_FOUND)
        return result;
    if (!result)
        return FMI_SM_RB_ERR;

    fmiSM_Reset(pSM);
    return 0;
}


//static INT sicSMblock_erase(INT chipSel, INT PBA)
INT sicSMblock_erase(INT chipSel, INT PBA)
{
    int result;
    FMI_SM_INFO_T *pSM;
    UINT32 page_no;

    sicSMselect(chipSel);
    if (chipSel == 0)
        pSM = pSM0;
    else
        pSM = pSM1;

    PBA += pSM->uLibStartBlock;

    // enable SM
    outpw(REG_FMICR, FMI_SM_EN);
    fmiSM_Initial(pSM);

    result = fmiCheckInvalidBlock(pSM, PBA);
    if (result == GNERR_NAND_NOT_FOUND)
        return result;
    if (result != 1)  // valid block
    {
        page_no = PBA * pSM->uPagePerBlock;     // get page address

        fmiSMClearRBflag(pSM);  // clear R/B flag

        if (inpw(REG_SMISR) & SMISR_ECC_FIELD_IF)
        {
            DBG_PRINTF("erase: error sector !!\n");
            outpw(REG_SMISR, SMISR_ECC_FIELD_IF);
        }

        outpw(REG_SMCMD, 0x60);     // erase setup command

        outpw(REG_SMADDR, (page_no & 0xff));        // PA0 - PA7
        if (!pSM->bIsMulticycle)
            outpw(REG_SMADDR, ((page_no >> 8) & 0xff)|EOA_SM);      // PA8 - PA15
        else
        {
            outpw(REG_SMADDR, ((page_no >> 8) & 0xff));     // PA8 - PA15
            outpw(REG_SMADDR, ((page_no >> 16) & 0xff)|EOA_SM);     // PA16 - PA17
        }

        outpw(REG_SMCMD, 0xd0);     // erase command

        result = fmiSMCheckRB(pSM);
        if (result == GNERR_NAND_NOT_FOUND)
            return result;
        if (!result)
            return FMI_SM_RB_ERR;

        if (fmiSMCheckStatus(pSM) != 0)
        {
            ERR_PRINTF("ERROR: sicSMblock_erase() NAND erase command fail!!\n");

#ifdef OPT_MARK_BAD_BLOCK_WHILE_ERASE_FAIL
            sicSMMarkBadBlock_WhileEraseFail(pSM,PBA);
#endif
            return FMI_SM_STATUS_ERR;
        }
    }
    else
    {
        return FMI_SM_INVALID_BLOCK;
    }

    return 0;
}


/*-----------------------------------------------------------------------------
 * Force to erase a block even if it is a bad block.
 * INPUT:
 *      chipSel: 0 for NAND0 port; 1 for NAND1 port.
 *      PBA: physical block address include reserve area.
 * OUTPUT:
 *      None.
 * RETURN:
 *      0 : erase successfully
 *      -1: erase fail and return error code
 *---------------------------------------------------------------------------*/
INT sicSMblock_erase_test(INT chipSel, INT PBA)
{
    int result;
    FMI_SM_INFO_T *pSM;
    UINT32 page_no;

    sicSMselect(chipSel);
    if (chipSel == 0)
        pSM = pSM0;
    else
        pSM = pSM1;

    // enable SM
    outpw(REG_FMICR, 0x08);
    fmiSM_Initial(pSM);

    page_no = PBA * pSM->uPagePerBlock;     // get page address
    fmiSMClearRBflag(pSM);  // clear R/B flag

    if (inpw(REG_SMISR) & SMISR_ECC_FIELD_IF)
    {
        DBG_PRINTF("erase: error sector !!\n");
        outpw(REG_SMISR, SMISR_ECC_FIELD_IF);
    }

    outpw(REG_SMCMD, 0x60);     // erase setup command

    outpw(REG_SMADDR, (page_no & 0xff));                    // PA0 - PA7
    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((page_no >> 8) & 0xff)|EOA_SM);  // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, ((page_no >> 8) & 0xff));         // PA8 - PA15
        outpw(REG_SMADDR, ((page_no >> 16) & 0xff)|EOA_SM); // PA16 - PA17
    }

    outpw(REG_SMCMD, 0xd0);     // erase command

    result = fmiSMCheckRB(pSM);
    if (result == GNERR_NAND_NOT_FOUND)
        return result;
    if (!result)
        return FMI_SM_RB_ERR;

    if (fmiSMCheckStatus(pSM) != 0)
    {
        ERR_PRINTF("ERROR: sicSMblock_erase_test() NAND erase command fail!!\n");
        return FMI_SM_STATUS_ERR;
    }
    return 0;
}


static INT sicSMchip_erase(INT chipSel)
{
    int i, status=0;
    FMI_SM_INFO_T *pSM;

    sicSMselect(chipSel);
    if (chipSel == 0)
        pSM = pSM0;
    else
        pSM = pSM1;

    // enable SM
    outpw(REG_FMICR, FMI_SM_EN);
    fmiSM_Initial(pSM);

    // erase all chip
    for (i=0; i<=pSM->uBlockPerFlash; i++)
    {
        status = sicSMblock_erase(chipSel, i);
        if (status == GNERR_NAND_NOT_FOUND)
            return status;
        if (status < 0)
        {
            ERR_PRINTF("ERROR: SM block erase fail for block <%d>!!\n", i);
        }
    }
    return 0;
}

/* driver function */
INT nandInit0(NDISK_T *NDISK_info)
{
    return (sicSMInit(0, NDISK_info));
}

INT nandpread0(INT PBA, INT page, UINT8 *buff)
{
    return (sicSMpread(0, PBA, page, buff));
}

INT nandpwrite0(INT PBA, INT page, UINT8 *buff)
{
#ifdef OPT_SW_WP
    int status;
    UINT32 ii;

    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) | 0x0080);   // port A7 high (WP)
    for (ii=0; ii<SW_WP_DELAY_LOOP; ii++);
    status = sicSMpwrite(0, PBA, page, buff);
    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & ~ 0x0080);     // port A7 low (WP)
    return status;

#elif defined __OPT_SW_WP_GPA0
    int status;
    UINT32 ii;

    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) | 0x0001);   // output 1 to GPA0 to non-PROTECTED mode
    for (ii=0; ii<SW_WP_DELAY_LOOP; ii++);
    status = sicSMpwrite(0, PBA, page, buff);
    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & ~0x0001);  // output 0 to GPA0 to PROTECTED mode
    return status;

#else
    return (sicSMpwrite(0, PBA, page, buff));
#endif
}

INT nand_is_page_dirty0(INT PBA, INT page)
{
    return (sicSM_is_page_dirty(0, PBA, page));
}


INT nand_is_valid_block0(INT PBA)
{
    return (sicSM_is_valid_block(0, PBA));
}


INT nand_block_erase0(INT PBA)
{
#ifdef OPT_SW_WP
    int status;
    UINT32 ii;

    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) | 0x0080);   // port A7 high (WP)
    for (ii=0; ii<SW_WP_DELAY_LOOP; ii++);
    status = sicSMblock_erase(0, PBA);
    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & ~ 0x0080);     // port A7 low (WP)
    return status;

#elif defined __OPT_SW_WP_GPA0
    int status;
    UINT32 ii;

    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) | 0x0001);   // output 1 to GPA0 to non-PROTECTED mode
    for (ii=0; ii<SW_WP_DELAY_LOOP; ii++);
    status = sicSMblock_erase(0, PBA);
    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & ~0x0001);  // output 0 to GPA0 to PROTECTED mode
    return status;

#else
    return (sicSMblock_erase(0, PBA));
#endif
}


INT nand_chip_erase0(void)
{
#ifdef OPT_SW_WP
    int status;
    UINT32 ii;

    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) | 0x0080);   // port A7 high (WP)
    for (ii=0; ii<SW_WP_DELAY_LOOP; ii++);
    status = sicSMchip_erase(0);
    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & ~ 0x0080);     // port A7 low (WP)
    return status;

#elif defined __OPT_SW_WP_GPA0
    int status;
    UINT32 ii;

    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) | 0x0001);   // output 1 to GPA0 to non-PROTECTED mode
    for (ii=0; ii<SW_WP_DELAY_LOOP; ii++);
    status = sicSMchip_erase(0);
    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & ~0x0001);  // output 0 to GPA0 to PROTECTED mode
    return status;

#else
    return (sicSMchip_erase(0));
#endif
}


INT nand_ioctl(INT param1, INT param2, INT param3, INT param4)
{
    return 0;
}


/*-----------------------------------------------------------------------------
 * Found the IBR read area size that IBR will read by different BCH rule.
 *      The IBR read area not always 4 blocks any more if we use a NAND flash that IBR don't support it.
 *      We can found the IBR read area size according to power-on setting and IBR's rule.
 * INPUT:
 *      pSM: pointer to data stucture of CS0 or CS1
 * OUTPUT:
 *      None
 * RETURN:
 *      the number of block in IBR read area.
 *---------------------------------------------------------------------------*/
UINT32 fmiSM_GetIBRAreaSize(FMI_SM_INFO_T *pSM)
{
    UINT32 uIBRAreaSize = 0;
    UINT32 u8PowerOn;
    UINT32 u_PagePerBlock = 64;
    UINT32 u_PageSize;

    if (pSM == pSM0)    // for CS0 that should be on board bootable Nand
    {
        u8PowerOn = (inpw(REG_CHIPCFG) & 0x0780 /*(NPAGE|NADDR|NTYPE)*/) >> 7;
        if((u8PowerOn & 0x6) != 0x6)    /* With Power-On-Setting for NAND */
        {
            u8PowerOn = (u8PowerOn >> 1) & 0x3;
            u_PageSize = 1024 << (u8PowerOn + 1);
            // IBR's rule for pages of block
            switch(u8PowerOn)
            {
                case 0:     // 2KB page size with 64 pages of block
                    u_PagePerBlock = 64;
                    break;
                case 1:     // 4KB page size with 128 pages of block
                    u_PagePerBlock = 128;
                    break;
                case 2:     // 8KB page size with 128 pages of block
                    u_PagePerBlock = 128;
                    break;
            }
            uIBRAreaSize = (u_PageSize * u_PagePerBlock * 4) / (pSM->nPageSize * pSM->uPagePerBlock);
        }
        else    // Without Power-On-Setting for NAND
        {
            uIBRAreaSize = 4;   // default is 4 blocks
        }
    }
    else    // for CS1 that should be external Nand Card
    {
        if (pSM->bIsMLCNand)
        {
            // MLC NAND could ask more BCH correct bits, so all blocks don't use IBR BCH rule.
            uIBRAreaSize = 0;
        }
        else
        {
            // Since SLC NAND need less BCH correct bits and for backward compatible (NAND card in other project),
            // so keep first 4 blocks to use IBR BCH rule.
            uIBRAreaSize = 4;
        }
    }

    INF_PRINTF("uIBRAreaSize = %d blocks\n", uIBRAreaSize);
    return uIBRAreaSize;
}


/*-----------------------------------------------------------------------------
 * Check system area and find out the start block of data area.
 * The start block of data area depend on both reserved by TurboWriter and really image file size.
 * INPUT:
 *      chipSel: for CS0 or CS1
 *      pSM: pointer to data stucture of CS0 or CS1
 * OUTPUT:
 *      pSM->uLibStartBlock: block index of the start block of data area.
 * RETURN:
 *      =0 : OK
 *      <0: Fail, reture value of sicSMpread()
 *---------------------------------------------------------------------------*/
INT fmiSMCheckBootHeader(INT chipSel, FMI_SM_INFO_T *pSM)
{
    int fmiNandSysArea=0;
    int volatile status, imageCount, i, block;
    unsigned int *pImageList;
    volatile int ii;
    UINT32 startBlock = 0;

    pSM->uLibStartBlock = 0;
    pSM->uIBRBlock = fmiSM_GetIBRAreaSize(pSM);

    _fmi_pSMBuffer = (UINT8 *)((UINT32)_fmi_ucSMBuffer | 0x80000000);
    pImageList = (UINT32 *)((UINT32)_fmi_ucSMBuffer | 0x80000000);

    /* read physical block 0 last page for system area size that TurboWriter write into. */
    for (ii=0; ii<4; ii++)
    {
        status = sicSMpread(chipSel, ii, pSM->uPagePerBlock-1, _fmi_pSMBuffer);
        if (!status)
        {
            if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
                break;
        }
    }
    if (status < 0)
        return status;  // sicSMpread() fail

    if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
    {
        fmiNandSysArea = *(pImageList+1);   // sectors for system area in NAND that TurboWriter write into.
    }
    DBG_PRINTF("fmiSMCheckBootHeader(): fmiNandSysArea = %d sectors\n", fmiNandSysArea);

    if ((fmiNandSysArea != 0xFFFFFFFF) && (fmiNandSysArea != 0))
    {
        //--- TurboWriter wrote a valid value about system area size into NAND,
        //      got it and convert unit from sector to block.
        startBlock = (fmiNandSysArea / pSM->uSectorPerBlock) + 1;

        //--- Note at 2012/3/14:
        //      startBlock is the block INDEX of first block of data area;
        //      (fmiNandSysArea / pSM->uSectorPerBlock) is the block COUNT of system area;
        //      Since the block index is 0-BASE, the correct value should be
        //          startBlock = (fmiNandSysArea / pSM->uSectorPerBlock) + 0;
        //      NOT + 1.
        //      Howevern, for BACKWARD COMPATIBLE, we should keep this issue and don't modify it.
        //      Please keep in mind that if you reserve n blocks for system area by TurboWriter,
        //          NAND driver will reserve n+1 blocks actually.

        if (fmiNandSysArea % pSM->uSectorPerBlock > 0)
            startBlock++;
        DBG_PRINTF("fmiSMCheckBootHeader(): according to fmiNandSysArea, startBlock = %d\n", startBlock);
    }

    //--- always scan the image table to find out the ending block that all images really used.
    //      If the scan result larger than TurboWriter wrote into, use the larger one.
    /* read physical block 0 second last page for image table */
    for (ii=0; ii<4; ii++)
    {
        // MUST keep pSM->uLibStartBlock to 0 here since sicSMpread() used pSM->uLibStartBlock as block offset.
        status = sicSMpread(chipSel, ii, pSM->uPagePerBlock-2, _fmi_pSMBuffer);
        if (!status)
        {
            if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
                break;
        }
    }
    if (status < 0)
        return status;

    if (((*(pImageList+0)) == 0x574255aa) && ((*(pImageList+3)) == 0x57425963))
    {
        imageCount = *(pImageList+1);

        /* pointer to image information */
        pImageList = pImageList+4;
        for (i=0; i<imageCount; i++)
        {
            block = (*(pImageList + 1) & 0xFFFF0000) >> 16;     // block INDEX of ending block of image file
            block++;                                            // block INDEX of start block of data area
            if (block > startBlock)    // choose the larger one as start block of data area
                startBlock = block;

            /* pointer to next image */
            pImageList = pImageList+12;
        }
    }
    DBG_PRINTF("fmiSMCheckBootHeader(): after scan image information, uLibStartBlock = %d\n", startBlock);

    pSM->uLibStartBlock = startBlock;
    return 0;
}


VOID fmiSMClose(INT chipSel)
{
    if (chipSel == 0)
    {
        _nand_init0 = 0;
        if (pSM0 != 0)
        {
            free(pSM0);
            pSM0 = 0;
        }
    }
    else
    {
        _nand_init1 = 0;
        if (pSM1 != 0)
        {
            free(pSM1);
            pSM1 = 0;
        }
    }

    if ((_nand_init0 == 0) && (_nand_init1 == 0))
    {
        outpw(REG_SMCSR, inpw(REG_SMCSR)|0x06000000);       // disable both CS-0 and CS-1
        outpw(REG_SMISR, 0xfff);                            // clear all SM interrupt flag
        outpw(REG_FMICR, 0x00);                             // disable both SD and SM engine
        outpw(REG_GPDFUN0, inpw(REG_GPDFUN0) & (~0xFFF00000));   // disable NRE/RB0/RB1 pins
        outpw(REG_GPDFUN1, inpw(REG_GPDFUN1) & (~0x0000000F));   // disable NWR pins
        outpw(REG_GPEFUN1, inpw(REG_GPEFUN1) & (~0x000FFFFF));   // disable CS0/ALE/CLE/ND3/CS1 pins
    }
}


INT nandInit1(NDISK_T *NDISK_info)
{
    return (sicSMInit(1, NDISK_info));
}


INT nandpread1(INT PBA, INT page, UINT8 *buff)
{
    return (sicSMpread(1, PBA, page, buff));
}


INT nandpwrite1(INT PBA, INT page, UINT8 *buff)
{
#ifdef OPT_SW_WP
    int status;
    UINT32 ii;

    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) | 0x0080);   // port A7 high (WP)
    for (ii=0; ii<SW_WP_DELAY_LOOP; ii++);
    status = sicSMpwrite(1, PBA, page, buff);
    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & ~ 0x0080);     // port A7 low (WP)
    return status;
#else
    return (sicSMpwrite(1, PBA, page, buff));
#endif
}


INT nand_is_page_dirty1(INT PBA, INT page)
{
    return (sicSM_is_page_dirty(1, PBA, page));
}


INT nand_is_valid_block1(INT PBA)
{
    return (sicSM_is_valid_block(1, PBA));
}


INT nand_block_erase1(INT PBA)
{
#ifdef OPT_SW_WP
    int status;
    UINT32 ii;

    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) | 0x0080);   // port A7 high (WP)
    for (ii=0; ii<SW_WP_DELAY_LOOP; ii++);
    status = sicSMblock_erase(1, PBA);
    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & ~ 0x0080);     // port A7 low (WP)
    return status;
#else
    return (sicSMblock_erase(1, PBA));
#endif
}


INT nand_chip_erase1(void)
{
#ifdef OPT_SW_WP
    int status;
    UINT32 ii;

    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) | 0x0080);   // port A7 high (WP)
    for (ii=0; ii<SW_WP_DELAY_LOOP; ii++);
    status = sicSMchip_erase(1);
    outpw(REG_GPIOA_DOUT, inpw(REG_GPIOA_DOUT) & ~ 0x0080);     // port A7 low (WP)
    return status;
#else
    return (sicSMchip_erase(1));
#endif
}


/*-----------------------------------------------------------------------------
 * Config pSM and register about Region Protect feature.
 * INPUT:
 *      PBA/Page: 0 to disable feature; others to define Region Protect end address
 * OUTPUT:
 *      Assign Region Protect end address to pSM->uRegionProtect
 * RETURN:
 *      0 : OK
 *      -1: Fail since invalid input
 *---------------------------------------------------------------------------*/
INT sicSMRegionProtect(INT chipSel, INT PBA, INT page)
{
    FMI_SM_INFO_T *pSM;
    int pageNo;

    sicSMselect(chipSel);
    if (chipSel == 0)
        pSM = pSM0;
    else
        pSM = pSM1;

    if ((PBA < 0) || (PBA > pSM->uBlockPerFlash) || (page < 0) || (page > pSM->uPagePerBlock))
    {
        ERR_PRINTF("ERROR: sicSMRegionProtect(): Invalid block %d and page %d for Region Protect End Address!!\n", PBA, page);
        return -1;  // fail
    }

    if ((PBA==0) && (page==0))
        pSM->uRegionProtect = 0;    // disable Region Protect
    else
    {
        PBA += pSM->uLibStartBlock;
        pageNo = PBA * pSM->uPagePerBlock + page;
        pSM->uRegionProtect = pageNo;
    }
    fmiSM_Initial(pSM);
    return 0;
}


INT nandRegionProtect0(INT PBA, INT page)
{
    return (sicSMRegionProtect(0, PBA, page));
}


INT nandRegionProtect1(INT PBA, INT page)
{
    return (sicSMRegionProtect(1, PBA, page));
}


/*-----------------------------------------------------------------------------
 * Report the External NAND Card insertion status.
 * INPUT:
 *      None.
 * OUTPUT:
 *      None.
 * RETURN:
 *      0 : Inserted
 *      -1: Removed
 *---------------------------------------------------------------------------*/
INT nand_is_card_inserted()
{
#ifdef __OPT_NAND_CARD_DETECT_GPD14
    int i, status, old_status;

    old_status = inpw(REG_GPIOD_PIN) & 0x00004000;  // read value of GPD14
    // debounce by get same value 3 times
    i = 1;
    while (i < 3)
    {
        status = inpw(REG_GPIOD_PIN) & 0x00004000;
        if (status == old_status)
            i++;
        else
        {
            i = 1;
            old_status = status;
        }
    }

    if (status == 0)
        return 0;   // NAND card inserted if GPD14 = 0
    else
        return -1;  // NAND card removed if GPD14 = 1
#else
    sysprintf("*** Don't support external NAND card detection in this SIC driver !!\n");
    return -1;  // NAND card removed
#endif
}
