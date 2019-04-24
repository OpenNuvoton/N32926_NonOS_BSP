/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2008 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
/*
 * Driver for FMI devices
 * SDIO layer glue code
 *
 */

#ifdef ECOS
    #include "stdlib.h"
    #include "string.h"
    #include "drv_api.h"
    #include "diag.h"
    #include "wbtypes.h"
    #include "wbio.h"
#else
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "wblib.h"
#endif

#include "w55fa92_reg.h"
#include "w55fa92_sdio.h"

#include "sdio_fmi.h"
#include "nvtfat.h"


#ifdef DEBUG
    #define DBG_PRINTF    		sysprintf
//    #define DBG_PRINTF		printf
#else
    #define DBG_PRINTF(...)
#endif


SDIO_DISK_DATA_T sdio_diskInfo0;
SDIO_DISK_DATA_T sdio_diskInfo1;

FMI_SDIO_INFO_T *pSDIO0 = NULL;
FMI_SDIO_INFO_T *pSDIO1 = NULL;

UINT8 pSDIO0_offset = 0;
UINT8 pSDIO1_offset = 0;

PDISK_T *pDisk_SDIO0 = NULL;
PDISK_T *pDisk_SDIO1 = NULL;

INT32 g_SDIO0_card_detect = FALSE;  // default is DISABLE SDIO0 card detect feature.
INT32 g_SDIO1_card_detect = FALSE;  // default is DISABLE SDIO1 card detect feature.


static INT  sdio_disk_init(PDISK_T  *lDisk)
{
    return 0;
}

static INT  sdio_disk_ioctl(PDISK_T *lDisk, INT control, VOID *param)
{
    return 0;
}

static INT  sdio_disk_read(PDISK_T *pDisk, UINT32 sector_no, INT number_of_sector, UINT8 *buff)
{
    int status;

    fmiSDIO_CardSel(0);

    // enable SD
    outpw(REG_SDIOFMICR, FMI_SD_EN);
    status = fmiSDIO_Read_in(pSDIO0, sector_no, number_of_sector, (unsigned int)buff);

    if (status != Successful)
        return status;

    return FS_OK;
}

static INT  sdio_disk_write(PDISK_T *pDisk, UINT32 sector_no, INT number_of_sector, UINT8 *buff, BOOL IsWait)
{
    int status;

    fmiSDIO_CardSel(0);

    // enable SD
    outpw(REG_SDIOFMICR, FMI_SD_EN);
    status = fmiSDIO_Write_in(pSDIO0, sector_no, number_of_sector, (unsigned int)buff);

    if (status != Successful)
        return status;

    return FS_OK;
}

static INT  sdio_disk_init0(PDISK_T  *lDisk)
{
    return 0;
}

static INT  sdio_disk_ioctl0(PDISK_T *lDisk, INT control, VOID *param)
{
    return 0;
}

static INT  sdio_disk_read0(PDISK_T *pDisk, UINT32 sector_no, INT number_of_sector, UINT8 *buff)
{
    return sdio_disk_read(pDisk, sector_no, number_of_sector, buff);
}

static INT  sdio_disk_write0(PDISK_T *pDisk, UINT32 sector_no, INT number_of_sector, UINT8 *buff, BOOL IsWait)
{
    return sdio_disk_write(pDisk, sector_no, number_of_sector, buff, IsWait);
}

static INT  sdio_disk_init1(PDISK_T  *lDisk)
{
    return 0;
}

static INT  sdio_disk_ioctl1(PDISK_T *lDisk, INT control, VOID *param)
{
    return 0;
}

static INT  sdio_disk_read1(PDISK_T *pDisk, UINT32 sector_no, INT number_of_sector, UINT8 *buff)
{
    int status;

    fmiSDIO_CardSel(1);

    // enable SD
    outpw(REG_SDIOFMICR, FMI_SD_EN);
    status = fmiSDIO_Read_in(pSDIO1, sector_no, number_of_sector, (unsigned int)buff);

    if (status != Successful)
        return status;

    return FS_OK;
}

