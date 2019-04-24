/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2008 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>

#include "w55fa92_reg.h"
#include "wblib.h"
#include "w55fa92_sic.h"
#include "nvtfat.h"
#include "Font.h"
#include "writer.h"

extern void sicSMsetBCH(FMI_SM_INFO_T *pSM, int inIBR);
extern UINT32 fmiSM_GetIBRAreaSize(FMI_SM_INFO_T *pSM);
extern VOID fmiSM_Initial(FMI_SM_INFO_T *pSM);
extern INT fmiSM_Reset(FMI_SM_INFO_T *pSM);
extern INT fmiSMCheckRB(FMI_SM_INFO_T *pSM);
extern INT fmiSMCheckStatus(FMI_SM_INFO_T *pSM);
//extern UINT32 NAND_BACKUP_BASE;

#define OPT_MARK_BAD_BLOCK_WHILE_ERASE_FAIL

INT8 nIsSysImage;
int volatile gMarkCount=0;
UINT8 *pInfo;
UINT32 gNandBackupAddress=0;
UINT32 gNandBackupAddressTmp=0;
int volatile g_u32ExtraDataSize;
BOOL volatile gbSystemImage;
extern INT32 gNandLoaderSize;

#define NAND_EXTRA_512      16
#define NAND_EXTRA_2K       64
#define NAND_EXTRA_4K       128
#define NAND_EXTRA_8K       376

#define PRINTF  sysprintf

// SM functions
/*-----------------------------------------------------------------------------
 * Read NAND chip ID from chip and then set pSM and NDISK by chip ID.
 *---------------------------------------------------------------------------*/
INT nvtSM_ReadID(FMI_SM_INFO_T *pSM)
{
    UINT32 tempID[5];

    fmiSM_Reset(pSM);
    outpw(REG_SMCMD, 0x90);     // read ID command
    outpw(REG_SMADDR, EOA_SM);  // address 0x00

// 2011/12/13, support Hynix H27UAG8T2A that with larger redundancy area size 224.
//      fmiSM_Initial() will set Redundancy Area size to 224 for 4K page NAND if pSM->bIsRA224 is TRUE.
//      Else, the default Redundancy Area size for other 4K page NAND is 216.
    pSM->bIsRA224 = 0;

    tempID[0] = inpw(REG_SMDATA);
    tempID[1] = inpw(REG_SMDATA);
    tempID[2] = inpw(REG_SMDATA);
    tempID[3] = inpw(REG_SMDATA);
    tempID[4] = inpw(REG_SMDATA);

    if (tempID[0] == 0xC2)
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
        case 0x5A:
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
            pSM->uSectorPerFlash = 31968;
            pSM->uBlockPerFlash = 1023;
            pSM->uPagePerBlock = 32;
            pSM->uSectorPerBlock = 32;
            pSM->bIsMulticycle = FALSE;
            pSM->nPageSize = NAND_PAGE_512B;
            pSM->bIsNandECC4 = TRUE;
            pSM->bIsMLCNand = FALSE;
            break;

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
            break;

        case 0xdc:  // 512M
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
                pSM->uSectorPerBlock = 1024;
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
                pSM->uSectorPerBlock = 512;
                pSM->nPageSize = NAND_PAGE_4KB;
                pSM->bIsMLCNand = FALSE;
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
                pSM->uSectorPerBlock = 1024;
                pSM->nPageSize = NAND_PAGE_4KB;
                pSM->bIsMLCNand = TRUE;

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
                    pSM->uSectorPerBlock = 1024;
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
                pSM->uBlockPerFlash  = 2047;        // block index with 0-base. = physical blocks - 1
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
                pSM->uBlockPerFlash  = 4095;        // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 256;
                pSM->nPageSize       = NAND_PAGE_4KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = TRUE;
                pSM->bIsMulticycle   = TRUE;
                pSM->bIsNandECC24    = TRUE;

                pSM->uSectorPerFlash = 8183808;
                break;
            }

            // 2013/9/25, support MXIC MX30LF1208AA NAND flash
            else if ((tempID[0]==0xC2)&&(tempID[1]==0xF0)&&(tempID[2]==0x80)&&(tempID[3]==0x1D))
            {
                pSM->uBlockPerFlash  = 511;         // block index with 0-base. = physical blocks - 1
                pSM->uPagePerBlock   = 64;
                pSM->nPageSize       = NAND_PAGE_2KB;
                pSM->uSectorPerBlock = pSM->nPageSize / 512 * pSM->uPagePerBlock;
                pSM->bIsMLCNand      = FALSE;
                pSM->bIsMulticycle   = FALSE;
                pSM->bIsNandECC8     = TRUE;
                pSM->uSectorPerFlash = 127872;      // pSM->uSectorPerBlock * 500 / 1000 * 999;

                // The first ID of this NAND is 0xC2 BUT it is NOT NAND ROM (read only)
                // So, we MUST change pSM->bIsCheckECC to TRUE to enable ECC feature.
                pSM->bIsCheckECC = TRUE;

                break;
            }

            PRINTF("ERROR: SM ID not support!! [%02X][%02X][%02X][%02X]\n", tempID[0], tempID[1], tempID[2], tempID[3]);
            return FMI_SM_ID_ERR;
    }

    PRINTF("nvtSM_ReadID: Found %s NAND, ID [%02X][%02X][%02X][%02X], page size %d, BCH T%d\n",
        pSM->bIsMLCNand ? "MLC" : "SLC",
        tempID[0], tempID[1], tempID[2], tempID[3],
        pSM->nPageSize,
        pSM->bIsNandECC4*4 + pSM->bIsNandECC8*8 + pSM->bIsNandECC12*12 + pSM->bIsNandECC15*15 + pSM->bIsNandECC24*24
        );
    return 0;
}


