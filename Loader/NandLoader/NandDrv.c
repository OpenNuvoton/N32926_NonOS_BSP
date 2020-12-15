/**************************************************************************//**
 * @file     NandDrv.c
 * @brief    NandLoader source code for NAND driver.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "W55FA92_reg.h"
#include "wblib.h"
#include "turbowriter.h"

#define OPT_FIRST_4BLOCKS_ECC4

//#define DBG_PRINTF  sysprintf
#define DBG_PRINTF(...)

#define ERR_PRINTF  sysprintf

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

// return 0 for R/B timeout
// return 1 for Ready
INT fmiSMCheckRB()
{
    int timeout;

    timeout = 0;
    while(1)
    {
        if (inpw(REG_SMISR) & SMISR_RB0_IF)
        {
            outpw(REG_SMISR, SMISR_RB0_IF);
            return 1;
        }

        if (++timeout > 0x80000)
            return 0;
    }
}


// SM functions
INT fmiSM_Reset(void)
{
    UINT32 volatile i;

    outpw(REG_SMISR, SMISR_RB0_IF);
    outpw(REG_SMCMD, 0xff);
    for (i=100; i>0; i--);

    if (!fmiSMCheckRB())
        return -1;
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
    outpw(REG_SMCSR, inpw(REG_SMCSR) & ~SMCR_BCH_TSEL);     // clear BCH select

    //--- Set registers that depend on page size. According to the sepc, the correct order is
    //--- 1. SMCR_BCH_TSEL  : to support T24, MUST set SMCR_BCH_TSEL before SMCR_PSIZE.
    //--- 2. SMCR_PSIZE     : set SMCR_PSIZE will auto change SMRE_REA128_EXT to default value.
    //--- 3. SMRE_REA128_EXT: to use non-default value, MUST set SMRE_REA128_EXT after SMCR_PSIZE.
    if (inIBR)
    {
        DBG_PRINTF("sicSMsetBCH() set BCH to IBR rule\n");
        // page size 512B: BCH T4, Spare area size 16 bytes
        // page size 2KB:  BCH T4, Spare area size 64 bytes
        // page size 4KB:  BCH T8, Spare area size 128 bytes
        // page size 8K:   BCH T12, Spare area size 376 bytes
        if (pSM->nPageSize == NAND_PAGE_512B)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T4);
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_512));
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 16);
        }
        else if (pSM->nPageSize == NAND_PAGE_2KB)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T4);
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_2K));
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 64);
        }
        else if (pSM->nPageSize == NAND_PAGE_4KB)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T8);
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_4K));
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 128);
        }
        else if (pSM->nPageSize == NAND_PAGE_8KB)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T12);            // BCH_12 is selected
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_8K));
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 376);
        }
    }
    else    // not in IBR area
    {
        DBG_PRINTF("sicSMsetBCH() set BCH to Normal rule\n");
        // page size 512B: BCH T4, Spare area size 16 bytes
        // page size 2KB:  BCH T8, Spare area size 64 bytes
        // page size 4KB:  BCH T8, Spare area size 128 bytes or
        //                 BCH T12, Spare area size 224 bytes or
        //                 BCH T24, Spare area size 216 bytes
        // page size 8K:   BCH T24, Spare area size 376 bytes
        if (pSM->nPageSize == NAND_PAGE_512B)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T4);
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_512));
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 16);
        }
        else if (pSM->nPageSize == NAND_PAGE_2KB)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T8);
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_2K));
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 64);
        }
        else if (pSM->nPageSize == NAND_PAGE_4KB)
        {
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_4K));
            if (pSM->bIsNandECC8 == TRUE)
            {
                outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T8);
                //outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_4K));
                outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 128);
            }
            else if (pSM->bIsNandECC12 == TRUE) // for Hynix H27UAG8T2A
            {
                outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T12);
                //outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_4K));
                outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 224);
            }
            else if (pSM->bIsNandECC24 == TRUE) // for Micron MT29F16G08CBACA and MT29F32G08CBACA
            {
                outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T24);
                //outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_4K));
                outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 216);
            }
        }
        else if (pSM->nPageSize == NAND_PAGE_8KB)
        {
            outpw(REG_SMCSR, inpw(REG_SMCSR) | BCH_T24);
            outpw(REG_SMCSR, (inpw(REG_SMCSR)&(~SMCR_PSIZE)) | (PSIZE_8K));
            outpw(REG_SMREAREA_CTL, (inpw(REG_SMREAREA_CTL) & ~SMRE_REA128_EXT) | 376);
        }
    }
}


VOID fmiSM_Initial(FMI_SM_INFO_T *pSM)
{
    sicSMsetBCH(pSM, FALSE);
}


INT fmiSM_ReadID(FMI_SM_INFO_T *pSM)
{
    UINT32 tempID[5];

    fmiSM_Reset();
    outpw(REG_SMCMD, 0x90);     // read ID command
    outpw(REG_SMADDR, EOA_SM);  // address 0x00

    tempID[0] = inpw(REG_SMDATA);
    tempID[1] = inpw(REG_SMDATA);
    tempID[2] = inpw(REG_SMDATA);
    tempID[3] = inpw(REG_SMDATA);
    tempID[4] = inpw(REG_SMDATA);

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
            break;

        case 0x76:  // 64M
            pSM->uSectorPerFlash = 127872;
            pSM->uBlockPerFlash = 4095;
            pSM->uPagePerBlock = 32;
            pSM->uSectorPerBlock = 32;
            pSM->bIsMulticycle = TRUE;
            pSM->nPageSize = NAND_PAGE_512B;
            pSM->bIsNandECC4 = TRUE;
            pSM->bIsMLCNand = FALSE;
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

            // 2013/10/22, support MXIC MX30LF1G08AA NAND flash
            // 2015/06/22, support MXIC MX30LF1G18AC NAND flash
            if ( ((tempID[0]==0xC2)&&(tempID[1]==0xF1)&&(tempID[2]==0x80)&&(tempID[3]==0x1D)) ||
                 ((tempID[0]==0xC2)&&(tempID[1]==0xF1)&&(tempID[2]==0x80)&&(tempID[3]==0x95)&&(tempID[4]==0x02)) )
            {
                // The first ID of this NAND is 0xC2 BUT it is NOT NAND ROM (read only)
                // So, we MUST change pSM->bIsCheckECC to TRUE to enable ECC feature.
                pSM->bIsCheckECC = TRUE;
            }

            break;

        case 0xda:  // 256M
            if ((tempID[3] & 0x33) == 0x11)
            {
                pSM->uBlockPerFlash = 2047;
                pSM->uPagePerBlock = 64;
                pSM->uSectorPerBlock = 256;
                pSM->bIsMLCNand = FALSE;
            }
            else if ((tempID[3] & 0x33) == 0x21)
            {
                pSM->uBlockPerFlash = 1023;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 512;
                pSM->bIsMLCNand = TRUE;
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
                pSM->bIsCheckECC = TRUE;
            }
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
                pSM->uSectorPerFlash = 1022976;
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
                pSM->uSectorPerFlash = 1022976;
                break;
            }

            if ((tempID[3] & 0x33) == 0x11)
            {
                pSM->uBlockPerFlash = 4095;
                pSM->uPagePerBlock = 64;
                pSM->uSectorPerBlock = 256;
                pSM->bIsMLCNand = FALSE;
            }
            else if ((tempID[3] & 0x33) == 0x21)
            {
                pSM->uBlockPerFlash = 2047;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 512;
                pSM->bIsMLCNand = TRUE;
            }

            pSM->uSectorPerFlash = 1022976;
            pSM->bIsMulticycle = TRUE;
            pSM->nPageSize = NAND_PAGE_2KB;
            pSM->bIsNandECC8 = TRUE;

            // 2018/10/29, support MXIC MX30LF4G18AC NAND flash
            if ((tempID[0]==0xC2)&&(tempID[1]==0xDC)&&(tempID[2]==0x90)&&(tempID[3]==0x95)&&(tempID[4]==0x56))
            {
                // The first ID of this NAND is 0xC2 BUT it is NOT NAND ROM (read only)
                // So, we MUST modify the configuration of it
                //      1. change pSM->bIsCheckECC to TRUE to enable ECC feature;
                pSM->bIsCheckECC = TRUE;
            }
            break;

        case 0xd3:  // 1024M
            pSM->bIsMLCNand = FALSE;
            pSM->bIsMulticycle = TRUE;
            pSM->bIsNandECC8 = TRUE;
            pSM->uSectorPerFlash = 2045952;

            // 2014/4/2, To support Samsung K9WAG08U1D 512MB NAND flash
            if ((tempID[0]==0xEC)&&(tempID[2]==0x51)&&(tempID[3]==0x95)&&(tempID[4]==0x58))
            {
                pSM->uBlockPerFlash  = 4095;        // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 64;
                pSM->nPageSize       = NAND_PAGE_2KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->uSectorPerFlash = 1022976;
                break;
            }

            // 2016/9/29, support MXIC MX60LF8G18AC NAND flash
            if ((tempID[0]==0xC2)&&(tempID[2]==0xD1)&&(tempID[3]==0x95)&&(tempID[4]==0x5A))
            {
                // The first ID of this NAND is 0xC2 BUT it is NOT NAND ROM (read only)
                // So, we MUST change pSM->bIsCheckECC to TRUE to enable ECC feature.
                pSM->bIsCheckECC = TRUE;
            }

            if ((tempID[3] & 0x33) == 0x32)
            {
                pSM->uBlockPerFlash = 2047;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 1024;    /* 128x8 */
                pSM->nPageSize = NAND_PAGE_4KB;
                pSM->bIsMLCNand = TRUE;
            }
            else if ((tempID[3] & 0x33) == 0x11)
            {
                pSM->uBlockPerFlash = 8191;
                pSM->uPagePerBlock = 64;
                pSM->uSectorPerBlock = 256;
                pSM->nPageSize = NAND_PAGE_2KB;
                pSM->bIsMLCNand = FALSE;
            }
            else if ((tempID[3] & 0x33) == 0x21)
            {
                pSM->uBlockPerFlash = 4095;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 512;
                pSM->nPageSize = NAND_PAGE_2KB;
                pSM->bIsMLCNand = TRUE;
            }
            else if ((tempID[3] & 0x33) == 0x22)
            {
                pSM->uBlockPerFlash = 4095;
                pSM->uPagePerBlock = 64;
                pSM->uSectorPerBlock = 512; /* 64x8 */
                pSM->nPageSize = NAND_PAGE_4KB;
                pSM->bIsMLCNand = FALSE;
            }
            break;

        case 0xd5:
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

                pSM->uSectorPerFlash = 4091904;
                break;
            }

            if ((tempID[0]==0xAD)&&(tempID[2] == 0x94)&&(tempID[3] == 0x25))
            {
                pSM->uBlockPerFlash = 4095;
                pSM->uPagePerBlock = 128;
                pSM->uSectorPerBlock = 1024;    /* 128x8 */
                pSM->nPageSize = NAND_PAGE_4KB;
                pSM->bIsMLCNand = TRUE;
                pSM->uSectorPerFlash = 4091904;
                pSM->bIsMulticycle = TRUE;
                pSM->bIsNandECC12 = TRUE;
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
                }
                else if ((tempID[3] & 0x33) == 0x11)
                {
                    pSM->uBlockPerFlash = 16383;
                    pSM->uPagePerBlock = 64;
                    pSM->uSectorPerBlock = 256;
                    pSM->nPageSize = NAND_PAGE_2KB;
                    pSM->bIsMLCNand = FALSE;
                }
                else if ((tempID[3] & 0x33) == 0x21)
                {
                    pSM->uBlockPerFlash = 8191;
                    pSM->uPagePerBlock = 128;
                    pSM->uSectorPerBlock = 512;
                    pSM->nPageSize = NAND_PAGE_2KB;
                    pSM->bIsMLCNand = TRUE;
                }

                pSM->uSectorPerFlash = 4091904;
                pSM->bIsMulticycle = TRUE;
                pSM->bIsNandECC8 = TRUE;
                break;
            }

        default:
           // 2012/3/8, To support Micron MT29F16G08CBACA NAND flash
            if ((tempID[0]==0x2C)&&(tempID[1]==0x48)&&(tempID[2]==0x04)&&(tempID[3]==0x4A)&&(tempID[4]==0xA5))
            {
                pSM->uBlockPerFlash  = 2047;    // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 256;
                pSM->nPageSize       = NAND_PAGE_4KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = TRUE;
                pSM->bIsMulticycle   = TRUE;
                pSM->bIsNandECC24    = TRUE;

                pSM->uSectorPerFlash = 4091904;
                break;
            }
           // 2012/3/27, To support Micron MT29F32G08CBACA NAND flash
            else if ((tempID[0]==0x2C)&&(tempID[1]==0x68)&&(tempID[2]==0x04)&&(tempID[3]==0x4A)&&(tempID[4]==0xA9))
            {
                pSM->uBlockPerFlash  = 4095;    // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 256;
                pSM->nPageSize       = NAND_PAGE_4KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = TRUE;
                pSM->bIsMulticycle   = TRUE;
                pSM->bIsNandECC24    = TRUE;

                pSM->uSectorPerFlash = 8183808; // = pSM->uSectorPerBlock * NDISK_info->nLBPerZone / 1000 * 999
                break;
            }
            // 2013/9/25, To support MXIC MX30LF1208AA NAND flash
            else if ((tempID[0]==0xC2)&&(tempID[1]==0xF0)&&(tempID[2]==0x80)&&(tempID[3]==0x1D))
            {
                pSM->uBlockPerFlash  = 511;     // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 64;
                pSM->nPageSize       = NAND_PAGE_2KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = FALSE;
                pSM->bIsMulticycle   = FALSE;
                pSM->bIsNandECC8     = TRUE;
                pSM->uSectorPerFlash = 127872;  // = pSM->uSectorPerBlock * NDISK_info->nLBPerZone / 1000 * 999

                // The first ID of this NAND is 0xC2 BUT it is NOT NAND ROM.
                // So, we MUST change pSM->bIsCheckECC to TRUE.
                pSM->bIsCheckECC = TRUE;

                break;
            }

            ERR_PRINTF("ERROR: SM ID not support!! [%02x][%02x][%02x][%02x][%02x]\n", tempID[0], tempID[1], tempID[2], tempID[3], tempID[4]);
            return -1;
    }

    DBG_PRINTF("SM ID [%02x][%02x][%02x][%02x][%02x]\n", tempID[0], tempID[1], tempID[2], tempID[3], tempID[4]);
    return 0;
}