static INT  sdio_disk_write1(PDISK_T *pDisk, UINT32 sector_no, INT number_of_sector, UINT8 *buff, BOOL IsWait)
{
    int status;

    fmiSDIO_CardSel(1);

    // enable SD
    outpw(REG_SDIOFMICR, FMI_SD_EN);
    status = fmiSDIO_Write_in(pSDIO1, sector_no, number_of_sector, (unsigned int)buff);

    if (status != Successful)
        return status;

    return FS_OK;
}

STORAGE_DRIVER_T  _SDIODiskDriver =
{
    sdio_disk_init,
    sdio_disk_read,
    sdio_disk_write,
    sdio_disk_ioctl
};

STORAGE_DRIVER_T  _SDIO0DiskDriver =
{
    sdio_disk_init0,
    sdio_disk_read0,
    sdio_disk_write0,
    sdio_disk_ioctl0
};

STORAGE_DRIVER_T  _SDIO1DiskDriver =
{
    sdio_disk_init1,
    sdio_disk_read1,
    sdio_disk_write1,
    sdio_disk_ioctl1
};

static int sd0_ok = 0;
static int sd1_ok = 0;


/*-----------------------------------------------------------------------------
 * Switch SD port.
 * fmiSDIO_CardSel() config GPIO/SDCR register/SD clock/Driver strength for selected SD port.
 * Do nothing if the selected SD port is same to original.
 *---------------------------------------------------------------------------*/
INT  fmiSDIO_CardSel(INT cardSel)
{
    FMI_SDIO_INFO_T *pSDIO;

    if (cardSel==0)
    {
        //--- Set GPIO to SDIO card mode
        if (g_SDIO0_card_detect)
        {
            // Set GPC14 to SDIO mode for SDIO port 0 card detection.
            outpw(REG_GPCFUN1, (inpw(REG_GPCFUN1) & (~0x0F000000)) | 0x05000000);
        }

        // SDIO 0 GPIO select: set GPC8~13 to SDIO card mode (SD0_CLK/SD0_CMD/DAT0[0~3]).
        outpw(REG_GPCFUN1, (inpw(REG_GPCFUN1) & (~0x00FFFFFF)) | 0x00555555);
        outpw(REG_SDIOCR, inpw(REG_SDIOCR) & (~SDCR_SDPORT));                   // SD_0 port selected
        pSDIO = pSDIO0;

        // Set SDIO0 card detect source to GPIO pin.
        outpw(REG_SDIOIER, inpw(REG_SDIOIER) | SDIER_CDSRC);
    }
    else if (cardSel==1)
    {
        if (g_SDIO1_card_detect)
        {
            // Set GPG4 to SDIO mode for SDIO port 1 card detection.
    	    outpw(REG_GPGFUN0, (inpw(REG_GPGFUN0) & (~0x000F0000)) | 0x00010000);
        }

        // SDIO 1 GPIO select: set GPG2~3, 12~15 to SDIO card mode (SD1_CLK/SD1_CMD/DAT1[0~3]).
    	outpw(REG_GPGFUN0, (inpw(REG_GPGFUN0) & (~0x0000FF00)) | 0x00001100);
    	outpw(REG_GPGFUN1, (inpw(REG_GPGFUN1) & (~0xFFFF0000)) | 0x11110000);
      	outpw(REG_SDIOCR, inpw(REG_SDIOCR) & (~SDCR_SDPORT) | SDCR_SDPORT_1);   // SD_1 port selected

      	outpw(REG_SHRPIN_TVDAC, inpw(REG_SHRPIN_TVDAC) & (~SMTVDACAEN));    // set GPG2~5 to digital mode
      	outpw(REG_SHRPIN_TOUCH, inpw(REG_SHRPIN_TOUCH) & (~TP_AEN));        // set GPG12~15 to digital mode
        pSDIO = pSDIO1;

        // Set SDIO1 card detect source to GPIO pin.
        outpw(REG_SDIOIER, inpw(REG_SDIOIER) | SDIER_CD1SRC);
    }
    else
        return 1;       // wrong SD card select

    //--- 2014/2/26, Reset SDIO controller and DMAC to keep clean status for next access.
    // Reset DMAC engine and interrupt satus
    outpw(REG_SDIODMACCSR, inpw(REG_SDIODMACCSR) | DMAC_SWRST | DMAC_EN);
    while(inpw(REG_SDIODMACCSR) & DMAC_SWRST);
    outpw(REG_SDIODMACCSR, inpw(REG_SDIODMACCSR) | DMAC_EN);
    outpw(REG_SDIODMACISR, WEOT_IF | TABORT_IF);    // clear all interrupt flag

    // Reset FMI engine and interrupt status
    outpw(REG_SDIOFMICR, FMI_SWRST);
    while(inpw(REG_SDIOFMICR) & FMI_SWRST);
    outpw(REG_SDIOFMIISR, FMI_DAT_IF);              // clear all interrupt flag

    // Reset SDIO engine and interrupt status
    outpw(REG_SDIOFMICR, FMI_SD_EN);
    outpw(REG_SDIOCR, inpw(REG_SDIOCR) | SDCR_SWRST);
    while(inpw(REG_SDIOCR) & SDCR_SWRST);
    outpw(REG_SDIOISR, 0xFFFFFFFF);               // clear all interrupt flag

    if (pSDIO != NULL)
    {
        //--- change SD clock for SD card on selected SD port.
        //--- NOTE: the clock setting also delay time to wait for stability of SD port change, especially for SD card.
        switch (pSDIO->uCardType)
        {
            case FMISDIO_TYPE_MMC:              fmiSDIO_Set_clock(SDIO_MMC_FREQ);  break;
            case FMISDIO_TYPE_MMC_SECTOR_MODE:  fmiSDIO_Set_clock(SDIO_EMMC_FREQ); break;
            case FMISDIO_TYPE_SD_LOW:           fmiSDIO_Set_clock(SDIO_SD_FREQ);   break;
            case FMISDIO_TYPE_SD_HIGH:          fmiSDIO_Set_clock(SDIO_SDHC_FREQ); break;
        }
        fmiSDIO_Change_Driver_Strength(cardSel, pSDIO->uCardType);
    }

    return 0;
}