INT nvtSM_Write_512(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT32 ucColAddr, UINT32 uSAddr)
{
    int temp;

    // set the spare area configuration
    if (nIsSysImage != 0xFF)
    {
        // set Stop Marker on the first empty page
        if ((gMarkCount >> 8) * pSM->nPageSize >= gNandLoaderSize)
        {
            outpw(REG_SMRA_0, 0x00A55AFF);      // Stop Marker
            gMarkCount += 0x100;
        }
        else
        {
            temp = (((nIsSysImage + gMarkCount) & 0xFFFF) << 8) + 0x000000FF;
            outpw(REG_SMRA_0, temp);
            gMarkCount += 0x100;
            if(gMarkCount == 0x1D)  /* Next page is 0x1D (page 29) */
                gMarkCount = 0xA500;
        }
    }
    else
    {
        outpw(REG_SMRA_0, 0x0000FFFF);
        gMarkCount = 0;
    }

    outpw(REG_FMICR, inpw(REG_FMICR)|FMI_SM_EN);

    fmiSM_Reset(pSM);

    /* set Page Size and BCH */
    sicSMsetBCH(pSM, gbSystemImage);
    g_u32ExtraDataSize = inpw(REG_SMREAREA_CTL) & SMRE_REA128_EXT;

    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);    // enable Auto Write
    outpw(REG_SMCSR, inpw(REG_SMCSR) & ~SMCR_REDUN_REN);        // enable Read
    /* enable DMAC */
    while(inpw(REG_DMACCSR) & FMI_BUSY);                // wait DMAC FMI ready;
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_EN);

    /* set DMA Transfer Starting Address */
    outpw(REG_DMACSAR, uSAddr);

    /* set SM columm & page address */
    outpw(REG_SMCMD, 0x80);                 // Program setup command
    outpw(REG_SMADDR, ucColAddr & 0xFF);    // CA0 - CA7
    outpw(REG_SMADDR, uSector & 0xFF);      // PA0 - PA7

    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((uSector >> 8) & 0xFF) | EOA_SM);    // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (uSector >> 8) & 0xFF);               // PA8 - PA15
        outpw(REG_SMADDR, ((uSector >> 16) & 0xFF) | EOA_SM);   // PA16 - PA17
    }

    outpw(REG_SMISR, SMISR_DMA_IF);                     // clear DMA flag
    outpw(REG_SMISR, SMISR_ECC_FIELD_IF);               // clear ECC_FIELD flag
    outpw(REG_SMISR, SMISR_PROT_REGION_WR_IF);          // clear Region Protect flag
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);    // auto write redundancy data to NAND after page data written
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DWR_EN);            // begin to write one page data to NAND flash

    while(!(inpw(REG_SMISR) & SMISR_DMA_IF));   // wait to finish DMAC transfer.
    outpw(REG_SMISR, SMISR_DMA_IF);             // clear DMA flag

    outpw(REG_SMCMD, 0x10);     // Program command

    if (!fmiSMCheckRB(pSM))
        return -1;

    if (fmiSMCheckStatus(pSM) != 0)
    {
        PRINTF("nvtSM_Write_512: data error [sector %d]!!\n", uSector);
        return -1;  // Program Fail
    }
    return 0;
}