INT fmiSM2BufferM(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT8 ucColAddr)
{
    /* clear R/B flag */
    while(!(inpw(REG_SMISR) & SMISR_RB0));
    outpw(REG_SMISR, SMISR_RB0_IF);

    outpw(REG_SMCMD, 0x00);     // read command
    outpw(REG_SMADDR, ucColAddr);       // CA0 - CA7
    outpw(REG_SMADDR, uSector & 0xff);  // PA0 - PA7
    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((uSector >> 8) & 0xff)|0x80000000);      // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (uSector >> 8) & 0xff);                   // PA8 - PA15
        outpw(REG_SMADDR, ((uSector >> 16) & 0xff)|0x80000000);     // PA16 - PA17
    }

    if (!fmiSMCheckRB())
        return -1;

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
    /* clear R/B flag */
    while(!(inpw(REG_SMISR) & SMISR_RB0));
    outpw(REG_SMISR, SMISR_RB0_IF);

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

    if (!fmiSMCheckRB())
        return -1 /*FMI_SM_RB_ERR*/;
    else
        return 0;
}


static VOID fmiSM_CorrectData_BCH(UINT8 ucFieidIndex, UINT8 ucErrorCnt, UINT8* pDAddr)
{
    UINT32 uaData[24], uaAddr[24];
    UINT32 uaErrorData[4];
    UINT8  ii, jj;
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
            DBG_PRINTF("ERROR: fmiSM_CorrectData_BCH(): invalid SMCR_BCH_TSEL = 0x%08X\n", (UINT32)(inpw(REG_SMCSR) & SMCR_BCH_TSEL));
            return;
    }

    total_field_num = pSM0->nPageSize / field_len;

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
            *(pDAddr+uaAddr[ii]) ^= uaData[ii];
        }
        // for wrong first-3-bytes in redundancy area
        else if (uaAddr[ii] < (field_len+3))
        {
            uaAddr[ii] -= field_len;
            uaAddr[ii] += (parity_len*(ucFieidIndex-1));    // field offset
            *((UINT8 *)REG_SMRA_0+uaAddr[ii]) ^= uaData[ii];
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
        }
    }   // end of for (ii<ucErrorCnt)
}


