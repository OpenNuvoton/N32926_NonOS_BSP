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
#else
#include "wblib.h"
#endif

#include "w55fa92_reg.h"
#include "w55fa92_sdio.h"

#include "sdio_fmi.h"
#include "nvtfat.h"

/*-----------------------------------------------------------------------------
 * Define message display level
 *---------------------------------------------------------------------------*/
// show large messages for debug

//#define DEBUG
#ifdef DEBUG
    #define DBG_PRINTF    	sysprintf
	// show improtant information for normal operation
	#define INF_PRINTF  sysprintf
	// show error message
	#define ERR_PRINTF  sysprintf

#else
    #define DBG_PRINTF(...)
	// show improtant information for normal operation
	#define INF_PRINTF  sysprintf
	// show error message
	#define ERR_PRINTF  sysprintf

#endif

#define SDIO_BLOCK_SIZE   512

#define FMISDIO_SD_INITCOUNT    2000
#define FMISDIO_TICKCOUNT       1000

// global variables
// For response R3 (such as ACMD41, CRC-7 is invalid; but SD controller will still
//      calculate CRC-7 and get an error result, software should ignore this error and clear SDISR [CRC_IF] flag
//      _fmiSDIO_uR3_CMD is the flag for it. 1 means software should ignore CRC-7 error
UINT32 _fmiSDIO_uR3_CMD=0;
UINT32 _fmiSDIO_uR7_CMD=0;

__align(4096) UCHAR _fmi_ucSDIOHCBuffer[512];
UINT8 *_fmi_pSDIOHCBuffer;


//--- 2014/3/27, check the sector number is valid or not for current SD card.
unsigned int g_sdio_max_valid_sector;    // The max valid sector number for current SD card.
INT fmiSDIOCheckSector(UINT32 uSector, UINT32 uBufcnt)
{
    if ((uSector + uBufcnt - 1) > g_sdio_max_valid_sector)
    {
        sysprintf("ERROR: Fail to access invalid sector number %d from SD card !!\n", uSector+uBufcnt-1);
        sysprintf("       The max valid sector number for current SD card is %d.\n", g_sdio_max_valid_sector);
        return FMISDIO_SD_SELECT_ERROR; // invalid sector
    }
    return 0;   // valid sector
}


void fmiSDIOCheckRB()
{
    while(1)
    {
        outpw(REG_SDIOCR, inpw(REG_SDIOCR)|SDCR_8CLK_OE);
        while(inpw(REG_SDIOCR) & SDCR_8CLK_OE);
        if (inpw(REG_SDIOISR) & SDISR_SD_DATA0)
            break;
    }
}


INT fmiSDIOCommand(FMI_SDIO_INFO_T *pSDIO, UINT8 ucCmd, UINT32 uArg)
{
    outpw(REG_SDIOARG, uArg);
    outpw(REG_SDIOCR, (inpw(REG_SDIOCR)&(~SDCR_CMD_CODE))|(ucCmd << 8)|(SDCR_CO_EN));

    while(inpw(REG_SDIOCR) & SDCR_CO_EN)
    {
        fmiSDIO_CardStatus(pSDIO);
        if (pSDIO->bIsCardInsert == FALSE)
            return FMISDIO_NO_SD_CARD;
    }
    return Successful;
}


INT fmiSDIOCmdAndRsp(FMI_SDIO_INFO_T *pSDIO, UINT8 ucCmd, UINT32 uArg, INT ntickCount)
{
    outpw(REG_SDIOARG, uArg);
    outpw(REG_SDIOCR, (inpw(REG_SDIOCR)&(~SDCR_CMD_CODE))|(ucCmd << 8)|(SDCR_CO_EN | SDCR_RI_EN));

    if (ntickCount > 0)
    {
        while(inpw(REG_SDIOCR) & SDCR_RI_EN)
        {
            if(ntickCount-- == 0) {
                outpw(REG_SDIOCR, inpw(REG_SDIOCR)|SDCR_SWRST); // reset SD engine
                return FMISDIO_SD_INIT_TIMEOUT;
            }
            fmiSDIO_CardStatus(pSDIO);
            if (pSDIO->bIsCardInsert == FALSE)
                return FMISDIO_NO_SD_CARD;
        }
    }
    else
    {
        while(inpw(REG_SDIOCR) & SDCR_RI_EN)
        {
            fmiSDIO_CardStatus(pSDIO);
            if (pSDIO->bIsCardInsert == FALSE)
                return FMISDIO_NO_SD_CARD;
        }
    }

    if (_fmiSDIO_uR7_CMD)
    {
        if (((inpw(REG_SDIORSP1) & 0xff) != 0x55) && ((inpw(REG_SDIORSP0) & 0xf) != 0x01))
        {
            _fmiSDIO_uR7_CMD = 0;
            return FMISDIO_SD_CMD8_ERROR;
        }
    }

    if (!_fmiSDIO_uR3_CMD)
    {
        if (inpw(REG_SDIOISR) & SDISR_CRC_7)      // check CRC7
            return Successful;
        else
        {
            DBG_PRINTF("response error [%d]!\n", ucCmd);
            return FMISDIO_SD_CRC7_ERROR;
        }
    }
    else    // ignore CRC error for R3 case
    {
        _fmiSDIO_uR3_CMD = 0;
        outpw(REG_SDIOISR, SDISR_CRC_IF);
        return Successful;
    }
}


// Get 16 bytes CID or CSD
INT fmiSDIOCmdAndRsp2(FMI_SDIO_INFO_T *pSDIO, UINT8 ucCmd, UINT32 uArg, UINT *puR2ptr)
{
    unsigned int i;
    unsigned int tmpBuf[5];

    outpw(REG_SDIOARG, uArg);
    outpw(REG_SDIOCR, (inpw(REG_SDIOCR)&(~SDCR_CMD_CODE))|(ucCmd << 8)|(SDCR_CO_EN | SDCR_R2_EN));

    while(inpw(REG_SDIOCR) & SDCR_R2_EN)
    {
        fmiSDIO_CardStatus(pSDIO);
        if (pSDIO->bIsCardInsert == FALSE)
            return FMISDIO_NO_SD_CARD;
    }

    if (inpw(REG_SDIOISR) & SDISR_CRC_7)
    {
        for (i=0; i<5; i++)
            tmpBuf[i] = Swap32(inpw(REG_SDIOFB_0+i*4));

        for (i=0; i<4; i++)
            *puR2ptr++ = ((tmpBuf[i] & 0x00ffffff)<<8) | ((tmpBuf[i+1] & 0xff000000)>>24);
        return Successful;
    }
    else
        return FMISDIO_SD_CRC7_ERROR;
}