/*-----------------------------------------------------------------------------
 * Get SD card status for SDIO port 0/1 and assign the status to value pSDIO->bIsCardInsert.
 * Return:  0                   is SD card inserted;
 *          FMISDIO_NO_SD_CARD  is SD card removed.
 *---------------------------------------------------------------------------*/
INT  fmiSDIO_CardStatus(FMI_SDIO_INFO_T *pSDIO)
{
    unsigned int volatile isr_cd = 0;

    if (pSDIO == pSDIO0)
    {
        if (g_SDIO0_card_detect)
            isr_cd = inpw(REG_SDIOISR) & SDISR_CD_Card;
        else
            isr_cd = 0;     // always report card inserted.
    }
    else if (pSDIO == pSDIO1)
    {
        if (g_SDIO1_card_detect)
            isr_cd = inpw(REG_SDIOISR) & SDISR_CD1_Card;
        else
            isr_cd = 0;     // always report card inserted.        
    }
    else
    {
        pSDIO->bIsCardInsert = FALSE;
        return FMISDIO_NO_SD_CARD;
    }

    if (isr_cd)     // CD pin status
    {
        pSDIO->bIsCardInsert = FALSE;
        return FMISDIO_NO_SD_CARD;
    }
    else
    {
        pSDIO->bIsCardInsert = TRUE;
        return 0;
    }
}