/*-----------------------------------------------------------------------------
 * 2011/7/28, To move data from NAND chip side to FMI by DMA,
 *  and then check the ECC error.
 *  Support page size 2K / 4K / 8K.
 *---------------------------------------------------------------------------*/
INT fmiSM_Read_move_data_ecc_check(FMI_SM_INFO_T *pSM, UINT32 uDAddr)
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

    outpw(REG_DMACSAR, uDAddr);                         // set DMA transfer starting address
    outpw(REG_SMISR, SMISR_DMA_IF);                     // clear DMA flag
    outpw(REG_SMISR, SMISR_ECC_FIELD_IF);               // clear ECC_FIELD flag
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DRD_EN);    // begin to move data by DMA

    //--- waiting for DMA transfer stop since complete or ECC error
    // IF no ECC error, DMA transfer complete and make SMCR[DRD_EN]=0
    // IF ECC error, DMA transfer suspend     and make SMISR[ECC_FIELD_IF]=1 but keep keep SMCR[DRD_EN]=1
    //      If we clear SMISR[ECC_FIELD_IF] to 0, DMA transfer will resume.
    // So, we should keep wait if DMA not complete (SMCR[DRD_EN]=1) and no ERR error (SMISR[ECC_FIELD_IF]=0)
    while((inpw(REG_SMCSR) & SMCR_DRD_EN) && ((inpw(REG_SMISR) & SMISR_ECC_FIELD_IF)==0))
        ;

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
                            DBG_PRINTF("Warning: Field %d have %d BCH error. Corrected!!\n", jj*4+ii, uErrorCnt);
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

            if (inpw(REG_SMISR) & SMISR_DMA_IF)      // wait to finish DMAC transfer.
            {
                if ( !(inpw(REG_SMISR) & SMISR_ECC_FIELD_IF) )
                    break;
            }
        }   // end of while(1)
    }
    //--- Don't check ECC. Just wait the DMA finish.
    else
    {
        while(1)
        {
            outpw(REG_SMISR, SMISR_ECC_FIELD_IF);
            if (inpw(REG_SMISR) & SMISR_DMA_IF)
            {
                outpw(REG_SMISR, SMISR_DMA_IF);                 // clear DMA flag
                break;
            }
        }   // end of while(1)
    }

    if (uError)
        return -1;
    else
        return 0;
}