INT fmiSDIOCmdAndRspDataIn(FMI_SDIO_INFO_T *pSDIO, UINT8 ucCmd, UINT32 uArg)
{
    outpw(REG_SDIOARG, uArg);
    outpw(REG_SDIOCR, (inpw(REG_SDIOCR)&(~SDCR_CMD_CODE))|(ucCmd << 8)|(SDCR_CO_EN | SDCR_RI_EN | SDCR_DI_EN));

    while (inpw(REG_SDIOCR) & SDCR_RI_EN)
    {
        fmiSDIO_CardStatus(pSDIO);
        if (pSDIO->bIsCardInsert == FALSE)
            return FMISDIO_NO_SD_CARD;
    }

    while (inpw(REG_SDIOCR) & SDCR_DI_EN)
    {
        fmiSDIO_CardStatus(pSDIO);
        if (pSDIO->bIsCardInsert == FALSE)
            return FMISDIO_NO_SD_CARD;
    }

    if (!(inpw(REG_SDIOISR) & SDISR_CRC_7))       // check CRC7
    {
        DBG_PRINTF("fmiSDIOCmdAndRspDataIn: response error [%d]!\n", ucCmd);
        return FMISDIO_SD_CRC7_ERROR;
    }

    if (!(inpw(REG_SDIOISR) & SDISR_CRC_16))      // check CRC16
    {
        DBG_PRINTF("fmiSDIOCmdAndRspDataIn: read data error!\n");
        return FMISDIO_SD_CRC16_ERROR;
    }
    return Successful;
}


/*-----------------------------------------------------------------------------
 * 2012/3/2, change SDIO pins driver strength according to the card type.
 *      For SDHC run on 50MHz, set SDIOCLK driver strength to 8mA;
 *      For SD   run on 25MHz, set SDIOCLK driver strength to 4mA;
 *      For MMC  run on 20MHz, set SDIOCLK driver strength to 4mA.
 *---------------------------------------------------------------------------*/
VOID fmiSDIO_Change_Driver_Strength(int card_no, int card_type)
{
    if ((card_type == FMISDIO_TYPE_MMC) || (card_type == FMISDIO_TYPE_MMC_SECTOR_MODE) || (card_type == FMISDIO_TYPE_SD_LOW))
    {
        // change driver strength to 4mA (set REG_MISC_DS_GPx bit to 00)
        if (card_no == 0)       // GPC8~GPC13 for SDIO0 pins
        {
            outpw(REG_MISC_DS_GPC, inpw(REG_MISC_DS_GPC) & (~(0x0FFF0000)));    // clear for 4mA
        }
        // SDIO port 1 use GPGx pin and cannot config driver strength since chip design limitation.
    }
    else if (card_type == FMISDIO_TYPE_SD_HIGH)
    {
        // change driver strength to 8mA (set REG_MISC_DS_GPx bit to 01)
        if (card_no == 0)       // GPC8~GPC13 for SDIO0 pins
        {
            outpw(REG_MISC_DS_GPC, inpw(REG_MISC_DS_GPC) & (~(0x0FFF0000)));    // clear for 4mA
            outpw(REG_MISC_DS_GPC, inpw(REG_MISC_DS_GPC) | 0x05550000);         // set to bits 01 for 8mA
        }
    }
}


/*-----------------------------------------------------------------------------
 * 2011/6/24, To set up the clock for SD_CLK
 *      SD_CLK = UPLL / ((CLKDIV2[SD_N0] + 1) * (CLKDIV2[SD_N1] + 1))
 *      Support both FPGA board and real chip.
 *          FPGA board don't support CLKDIV2[SD_N0] divider.
 *      INPUT: sd_clock_khz: the SD clock you wanted with unit KHz.
 *---------------------------------------------------------------------------*/
// there are 3 bits for divider N0, maximum is 8
#define SD_CLK_DIV0_MAX     8
// there are 8 bits for divider N1, maximum is 256
#define SD_CLK_DIV1_MAX     256
UINT32 current_sdio_clock_khz = 0;