INT nvtSM_Write_2K(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT32 ucColAddr, UINT32 uSAddr)
{
    int temp;

    if (nIsSysImage != 0xFF)
    {
        // set Stop Marker on the first empty page
        if ((gMarkCount >> 8) * pSM->nPageSize >= gNandLoaderSize)
        {
            outpw(REG_SMRA_0, 0x00A55AFF);      // Stop Marker
            gMarkCount += 0x100;
        }
        else
        {
            temp = (((nIsSysImage + gMarkCount) & 0xFFFF) << 8) + 0x000000FF;
            outpw(REG_SMRA_0, temp);
            gMarkCount += 0x100;
        }
    }
    else
    {
        outpw(REG_SMRA_0, 0x0000FFFF);
        gMarkCount = 0;
    }

    outpw(REG_FMICR, inpw(REG_FMICR)|FMI_SM_EN);

    fmiSM_Reset(pSM);

    /* set Page Size and BCH */
    sicSMsetBCH(pSM, gbSystemImage);
    g_u32ExtraDataSize = inpw(REG_SMREAREA_CTL) & SMRE_REA128_EXT;

    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);            // enable Auto Write

    /* enable DMAC */
    while(inpw(REG_DMACCSR) & FMI_BUSY);    // wait DMAC FMI ready;
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_EN);

    /* Set DMA Transfer Starting Address */
    outpw(REG_DMACSAR, uSAddr);

    outpw(REG_SMCMD, 0x80);     // Program setup command

    outpw(REG_SMADDR, ucColAddr & 0xFF);
    outpw(REG_SMADDR, (ucColAddr >> 8) & 0xFF);
    outpw(REG_SMADDR, uSector & 0xFF);

    if (!pSM->bIsMulticycle)
        outpw(REG_SMADDR, ((uSector >> 8) & 0xFF) | EOA_SM);    // PA8 - PA15
    else
    {
        outpw(REG_SMADDR, (uSector >> 8) & 0xFF);               // PA8 - PA15
        outpw(REG_SMADDR, ((uSector >> 16) & 0xFF) | EOA_SM);   // PA16 - PA17
    }

    outpw(REG_SMISR, SMISR_DMA_IF);                     // clear DMA flag
    outpw(REG_SMISR, SMISR_ECC_FIELD_IF);               // clear ECC_FIELD flag
    outpw(REG_SMISR, SMISR_PROT_REGION_WR_IF);          // clear Region Protect flag
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);    // auto write redundancy data to NAND after page data written
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DWR_EN);            // begin to write one page data to NAND flash

    while(!(inpw(REG_SMISR) & SMISR_DMA_IF));   // wait to finish DMAC transfer.
    outpw(REG_SMISR, SMISR_DMA_IF);             // clear DMA flag

    outpw(REG_SMCMD, 0x10);     // Program command

    if (!fmiSMCheckRB(pSM))
        return -1;

    if (fmiSMCheckStatus(pSM) != 0)
    {
        PRINTF("nvtSM_Write_2K: data error [sector %d]!!\n", uSector);
        return -1;  // Program Fail
    }
    return 0;       // Program Success
}