INT fmiSM_Read_512(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT32 uDAddr)
{
    int volatile ret=0;
    volatile UINT32 uStatus, uError;
    UINT32 uErrorCnt;

    outpw(REG_DMACSAR, uDAddr);
    ret = fmiSM2BufferM(pSM, uSector, 0);
    if (ret < 0)
        return ret;

    outpw(REG_SMISR, SMISR_DMA_IF);                 // clear DMA flag
    outpw(REG_SMISR, SMISR_ECC_FIELD_IF);           // clear ECC_FIELD flag
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DRD_EN);

    uError = 0;
    while(1)
    {
        if (!(inpw(REG_SMCSR) & SMCR_DRD_EN))
            break;
    }

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
                        uErrorCnt = uStatus >> 2;
                        fmiSM_CorrectData_BCH(1, uErrorCnt, (UINT8*)uDAddr);
                        DBG_PRINTF("Field 1 have %d error!!\n", uErrorCnt);
                    }
                    else if (((uStatus & 0x03)==0x02)
                          ||((uStatus & 0x03)==0x03))   // uncorrectable error or ECC error
                    {
                        DBG_PRINTF("SM uncorrectable error is encountered, %4x !!\n", uStatus);
                        uError = 1;
                    }
                }
                else
                {
                    DBG_PRINTF("Wrong BCH setting for page-512 NAND !!\n");
                }

                outpw(REG_SMISR, SMISR_ECC_FIELD_IF);       // clear ECC_FLD_Error
            }

            if (inpw(REG_SMISR) & SMISR_DMA_IF)      // wait to finish DMAC transfer.
            {
                if ( !(inpw(REG_SMISR) & SMISR_ECC_FIELD_IF) )
                    break;
            }
        }
    }
    else
        outpw(REG_SMISR, SMISR_ECC_FIELD_IF);

    outpw(REG_SMISR, SMISR_DMA_IF);                 // clear DMA flag
    if (uError)
        return -1;
    return 0;
}