INT  fmiSDIOInitSDDevice(INT cardSel)
{
    PDISK_T *pDisk;
    SDIO_DISK_DATA_T* pSDDisk;
    FMI_SDIO_INFO_T *pSD_temp = NULL;

    // Enable AHB clock for SDIO. MUST also enable SIC clock for SDIO engine.
    outpw(REG_AHBCLK, inpw(REG_AHBCLK) | SDIO_CKE | SIC_CKE);

    // Reset FMI
    outpw(REG_SDIOFMICR, FMI_SWRST);        // Start reset FMI controller.
    while(inpw(REG_SDIOFMICR)&FMI_SWRST);

    // enable SD
    outpw(REG_SDIOFMICR, FMI_SD_EN);
    outpw(REG_SDIOCR, inpw(REG_SDIOCR) | SDCR_SWRST);   // SD software reset
    while(inpw(REG_SDIOCR) & SDCR_SWRST);

    outpw(REG_SDIOISR, 0xFFFFFFFF);     // write bit 1 to clear all SDISR

    outpw(REG_SDIOCR, inpw(REG_SDIOCR) & ~0xFF);        // disable SD clock ouput

    if (cardSel == 0)
    {
        if(sd0_ok == 1)
            return(0);
        pSDDisk = &sdio_diskInfo0;
    }
    else if (cardSel == 1)
    {
        if(sd1_ok == 1)
            return(0);
        pSDDisk = &sdio_diskInfo1;
    }

    // enable SD-0/1/2 pin?
    if (fmiSDIO_CardSel(cardSel))
        return FMISDIO_NO_SD_CARD;

    //Enable SD-0 card detectipn pin
    if (cardSel==0)
    {
        outpw(REG_SDIOIER, inpw(REG_SDIOIER) | SDIER_CDSRC);    // SD card detection source from GPIO but not DAT3
    }
    else if (cardSel==1)
    {
        outpw(REG_SDIOIER, inpw(REG_SDIOIER) | SDIER_CD2SRC);   // SD card detection source from GPIO but not DAT3
    }

    // Disable FMI/SD host interrupt
    outpw(REG_FMIIER, 0);

//  outpw(REG_SDIOCR, (inpw(REG_SDIOCR) & ~SDCR_SDNWR) | (0x01 << 24));     // set SDNWR = 1
    outpw(REG_SDIOCR, (inpw(REG_SDIOCR) & ~SDCR_SDNWR) | (0x09 << 24));     // set SDNWR = 9
    outpw(REG_SDIOCR, (inpw(REG_SDIOCR) & ~SDCR_BLKCNT) | (0x01 << 16));    // set BLKCNT = 1
    outpw(REG_SDIOCR, inpw(REG_SDIOCR) & ~SDCR_DBW);        // SD 1-bit data bus

    pSD_temp = malloc(sizeof(FMI_SDIO_INFO_T)+4);
    if (pSD_temp == NULL)
        return FMISDIO_NO_MEMORY;

    memset((char *)pSD_temp, 0, sizeof(FMI_SDIO_INFO_T)+4);

    if (cardSel==0)
    {
        pSDIO0_offset = (UINT32)pSD_temp %4;
        pSDIO0 = (FMI_SDIO_INFO_T *)((UINT32)pSD_temp + pSDIO0_offset);
    }
    else if (cardSel==1)
    {
        pSDIO1_offset = (UINT32)pSD_temp %4;
        pSDIO1 = (FMI_SDIO_INFO_T *)((UINT32)pSD_temp + pSDIO1_offset);
    }

#ifdef _SDIO_USE_INT_
    outpw(REG_SDIOIER, inpw(REG_SDIOIER) | SDIER_CD_IEN);   // enable card detect interrupt
    outpw(REG_SDIOIER, inpw(REG_SDIOIER) | SDIER_CD1_IEN);  // enable card detect interrupt
//    outpw(REG_SDIOIER, inpw(REG_SDIOIER) | SDIER_CRC_IEN);  // enable CRC error interrupt
//    outpw(REG_SDIOIER, inpw(REG_SDIOIER) | SDISR_RITO_IF);  // enable RI timeout
//    outpw(REG_SDIOIER, inpw(REG_SDIOIER) | SDISR_DITO_IF);  // enable DI timeout
    outpw(REG_SDIOIER, inpw(REG_SDIOIER) | SDIER_BLKD_IEN);  // enable block transfer end interrupt

#endif

    if (cardSel==0)
    {
        if (g_SDIO0_card_detect)
        {
            if (inpw(REG_SDIOISR) & SDISR_CD_Card)    // CD pin status
            {
                pSDIO0->bIsCardInsert = FALSE;
                if (pSDIO0 != NULL)
                {
                    free((FMI_SDIO_INFO_T *)((UINT32)pSDIO0 - pSDIO0_offset));
                    pSDIO0 = 0;
                }
                free((FMI_SDIO_INFO_T *)((UINT32)pSDIO0 - pSDIO0_offset));
                return FMISDIO_NO_SD_CARD;
            }
            else
            {
                pSDIO0->bIsCardInsert = TRUE;
            }
        }
        else
        {
            pSDIO0->bIsCardInsert = TRUE;
        }

        if (fmiSDIO_Init(pSDIO0) < 0)
            return FMISDIO_SD_INIT_ERROR;

        /* divider */
        switch (pSDIO0->uCardType)
        {
            case FMISDIO_TYPE_MMC:              fmiSDIO_Set_clock(SDIO_MMC_FREQ);  break;
            case FMISDIO_TYPE_MMC_SECTOR_MODE:  fmiSDIO_Set_clock(SDIO_EMMC_FREQ); break;
            case FMISDIO_TYPE_SD_LOW:           fmiSDIO_Set_clock(SDIO_SD_FREQ);   break;
            case FMISDIO_TYPE_SD_HIGH:          fmiSDIO_Set_clock(SDIO_SDHC_FREQ); break;
            default:                            sysprintf("ERROR: Unknown card type !!\n"); break;
        }
        fmiSDIO_Change_Driver_Strength(cardSel, pSDIO0->uCardType);
    }
    else if (cardSel==1)
    {
        if (g_SDIO1_card_detect)
        {
            if (inpw(REG_SDIOISR) & SDISR_CD1_Card)     // CD1 pin status
            {
                pSDIO1->bIsCardInsert = FALSE;
                if (pSDIO1 != NULL)
                {
                    free((FMI_SDIO_INFO_T *)((UINT32)pSDIO1 - pSDIO1_offset));
                    pSDIO1 = 0;
                }
                free((FMI_SDIO_INFO_T *)((UINT32)pSDIO1 - pSDIO1_offset));
                return FMISDIO_NO_SD_CARD;
            }
            else
            {
                pSDIO1->bIsCardInsert = TRUE;
            }
        }
        else
        {
            pSDIO1->bIsCardInsert = TRUE;
        }

        // SD-1 initial
        if (fmiSDIO_Init(pSDIO1) < 0)
            return FMISDIO_SD_INIT_ERROR;

        /* divider */
        switch (pSDIO1->uCardType)
        {
            case FMISDIO_TYPE_MMC:              fmiSDIO_Set_clock(SDIO_MMC_FREQ);  break;
            case FMISDIO_TYPE_MMC_SECTOR_MODE:  fmiSDIO_Set_clock(SDIO_EMMC_FREQ); break;
            case FMISDIO_TYPE_SD_LOW:           fmiSDIO_Set_clock(SDIO_SD_FREQ);   break;
            case FMISDIO_TYPE_SD_HIGH:          fmiSDIO_Set_clock(SDIO_SDHC_FREQ); break;
            default:                            sysprintf("ERROR: Unknown card type !!\n"); break;
        }
        fmiSDIO_Change_Driver_Strength(cardSel, pSDIO1->uCardType);
    }

    /* init SD interface */
    if (cardSel==0)
    {
        fmiGet_SDIO_info(pSDIO0, pSDDisk);
        if (fmiSDIOSelectCard(pSDIO0))
            return FMISDIO_SD_SELECT_ERROR;
    }
    else if (cardSel==1)
    {
        fmiGet_SDIO_info(pSDIO1, pSDDisk);
        if (fmiSDIOSelectCard(pSDIO1))
            return FMISDIO_SD_SELECT_ERROR;
    }

    /*
     * Create physical disk descriptor
     */
    pDisk = malloc(sizeof(PDISK_T));
    if (pDisk == NULL)
        return FMISDIO_NO_MEMORY;
    memset((char *)pDisk, 0, sizeof(PDISK_T));

    /* read Disk information */
    pDisk->szManufacture[0] = '\0';
    strcpy(pDisk->szProduct, (char *)pSDDisk->product);
    strcpy(pDisk->szSerialNo, (char *)pSDDisk->serial);

    pDisk->nDiskType = DISK_TYPE_SD_MMC;

    pDisk->nPartitionN = 0;
    pDisk->ptPartList = NULL;

    pDisk->nSectorSize = 512;
    pDisk->uTotalSectorN = pSDDisk->totalSectorN;
    pDisk->uDiskSize = pSDDisk->diskSize;

    /* create relationship between UMAS device and file system hard disk device */
    if (cardSel==0)
        pDisk->ptDriver = &_SDIO0DiskDriver;
    else if (cardSel==1)
        pDisk->ptDriver = &_SDIO1DiskDriver;

#ifdef DEBUG
    DBG_PRINTF("SD disk found: size=%d MB\n", (int)pDisk->uDiskSize / 1024);
#endif

    if (cardSel==0)
    {
        pDisk_SDIO0 = pDisk;
    }
    else if (cardSel==1)
    {
        pDisk_SDIO1 = pDisk;
    }

    fsPhysicalDiskConnected(pDisk);

    if (cardSel == 0)
        sd0_ok = 1;
    else if (cardSel == 1)
        sd1_ok = 1;

    return pDisk->uTotalSectorN;
}