VOID fmiSDIO_Set_clock(UINT32 sd_clock_khz)
{
    UINT32 rate, div0, div1, i;
    UINT32 sd_clk_div0_min;

    //--- calculate the rate that 2 divider have to divide
    // _fmiSDIO_uFMIReferenceClock is the input clock with unit KHz like as APLL/UPLL and
    //      assign by sicIoctl(SDIO_SET_CLOCK, , , );
    if (sd_clock_khz > _fmiSDIO_uFMIReferenceClock)
    {
        sysprintf("ERROR: wrong SDIO clock %dKHz since it is faster than input clock %dKHz !\n",
            sd_clock_khz, _fmiSDIO_uFMIReferenceClock);
        return;
    }

    if (sd_clock_khz == 0)
    {
        sysprintf("WARNING: cannot set SDIO clock to 0Hz. Ignore it !\n");
        return;
    }

    rate = _fmiSDIO_uFMIReferenceClock / sd_clock_khz;
    // choose slower clock if system clock cannot divisible by wanted clock
    if (_fmiSDIO_uFMIReferenceClock % sd_clock_khz != 0)
        rate++;
    if (rate > (SD_CLK_DIV0_MAX * SD_CLK_DIV1_MAX)) // the maximum divider for SD_CLK is (SD_CLK_DIV0_MAX * SD_CLK_DIV1_MAX)
    {
        sysprintf("ERROR: wrong SDIO clock %dKHz since it is slower than input clock %dKHz/%d !\n",
            sd_clock_khz, _fmiSDIO_uFMIReferenceClock, SD_CLK_DIV0_MAX * SD_CLK_DIV1_MAX);
        return;
    }

    //--- Ignore the request to set SD clock to same frequency in order to improve SD card access performance.
    if (sd_clock_khz == current_sdio_clock_khz)
    {
        DBG_PRINTF("fmiSDIO_Set_clock(): ignore SDIO clock %dKHz setting since it is same frequency.\n", sd_clock_khz);
        return;
    }
    else
        current_sdio_clock_khz = sd_clock_khz;

    //--- choose a suitable value for first divider CLKDIV2[SD_N0]
#ifdef  OPT_FPGA_DEBUG
    // div0 always is 1 for FPGA board since FPGA board don't support CLKDIV2[SD_N0]
    div0 = 1;
#else
    // 2014/12/29, the frequency after first divider MUST <= 177MHz because of FA92 hardware limitation.
    sd_clk_div0_min = _fmiSDIO_uFMIReferenceClock / 177000;
    if (_fmiSDIO_uFMIReferenceClock % 177000 != 0)
        sd_clk_div0_min++;
    if (sd_clk_div0_min > SD_CLK_DIV0_MAX)
        sd_clk_div0_min = SD_CLK_DIV0_MAX;
    if (sd_clk_div0_min == 0)
        sd_clk_div0_min = 1;

    for (div0 = SD_CLK_DIV0_MAX; div0 >= sd_clk_div0_min; div0--)    // choose the maximum value if can exact division
    {
        if (rate % div0 == 0)
            break;
    }
    if (div0 < sd_clk_div0_min) // cannot exact division
    {
        // keep div0 as small as possible to improve the accuracy of final SD clock
        div0 = rate / SD_CLK_DIV1_MAX;
        if (rate % SD_CLK_DIV1_MAX != 0)
            div0++;
        if (div0 < sd_clk_div0_min)
            div0 = sd_clk_div0_min;
    }
#endif

    //--- calculate the second divider CLKDIV2[SD_N1]
    div1 = rate / div0;
    if (rate % div0 != 0)
        div1++;
    div1 &= 0xFF;
    DBG_PRINTF("fmiSDIO_Set_clock(): wanted clock=%d, rate=%d, div0=%d, div1=%d\n", sd_clock_khz, rate, div0, div1);
    DBG_PRINTF("                     clock source=%d, final clock=%d\n", _fmiSDIO_uFMIReferenceClock, _fmiSDIO_uFMIReferenceClock/(div0*div1));

    //--- setup register
    outpw(REG_CLKDIV8, (inpw(REG_CLKDIV8) & ~SDIO_S) | (0x03 << 3));        // SD clock from UPLL
    outpw(REG_CLKDIV8, (inpw(REG_CLKDIV8) & ~SDIO_N0) | (div0-1) );   	 	// SD clock divided by CLKDIV2[SD_N0]
    outpw(REG_CLKDIV8, (inpw(REG_CLKDIV8) & ~SDIO_N1) | ((div1-1) << 5));  	// SD clock divider by CLKDIV2[SD_N1]

#ifdef  OPT_FPGA_DEBUG
    if (inpw(REG_CLKDIV8) == 0)
    {
        DBG_PRINTF("*** NOTE: This FPGA code don't support CLKDIV_8 for SDIO. Use CLKDIV_2 now !\n");
        outpw(REG_CLKDIV2, (inpw(REG_CLKDIV2) & ~SD_S) | (0x03 << 19));         // SD clock from UPLL
        outpw(REG_CLKDIV2, (inpw(REG_CLKDIV2) & ~SD_N0) | ((div0-1) << 16));    // SD clock divided by CLKDIV2[SD_N0]
        outpw(REG_CLKDIV2, (inpw(REG_CLKDIV2) & ~SD_N1) | ((div1-1) << 24));    // SD clock divider by CLKDIV2[SD_N1]
    }
#endif

    for(i=0; i<1000; i++);  // waiting for clock become stable
    return;
}