INT fmiSM_Read_RA(FMI_SM_INFO_T *pSM, UINT32 uPage, UINT32 ucColAddr)
{
    return fmiSM2BufferM_large_page(pSM, uPage, ucColAddr);
}


INT fmiSM_Read_RA_512(FMI_SM_INFO_T *pSM, UINT32 uPage, UINT32 uColumm)
{
    /* clear R/B flag */
    while(!(inpw(REG_SMISR) & SMISR_RB0));
    outpw(REG_SMISR, SMISR_RB0_IF);

    outpw(REG_SMCMD, 0x50);     // read command
    outpw(REG_SMADDR, uColumm);
    outpw(REG_SMADDR, uPage & 0xff);    // PA0 - PA7
    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((uPage >> 8) & 0xff)|EOA_SM);        // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (uPage >> 8) & 0xff);     // PA8 - PA15
        outpw(REG_SMADDR, ((uPage >> 16) & 0xff)|EOA_SM);       // PA16 - PA17
    }

    if (!fmiSMCheckRB())
        return -1;

    return 0;
}


/* function pointer */
BOOL volatile bIsNandInit = FALSE;
FMI_SM_INFO_T SMInfo, *pSM0, *pSM1;

volatile UINT32 systemAreaSize = 0;     // the system area size that IBR regarded as.
volatile UINT8  u8PowerOn;
volatile UINT32 gu_fmiSM_PagePerBlock;
volatile UINT32 gu_fmiSM_PageSize;