INT nvtSM_Write_4K(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT32 ucColAddr, UINT32 uSAddr)
{
    int temp;

    outpw(REG_FMICR, FMI_SM_EN);

    /* set Page Size and BCH */
    sicSMsetBCH(pSM, gbSystemImage);
    g_u32ExtraDataSize = inpw(REG_SMREAREA_CTL) & SMRE_REA128_EXT;

    /* Set DMA Transfer Starting Address */
    outpw(REG_DMACSAR, uSAddr);

    if (nIsSysImage != 0xFF)
    {
        // set Stop Marker on the first empty page
        if ((gMarkCount >> 8) * pSM->nPageSize >= gNandLoaderSize)
        {
            outpw(REG_SMRA_0, 0x00A55AFF);      // Stop Marker
            gMarkCount += 0x100;
        }
        else
        {
            temp = (((nIsSysImage + gMarkCount) & 0xFFFF) << 8) + 0x000000FF;
            outpw(REG_SMRA_0, temp);
            gMarkCount += 0x100;
        }
    }
    else
    {
        outpw(REG_SMRA_0, 0x0000FFFF);
        gMarkCount = 0;
    }

    // clear R/B flag
    while(!(inpw(REG_SMISR) & SMISR_RB0));
    outpw(REG_SMISR, SMISR_RB0_IF);

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

    outpw(REG_SMISR, SMISR_DMA_IF);                     // clear DMA flag
    outpw(REG_SMISR, SMISR_ECC_FIELD_IF);               // clear ECC_FIELD flag
    outpw(REG_SMISR, SMISR_PROT_REGION_WR_IF);          // clear Region Protect flag
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);    // auto write redundancy data to NAND after page data written
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DWR_EN);            // begin to write one page data to NAND flash

    while(!(inpw(REG_SMISR) & SMISR_DMA_IF));   // wait to finish DMAC transfer.
    outpw(REG_SMISR, SMISR_DMA_IF); // clear DMA flag
    outpw(REG_SMCMD, 0x10);         // auto program command

    if (!fmiSMCheckRB(pSM))
    {
        PRINTF("nvtSM_Write_4K: R/B timeout error [uSector = %d]!!\n", uSector);
        return -1;
    }

    if (fmiSMCheckStatus(pSM) != 0)
    {
        PRINTF("nvtSM_Write_4K: data error [%d]!!\n", uSector);
        return -1;  // Program Fail
    }
    return 0;
}


INT nvtSM_Write_8K(FMI_SM_INFO_T *pSM, UINT32 uSector, UINT32 ucColAddr, UINT32 uSAddr) //Added
{
    int temp;

    if (nIsSysImage != 0xFF)
    {
        // set Stop Marker on the first empty page
        if ((gMarkCount >> 8) * pSM->nPageSize >= gNandLoaderSize)
        {
            outpw(REG_SMRA_0, 0x00A55AFF);      // Stop Marker
            gMarkCount += 0x100;
        }
        else
        {
            temp = (((nIsSysImage + gMarkCount) & 0xFFFF) << 8) + 0x000000FF;
            outpw(REG_SMRA_0, temp);
            gMarkCount += 0x100;
        }
    }
    else
    {
        outpw(REG_SMRA_0, 0x0000FFFF);
        gMarkCount = 0;
    }

    outpw(REG_FMICR, inpw(REG_FMICR)|FMI_SM_EN);

    fmiSM_Reset(pSM);

    /* set Page Size and BCH */
    sicSMsetBCH(pSM, gbSystemImage);
    g_u32ExtraDataSize = inpw(REG_SMREAREA_CTL) & SMRE_REA128_EXT;

    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);    // enable Auto Write

    /* enable DMAC */
    while(inpw(REG_DMACCSR) & FMI_BUSY);    // wait DMAC FMI ready;
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_EN);

    /* Set DMA Transfer Starting Address */
    outpw(REG_DMACSAR, uSAddr);

    outpw(REG_SMCMD, 0x80);                 // Program setup command

    outpw(REG_SMADDR, ucColAddr & 0xFF);
    outpw(REG_SMADDR, (ucColAddr >> 8) & 0xFF);
    outpw(REG_SMADDR, uSector & 0xFF);

    if (!pSM->bIsMulticycle)
    {
        outpw(REG_SMADDR, ((uSector >> 8) & 0xFF) | EOA_SM);    // PA8 - PA15
    }
    else
    {
        outpw(REG_SMADDR, (uSector >> 8) & 0xFF);               // PA8 - PA15
        outpw(REG_SMADDR, ((uSector >> 16) & 0xFF) | EOA_SM);   // PA16 - PA17
    }

    outpw(REG_SMISR, SMISR_DMA_IF);                     // clear DMA flag
    outpw(REG_SMISR, SMISR_ECC_FIELD_IF);               // clear ECC_FIELD flag
    outpw(REG_SMISR, SMISR_PROT_REGION_WR_IF);          // clear Region Protect flag
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_REDUN_AUTO_WEN);    // auto write redundancy data to NAND after page data written
    outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_DWR_EN);            // begin to write one page data to NAND flash

    while(!(inpw(REG_SMISR) & SMISR_DMA_IF));   // wait to finish DMAC transfer.
    outpw(REG_SMISR, SMISR_DMA_IF);             // clear DMA flag

    outpw(REG_SMCMD, 0x10);     // auto program command

    if (!fmiSMCheckRB(pSM))
        return -1;

    if (fmiSMCheckStatus(pSM) != 0)
    {
        PRINTF("nvtSM_Write_8K: data error [sector %d]!!\n", uSector);
        return -1;  // Program Fail
    }
    return 0;
}