// Initial
INT fmiSDIO_Init(FMI_SDIO_INFO_T *pSDIO)
{
    int volatile i, status;
    unsigned int resp;
    unsigned int CIDBuffer[4];
    unsigned int volatile u32CmdTimeOut;
    int sdport;

    if (pSDIO == pSDIO0)
        sdport = 0;
    else if (pSDIO == pSDIO1)
        sdport = 1;
    else
        return FMISDIO_SD_INIT_ERROR;
    INF_PRINTF("Initial SDIO Non-OS Driver (%s) for SDIO port %d\n", SDIO_DATE_CODE, sdport);

    // set the clock to 300KHz
    fmiSDIO_Set_clock(300);

    // power ON 74 clock
    outpw(REG_SDIOCR, inpw(REG_SDIOCR) | SDCR_74CLK_OE);

    while(inpw(REG_SDIOCR) & SDCR_74CLK_OE)
    {
        fmiSDIO_CardStatus(pSDIO);
        if (pSDIO->bIsCardInsert == FALSE)
            return FMISDIO_NO_SD_CARD;
    }

    fmiSDIOCommand(pSDIO, 0, 0);        // reset all cards
    for (i=0x100; i>0; i--);

    // initial SDHC
    _fmiSDIO_uR7_CMD = 1;
    u32CmdTimeOut = 5000;

#if 0	// SDIO access test

    if ( fmiSDIOCmdAndRsp(pSDIO, 5, 0x00, u32CmdTimeOut*3))
    {
       	do
        {
        	// set card voltage from 3.0V ~ 3.4V
        	fmiSDIOCmdAndRsp(pSDIO, 5, 0x003C0000, u32CmdTimeOut*3);
           	resp = inpw(REG_SDIORSP0);
        } while (!(resp & 0x00800000));        // check if card is ready
     }

#endif

    i = fmiSDIOCmdAndRsp(pSDIO, 8, 0x00000155, u32CmdTimeOut);
    _fmiSDIO_uR7_CMD = 0;   // Disable R7 checking for commands that are not CMD8
    if (i == Successful)
    {
        // SD 2.0
        fmiSDIOCmdAndRsp(pSDIO, 55, 0x00, u32CmdTimeOut);
        _fmiSDIO_uR3_CMD = 1;
        fmiSDIOCmdAndRsp(pSDIO, 41, 0x40ff8000, u32CmdTimeOut); // 2.7v-3.6v
        resp = inpw(REG_SDIORSP0);

        while (!(resp & 0x00800000))        // check if card is ready
        {
            fmiSDIOCmdAndRsp(pSDIO, 55, 0x00, u32CmdTimeOut);
            _fmiSDIO_uR3_CMD = 1;
            fmiSDIOCmdAndRsp(pSDIO, 41, 0x40ff8000, u32CmdTimeOut); // 3.0v-3.4v
            resp = inpw(REG_SDIORSP0);
        }
        if (resp & 0x00400000)
            pSDIO->uCardType = FMISDIO_TYPE_SD_HIGH;
        else
            pSDIO->uCardType = FMISDIO_TYPE_SD_LOW;
    }
    else
    {
        // SD 1.1 or MMC
        fmiSDIOCommand(pSDIO, 0, 0);        // reset all cards
        for (i=0x100; i>0; i--);

        i = fmiSDIOCmdAndRsp(pSDIO, 55, 0x00, u32CmdTimeOut);
        if (i == FMISDIO_SD_INIT_TIMEOUT)     // MMC memory
        {
            fmiSDIOCommand(pSDIO, 0, 0);        // reset
            for (i=0x100; i>0; i--);

            _fmiSDIO_uR3_CMD = 1;
            // 2014/8/6, to support eMMC v4.4, the argument of CMD1 should be 0x40ff8000 to support both MMC plus and eMMC cards.
            if (fmiSDIOCmdAndRsp(pSDIO, 1, 0x40ff8000, u32CmdTimeOut) != FMISDIO_SD_INIT_TIMEOUT) // MMC memory
            {
                resp = inpw(REG_SDIORSP0);
                while (!(resp & 0x00800000))        // check if card is ready
                {
                    _fmiSDIO_uR3_CMD = 1;
                    fmiSDIOCmdAndRsp(pSDIO, 1, 0x40ff8000, u32CmdTimeOut);      // high voltage
                    resp = inpw(REG_SDIORSP0);
                }
                // MMC card is ready. Check the access mode of MMC card.
                if (resp & 0x00400000)
                    pSDIO->uCardType = FMISDIO_TYPE_MMC_SECTOR_MODE;
                else
                    pSDIO->uCardType = FMISDIO_TYPE_MMC;
            }
            else
            {
                pSDIO->uCardType = FMISDIO_TYPE_UNKNOWN;
                return FMISDIO_ERR_DEVICE;
            }
        }
        else if (i == Successful)    // SD Memory
        {
            _fmiSDIO_uR3_CMD = 1;
            fmiSDIOCmdAndRsp(pSDIO, 41, 0x00ff8000, u32CmdTimeOut); // 3.0v-3.4v
            resp = inpw(REG_SDIORSP0);
            while (!(resp & 0x00800000))        // check if card is ready
            {
                fmiSDIOCmdAndRsp(pSDIO, 55, 0x00,u32CmdTimeOut);
                _fmiSDIO_uR3_CMD = 1;
                fmiSDIOCmdAndRsp(pSDIO, 41, 0x00ff8000, u32CmdTimeOut); // 3.0v-3.4v
                resp = inpw(REG_SDIORSP0);
            }
            pSDIO->uCardType = FMISDIO_TYPE_SD_LOW;
        }
        else
        {
            pSDIO->uCardType = FMISDIO_TYPE_UNKNOWN;
            DBG_PRINTF("CMD55 CRC error !!\n");
            return FMISDIO_SD_INIT_ERROR;
        }
    }

    // CMD2, CMD3
    if (pSDIO->uCardType != FMISDIO_TYPE_UNKNOWN)
    {
        fmiSDIOCmdAndRsp2(pSDIO, 2, 0x00, CIDBuffer);
        if ((pSDIO->uCardType == FMISDIO_TYPE_MMC) || (pSDIO->uCardType == FMISDIO_TYPE_MMC_SECTOR_MODE))
        {
            // Increase RCA for next MMC card.
            // The RCA value 0 is reserved to set all cards with CMD7.
            // The default value is 1.
            pSDIO->uRCA = (pSDIO->uRCA + 0x10000) & 0xFFFF0000;   // RCA is 16-bit value at MSB
            if (pSDIO->uRCA == 0)
                pSDIO->uRCA = 0x10000;

            if ((status = fmiSDIOCmdAndRsp(pSDIO, 3, pSDIO->uRCA, 0)) != Successful)        // set RCA for MMC
                return status;
        }
        else
        {
            if ((status = fmiSDIOCmdAndRsp(pSDIO, 3, 0x00, 0)) != Successful)       // get RCA for SD
                return status;
            else
                pSDIO->uRCA = (inpw(REG_SDIORSP0) << 8) & 0xffff0000;
        }
    }

    switch (pSDIO->uCardType)
    {
        case FMISDIO_TYPE_SD_HIGH:
            DBG_PRINTF("This is high capacity SD memory card\n");       break;
        case FMISDIO_TYPE_SD_LOW:
            DBG_PRINTF("This is standard capacity SD memory card\n");   break;
        case FMISDIO_TYPE_MMC:
            DBG_PRINTF("This is standard capacity MMC memory card\n");  break;
        case FMISDIO_TYPE_MMC_SECTOR_MODE:
            DBG_PRINTF("This is high capacity MMC memory card\n");      break;
        default:
            sysprintf("ERROR: Unknown card type !!\n");                 break;
    }

    // set data transfer clock
    return Successful;
}


INT fmiSDIOSwitchToHighSpeed(FMI_SDIO_INFO_T *pSDIO)
{
    int volatile status=0;
    UINT16 current_comsumption, busy_status0, busy_status1;
    //UINT16 fun1_info, switch_status;

    outpw(REG_SDIODMACSAR, (UINT32)_fmi_pSDIOHCBuffer);   // set DMA transfer starting address
    outpw(REG_SDIOBLEN, 63);  // 512 bit

    if ((status = fmiSDIOCmdAndRspDataIn(pSDIO, 6, 0x00ffff01)) != Successful)
        return Fail;

    current_comsumption = _fmi_pSDIOHCBuffer[0]<<8 | _fmi_pSDIOHCBuffer[1];
    if (!current_comsumption)
        return Fail;

    //fun1_info =  _fmi_pSDIOHCBuffer[12]<<8 | _fmi_pSDIOHCBuffer[13];
    //switch_status =  _fmi_pSDIOHCBuffer[16] & 0xf;
    busy_status0 = _fmi_pSDIOHCBuffer[28]<<8 | _fmi_pSDIOHCBuffer[29];

    if (!busy_status0)  // function ready
    {
        outpw(REG_SDIODMACSAR, (UINT32)_fmi_pSDIOHCBuffer);   // set DMA transfer starting address
        outpw(REG_SDIOBLEN, 63);  // 512 bit

        if ((status = fmiSDIOCmdAndRspDataIn(pSDIO, 6, 0x80ffff01)) != Successful)
            return Fail;

        // function change timing: 8 clocks
        outpw(REG_SDIOCR, inpw(REG_SDIOCR)|SDCR_8CLK_OE);
        while(inpw(REG_SDIOCR) & SDCR_8CLK_OE);

        current_comsumption = _fmi_pSDIOHCBuffer[0]<<8 | _fmi_pSDIOHCBuffer[1];
        if (!current_comsumption)
            return Fail;

        busy_status1 = _fmi_pSDIOHCBuffer[28]<<8 | _fmi_pSDIOHCBuffer[29];
        if (!busy_status1)
            DBG_PRINTF("switch into high speed mode !!!\n");
        return Successful;
    }
    else
        return Fail;
}


