/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.  *
 *                                                              *
 ****************************************************************/

#include "string.h"
#include "stdlib.h"

#include "w55fa92_reg.h"
#include "w55fa92_spu.h"
#include "spu.h"

#define E_SUCCESS   0

UINT8 DacOnOffLevel;

/*
static void delay(UINT32 kk)
{
    UINT32 ii, jj;

    for(ii=0; ii < kk; ii++)
    {
        for(jj=0; jj < 0x10; jj++);
    }
}
*/

VOID DrvSPU_WriteDACReg (
    UINT8 DACRegIndex,
    UINT8 DACRegData
)
{
    UINT32 jj;
    UINT32 u32Reg = 0x30810000;     // clock divider = 0x30, ID = 0x80

    u32Reg |= DACRegIndex << 8;
    u32Reg |= DACRegData;
    while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);
    outp32(REG_SPU_DAC_CTRL, u32Reg);
    //delay(2);
    for(jj=0; jj < 0x20; jj++);
    while(inp32(REG_SPU_DAC_CTRL) & V_I2C_BUSY);
}


VOID spuDacPLLEnable (void)
{
    DrvSPU_WriteDACReg(0x05, 0x3F);
}


VOID spuDacPrechargeEnable (void)
{
    // software reset DAC in I2S master
    DrvSPU_WriteDACReg(0x08, 0x00);
    DrvSPU_WriteDACReg(0x08, 0x02);

    // keep DAC ON and enable DAC channel
    DrvSPU_WriteDACReg(0x05, 0x33);

    // set DAC analog volume to mute
    DrvSPU_WriteDACReg(0x00, 0x1f);
    DrvSPU_WriteDACReg(0x01, 0x1f);

    // set DAC bias enable
    DrvSPU_WriteDACReg(0x05, 0x13);

    // set PCHGL/PCHGR=1, precharge with big AC coupled Cap. (220uF) for Headphone(HP).  ? Reg_0x03 = 0x35
    DrvSPU_WriteDACReg(0x03, 0x35);
}


ERRCODE
DrvSPU_ClearInt(
    E_DRVSPU_CHANNEL eChannel,
    UINT32 u32InterruptFlag
)
{
//    if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
    {
        // wait to finish previous channel settings
        while(inp32(REG_SPU_CH_CTRL) & CH_FN);

        // load previous channel settings
        outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));
        outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
        while(inp32(REG_SPU_CH_CTRL) & CH_FN);

        // set new channel settings for previous channel settings
//        if (u32InterruptFlag & DRVSPU_USER_INT)
        {
            outp32(REG_SPU_CH_EVENT, (inp32(REG_SPU_CH_EVENT) & ~0x3F00) | EV_USR_FG);
        }
//        if (u32InterruptFlag & DRVSPU_SILENT_INT)
        {
            outp32(REG_SPU_CH_EVENT, (inp32(REG_SPU_CH_EVENT) & ~0x3F00) | EV_SLN_FG);
        }
//        if (u32InterruptFlag & DRVSPU_LOOPSTART_INT)
        {
            outp32(REG_SPU_CH_EVENT, (inp32(REG_SPU_CH_EVENT) & ~0x3F00) | EV_LP_FG);
        }
//        if (u32InterruptFlag & DRVSPU_END_INT)
        {
            outp32(REG_SPU_CH_EVENT, (inp32(REG_SPU_CH_EVENT) & ~0x3F00) | EV_END_FG);
        }
//        if (u32InterruptFlag & DRVSPU_ENDADDRESS_INT)
        {
            outp32(REG_SPU_CH_EVENT, (inp32(REG_SPU_CH_EVENT) & ~0x3F00) | END_FG);
        }