/* function pointer */
BOOL volatile bIsNandInit = FALSE;
FMI_SM_INFO_T nvtSMInfo, *pNvtSM0, *pNvtSMInfo;

INT nvtSMInit(void)
{
    if (!bIsNandInit)
    {
        //--- Initial DMAC module in FMI engine
        outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_EN);
        outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_SWRST);
        outpw(REG_DMACCSR, inpw(REG_DMACCSR) & ~DMAC_SWRST);

        //--- Initial SM module in FMI engine
        outpw(REG_FMICR, FMI_SM_EN);                            // enable SM module in FMI engine
        outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_SM_SWRST);      // software reset SM module
        while(inpw(REG_SMCSR)&SMCR_SM_SWRST);                   // waiting for SM module reset done

        outpw(REG_SMTCR, 0x20305);
        outpw(REG_SMCSR, (inpw(REG_SMCSR) & ~SMCR_PSIZE) | PSIZE_512);  // default is 512B page size
        outpw(REG_SMCSR, inpw(REG_SMCSR) | SMCR_ECC_EN | SMCR_ECC_CHK | SMCR_ECC_3B_PROTECT | SMCR_REDUN_AUTO_WEN); // enable ECC

        outpw(REG_GPDFUN0, (inpw(REG_GPDFUN0) & (~0xF0F00000)) | 0x20200000);   // enable NRE/RB0 pins
        outpw(REG_GPDFUN1, (inpw(REG_GPDFUN1) & (~0x0000000F)) | 0x00000002);   // enable NWR pins
        outpw(REG_GPEFUN1, (inpw(REG_GPEFUN1) & (~0x000FFF0F)) | 0x00022202);   // enable CS0/ALE/CLE/ND3 pins
        outpw(REG_SMCSR, inpw(REG_SMCSR) & ~SMCR_CS0);                          // enable CS0
        outpw(REG_SMCSR, inpw(REG_SMCSR) |  SMCR_CS1);                          // disable CS1

        pNvtSMInfo = (FMI_SM_INFO_T *)malloc(sizeof(NVT_SM_INFO_T));
        memset((char *)pNvtSMInfo, 0, sizeof(NVT_SM_INFO_T));
        pSM0= pNvtSMInfo;
        pNvtSM0 = pSM0;

        if (nvtSM_ReadID(pNvtSM0) < 0)
            return -1;
        fmiSM_Initial(pNvtSM0);

        pNvtSM0->uIBRBlock = fmiSM_GetIBRAreaSize(pNvtSM0);

        bIsNandInit = TRUE;
    }
    return 0;
}


INT nvtSMpwrite(INT PBA, INT page, UINT8 *buff)
{
    FMI_SM_INFO_T *pSM;
    int pageNo;

    pSM = pNvtSM0;

    // enable SM
    outpw(REG_FMICR, 0x08);

    if(PBA >= pNvtSM0->uIBRBlock)
        nIsSysImage = 0xFF;     // NOT System Image (Loader), write 0xFFFF0000 to Redundancy Area
    else
    {
        if (page == 0)
            gMarkCount = 0;     // Reset Sequence Mark (0xFF5Axx00) to 0 in Redundancy Area
        else if (page > pSM->uPagePerBlock)
        {
            gMarkCount = 0;
            nIsSysImage = 0xFF;
        }
    }

    if(PBA < pNvtSM0->uIBRBlock)
        gbSystemImage = TRUE;
    else
        gbSystemImage = FALSE;

    pageNo = PBA * pSM->uPagePerBlock + page;

    outpw(REG_SMIER, inpw(REG_SMIER) & (~SMIER_DMA_IE));    // disable DMA interrupt

    if (pSM->nPageSize == NAND_PAGE_2KB)
        return (nvtSM_Write_2K(pSM, pageNo, 0, (UINT32)buff));
    else if (pSM->nPageSize == NAND_PAGE_4KB)
        return (nvtSM_Write_4K(pSM, pageNo, 0, (UINT32)buff));
    else if (pSM->nPageSize == NAND_PAGE_8KB)
        return (nvtSM_Write_8K(pSM, pageNo, 0, (UINT32)buff));
    else
        return (nvtSM_Write_512(pSM, pageNo, 0, (UINT32)buff));
}