INT fmiSDIOSelectCard(FMI_SDIO_INFO_T *pSDIO)
{
    int volatile status=0;
    int card_no = 0;
    UINT32 arg;

    if ((status = fmiSDIOCmdAndRsp(pSDIO, 7, pSDIO->uRCA, 0)) != Successful)
        return status;

    fmiSDIOCheckRB();

    if (pSDIO == pSDIO0)
        card_no = 0;
    else if (pSDIO == pSDIO1)
        card_no = 1;

    if (pSDIO->uCardType == FMISDIO_TYPE_SD_HIGH)
    {
        _fmi_pSDIOHCBuffer = (UINT8 *)((UINT32)_fmi_ucSDIOHCBuffer | 0x80000000);
        outpw(REG_SDIODMACSAR, (UINT32)_fmi_pSDIOHCBuffer);   // set DMA transfer starting address
        outpw(REG_SDIOBLEN, 7);   // 64 bit

        if ((status = fmiSDIOCmdAndRsp(pSDIO, 55, pSDIO->uRCA, 0)) != Successful)
            return status;
        if ((status = fmiSDIOCmdAndRspDataIn(pSDIO, 51, 0x00)) != Successful)
            return status;

        if ((_fmi_ucSDIOHCBuffer[0] & 0xf) == 0x2)
        {
            // support SD spec v2.0
            status = fmiSDIOSwitchToHighSpeed(pSDIO);
            if (status == Successful)
            {
                fmiSDIO_Set_clock(SDIO_SDHC_FREQ); // set divider
                fmiSDIO_Change_Driver_Strength(card_no, FMISDIO_TYPE_SD_HIGH);
            }
        }

        if ((status = fmiSDIOCmdAndRsp(pSDIO, 55, pSDIO->uRCA, 0)) != Successful)
            return status;
        if ((status = fmiSDIOCmdAndRsp(pSDIO, 6, 0x02, 0)) != Successful)   // set bus width, 0 is 1-bit, 2 is 4-bit
            return status;

        outpw(REG_SDIOCR, inpw(REG_SDIOCR)|SDCR_DBW);       // 4-bit mode
    }
    else if (pSDIO->uCardType == FMISDIO_TYPE_SD_LOW)
    {
        fmiSDIO_Set_clock(SDIO_SD_FREQ);
        fmiSDIO_Change_Driver_Strength(card_no, FMISDIO_TYPE_SD_LOW);

        // set data bus width. ACMD6 for SD card, SDCR_DBW for host.
        if ((status = fmiSDIOCmdAndRsp(pSDIO, 55, pSDIO->uRCA, 0)) != Successful)
            return status;
        if ((status = fmiSDIOCmdAndRsp(pSDIO, 6, 0x02, 0)) != Successful)   // set bus width, 0 is 1-bit, 2 is 4-bit
            return status;

        outpw(REG_SDIOCR, inpw(REG_SDIOCR)|SDCR_DBW);       // 4-bit mode
    }
    else if ((pSDIO->uCardType == FMISDIO_TYPE_MMC) || (pSDIO->uCardType == FMISDIO_TYPE_MMC_SECTOR_MODE))
    {
        if (pSDIO->uCardType == FMISDIO_TYPE_MMC)
            fmiSDIO_Set_clock(SDIO_MMC_FREQ);
        else
            fmiSDIO_Set_clock(SDIO_EMMC_FREQ);
        fmiSDIO_Change_Driver_Strength(card_no, pSDIO->uCardType);

        //--- sent CMD6 to MMC card to set bus width to 4 bits mode
        // set CMD6 argument Access field to 3, Index to 183, Value to 1 (4-bit mode)
        arg = (3 << 24) | (183 << 16) | (1 << 8);
        if ((status = fmiSDIOCmdAndRsp(pSDIO, 6, arg, 0)) != Successful)
            return status;
        fmiSDIOCheckRB();

        outpw(REG_SDIOCR, inpw(REG_SDIOCR)|SDCR_DBW);   // set bus width to 4-bit mode for SD host controller
    }

    if ((status = fmiSDIOCmdAndRsp(pSDIO, 16, SDIO_BLOCK_SIZE, 0)) != Successful) // set block length
        return status;
    outpw(REG_SDIOBLEN, SDIO_BLOCK_SIZE - 1);           // set the block size

    fmiSDIOCommand(pSDIO, 7, 0);

    // According to SD spec v2.0 chapter 4.4,
    // "After the last SD Memory Card bus transaction, the host is required,
    //  to provide 8 (eight) clock cycles for the card to complete the
    //  operation before shutting down the clock."
    outpw(REG_SDIOCR, inpw(REG_SDIOCR)|SDCR_8CLK_OE);

#ifdef _SDIO_USE_INT_
    outpw(REG_SDIOIER, inpw(REG_SDIOIER)|SDIER_BLKD_IEN);
#endif  //_SDIO_USE_INT_

    DBG_PRINTF("end of fmiSDIOSelectCard()\n");

    return Successful;
}

/*-----------------------------------------------------------------------------
 * fmiSDIO_Read_in(), To read data with default black size SDIO_BLOCK_SIZE
 *---------------------------------------------------------------------------*/
INT fmiSDIO_Read_in(FMI_SDIO_INFO_T *pSDIO, UINT32 uSector, UINT32 uBufcnt, UINT32 uDAddr)
{
    return fmiSDIO_Read_in_blksize(pSDIO, uSector, uBufcnt, uDAddr, SDIO_BLOCK_SIZE);
}