INT  fmiSDIO_Read(UINT32 uSector, UINT32 uBufcnt, UINT32 uDAddr)
{
    int status=0;

    // enable SD
    outpw(REG_SDIOFMICR, FMI_SD_EN);
    status = fmiSDIO_Read_in(pSDIO0, uSector, uBufcnt, uDAddr);

    return status;
}

INT  fmiSDIO_Write(UINT32 uSector, UINT32 uBufcnt, UINT32 uSAddr)
{
    int status=0;

    // enable SD
    outpw(REG_SDIOFMICR, FMI_SD_EN);
    status = fmiSDIO_Write_in(pSDIO0, uSector, uBufcnt, uSAddr);

    return status;
}

VOID sdioSdClose_sel(INT cardSel)
{
    if (cardSel==0)
    {
        sd0_ok = 0;
        //--- 2014/12/4, MUST free pDisk_SDIO0 BEFORE free pSDIO0
        //      because pSDIO0 could be used within fsUnmountPhysicalDisk().
        if (pDisk_SDIO0 != NULL)
        {
            fsUnmountPhysicalDisk(pDisk_SDIO0);
            free(pDisk_SDIO0);
            pDisk_SDIO0 = NULL;
        }
        if (pSDIO0 != NULL)
        {
            free((FMI_SDIO_INFO_T *)((UINT32)pSDIO0 - pSDIO0_offset));
            pSDIO0 = 0;
        }
    }
    else if (cardSel==1)
    {
        sd1_ok = 0;
        if (pDisk_SDIO1 != NULL)
        {
            fsUnmountPhysicalDisk(pDisk_SDIO1);
            free(pDisk_SDIO1);
            pDisk_SDIO1 = NULL;
        }
        if (pSDIO1 != NULL)
        {
            free((FMI_SDIO_INFO_T *)((UINT32)pSDIO1 - pSDIO1_offset));
            pSDIO1 = 0;
        }
    }
}

VOID sdioSdClose(void)
{
    sdioSdClose_sel(0);
}

VOID sdioSdClose0(void)
{
    sdioSdClose_sel(0);
}

VOID sdioSdClose1(void)
{
    sdioSdClose_sel(1);
}


/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sdioSdOpen                                                                       */
/*                                                                                   */
/* Parameters:                                                                       */
/*   sdPortNo               SD port number(port0 or port1).                          */
/*                                                                                   */
/* Returns:                                                                          */
/*   >0                     Total sector.                                            */
/*   FMISDIO_NO_SD_CARD         No SD card insert.                                       */
/*   FMISDIO_SD_INIT_ERROR      Card initial and identify error.                         */
/*   FMISDIO_SD_SELECT_ERROR    Select card from identify mode to stand-by mode error.   */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function initial SD from identification to stand-by mode.                  */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
INT sdioSdOpen_sel(INT cardSel)
{
    return fmiSDIOInitSDDevice(cardSel);
}

INT sdioSdOpen(void)
{
    return fmiSDIOInitSDDevice(0);
}

INT sdioSdOpen0(void)
{
    return fmiSDIOInitSDDevice(0);
}

INT sdioSdOpen1(void)
{
    return fmiSDIOInitSDDevice(1);
}