//        if (u32InterruptFlag & DRVSPU_THADDRESS_INT)
        {
            outp32(REG_SPU_CH_EVENT, (inp32(REG_SPU_CH_EVENT) & ~0x3F00) | TH_FG);
        }
        outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~AT_CLR_EN);         // clear Auto Clear Enable bit
        outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) & ~DRVSPU_UPDATE_ALL_PARTIALS);
        outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | (DRVSPU_UPDATE_IRQ_PARTIAL + DRVSPU_UPDATE_PARTIAL_SETTINGS));
        while(inp32(REG_SPU_CH_CTRL) & CH_FN);

        return E_SUCCESS;
    }
//    else
//        return -1;
}


ERRCODE
DrvSPU_DisableInt(
    E_DRVSPU_CHANNEL eChannel,
    UINT32 u32InterruptFlag
)
{
//    if ( ((INT)eChannel >=eDRVSPU_CHANNEL_0) && ((INT)eChannel <=eDRVSPU_CHANNEL_31) )
    {
        // wait to finish previous channel settings
        while(inp32(REG_SPU_CH_CTRL) & CH_FN);

        // load previous channel settings
        outp32(REG_SPU_CH_CTRL, (inp32(REG_SPU_CH_CTRL) & ~CH_NO) | (eChannel << 24));
        outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | DRVSPU_LOAD_SELECTED_CHANNEL);
        while(inp32(REG_SPU_CH_CTRL) & CH_FN);

        // set new channel settings for previous channel settings
//        if (u32InterruptFlag & DRVSPU_USER_INT)
        {
            outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~EV_USR_EN);
        }
//        if (u32InterruptFlag & DRVSPU_SILENT_INT)
        {
            outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~EV_SLN_EN);
        }
//        if (u32InterruptFlag & DRVSPU_LOOPSTART_INT)
        {
            outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~EV_LP_EN);
        }
//        if (u32InterruptFlag & DRVSPU_END_INT)
        {
            outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~EV_END_EN);
        }
//        if (u32InterruptFlag & DRVSPU_ENDADDRESS_INT)
        {
            outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~END_EN);
        }
//        if (u32InterruptFlag & DRVSPU_THADDRESS_INT)
        {
            outp32(REG_SPU_CH_EVENT, inp32(REG_SPU_CH_EVENT) & ~TH_EN);
        }
        outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) & ~DRVSPU_UPDATE_ALL_PARTIALS);
        outp32(REG_SPU_CH_CTRL, inp32(REG_SPU_CH_CTRL) | (DRVSPU_UPDATE_IRQ_PARTIAL + DRVSPU_UPDATE_PARTIAL_SETTINGS));
        while(inp32(REG_SPU_CH_CTRL) & CH_FN);

        return E_SUCCESS;
    }

//    else
//        return -1;
}


ERRCODE
DrvSPU_Open(void)
{
    UINT8 ii;

    // enable SPU engine clock
    outp32(REG_AHBCLK, inp32(REG_AHBCLK) | ADO_CKE | SPU_CKE | HCLK4_CKE);          // enable SPU engine clock

    // disable SPU engine
//  outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) & ~SPU_EN);
    outp32(REG_SPU_CTRL, 0x00);

    // given FIFO size = 4
    outp32(REG_SPU_CTRL, 0x04000000);

    outp32(REG_SPU_DAC_PAR, inp32(REG_SPU_DAC_PAR) & ~DAC_RST);
    outp32(REG_SPU_DAC_PAR, inp32(REG_SPU_DAC_PAR) | DAC_RST);

    // reset SPU engine
//  outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) | SPU_EN);
    outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) & ~SPU_SWRST);

    outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) | SPU_SWRST);
    outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) & ~SPU_SWRST);

    // enable I2S interface
    outp32(REG_SPU_CTRL, inp32(REG_SPU_CTRL) | SPU_I2S_EN);

    // disable all channels
    outp32(REG_SPU_CH_EN, 0x00);

    for (ii=0; ii<32; ii++)
    {
        DrvSPU_ClearInt((E_DRVSPU_CHANNEL)ii, DRVSPU_ALL_INT);
        DrvSPU_DisableInt((E_DRVSPU_CHANNEL)ii, DRVSPU_ALL_INT);
    }

    return E_SUCCESS;
}