/*-----------------------------------------------------------------------------
 * fmiSDIO_Read_in_blksize(), To read data with black size "blksize"
 *---------------------------------------------------------------------------*/
INT fmiSDIO_Read_in_blksize(FMI_SDIO_INFO_T *pSDIO, UINT32 uSector, UINT32 uBufcnt, UINT32 uDAddr, UINT32 blksize)
{
    BOOL volatile bIsSendCmd=FALSE;
    unsigned int volatile reg;
    int volatile i, loop, status;

    //--- check input parameters
    status = fmiSDIOCheckSector(uSector, uBufcnt);
    if (status < 0)
        return status;  // invalid sector

    if (uBufcnt == 0)
    {
        DBG_PRINTF("ERROR: fmiSDIO_Read_in_blksize(): uBufcnt cannot be 0!!\n");
        return FMISDIO_SD_SELECT_ERROR;
    }

    if ((status = fmiSDIOCmdAndRsp(pSDIO, 7, pSDIO->uRCA, 0)) != Successful)
        return status;
    fmiSDIOCheckRB();

//  outpw(REG_SDIOBLEN, 0x1ff);           // 512 bytes
    outpw(REG_SDIOBLEN, blksize - 1);     // the actual byte count is equal to (SDBLEN+1)

    if ((pSDIO->uCardType == FMISDIO_TYPE_SD_HIGH) || (pSDIO->uCardType == FMISDIO_TYPE_MMC_SECTOR_MODE))
        outpw(REG_SDIOARG, uSector);
    else
        outpw(REG_SDIOARG, uSector * blksize);

    outpw(REG_SDIODMACSAR, uDAddr);

    loop = uBufcnt / 255;
    for (i=0; i<loop; i++)
    {
#ifdef _SDIO_USE_INT_
        _fmiSDIO_bIsSDDataReady = FALSE;
#endif  //_SDIO_USE_INT_

        reg = inpw(REG_SDIOCR) & ~SDCR_CMD_CODE;
        reg = reg | 0xff0000;   // set BLK_CNT to 255
        if (bIsSendCmd == FALSE)
        {
            outpw(REG_SDIOCR, reg|(18<<8)|(SDCR_CO_EN | SDCR_RI_EN | SDCR_DI_EN));
            bIsSendCmd = TRUE;
        }
        else
            outpw(REG_SDIOCR, reg | SDCR_DI_EN);

#ifdef _SDIO_USE_INT_
        while(!_fmiSDIO_bIsSDDataReady)
#else
        while(1)
#endif  //_SDIO_USE_INT_
        {
#ifndef _SDIO_USE_INT_
            if ((inpw(REG_SDIOISR) & SDISR_BLKD_IF) && (!(inpw(REG_SDIOCR) & SDCR_DI_EN)))
            {
                outpw(REG_SDIOISR, SDISR_BLKD_IF);
                break;
            }
#endif
            fmiSDIO_CardStatus(pSDIO);
            if (pSDIO->bIsCardInsert == FALSE)
                return FMISDIO_NO_SD_CARD;

            /* Call schedule() to release CPU power to other tasks during waiting SIC/DMA completed. */
            schedule();
        }

        if (!(inpw(REG_SDIOISR) & SDISR_CRC_7))       // check CRC7
        {
            DBG_PRINTF("fmiSDIO_Read_in_blksize(): response error!\n");
            return FMISDIO_SD_CRC7_ERROR;
        }

        if (!(inpw(REG_SDIOISR) & SDISR_CRC_16))      // check CRC16
        {
            DBG_PRINTF("fmiSDIO_Read_in_blksize() :read data error!\n");
            return FMISDIO_SD_CRC16_ERROR;
        }
    }

    loop = uBufcnt % 255;
    if (loop != 0)
    {
#ifdef _SDIO_USE_INT_
        _fmiSDIO_bIsSDDataReady = FALSE;
#endif  //_SDIO_USE_INT_

        reg = inpw(REG_SDIOCR) & (~SDCR_CMD_CODE);
        reg = reg & (~SDCR_BLKCNT);
        reg |= (loop << 16);    // setup SDCR_BLKCNT

        if (bIsSendCmd == FALSE)
        {
            outpw(REG_SDIOCR, reg|(18<<8)|(SDCR_CO_EN | SDCR_RI_EN | SDCR_DI_EN));
            bIsSendCmd = TRUE;
        }
        else
            outpw(REG_SDIOCR, reg | SDCR_DI_EN);

#ifdef _SDIO_USE_INT_
        while(!_fmiSDIO_bIsSDDataReady)
#else
        while(1)
#endif  //_SDIO_USE_INT_
        {
#ifndef _SDIO_USE_INT_
            if ((inpw(REG_SDIOISR) & SDISR_BLKD_IF) && (!(inpw(REG_SDIOCR) & SDCR_DI_EN)))
            {
                outpw(REG_SDIOISR, SDISR_BLKD_IF);
                break;
            }
#endif
            fmiSDIO_CardStatus(pSDIO);
            if (pSDIO->bIsCardInsert == FALSE)
                return FMISDIO_NO_SD_CARD;

            /* Call schedule() to release CPU power to other tasks during waiting SIC/DMA completed. */
            schedule();
        }

        if (!(inpw(REG_SDIOISR) & SDISR_CRC_7))       // check CRC7
        {
            DBG_PRINTF("fmiSDIO_Read_in_blksize(): response error!\n");
            return FMISDIO_SD_CRC7_ERROR;
        }

        if (!(inpw(REG_SDIOISR) & SDISR_CRC_16))      // check CRC16
        {
            DBG_PRINTF("fmiSDIO_Read_in_blksize(): read data error!\n");
            return FMISDIO_SD_CRC16_ERROR;
        }
    }

    if (fmiSDIOCmdAndRsp(pSDIO, 12, 0, 0))      // stop command
    {
        DBG_PRINTF("stop command fail !!\n");
        return FMISDIO_SD_CRC7_ERROR;
    }
    fmiSDIOCheckRB();

    fmiSDIOCommand(pSDIO, 7, 0);

    // According to SD spec v2.0 chapter 4.4,
    // "After the last SD Memory Card bus transaction, the host is required,
    //  to provide 8 (eight) clock cycles for the card to complete the
    //  operation before shutting down the clock."
    outpw(REG_SDIOCR, inpw(REG_SDIOCR)|SDCR_8CLK_OE);

    return Successful;
}