INT nvtSMchip_erase(UINT32 startBlcok, UINT32 endBlock)
{
    int i, status=0;
    pInfo = (UINT8 *)((UINT32)infoBuf | 0x80000000);

    // enable SM
    outpw(REG_FMICR, FMI_SM_EN);

    // erase all chip
    for (i=startBlcok; i<=endBlock; i++)
    {
        status = nand_block_erase0(i);
        if (status < 0)
            PRINTF("SM block erase fail for block <%d>!! Return code = 0x%x\n", i, status);
    }
    return 0;
}


INT fmiMarkBadBlock(UINT32 block)
{
    UINT32 ucColAddr, uSector;

    uSector = block * pNvtSM0->uPagePerBlock;
    if (pNvtSM0->nPageSize != NAND_PAGE_512B)
    {
        // send command
        if (pNvtSM0->nPageSize == NAND_PAGE_2KB)
            ucColAddr = 2048;
        else if (pNvtSM0->nPageSize == NAND_PAGE_4KB)
            ucColAddr = 4096;
        else if (pNvtSM0->nPageSize == NAND_PAGE_8KB)
            ucColAddr = 8192;
        else
            return -1;

        outpw(REG_SMCMD, 0x80);         // serial data input command
        outpw(REG_SMADDR, ucColAddr);       // CA0 - CA7
        outpw(REG_SMADDR, (ucColAddr >> 8) & 0x1f);
        outpw(REG_SMADDR, uSector & 0xff);  // PA0 - PA7
        if (!pNvtSM0->bIsMulticycle)
            outpw(REG_SMADDR, ((uSector >> 8) & 0xff)|0x80000000);      // PA8 - PA15
        else
        {
            outpw(REG_SMADDR, (uSector >> 8) & 0xff);                   // PA8 - PA15
            outpw(REG_SMADDR, ((uSector >> 16) & 0xff)|0x80000000);     // PA16 - PA17
        }
    }
    else
    {
        ucColAddr = 0;
        outpw(REG_SMCMD, 0x50);     // read RA command
        outpw(REG_SMCMD, 0x80);     // serial data input command
        outpw(REG_SMADDR, ucColAddr);       // CA0 - CA7
        outpw(REG_SMADDR, uSector & 0xff);  // PA0 - PA7
        if (!pNvtSM0->bIsMulticycle)
            outpw(REG_SMADDR, ((uSector >> 8) & 0xff)|0x80000000);      // PA8 - PA15
        else
        {
            outpw(REG_SMADDR, (uSector >> 8) & 0xff);                   // PA8 - PA15
            outpw(REG_SMADDR, ((uSector >> 16) & 0xff)|0x80000000);     // PA16 - PA17
        }
    }

    /* mark data */
    outpw(REG_SMDATA, 0x00);
    outpw(REG_SMDATA, 0x00);
    outpw(REG_SMDATA, 0x00);
    outpw(REG_SMDATA, 0x00);

    outpw(REG_SMCMD, 0x10);     // auto program command

    if (!fmiSMCheckRB(pSM0))
        return Fail;

    if (fmiSM_Reset(pSM0) < 0)
        return -1;

    return Successful;
}


/* Check the 1th byte of spare */
INT CheckBadBlockMark(UINT32 block)
{
    pInfo = (UINT8 *)((UINT32)infoBuf | 0x80000000);

    if (nandpread0(block, 0, pInfo) < 0)
        return -1;

    if (inpw(REG_SMRA_0) == 0x0)
        return Fail;
    else
        return Successful;
}