INT sicSMInit()
{
    if (!bIsNandInit)
    {
        // enable SM
        outp32(REG_FMICR, FMI_SM_EN);

        // select CS port 0
        outp32(REG_GPDFUN0, (inp32(REG_GPDFUN0) & (~0xF0F00000)) | 0x20200000);   // enable NRE/RB0 pins
        outp32(REG_GPDFUN1, (inp32(REG_GPDFUN1) & (~0x0000000F)) | 0x00000002);   // enable NWR pins
        outp32(REG_GPEFUN1, (inp32(REG_GPEFUN1) & (~0x000FFF0F)) | 0x00022202);   // enable CS0/ALE/CLE/ND3 pins
        //outp32(REG_SMCSR, inp32(REG_SMCSR) & (~SMCR_CS0));
        //outp32(REG_SMCSR, inp32(REG_SMCSR) |  SMCR_CS1);
        outp32(REG_SMCSR, (inp32(REG_SMCSR) & (~SMCR_CS0)) | SMCR_CS1);

        /* init SM interface */
        outp32(REG_SMCSR, inp32(REG_SMCSR) | SMCR_SM_SWRST);
        outp32(REG_SMCSR, inp32(REG_SMCSR) | SMCR_ECC_EN | SMCR_ECC_CHK | SMCR_REDUN_AUTO_WEN | SMCR_ECC_3B_PROTECT); /// enable ECC/ECC_CHK/Auto_Write/3B_PROTECT

        outp32(REG_SMREAREA_CTL, inp32(REG_SMREAREA_CTL) & ~SMRE_MECC); // disable to mask ECC parithenable BCH ECC algorithm

        /* set timing */
        outpw(REG_SMTCR, 0x20305);

        memset((char *)&SMInfo, 0, sizeof(FMI_SM_INFO_T));
        pSM0 = &SMInfo;

        if (fmiSM_ReadID(pSM0) < 0)
            return -1;
        fmiSM_Initial(pSM0);
        bIsNandInit = TRUE;

        // 2012/3/30, found the system area size that IBR regarded as.
        // The system area not always 4 blocks any more if we use a NAND flash that IBR don't support it.
        // In this case, we can found the system area size according to power-on setting and IBR's rule.
        u8PowerOn = (inp32(REG_CHIPCFG) & (NPAGE|NADDR|NTYPE)) >> 7;
        if((u8PowerOn & 0x6) != 0x6)    /* With Power-On-Setting for NAND */
        {
            u8PowerOn = (u8PowerOn >> 1) & 0x3;
            gu_fmiSM_PageSize = 1024 << (u8PowerOn + 1);
            // IBR's rule for pages of block
            switch(u8PowerOn)
            {
                case 0:     // 2KB page size with 64 pages of block
                    gu_fmiSM_PagePerBlock = 64;
                    break;
                case 1:     // 4KB page size with 128 pages of block
                case 2:     // 8KB page size with 128 pages of block
                    gu_fmiSM_PagePerBlock = 128;
                    break;
            }
            systemAreaSize = (gu_fmiSM_PageSize * gu_fmiSM_PagePerBlock * 4) / (pSM0->nPageSize * pSM0->uPagePerBlock);
        }
        else    /* Without Power-On-Setting for NAND */
        {
            systemAreaSize = 4;
        }
        DBG_PRINTF("systemAreaSize = %d blocks\n", systemAreaSize);
    }
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
    result = fmiSM_Read_move_data_ecc_check(pSM, uDAddr);
    return result;
}