/*-----------------------------------------------------------------------------
 * fmiSDIO_Write_in(), To write data with static black size SDIO_BLOCK_SIZE
 *---------------------------------------------------------------------------*/
INT fmiSDIO_Write_in(FMI_SDIO_INFO_T *pSDIO, UINT32 uSector, UINT32 uBufcnt, UINT32 uSAddr)
{
    BOOL volatile bIsSendCmd=FALSE;
    unsigned int volatile reg;
    int volatile i, loop, status;

    //--- check input parameters
    status = fmiSDIOCheckSector(uSector, uBufcnt);
    if (status < 0)
        return status;  // invalid sector

    if (uBufcnt == 0)
    {
        DBG_PRINTF("ERROR: fmiSDIO_Write_in(): uBufcnt cannot be 0!!\n");
        return FMISDIO_SD_SELECT_ERROR;
    }

    if ((status = fmiSDIOCmdAndRsp(pSDIO, 7, pSDIO->uRCA, 0)) != Successful)
        return status;
    fmiSDIOCheckRB();

    // According to SD Spec v2.0, the write CMD block size MUST be 512, and the start address MUST be 512*n.
    outpw(REG_SDIOBLEN, SDIO_BLOCK_SIZE - 1);           // set the block size

    if ((pSDIO->uCardType == FMISDIO_TYPE_SD_HIGH) || (pSDIO->uCardType == FMISDIO_TYPE_MMC_SECTOR_MODE))
        outpw(REG_SDIOARG, uSector);
    else
        outpw(REG_SDIOARG, uSector * SDIO_BLOCK_SIZE);  // set start address for SD CMD

    outpw(REG_SDIODMACSAR, uSAddr);
    loop = uBufcnt / 255;   // the maximum block count is 0xFF=255 for register SDCR[BLK_CNT]
    for (i=0; i<loop; i++)
    {
#ifdef _SDIO_USE_INT_
        _fmiSDIO_bIsSDDataReady = FALSE;
#endif  //_SDIO_USE_INT_

        reg = inpw(REG_SDIOCR) & 0xff00c080;
        reg = reg | 0xff0000;   // set BLK_CNT to 0xFF=255
        if (!bIsSendCmd)
        {
            outpw(REG_SDIOCR, reg|(25<<8)|(SDCR_CO_EN | SDCR_RI_EN | SDCR_DO_EN));
            bIsSendCmd = TRUE;
        }
        else
            outpw(REG_SDIOCR, reg | SDCR_DO_EN);

#ifdef _SDIO_USE_INT_
        while(!_fmiSDIO_bIsSDDataReady)
#else
        while(1)
#endif  //_SDIO_USE_INT_
        {
#ifndef _SDIO_USE_INT_
            if ((inpw(REG_SDIOISR) & SDISR_BLKD_IF) && (!(inpw(REG_SDIOCR) & SDCR_DO_EN)))
            {
                outpw(REG_SDIOISR, SDISR_BLKD_IF);
                break;
            }
#endif
            fmiSDIO_CardStatus(pSDIO);
            if (pSDIO->bIsCardInsert == FALSE)
                return FMISDIO_NO_SD_CARD;

            /* Call schedule() to release CPU power to other tasks during waiting SIC/DMA completed. */
            schedule();
        }

        if ((inpw(REG_SDIOISR) & SDISR_CRC_IF) != 0)      // check CRC
        {
            DBG_PRINTF("1. fmiSDIO_Write:write data error [0x%x]\n", inpw(REG_SDIOISR));
            outpw(REG_SDIOISR, SDISR_CRC_IF);
            return FMISDIO_SD_CRC_ERROR;
        }
    }

    loop = uBufcnt % 255;
    if (loop != 0)
    {
#ifdef _SDIO_USE_INT_
        _fmiSDIO_bIsSDDataReady = FALSE;
#endif  //_SDIO_USE_INT_

        reg = (inpw(REG_SDIOCR) & 0xff00c080) | (loop << 16);
        if (!bIsSendCmd)
        {
            outpw(REG_SDIOCR, reg|(25<<8)|(SDCR_CO_EN | SDCR_RI_EN | SDCR_DO_EN));
            bIsSendCmd = TRUE;
        }
        else
            outpw(REG_SDIOCR, reg | SDCR_DO_EN);

#ifdef _SDIO_USE_INT_
        while(!_fmiSDIO_bIsSDDataReady)
#else
        while(1)
#endif  //_SDIO_USE_INT_
        {
#ifndef _SDIO_USE_INT_
            if ((inpw(REG_SDIOISR) & SDISR_BLKD_IF) && (!(inpw(REG_SDIOCR) & SDCR_DO_EN)))
            {
                outpw(REG_SDIOISR, SDISR_BLKD_IF);
                break;
            }
#endif
            fmiSDIO_CardStatus(pSDIO);
            if (pSDIO->bIsCardInsert == FALSE)
                return FMISDIO_NO_SD_CARD;

            /* Call schedule() to release CPU power to other tasks during waiting SIC/DMA completed. */
            schedule();
        }

        if ((inpw(REG_SDIOISR) & SDISR_CRC_IF) != 0)      // check CRC
        {
            DBG_PRINTF("2. fmiSDIO_Write:write data error [0x%x]!\n", inpw(REG_SDIOISR));
            outpw(REG_SDIOISR, SDISR_CRC_IF);
            return FMISDIO_SD_CRC_ERROR;
        }
    }
    outpw(REG_SDIOISR, SDISR_CRC_IF);

    if (fmiSDIOCmdAndRsp(pSDIO, 12, 0, 0))      // stop command
    {
        DBG_PRINTF("stop command fail !!\n");
        return FMISDIO_SD_CRC7_ERROR;
    }
    fmiSDIOCheckRB();

    fmiSDIOCommand(pSDIO, 7, 0);

    // According to SD spec v2.0 chapter 4.4,
    // "After the last SD Memory Card bus transaction, the host is required,
    //  to provide 8 (eight) clock cycles for the card to complete the
    //  operation before shutting down the clock."
    outpw(REG_SDIOCR, inpw(REG_SDIOCR)|SDCR_8CLK_OE);

    return Successful;
}


VOID fmiGet_SDIO_info(FMI_SDIO_INFO_T *pSDIO, SDIO_DISK_DATA_T *_info)
{
    unsigned int i;
    unsigned int R_LEN, C_Size, MULT, size;
    unsigned int Buffer[4];
    unsigned char *ptr;
    int volatile status;

    fmiSDIOCmdAndRsp2(pSDIO, 9, pSDIO->uRCA, Buffer);

    DBG_PRINTF("max. data transfer rate [%x][%08x]\n", Buffer[0]&0xff, Buffer[0]);
    DBG_PRINTF("CSD = 0x%08X 0x%08X 0x%08X 0x%08X\n", Buffer[0], Buffer[1], Buffer[2], Buffer[3]);

    if ((pSDIO->uCardType == FMISDIO_TYPE_MMC) || (pSDIO->uCardType == FMISDIO_TYPE_MMC_SECTOR_MODE))
    {
        // for MMC/eMMC card
        if ((Buffer[0] & 0xc0000000) == 0xc0000000)
        {
            // CSD_STRUCTURE [127:126] is 3
            // CSD version depend on EXT_CSD register in eMMC v4.4 for card size > 2GB
            fmiSDIOCmdAndRsp(pSDIO, 7, pSDIO->uRCA, 0);

            _fmi_pSDIOHCBuffer = (UINT8 *)((UINT32)_fmi_ucSDIOHCBuffer | 0x80000000);
            outpw(REG_SDIODMACSAR, (UINT32)_fmi_pSDIOHCBuffer);   // set DMA transfer starting address
            outpw(REG_SDIOBLEN, 511);     // read 512 bytes for EXT_CSD
            if ((status = fmiSDIOCmdAndRspDataIn(pSDIO, 8, 0x00)) != Successful)
                return;

            fmiSDIOCommand(pSDIO, 7, 0);

            // According to SD spec v2.0 chapter 4.4,
            // "After the last SD Memory Card bus transaction, the host is required,
            //  to provide 8 (eight) clock cycles for the card to complete the
            //  operation before shutting down the clock."
            outpw(REG_SDIOCR, inpw(REG_SDIOCR)|SDCR_8CLK_OE);

            _info->totalSectorN = (*(UINT32 *)(_fmi_pSDIOHCBuffer+212));
            _info->diskSize = _info->totalSectorN / 2;
        }
        else
        {
            // CSD version v1.0/1.1/1.2 in eMMC v4.4 spec for card size <= 2GB
            R_LEN = (Buffer[1] & 0x000f0000) >> 16;
            C_Size = ((Buffer[1] & 0x000003ff) << 2) | ((Buffer[2] & 0xc0000000) >> 30);
            MULT = (Buffer[2] & 0x00038000) >> 15;
            size = (C_Size+1) * (1<<(MULT+2)) * (1<<R_LEN);

            _info->diskSize = size / 1024;
            _info->totalSectorN = size / 512;
        }
    }
    else
    {
        // for SD/SDHC card
        if (Buffer[0] & 0xc0000000)
        {
            // CSD version 2.0 in SD v2.0 spec for SDHC card
            C_Size = ((Buffer[1] & 0x0000003f) << 16) | ((Buffer[2] & 0xffff0000) >> 16);
            size = (C_Size+1) * 512;    // Kbytes

            _info->diskSize = size;
            _info->totalSectorN = size << 1;
        }
        else
        {
            // CSD version 1.0 in SD v2.0 spec for SD card
            R_LEN = (Buffer[1] & 0x000f0000) >> 16;
            C_Size = ((Buffer[1] & 0x000003ff) << 2) | ((Buffer[2] & 0xc0000000) >> 30);
            MULT = (Buffer[2] & 0x00038000) >> 15;
            size = (C_Size+1) * (1<<(MULT+2)) * (1<<R_LEN);

            _info->diskSize = size / 1024;
            _info->totalSectorN = size / 512;
        }
    }
    _info->sectorSize = 512;

    g_sdio_max_valid_sector = _info->totalSectorN;

    fmiSDIOCmdAndRsp2(pSDIO, 10, pSDIO->uRCA, Buffer);

    _info->vendor[0] = (Buffer[0] & 0xff000000) >> 24;
    ptr = (unsigned char *)Buffer;
    ptr = (unsigned char *)Buffer + 3;
    for (i=0; i<5; i++)
        _info->product[i] = *ptr++;
    ptr = (unsigned char *)Buffer + 9;
    for (i=0; i<4; i++)
        _info->serial[i] = *ptr++;

    DBG_PRINTF("SD card CID is %08X-%08X-%08X-%08X\n", Buffer[0], Buffer[1], Buffer[2], Buffer[3]);
}


/*-----------------------------------------------------------------------------
 * 2011/6/24, To show information about inserted card.
 *---------------------------------------------------------------------------*/
VOID fmiSDIO_Show_info(int sdport)
{
    FMI_SDIO_INFO_T *pSDIO;
    SDIO_DISK_DATA_T info;

    switch (sdport)
    {
        case 0:  pSDIO = pSDIO0; break;
        case 1:  pSDIO = pSDIO1; break;
        default: pSDIO = pSDIO0; break;
    }

    // get information about SD card
    fmiGet_SDIO_info(pSDIO, &info);

    // show card type
    switch (pSDIO->uCardType)
    {
        case FMISDIO_TYPE_SD_HIGH:  DBG_PRINTF("    SDHC card insert to SD port %d!\n",      sdport); break;
        case FMISDIO_TYPE_SD_LOW:   DBG_PRINTF("    SD1.1 card insert to SD port %d!\n",     sdport); break;
        case FMISDIO_TYPE_MMC:      DBG_PRINTF("    MMC card insert to SD port %d!\n",       sdport); break;
        case FMISDIO_TYPE_MMC_SECTOR_MODE:
                                    DBG_PRINTF("    High capacity MMC card insert to SD port %d!\n", sdport); break;
        default:                    DBG_PRINTF("    Unknown card insert to SD port %d!\n", sdport); break;
    }

    DBG_PRINTF("    SD Card size = %d Kbytes\n", info.diskSize);
    DBG_PRINTF("                 = %d bytes * %d sectors\n", info.sectorSize, info.totalSectorN);
    return;
}