INT sicSMpread(INT chipSel, INT PBA, INT page, UINT8 *buff)
{
    FMI_SM_INFO_T *pSM;
    int pageNo;
    int status=0;
    int i;
    char *ptr;
    int spareSize;

    // select CS port 0
    pSM = pSM0;

    // enable SM
    outpw(REG_FMICR, FMI_SM_EN);

//    PBA += pSM->uLibStartBlock;     // pSM->uLibStartBlock is always 0 since no check boot header by fmiSMCheckBootHeader() in NANDLoader.
    pageNo = PBA * pSM->uPagePerBlock + page;

#ifdef OPT_FIRST_4BLOCKS_ECC4
    if (PBA < systemAreaSize)
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
        DBG_PRINTF("read NAND page fail !!!\n");
    }

#ifdef OPT_FIRST_4BLOCKS_ECC4
    if (PBA < systemAreaSize)
        sicSMsetBCH(pSM, FALSE);
#endif
    return status;
}


VOID fmiInitDevice()
{
    // Enable NAND Card Host Controller operation and driving clock.
    outpw(REG_AHBCLK, inp32(REG_AHBCLK) | SIC_CKE | NAND_CKE & (~SD_CKE));  // enable NAND engine clock
    //outpw(REG_AHBCLK, inp32(REG_AHBCLK) & ~SD_CKE);             // disable SD engine clock

    // DMAC Initial
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_EN);
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_SWRST);

    outpw(REG_FMICR, FMI_SWRST);    // reset FMI engine
}


/*-----------------------------------------------------------------------------
 * To check if block is valid or not.
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
INT CheckBadBlockMark(UINT32 BlockNo)
{
    int volatile status=0;
    unsigned int volatile sector;
    unsigned char volatile data512=0xff, data517=0xff, blockStatus=0xff;

    FMI_SM_INFO_T *pSM;

    if (BlockNo == 0)
        return 0;

    pSM = pSM0;

    //--- check first page ...
    sector = BlockNo * pSM->uPagePerBlock;  // first page
    if (pSM->nPageSize == NAND_PAGE_512B)
        status = fmiSM_Read_RA_512(pSM, sector, 0);
    else
        status = fmiSM_Read_RA(pSM, sector, pSM->nPageSize);

    if (status < 0)
    {
        sysprintf("ERROR: CheckBadBlockMark() read fail, for block %d, return 0x%x\n", BlockNo, status);
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
            fmiSM_Reset();
            status = fmiSM_Read_RA_512(pSM, sector+1, 0);
            if (status < 0)
            {
                sysprintf("ERROR: CheckBadBlockMark() read fail, for block %d, return 0x%x\n", BlockNo, status);
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
                fmiSM_Reset();
                return 1;   // invalid block
            }
        }
        else
        {
            fmiSM_Reset();
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

            fmiSM_Reset();
            status = fmiSM_Read_RA(pSM, sector, pSM->nPageSize);
            if (status < 0)
            {
                sysprintf("ERROR: CheckBadBlockMark() read fail, for block %d, return 0x%x\n", BlockNo, status);
                return 1;
            }
            blockStatus = inpw(REG_SMDATA) & 0xff;
            if (blockStatus != 0xFF)    // check first byte
            {
                fmiSM_Reset();
                return 1;   // invalid block
            }
        }
        else
        {
            fmiSM_Reset();
            return 1;   // invalid block
        }
    }

    fmiSM_Reset();
    return 0;   // valid block
}
