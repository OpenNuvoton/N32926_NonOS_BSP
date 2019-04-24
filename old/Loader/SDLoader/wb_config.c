
/***************************************************************************
 *                                                                         *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              *
 *                                                                         *
 ***************************************************************************/
/****************************************************************************
 *
 * FILENAME : wb_config.c
 *
 * VERSION  : 1.1
 *
 * DESCRIPTION :
 *               PLL control functions of Nuvoton ARM9 MCU
 *
 * HISTORY
 *   2008-06-25  Ver 1.0 draft by Min-Nan Cheng
 * Modification
 *   2011-06-01  Ver 1.1 draft by Shih-Wen Chou
 *
 *      IBR set clocks default value
 *          UPLL= 240MHz
 *          SYS = 120MHz
 *          CPU = 60MHz
 *          HCLK = 60MHz
 *
 *
 *
 *
 ****************************************************************************/
#include <string.h>
#include "wblib.h"
#define REAL_CHIP

static UINT32 g_u32SysClkSrc;
static UINT32 g_u32UpllHz = 240000000; //g_u32ApllHz=240000000, g_u32SysHz = 120000000, g_u32CpuHz = 60000000, g_u32HclkHz = 60000000;
//static UINT32 g_u32MpllHz = 180000000;
static UINT32 /*g_i32REG_APLL,*/ g_i32REG_UPLL, g_i32REG_MPLL;
static UINT32 g_u32ExtClk = 12000000;

UINT32 sysGetHCLK234Clock(void);

extern UINT8  _tmp_buf[];

//#define W55FA92_A_VERSION
#ifdef W55FA92_A_VERSION
    #define REG_DLLMODE_R 0xb0003058
#else
    #define REG_DLLMODE_R   0xb0003058  /* Not fix in B version. still read in 3058 */
#endif


extern UINT32 u32UartPort;

BOOL bIsFirstReadClkSkew = TRUE;
UINT32 u32ClkSkewInit = 0;

//#define DBG_PRINTF        sysprintf
#define DBG_PRINTF(...)
/*-----------------------------------------------------------------------------------------------------------
 *
 * Function : sysGetPLLOutputHz
 *
 * DESCRIPTION :
 *               According to the external clock and expected PLL output clock to get the content of PLL controll register
 *
 * Parameters
 *              eSysPll : eSYS_APLL or eSYS_UPLL
 *          u32FinKHz: External clock. Unit: KHz
 * Return
 *          PLL clock.
 * HISTORY
 *               2010-07-15
 *
-----------------------------------------------------------------------------------------------------------*/
UINT32 sysGetPLLOutputHz(
    E_SYS_SRC_CLK eSysPll,
    UINT32 u32FinHz
    )
{
    UINT32 u32PllCntlReg=0, u32Fout;
    UINT32 NF, NR, NO;
    UINT32 u32NOArray[] = { 1, 2, 4, 8};

// 2014/4/30: NandLoader don't need it
//  if(eSysPll==eSYS_APLL)
//      u32PllCntlReg = inp32(REG_APLLCON);
//  else
    if(eSysPll==eSYS_UPLL)
        u32PllCntlReg = inp32(REG_UPLLCON);
    else if(eSysPll==eSYS_MPLL)
        u32PllCntlReg = inp32(REG_MPLLCON);

    if(u32PllCntlReg&0x10000)           //PLL power down.
        return 0;

    NF = (u32PllCntlReg & 0x7F)<<1;
    NR = (u32PllCntlReg & 0x780)>>7;
    NO = u32NOArray[((u32PllCntlReg&0x1800)>>11)];

    DBG_PRINTF("NR, NF, NO = %d, %d, %d\n", NR, NF, NO);
    u32Fout = u32FinHz/NO/NR*NF;
    #ifdef REAL_CHIP
    return u32Fout;
    #else
    return u32FinHz;
    #endif
}
/*-----------------------------------------------------------------------------------------------------------
 *
 * Function : sysGetPLLControlRegister
 *
 * DESCRIPTION :
 *               According to the external clock and expected PLL output clock to get the content of PLL controll register
 *
 * Parameters
 *              u32FinKHz : External clock.  Unit:KHz
 *          u32TargetKHz: PLL output clock. Unit:KHz
 * Return
 *                  0 : No any fit value for the specified PLL output clock
 *          PLL control register.
 * HISTORY
 *               2011-10-21
 *
-----------------------------------------------------------------------------------------------------------*/

BOOL bIsCheckConstraint = TRUE;
void sysCheckPllConstraint(BOOL bIsCheck)
{
    bIsCheckConstraint = bIsCheck;
}

#define MIN_FBDV_M      4
#define MAX_FBDV_M      256
#define MIN_INDV_N      2
#define MAX_INDV_N      16
INT32 _sysGetPLLControlRegister(UINT32 u32FinKHz, UINT32 u32TargetHz)
{
    UINT32 u32ClkOut;
    UINT32 u32NO;
    UINT32 u32IdxM, u32IdxN;
    INT32 i32IdxNO;
    UINT32 u32NOArray[] = { 1, 2, 4, 8};
#if 0
    for(u32IdxM=MIN_FBDV_M;u32IdxM<MAX_FBDV_M;u32IdxM=u32IdxM+1)
    {//u32IdxM=NR >=4. Fedback divider.
        for(u32IdxN=MIN_INDV_N;u32IdxN<MAX_INDV_N;u32IdxN=u32IdxN+1)
        {//u32IdxN=N >=2. (NR = u32IdxN). Input divider
            for(i32IdxNO=0;i32IdxNO<4;i32IdxNO=i32IdxNO+1)
            {
#else
    /* To get little jiter on PLL output, i32IdxNO has better =0x03 or 0x02 */
    for(i32IdxNO=3;i32IdxNO>0;i32IdxNO=i32IdxNO-1)
    {
        for(u32IdxM=MIN_FBDV_M;u32IdxM<MAX_FBDV_M;u32IdxM=u32IdxM+1)
        {//u32IdxM=NR >=4. Fedback divider.
            for(u32IdxN=MIN_INDV_N;u32IdxN<MAX_INDV_N;u32IdxN=u32IdxN+1)
            {//u32IdxN=N >=2. (NR = u32IdxN). Input divider
#endif
                if(bIsCheckConstraint==TRUE)
                {
                    if((u32FinKHz/u32IdxN)>50000)                   /* 1MHz < FIN/NR < 50MHz */
                        continue;
                    if((u32FinKHz/u32IdxN)<1000)
                        continue;
                }
                u32NO = u32NOArray[i32IdxNO];               /* FOUT = (FIN * NF/NR)/NO */
                u32ClkOut = u32FinKHz*u32IdxM/u32IdxN/u32NO;    /* Where NF = u32IdxM,  NR = u32IdxN, NO=u32NOArray[i32IdxNO]. */
                if((u32ClkOut*1000)==u32TargetHz)
                {
                    if(bIsCheckConstraint==TRUE)
                    {
                        if((u32ClkOut*u32NO)<500000)            /* 500MHz <= FIN/NO < 1500MHz */
                            continue;
                        if((u32ClkOut*u32NO)>1500000)
                            continue;
                    }
                    DBG_PRINTF("\n****************\n");
                    DBG_PRINTF("M = 0x%x\n",u32IdxM);
                    DBG_PRINTF("N = 0x%x\n",u32IdxN);
                    DBG_PRINTF("NO = 0x%x\n",i32IdxNO);
                    return ((u32IdxM>>1) | (u32IdxN<<7) | (i32IdxNO<<11));
                }
            }
        }
    }
    return -1;
}

/*-----------------------------------------------------------------------------------------------------------
* Function: sysSetPLLControlRegister
*
* Parameters:
*              u32PllValue - [in], PLL setting value
*
* Returns:
*      None
*
* Description:
*              To set the PLL control register.
*
-----------------------------------------------------------------------------------------------------------*/
void
sysSetPLLControlRegister(
    E_SYS_SRC_CLK eSysPll,
    UINT32 u32PllValue
    )
{
    if(eSysPll==eSYS_APLL)
        outp32(REG_APLLCON, u32PllValue);
    else if(eSysPll==eSYS_APLL)
        outp32(REG_UPLLCON, u32PllValue);
}

/*-----------------------------------------------------------------------------------------------------------
* Function: sysSetSystemClock
*
* Parameters:
*              u32PllValue - [in], PLL setting value
*
* Returns:
*      None
*
* Description:
*              To set the PLL control register.
*
*Note:
*       Switch systetm clock to external clock first.
*
*       refresh rate = REPEAT/Fmclk
*       1. Disable interrupt
*       2. Enter Self-refresh
*       3. Switch to external clock
*       4. Adjustment the sys divider.
*
*
*
*
*
*
-----------------------------------------------------------------------------------------------------------*/
void _sysClockSwitch(register E_SYS_SRC_CLK eSrcClk,
                        register UINT32 u32Hclk,
                        register UINT32 u32PllReg,
                        register UINT32 u32SysDiv)
{
    UINT32 u32IntTmp;
    /* disable interrupt (I will recovery it after clock changed) */
    u32IntTmp = inp32(REG_AIC_IMR);
    outp32(REG_AIC_MDCR, 0xFFFFFFFE);

    /* DRAM enter self refresh mode */
    outp32(REG_SDCMD, inp32(REG_SDCMD) | (SELF_REF| REF_CMD));
    __asm
    {
        mov         r2, #10000
        mov     r1, #0
        mov     r0, #1
    loop1:  add         r1, r1, r0
        cmp         r1, r2
        bne     loop1
    }
#if 0
    //outp32(REG_SDCMD, (inp32(REG_SDCMD) & ~0x20) | 0x10);
    /* Switch to external clock and divider to 0*/
    outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & ~(SYSTEM_N1 | SYSTEM_S | SYSTEM_N0)) );
#else
    outp32(REG_UPLLCON, inp32(REG_UPLLCON) | BIT15);
#endif

    if(eSrcClk==eSYS_EXT)
    {
        /* Fill system clock divider */
        outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & ~(SYSTEM_N1|SYSTEM_S|SYSTEM_N0)) | u32SysDiv );
    }
    else if((eSrcClk==eSYS_APLL)|| (eSrcClk==eSYS_UPLL))
    {
        //outp32(REG_CLKDIV0,  (inp32(REG_CLKDIV0) | 0x01));    //PLL/3 Safe consider
        /* system clock always comes from UPLL */
        outp32(REG_UPLLCON, u32PllReg | BIT15);

        __asm
        {
            mov         r2, #10000
            mov     r1, #0
            mov     r0, #1
        loop1a: add         r1, r1, r0
            cmp         r1, r2
            bne     loop1a
        }

        /* Fill system clock divider */
        outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & ~(SYSTEM_N1|SYSTEM_S|SYSTEM_N0)) |
                            u32SysDiv);
    }

    __asm
    {
        ;mov    r2, #20000
        mov         r2, #20000
        mov     r1, #0
        mov     r0, #1
    loop2:  add         r1, r1, r0
        cmp         r1, r2
        bne     loop2
    }

    outp32(REG_UPLLCON, inp32(REG_UPLLCON) & ~BIT15);

     /* DRAM escape self refresh mode */
    //outp32(REG_SDCMD, inp32(REG_SDCMD) & ~REF_CMD);
    outp32(0xB0003004,  0x20);

    __asm
    {
        mov     r2, #60000
        mov     r1, #0
        mov     r0, #1
    loop3a: add         r1, r1, r0
        cmp         r1, r2
        bne     loop3a
    }

    outp32(REG_AIC_MECR, u32IntTmp);

}
void _sysClockSwitchStart(E_SYS_SRC_CLK eSrcClk,
                        UINT32 u32Hclk,
                        UINT32 u32RegPll,
                        UINT32 u32SysDiv)
{
    UINT32   vram_base, aic_status = 0, aic_statush = 0;
    BOOL bIsCacheState = FALSE;
    UINT32 u32CacheMode;
    VOID    (*wb_func)(E_SYS_SRC_CLK,
                    UINT32,
                    UINT32,
                    UINT32);

    aic_status = inpw(REG_AIC_IMR);                 //Disable interrupt
    aic_statush = inpw(REG_AIC_IMRH);                   //Disable interrupt
    outpw(REG_AIC_MDCR, 0xFFFFFFFF);
    outpw(REG_AIC_MDCRH, 0xFFFFFFFF);



    vram_base = PD_RAM_BASE;
    memcpy((char *)((UINT32)_tmp_buf | 0x80000000),
            (char *)((UINT32)vram_base | 0x80000000),
            PD_RAM_SIZE);                   //Backup RAM content
    memcpy((VOID *)((UINT32)vram_base | 0x80000000),
            (VOID *)((UINT32)_sysClockSwitch | 0x80000000),
            PD_RAM_SIZE);                   //

    if(sysGetCacheState()==TRUE){
        //DBG_PRINTF("Cache enable\n");
        bIsCacheState = TRUE;
        u32CacheMode = sysGetCacheMode();
        sysDisableCache();
        sysFlushCache(I_D_CACHE);
    //}else{
    //    DBG_PRINTF("Cache disable\n");
    }

    wb_func = (void(*)(E_SYS_SRC_CLK,
                    UINT32, //u32Hclk
                    UINT32,
                    UINT32)) vram_base;

    DBG_PRINTF("SYS_DIV = %x\n", u32SysDiv);
//  DBG_PRINTF("CPU_DIV = %x\n", u32CpuDiv);
//  DBG_PRINTF("APB_DIV = %x\n", u32ApbDiv);
    DBG_PRINTF("Jump to SRAM\n");
    wb_func(eSrcClk,
            u32Hclk,
            u32RegPll,
            u32SysDiv);

    if(bIsCacheState==TRUE)
        sysEnableCache(u32CacheMode);

    DBG_PRINTF("Calibration Value = 0x%x\n", inp32(REG_SDOPM));
    //Restore VRAM
    memcpy((VOID *)((UINT32)vram_base | 0x80000000),
            (VOID *)((UINT32)_tmp_buf | 0x80000000),
            PD_RAM_SIZE);

    outpw(REG_AIC_MECR, aic_status);        // Restore AIC setting
    outpw(REG_AIC_MECRH, aic_statush);      // Restore AIC setting
}

/*-----------------------------------------------------------------------------------------------------------
* Function: sysSetSystemClock
*
* Parameters:
*              u32PllHz         - [in], Specified PLL Clock
*              u32SysHz     - [in], Specified SYS Clock
* Returns:
*      Error Code
*
* Description:
*              To set the PLL clock and system clock
*
-----------------------------------------------------------------------------------------------------------*/
ERRCODE
sysSetSystemClock(E_SYS_SRC_CLK eSrcClk,        // Specified the system clock come from external clock, APLL or UPLL
                UINT32 u32PllHz,            // Specified the APLL/UPLL clock
                UINT32 u32SysHz         // Specified the system clock
    )
{
    UINT32 u32RegPll;
    UINT32 u32RegSysDiv;
    //UINT32 u32CpuFreq, u32Hclk1Frq, u32MclkClock, u32Hclk234;
    UINT32 u32SysDiv, u32DivN0, u32DivN1;

    // 2014/4/30: NandLoader don't need it
    //UINT32 u32DramSrc = (inp32(REG_CLKDIV7)&DRAM_S)>>3;

    g_u32ExtClk = sysGetExternalClock();

    /* Error Check */
// 2014/4/30: NandLoader don't need it
#if 0
    if((u32PllHz%u32SysHz)!=0){
        sysprintf("Err to set memory clock\n");//System divider for integrate and fractional part is not workable for DDR2/DDR  */
        return (ERRCODE)WB_INVALID_CLOCK;
    }
    if((u32PllHz/u32SysHz)>(8*16)){
        sysprintf("Err to set memory clock- Over system divider\n");
        return (ERRCODE)WB_INVALID_CLOCK;
    }

    //Hear need to check the clocp want to set whether vilation the rule (MCLK>HCLK1, >HCLK3 and >HCLK4)
    u32Hclk234 = (u32SysHz/((inp32(REG_CLKDIV4)&HCLK234_N)+1))/2;

    /* Judge MCLK > HCLK1 */
    u32CpuFreq = sysGetCPUClock();
    if((inp32(REG_CLKDIV4)&CPU_N) == 0)
        u32Hclk1Frq = u32CpuFreq/2;
    else
        u32Hclk1Frq = u32CpuFreq;

    u32MclkClock = sysGetDramClock()/2;
    if(u32DramSrc==eSrcClk){
        if(u32MclkClock< u32Hclk1Frq){
            sysprintf("Err to set memory clock- Mclk>HCLK1\n");
            return (ERRCODE)WB_INVALID_CLOCK;
        }
    }else   {
        if(u32MclkClock<= u32Hclk1Frq){
            sysprintf("Err to set memory clock- Mclk>HCLK1\n");
            return (ERRCODE)WB_INVALID_CLOCK;
        }
    }
    /* Judge MCLK > HCLK3 and HCLK4, assume HCLK234_DIV = 0 */
    if(u32DramSrc==eSrcClk){
        if(u32MclkClock< u32Hclk234){
            sysprintf("Err to set memory clock- Mclk>HCLK3 and HCLK4\n");
            return (ERRCODE)WB_INVALID_CLOCK;
        }
    }else{
        if(u32MclkClock<= u32Hclk234){
            sysprintf("Err to set memory clock- Mclk>HCLK3 and HCLK4\n");
            return (ERRCODE)WB_INVALID_CLOCK;
        }
    }
#endif
    u32SysDiv = u32PllHz / u32SysHz;
    for(u32DivN1 = 1; u32DivN1<=16; u32DivN1=u32DivN1+1){
        for(u32DivN0 = 1; u32DivN0<=8; u32DivN0=u32DivN0+1){
            if(u32SysDiv==u32DivN1*u32DivN0)
                break;
        }
        if(u32DivN0>=9)
            continue;
        if(u32SysDiv==u32DivN1*u32DivN0)
                break;
    }
    if(u32DivN1>=17){
        sysprintf("Can not set the clock due to divider is %d\n", u32SysDiv);
        return (ERRCODE)WB_INVALID_CLOCK;
    }
    if(u32DivN0!=0)
        u32DivN0 = u32DivN0-1;
    if(u32DivN1!=0)
        u32DivN1= u32DivN1-1;


    switch(eSrcClk)
    {
// 2014/4/30: NandLoader don't need it
#if 0
        case eSYS_EXT:
            g_u32SysClkSrc =  eSYS_EXT;
             break;
        case eSYS_APLL:
            g_u32SysClkSrc = eSYS_APLL;
            g_u32ApllHz = u32PllHz;
            g_i32REG_APLL = _sysGetPLLControlRegister((g_u32ExtClk/1000), g_u32ApllHz);
            if(g_i32REG_APLL==-1)
                return (ERRCODE)E_ERR_CLK;
            break;
#endif
        case eSYS_UPLL:
            g_u32SysClkSrc = eSYS_UPLL;
            g_u32UpllHz = u32PllHz;
            g_i32REG_UPLL = _sysGetPLLControlRegister((g_u32ExtClk/1000), g_u32UpllHz);
            //printf("UPLL register = %d\n", g_i32REG_UPLL);
            if(g_i32REG_UPLL==-1)
                return (ERRCODE)E_ERR_CLK;
            break;
// 2014/4/30: NandLoader don't need it
#if 0
        case eSYS_MPLL:
            g_u32SysClkSrc = eSYS_MPLL;
            g_u32MpllHz = u32PllHz;
            g_i32REG_MPLL = _sysGetPLLControlRegister((g_u32ExtClk/1000), g_u32MpllHz);
            //printf("UPLL register = %d\n", g_i32REG_UPLL);
            if(g_i32REG_MPLL==-1)
                return (ERRCODE)E_ERR_CLK;
            break;
#endif
        default:
            return (ERRCODE)E_ERR_CLK;
    }
    if(eSrcClk==eSYS_UPLL)
    {
        u32RegPll = g_i32REG_UPLL;
        DBG_PRINTF("UPLL  = %d\n", u32RegPll);
        outp32(REG_UPLLCON , u32RegPll);
    }
// 2014/4/30: NandLoader don't need it
#if 0
    else if(eSrcClk==eSYS_APLL)
    {
        u32RegPll = g_i32REG_APLL;
        DBG_PRINTF("APLL = %d\n", u32RegPll);
        outp32(REG_APLLCON , u32RegPll);
    }
    else if(eSrcClk==eSYS_MPLL)
    {
        u32RegPll = g_i32REG_MPLL;
        DBG_PRINTF("MPLL = %d\n", u32RegPll);
        outp32(REG_MPLLCON , u32RegPll);
    }
#endif

    u32RegSysDiv = (inp32(REG_CLKDIV0) & ~(SYSTEM_N1 | SYSTEM_S|SYSTEM_N0))| (((u32DivN1&0x0F)<<8) | (eSrcClk<<3) | (u32DivN0));
    DBG_PRINTF("REG_PLL = 0x%x\n", u32RegPll);
    DBG_PRINTF("REG_CLKDIV0 = 0x%x\n", u32RegSysDiv);
#if 0
    _sysClockSwitchStart(eSrcClk,
                        u32SysHz/2,
                        u32RegPll,
                        u32RegSysDiv);
#else
    outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & ~(SYSTEM_N1|SYSTEM_S|SYSTEM_N0)) | u32RegSysDiv);
#endif
    /* Restore LVR */
//  outp32(REG_POR_LVRD, u32RegLVR);

    return Successful;
}
/*
    The function will set the CPU clock below the specified CPU clock
    And return the real clock of CPU.
    !!! Assume change CPU clock is workable in SDRAM!!!
*/
INT32 sysSetCPUClock(UINT32 u32CPUClock)
{
// 2014/4/30: NandLoader don't need it
#if 0
    UINT32 CPUClock, u32CPUDiv;
    UINT32 u32SysClock = sysGetSystemClock();
    if(u32CPUClock> u32SysClock)
        return (INT32)E_ERR_CLK;

    /* u32CPUDiv must be multiple of 2 */
    u32CPUDiv = u32SysClock/u32CPUClock;
    if(u32CPUDiv==1)
        u32CPUDiv = 0;
    else if(u32CPUDiv%2==0)
        u32CPUDiv=u32CPUDiv-1;  /* u32CPUDiv = 2, 4 ,6, .... Fill to register */
                                /* Otherwise CPU speed is slower than specified speed */
    if(u32CPUDiv>16)
        return (INT32)E_ERR_CLK;

    outp32(REG_CLKDIV4, (inp32(REG_CLKDIV4) &~CPU_N) | (u32CPUDiv | CHG_APB));
    CPUClock = u32SysClock/(u32CPUDiv+1);
    return CPUClock;
#else
    //--- for Loader, the u32CPUClock should equal to u32SysClock. Simplify this API to shrink code size.
    UINT32 u32CPUDiv;
    UINT32 u32SysClock = sysGetSystemClock();
    u32CPUDiv = 0;
    outp32(REG_CLKDIV4, (inp32(REG_CLKDIV4) &~CPU_N) | (u32CPUDiv | CHG_APB));
    return u32SysClock;
#endif
}
/*
     HCLK1 clcok is always equal to CPUCLK or CPUCLK/2 depends on CPU_N
     INT32 sysSetHCLK1Clock(UINT32 u32HCLK1Clock)
    {

    }
*/

/*
    Set APB clcok
*/
INT32 sysSetAPBClock(UINT32 u32APBClock)
{
    UINT32 u32APBDiv;
    UINT32 u32HCLK1Clock;
    u32HCLK1Clock = sysGetHCLK1Clock();
    if(u32APBClock> u32HCLK1Clock)
        return (INT32)E_ERR_CLK;
    u32APBDiv = (u32HCLK1Clock/u32APBClock)-1;
    if((u32HCLK1Clock%u32APBClock) != 0)
        u32APBDiv = u32APBDiv+1;

// 2014/4/30: NandLoader don't need it
#if 0
    if(u32APBDiv>7){
        sysprintf("APB divider must be less  8\n");
        return (INT32)E_ERR_CLK;
    }
    if(u32APBDiv<1){
        sysprintf("APB divider must be 1 at least\n");
        return (INT32)E_ERR_CLK;
    }
#endif

    outp32(REG_CLKDIV4, (inp32(REG_CLKDIV4) & ~APB_N)|
                        //(u32APBDiv<<8)); /* W55FA92's program */
                        ((u32APBDiv<<8) | CHG_APB)); /* CHG_APB: Enable change APB clock */
                                                    /* CHG_APB will auto clear */
    return (u32HCLK1Clock/(u32APBDiv+1));
}



BOOL bIsAPLLInitialize = FALSE;

UINT32 sysGetExternalClock(void)
{
#if 0
    if((inp32(REG_CHIPCFG) & 0xC) == 0x8)   //Different with FA93
        g_u32ExtClk = 27000000;
    else
        g_u32ExtClk = 12000000;
#else
    g_u32ExtClk = EXTERNAL_CRYSTAL_CLOCK;
#endif
    return g_u32ExtClk;
}
/*
    Get system clcok
*/
UINT32 sysGetSystemClock(void)
{
    UINT32 u32Fin;
    UINT32 u32SysSrc, u32PllPreDiv=1;
    UINT32 u32SysN1, u32SysN0;
    u32Fin = sysGetExternalClock();
    u32SysSrc = (inp32(REG_CLKDIV0) & SYSTEM_S)>>3;
    //u32PllPreDiv = (inp32(REG_CLKDIV0) & SYSTEM_N0)+1;
    switch(u32SysSrc)
    {
        case 0:
            u32SysSrc = u32Fin;                                     break;
        case 2:
            u32SysSrc = sysGetPLLOutputHz(eSYS_APLL, u32Fin )/u32PllPreDiv; break;
        case 3:
            u32SysSrc = sysGetPLLOutputHz(eSYS_UPLL, u32Fin)/u32PllPreDiv;  break;
    }

    u32SysN1 = ((inp32(REG_CLKDIV0) & SYSTEM_N1)>>8) + 1;
    u32SysN0 = (inp32(REG_CLKDIV0) & SYSTEM_N0) + 1;

    return (u32SysSrc/(u32SysN1*u32SysN0));
}
/*
    Get CPU clcok
*/
UINT32 sysGetCPUClock()
{
    UINT32 u32SysClock = sysGetSystemClock();
    UINT32 CPUClock;
    CPUClock = u32SysClock/((inp32(REG_CLKDIV4) & CPU_N)+1);
#ifdef REAL_CHIP
    return (UINT32)CPUClock;
#else
    return(sysGetExternalClock());
#endif
}
/*
    Get HCLK1 clcok
*/
UINT32 sysGetHCLK1Clock()
{
    UINT32 u32CPUClock;
    UINT32 u32CPUDiv;
    u32CPUClock = sysGetCPUClock();
    u32CPUDiv = inp32(REG_CLKDIV4) & CPU_N;
#ifdef REAL_CHIP
    if(u32CPUDiv == 0)
        return u32CPUClock/2;
    else
        return u32CPUClock;
#else
    return(sysGetExternalClock());
#endif
}
/*
    Get HCLK234 clcok
*/
UINT32 sysGetHCLK234Clock(void)
{
    UINT32 u32HCLK1Clock;
    UINT32 u32HCLK234Div;

    u32HCLK1Clock = sysGetHCLK1Clock();
    u32HCLK234Div = (inp32(REG_CLKDIV4) & HCLK234_N)>>4;
#ifdef REAL_CHIP
    return (u32HCLK1Clock/(u32HCLK234Div+1));
#else
    return(sysGetExternalClock());
#endif
}
/*
    Get APB clcok
*/
UINT32 sysGetAPBClock()
{
    UINT32 u32APBDiv;
    u32APBDiv = ((inp32(REG_CLKDIV4) & APB_N)>>8) +1;
#ifdef REAL_CHIP
    return (sysGetHCLK1Clock()/u32APBDiv);
#else
    return(sysGetExternalClock());
#endif

}


/*-----------------------------------------------------------------------------------------------------------
*   The Function is used to set the other PLL which is not the system clock source.
*   If system clock source come from eSYS_UPLL. The eSrcClk only can be eSYS_APLL
*   And if specified PLL not meet some costraint, the funtion will search the near frequency and not over the specified frequency
*
*   Paramter:
*       eSrcClk: eSYS_UPLL or eSYS_APLL
*       u32TargetKHz: The specified frequency. Unit:Khz.
*
*   Return:
*       The specified PLL output frequency really.
-----------------------------------------------------------------------------------------------------------*/
UINT32 sysSetPllClock(E_SYS_SRC_CLK eSrcClk, UINT32 u32TargetHz)
{
    UINT32 u32PllReg, /* u32PllOutFreqHz,*/ u32FinHz;


    u32FinHz = sysGetExternalClock();

// 2014/4/30: NandLoader don't need it
#if 0
    //Specified clock is system clock,  return working frequency directly.
    if( (inp32(REG_CLKDIV0) & SYSTEM_S)== 0x18 )
    {//System from UPLL
        if(eSrcClk==eSYS_UPLL)
        {
            u32PllOutFreqHz = sysGetPLLOutputHz(eSrcClk, u32FinHz);
            return u32PllOutFreqHz;
        }
    }
    if( (inp32(REG_CLKDIV0) & SYSTEM_S)== 0x10 )
    {//System from APLL
        if(eSrcClk==eSYS_APLL)
        {
            u32PllOutFreqHz = sysGetPLLOutputHz(eSrcClk, u32FinHz);
            return u32PllOutFreqHz;
        }
    }
    //Specified clock is not system clock,
    u32PllReg = _sysGetPLLControlRegister((u32FinHz/1000), u32TargetHz);
    if(eSrcClk == eSYS_APLL)
        outp32(REG_APLLCON, u32PllReg);
    else if(eSrcClk == eSYS_UPLL)
        outp32(REG_UPLLCON, u32PllReg);
    else if(eSrcClk == eSYS_MPLL)
        outp32(REG_MPLLCON, u32PllReg);
    if(((eSrcClk == eSYS_APLL)||(eSrcClk == eSYS_UPLL))||(eSrcClk == eSYS_MPLL))
    {
        u32PllOutFreqHz = sysGetPLLOutputHz(eSrcClk, u32FinHz);
        if(eSrcClk == eSYS_APLL)
            g_u32ApllHz = u32PllOutFreqHz;
        else if(eSrcClk == eSYS_UPLL)
            g_u32UpllHz = u32PllOutFreqHz;
        else if(eSrcClk == eSYS_UPLL)
            g_u32MpllHz = u32PllOutFreqHz;
        return u32PllOutFreqHz;
    }
    else
        return 0;
#else
    u32PllReg = _sysGetPLLControlRegister((u32FinHz/1000), u32TargetHz);
    outp32(REG_APLLCON, u32PllReg);
    return 0;   // NandLoader don't use return value.
#endif
}
//==========================================================================================

/*-----------------------------------------------------------------------------------------------------------
* Function: sysSetMPLLClock
*
* Parameters:
*              u32PllValue - [in], PLL setting value
*              u32DramClock - [in], DRAM working frequency
* Returns:
*      Successful or Error Code
*
* Description:
*              To set the MPLL control register and DRAM working requency.
*
*Note:
*       Switch systetm clock to external clock first.
*
*       refresh rate = REPEAT/Fmclk
*       1. Disable interrupt
*       2. Enter Self-refresh
*       3. Switch to external clock
*       4. Adjustment the sys divider.
*
*
*
*
*
*
-----------------------------------------------------------------------------------------------------------*/
#if defined(__CC_ARM)
#pragma O2
#endif
#if 1
#define dbg(u32LocalUartVar, x) while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000)); outpb(REG_UART_THR+u32LocalUartVar, x); while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000)); outpb(REG_UART_THR+u32LocalUartVar, '\n'); \
while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));

#define dbg_woc(u32LocalUartVar, x) while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000)); outpb(REG_UART_THR+u32LocalUartVar, x); while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
#else
#define dbg(...)
#define dbg_woc(...)
#endif

#define SRAM_SRCClk         0xFF001FE0
#define SRAM_PLLREG         0xFF001FE4
#define SRAM_DRAMFREQ       0xFF001FE8
#define SRAM_DRAMDIVI       0xFF001FEC
#define SRAM_REG_CLKDIV0    0xFF001FF0
#define SRAM_HIGH_FREQ      0xFF001FF4
#define SRAM_UARTPORT       0xFF001FF8
#define SRAM_VAR            0xFF001FFC
void _dramClockSwitch(register E_SYS_SRC_CLK eSrcClk,
                    register UINT32 u32PllReg,
                    register UINT32 u32DramFreq,
                    register UINT32 u32DramClkDiv)
{
    volatile UINT32 u32mem_1aaaa8;
    //UINT32 u32REG_CLKDIV0, High_Freq;

#if 0
    UINT32 tmp,i, change;
    UINT32 skew_19, skew_1a, skew_1b, skew_1c, skew_1d, skew_1e, skew_1f;
#else
    INT32 tmp,i;
    UINT32 skew = 0;
    UINT32 High_Freq;
#endif
    volatile UINT32 dly;
    register UINT32 u32LocalUartVar = u32UartPort;

    outp32(SRAM_SRCClk, eSrcClk);
    outp32(SRAM_PLLREG, u32PllReg);
    outp32(SRAM_DRAMFREQ, u32DramFreq);
    outp32(SRAM_DRAMDIVI, u32DramClkDiv);
    outp32(SRAM_UARTPORT, u32LocalUartVar);
    u32mem_1aaaa8 = inp32(0x1aaaa8);    /* Back up content in address 0x1aaaa8 */
    outp32(0x1aaaa8, 0x5555aaaa);

    // Very important: Disable FA92 DLL first with 100us delay for calibration stable **********
    outp32(REG_DLLMODE,  (inp32(REG_DLLMODE_R)&~0x8) | 0x10);   // Disable DLL of VA92
#if 0
    for(dly=0; dly<0x2800;dly++);
#else
        __asm
        {
            mov         r2, #0x2800
            mov     r1, #0
            mov     r0, #1
DRAM_A: add         r1, r1, r0
            cmp         r1, r2
            bne     DRAM_A
        }
#endif

#if 0
    dly = u32DramClkDiv;
    u32DramClkDiv = dly;

    dbg(u32LocalUartVar, '0'+(u32DramClkDiv&0x7));

    dly = u32DramFreq;
    u32DramFreq = dly;

    u32REG_CLKDIV0 = inp32(REG_CLKDIV0);
#else
    #if 0
    dbg(u32LocalUartVar, '0'+(u32DramClkDiv&0x7));
    #else
    dbg(inp32(SRAM_UARTPORT), '0'+(inp32(SRAM_DRAMDIVI)&0x7));
    #endif
    outp32(SRAM_REG_CLKDIV0, inp32(REG_CLKDIV0));
#endif

    /* DRAM enter self refresh mode */
    outp32(REG_SDCMD, (inp32(REG_SDCMD) & ~0x20) | 0x10);
#if 0
    for(dly=0; dly<100;dly++);
#else
        __asm
        {
            mov         r2, #0x1000
            mov     r1, #0
            mov     r0, #1
DRAM_S0:
            add         r1, r1, r0
            cmp         r1, r2
            bne     DRAM_S0
        }
#endif
#if 0
    if(u32DramFreq>=96000000){
        dbg(u32LocalUartVar, 'H');
    }else{
        dbg(u32LocalUartVar, 'L');
    }
#else
    if(inp32(SRAM_DRAMFREQ)>=96000000){
        dbg(inp32(SRAM_UARTPORT), 'H');
    }else{
        dbg(inp32(SRAM_UARTPORT), 'L');
    }
#endif
    outp32(REG_DLLMODE,  (inp32(REG_DLLMODE_R)&~0x8) | 0x10);   // Disable DLL of VA92


    /* Switch system clock to external clock and divider to 0*/ /* Due to delay time. Switch to external clock for delay loop */
    outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & ~(SYSTEM_N1 | SYSTEM_S | SYSTEM_N0)) );  //HCLK = 6MHz.

    /* Switch DRAM clock to external clock and divider to 0 */
    outp32(REG_CLKDIV7, (inp32(REG_CLKDIV7) & ~(DRAM_N1 | DRAM_S | DRAM_N0)) );         //DRAM = 6MHz.


    //Set Pll clock
    if(eSrcClk == eSYS_MPLL){
        outp32(REG_MPLLCON, u32PllReg);
        while((inp32(REG_POR_LVRD)&MPLL_LKDT)==0);
    }
// 2014/4/30: NandLoader don't need it
#if 0
    else if(eSrcClk == eSYS_APLL){
        outp32(REG_APLLCON, u32PllReg);
        while((inp32(REG_POR_LVRD)&APLL_LKDT)==0);
    }
#endif
    else if(eSrcClk == eSYS_UPLL){
        outp32(REG_UPLLCON, u32PllReg);
        while((inp32(REG_POR_LVRD)&UPLL_LKDT)==0);
    }

    //Set DRAM clock divider and source
    outp32(REG_CLKDIV7,  u32DramClkDiv);
#if 0
    for(dly=0; dly<10;dly++);
#else
        __asm
        {
            mov         r2, #1000
            mov     r1, #0
            mov     r0, #1
DRAM_S1:    add         r1, r1, r0
            cmp         r1, r2
            bne     DRAM_S1
        }
#endif
#if 0
    outp32(REG_CLKDIV0, u32REG_CLKDIV0);            //CPU clock from UPLL
#else
    outp32(REG_CLKDIV0, inp32(SRAM_REG_CLKDIV0));
#endif

    /* DRAM escape self refresh mode */
    outp32(REG_SDCMD,  0x20);
#if 0
    for(dly=0; dly<200;dly++);                      //Wait 200T
#else
        __asm
        {
            mov         r2, #200
            mov     r1, #0
            mov     r0, #1
DRAM1a: add         r1, r1, r0
            cmp         r1, r2
            bne     DRAM1a
        }
#endif

#ifdef __TEST__
    while( (inp32(REG_GPIOB_PIN) & 0x01)==0 ); /* set system clock */
#endif

    dly = (inp32(REG_CHIPCFG)&SDRAMSEL)>>4;  /* SDRAM has escaped self-refresh mode */

    if(dly==2)
        High_Freq = 64000000;
    else
        High_Freq = 96000000;
    //DRAM auto-calibration for optimal DRAM Phase
#if 0
    if(u32DramFreq>=High_Freq)
#else
    if(inp32(SRAM_DRAMFREQ)>=High_Freq)
#endif
    {//High Frequency

        outp32(REG_SDEMR, inp32(REG_SDEMR) & ~DLLEN);               // Enable  DLL of DDR2
        outp32(REG_SDMR,  0x532);                                   // RESET DLL(bit[8]) of DDR2
#if 0
        for(dly=0; dly<50;dly++);
#else
        __asm
        {
            mov         r2, #50
            mov     r1, #0
            mov     r0, #1
DRAMHA: add         r1, r1, r0
            cmp         r1, r2
            bne     DRAMHA
        }
#endif
        outp32(REG_SDMR,  0x432);                                   // RESET DLL(bit[8]) of DDR2
again:
        outp32(REG_DLLMODE,  (inp32(REG_DLLMODE_R)&~0x8) | 0x10);   // Disable DLL of VA92
        outp32(REG_DLLMODE,   inp32(REG_DLLMODE_R)       | 0x18);   // Enable  DLL of VA92
        outp32(REG_CKDQSDS, 0x00888800);                        // Skew for high freq

        //change = 0;
            for(i=0; i<7; i=i+1)
            {
                    outp32(REG_DLLMODE, (0x19+i) );              //DLLMODE phase search
#if 0
                        for(dly=0; dly<0x2000;dly++);
#else
                                __asm
                                {
                                    mov         r2, #0x2000
                                    mov     r1, #0
                                    mov     r0, #1
                        DRAMHB: add         r1, r1, r0
                                    cmp         r1, r2
                                    bne     DRAMHB
                                }
#endif
#if 0
                 dly = (inp32(REG_CHIPCFG)&SDRAMSEL)>>4;
#else
                            __asm{
                                        MOV     r0, #0xb0000000
                                        LDR     r0,[r0, #4]
                                        MOV     r0, r0, LSL #26
                                        MOV     r0, r0, LSR #30
                                        MOV     dly, r0
                            }
#endif
                if(dly==3){//DDR2 type
                        outp32(REG_SDOPM, 0x01130476);              //Set DQS_PHASE_RST
                        outp32(REG_SDOPM, 0x01030476);              //Clr DQS_PHASE_RST
                 }else if(dly==2){//DDR type
                    outp32(REG_SDOPM, 0x01130456);              //Set DQS_PHASE_RST
                        outp32(REG_SDOPM, 0x01030456);              //Clr DQS_PHASE_RST
                 }else if(dly==0){//DDR type
                    outp32(REG_SDOPM, 0x01130416);              //Set DQS_PHASE_RST
                        outp32(REG_SDOPM, 0x01030416);          //Clr DQS_PHASE_RST
                 }
                    tmp = inp32(0x1aaaa8);                      //Dummy Read DRAM
                    skew = skew <<1;
                    switch(i)
                    {
                    #if 0
                       case 0:     skew_19 = (inp32(REG_SDOPM) & 0x10000000)!=0 ;    break;
                       case 1:     skew_1a = (inp32(REG_SDOPM) & 0x10000000)!=0 ;    break;
                       case 2:     skew_1b = (inp32(REG_SDOPM) & 0x10000000)!=0 ;    break;
                       case 3:     skew_1c = (inp32(REG_SDOPM) & 0x10000000)!=0 ;    break;
                       case 4:     skew_1d = (inp32(REG_SDOPM) & 0x10000000)!=0 ;    break;
                       case 5:     skew_1e = (inp32(REG_SDOPM) & 0x10000000)!=0 ;    break;
                       case 6:     skew_1f  = (inp32(REG_SDOPM) & 0x10000000)!=0 ;    break;
                    #else
                       case 0:
                               skew = (inp32(REG_SDOPM) & 0x10000000)!=0 ;    break;
                       case 1:     skew = skew | ((inp32(REG_SDOPM) & 0x10000000)!=0);    break;
                       case 2:     skew = skew | ((inp32(REG_SDOPM) & 0x10000000)!=0);    break;
                       case 3:     skew = skew | ((inp32(REG_SDOPM) & 0x10000000)!=0);    break;
                       case 4:     skew = skew | ((inp32(REG_SDOPM) & 0x10000000)!=0);    break;
                       case 5:     skew = skew | ((inp32(REG_SDOPM) & 0x10000000)!=0);    break;
                       case 6:     skew = skew | ((inp32(REG_SDOPM) & 0x10000000)!=0);    break;
                    #endif
                    }
            }

/*
            //Choose final DLLMODE value and re-calibration
            if( (skew_19==skew_1a) &&  (skew_1a==skew_1b)  && (skew_1b==skew_1c) && (skew_1c==skew_1d) ) {
            //////////// New modification ////////////////////////////
                if( ((skew_1d!=skew_1e)  || (skew_1e!=skew_1f) )  && ((inp32(REG_SDOPM)>>28)>=3) ){ //>= 360M & SS case
//                      outp32(REG_DLLMODE, 0x1f);
                        change = 5;
                    }
                    else
                    outp32(REG_DLLMODE, 0x1a);
            }
            else if(                   (skew_1a==skew_1b)  && (skew_1b==skew_1c) && (skew_1c==skew_1d) )
            {
                    outp32(REG_DLLMODE, 0x1b);
                    change = 1;
            }
            else if(                                          (skew_1b==skew_1c) && (skew_1c==skew_1d) )
            {   //00111 or 11000
                    outp32(REG_DLLMODE, 0x1c);
                    change = 2;
            }
            else if(                                                                 skew_1c==skew_1d )
            {
                    outp32(REG_DLLMODE, 0x1d);
                    change = 3;
            }
             else if(                                                                skew_1c!=skew_1d )
            {
                    outp32(REG_DLLMODE, 0x1e);
                    change = 4;
            }
            else
                        outp32(REG_DLLMODE, 0x1a);
*/
#if 0
            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
        outpb(REG_UART_THR+u32LocalUartVar, skew_19+48);
            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
        outpb(REG_UART_THR+u32LocalUartVar, skew_1a+48);
            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
        outpb(REG_UART_THR+u32LocalUartVar, skew_1b+48);
            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
        outpb(REG_UART_THR+u32LocalUartVar, skew_1c+48);
            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
        outpb(REG_UART_THR+u32LocalUartVar, skew_1d+48);
            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
        outpb(REG_UART_THR+u32LocalUartVar, '.');
            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
        outpb(REG_UART_THR+u32LocalUartVar, skew_1e+48);
#else
            for(i=6; i>=0; i=i-1)
            {
                dly = (skew >>i)&0x01;
            outpb(REG_UART_THR+u32LocalUartVar, dly+48);
            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
            }
#endif
#if 0
                    for(dly=0; dly<0x2800;dly++);
#else
                                __asm
                                {
                                    mov         r2, #0x2800
                                    mov     r1, #0
                                    mov     r0, #1
                        DRAMHC: add         r1, r1, r0
                                    cmp         r1, r2
                                    bne     DRAMHC
                                }
#endif

            dly = (inp32(REG_CHIPCFG)&SDRAMSEL)>>4;
            if(dly==3){//DDR2 type
                outp32(REG_SDOPM, 0x01130476);          //DQS_PHASE_RST
                outp32(REG_SDOPM, 0x01030476);
            }else if(dly==2){//DDR type
                outp32(REG_SDOPM, 0x01130456);          //DQS_PHASE_RST
                outp32(REG_SDOPM, 0x01030456);
            }else if(dly==0){//SDRAM type
                outp32(REG_SDOPM, 0x01130416);          //DQS_PHASE_RST
                outp32(REG_SDOPM, 0x01030416);
            }


            tmp = inp32(0x1aaaa8);                          //Dummy Read DRAM


            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
        outpb(REG_UART_THR+u32LocalUartVar, (inp32(REG_SDOPM)>>28)+48);     //Phase

//Bon           outp32(REG_DLLMODE, (inp32(REG_DLLMODE_R) - change) | 0x10 );  //Real DLL phase
        outp32(REG_DLLMODE,  0x1a );  //Real DLL phase
#if 0
                    for(dly=0; dly<0x2800;dly++);
#else
                                __asm
                                {
                                    mov         r2, #0x2800
                                    mov     r1, #0
                                    mov     r0, #1
                        DRAMHD: add         r1, r1, r0
                                    cmp         r1, r2
                                    bne     DRAMHD
                                }
#endif
            tmp = inp32(0x1aaaa8);                      //Read DRAM

            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
        outpb(REG_UART_THR+u32LocalUartVar, '.');

            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
        outpb(REG_UART_THR+u32LocalUartVar, inp32(REG_DLLMODE_R)+39);

            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
            if( tmp == 0x5555aaaa )
            outpb(REG_UART_THR+u32LocalUartVar, 'p');
        else
            outpb(REG_UART_THR+u32LocalUartVar, 'f');

            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
        outpb(REG_UART_THR+u32LocalUartVar,(inp32(REG_SDOPM)>>28)+48);  //Final Phase

            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
        outpb(REG_UART_THR+u32LocalUartVar, ':');
        // add carriage return to show application messages on next line.
        outpb(REG_UART_THR+u32LocalUartVar, '\n');
        outpb(REG_UART_THR+u32LocalUartVar, '\r');

        if(tmp != 0x5555aaaa)
            goto again;


    #ifdef W55FA92_A_VERSION

    #else
        #if 0
        if(u32DramFreq>=180000000)
            outp32(REG_SDOPM, 0x0103046E);          // Enable open page
        else
            outp32(REG_SDOPM, 0x01030476);          // Disable open page
        #else
        dly = (inp32(REG_CHIPCFG)&SDRAMSEL)>>4;
        if(u32DramFreq>=180000000){
            if(dly==3){//DDR2 type
                outp32(REG_SDOPM, 0x0103046E);          // Enable open page
             }else if(dly==2){//DDR type
                outp32(REG_SDOPM, 0x0103044E);          // Enable open page
             }else if(dly==0){//SDRAM type
                outp32(REG_SDOPM, 0x0103040E);          // Enable open page
             }
        }
        else
            if(dly==3){//DDR2 type
                outp32(REG_SDOPM, 0x01030476);          // Disable open page
            }else if(dly==2){//DDR type
                outp32(REG_SDOPM, 0x01030456);          // Disable open page
            }else if(dly==0){//SDRAM type
                outp32(REG_SDOPM, 0x01030416);          // Disable open page
             }
        #endif
    #endif
        if(dly==2)//DDR
            outp32(REG_SDMR, inp32(REG_SDMR) & (~0xF0) | 0x30);     //CL = 3;

    }
    else  //Low freq. mode setting
    {
            outp32(REG_SDEMR, inp32(REG_SDEMR)  | DLLEN);           //Disable DLL of DRAM device
#if 0
            for(dly=0; dly<0x2800;dly++);
#else
        __asm
        {
            mov         r2, #0x2800
            mov     r1, #0
            mov     r0, #1
DRAM_L: add         r1, r1, r0
            cmp         r1, r2
            bne     DRAM_L
        }
#endif


        outp32(REG_DLLMODE, ((inp32(REG_DLLMODE_R) & ~0x18)) );  //Disable DLL of FA92
#if 0
            dly = (inp32(REG_CHIPCFG)&SDRAMSEL)>>4;
#else
            __asm{
                MOV     r0, #0xb0000000
                LDR     r0,[r0, #4]
              MOV       r0, r0, LSL #26
                MOV     r0, r0, LSR #30
                MOV     dly, r0
            }
#endif
            if(dly==3){//DDR2 type
                outp32(REG_SDOPM,   0x00078476);                    //bit[24]=0, bit[18]&[15]=1  and bit[4]=0 (SEL_USE_DLL)
            }else if(dly==2){//DDR type
                outp32(REG_SDOPM,   0x00078456);                    //bit[24]=0, bit[18]&[15]=1  and bit[4]=0 (SEL_USE_DLL)
                outp32(REG_SDMR, inp32(REG_SDMR) & (~0xF0) | 0x20); //CL=2
            }else if(dly==0){//SDRAM type
                outp32(REG_SDOPM,   0x00078416);                    //bit[24]=0, bit[18]&[15]=1  and bit[4]=0 (SEL_USE_DLL)
            }
            outp32(REG_CKDQSDS, 0x0000ff00);                        // Skew for low freq
#if 0
    for(dly=0; dly<0x30;dly++);
#else
        __asm
        {
            mov         r2, #2000
            mov     r1, #0
            mov     r0, #1
DRAM1B: add         r1, r1, r0
            cmp         r1, r2
            bne     DRAM1B
        }
#endif
//Zentel_mode:
        tmp = inp32(0x1aaaa8);
         while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
        if( tmp == 0x5555aaaa ){
            outpb(REG_UART_THR+u32LocalUartVar, 'W');
            while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
            outpb(REG_UART_THR+u32LocalUartVar, '\n');
        }else{
            outpb(REG_UART_THR+u32LocalUartVar, 'Z');
             while (!(inpw(REG_UART_FSR+u32LocalUartVar) & 0x400000));
            outpb(REG_UART_THR+u32LocalUartVar, '\n');
            outp32(REG_CKDQSDS, inp32(REG_CKDQSDS) | 0x01000000);   // bit[25:24] = 1 for Zentel 3T+4ns.
        }
    }
    outp32(0x1aaaa8, u32mem_1aaaa8);    /* Restore content in address 0x1aaaa8 */
}



void _dramClockSwitchStart(E_SYS_SRC_CLK eSrcClk,
                        UINT32 u32RegPll,
                        UINT32 u32DramClock,
                        UINT32 u32DramClockReg)
{
    BOOL bIsCacheState=FALSE;
    INT32  u32CacheMode=0;
    UINT32   vram_base, aic_status = 0, aic_statush=0;

    //VOID    (*wb_func)(UINT32,UINT32);
    VOID    (*wb_func)(UINT32,
                    UINT32,
                    UINT32,
                    UINT32);

//  DBG_PRINTF("_dramClockSwitchStart\n");


    aic_status = inpw(REG_AIC_IMR);
    aic_statush = inpw(REG_AIC_IMRH);
    outpw(REG_AIC_MDCR, 0xFFFFFFFF);    //Disable interrupt
    outpw(REG_AIC_MDCRH, 0xFFFFFFFF);   //Disable interrupt

    vram_base = PD_RAM_BASE;

//  outp32(0xff000f80,8);
    memcpy((char *)((UINT32)_tmp_buf | 0x80000000),
            (char *)((UINT32)vram_base | 0x80000000),
            PD_RAM_SIZE);
    memcpy((VOID *)((UINT32)vram_base | 0x80000000),
            (VOID *)(((UINT32)_dramClockSwitch-(PD_RAM_START-PD_RAM_BASE)) | 0x80000000),
            PD_RAM_SIZE);
//  outp32(0xff000f80,9);
    DBG_PRINTF("memcpy ok\n");

//  outp32(0xff000f80,10);
    if(sysGetCacheState()==TRUE){
        //DBG_PRINTF("Cache enable\n");
        bIsCacheState = TRUE;
        u32CacheMode = sysGetCacheMode();
        sysDisableCache();
        sysFlushCache(I_D_CACHE);
    //}else{
        //DBG_PRINTF("Cache disable\n");
    }

//  outp32(0xff000f80,11);
    //sysFlushCache(I_D_CACHE);
    vram_base = PD_RAM_START;
    //wb_func = (void(*)(UINT32, UINT32)) vram_base;
    wb_func = (void(*)(UINT32, UINT32, UINT32, UINT32)) vram_base;

    //--------------------------------------
    DBG_PRINTF("Jump to SRAM\n");
    //wb_func(eSrcClk, u32RegPll);
    wb_func(eSrcClk, u32RegPll, u32DramClock, u32DramClockReg);
    vram_base = PD_RAM_BASE;
    memcpy((VOID *)((UINT32)vram_base | 0x80000000),
            (VOID *)((UINT32)_tmp_buf | 0x80000000),
            PD_RAM_SIZE);
    if(bIsCacheState==TRUE)
        sysEnableCache(u32CacheMode);

    outpw(REG_AIC_MECR, aic_status);        // Restore AIC setting
    outpw(REG_AIC_MECRH,aic_statush);
}
/*
    Memory clock constraint
    1. SDIC' clock > AHB1/AHB3/AHB4


    E_SYS_SRC_CLK eSrcClk           Memory clock source. It will always = MPLL
    UINT32 u32PLLClockHz,           MPLL frequency
    UINT32 u32DramClock         DDR clock (2* SDIC)

*/
UINT32 sysSetDramClock(E_SYS_SRC_CLK eSrcClk, UINT32 u32PLLClockHz, UINT32 u32DdrClock)
{
    UINT32 u32FinHz, u32DramDiv;
    UINT32 u32DivN1, u32DivN0, u32DramClockReg;
    UINT32 u32CpuFreq, u32Hclk1Frq, u32Hclk234;
    UINT32 u32DramClock = u32DdrClock/2;
    UINT32 u32sysSrc = (inp32(REG_CLKDIV0)&SYSTEM_S)>>3;

    /* Judge MCLK > HCLK1 */
    u32CpuFreq = sysGetCPUClock();
    if((inp32(REG_CLKDIV4)&CPU_N) == 0)
        u32Hclk1Frq = u32CpuFreq/2;
    else
        u32Hclk1Frq = u32CpuFreq;
    if(u32sysSrc==eSrcClk){
        if(u32DramClock< u32Hclk1Frq){
            sysprintf("Err to set memory clock\n");
            return WB_INVALID_CLOCK;
        }
    }else{
        if(u32DramClock<= u32Hclk1Frq){
            sysprintf("Err to set memory clock\n");
            return WB_INVALID_CLOCK;
        }
    }
    /* Judge MCLK > HCLK3 and HCLK4, assume HCLK234_DIV = 0 */
    u32Hclk234 = sysGetHCLK234Clock();  /* HCLK234  */
    if(u32sysSrc==eSrcClk){
        if(u32DramClock< u32Hclk234){
            sysprintf("Err to set memory clock\n");
            return WB_INVALID_CLOCK;
        }
    }else{
        if(u32DramClock<= u32Hclk234){
            sysprintf("Err to set memory clock\n");
            return WB_INVALID_CLOCK;
        }
    }
    u32DramDiv = u32PLLClockHz/u32DdrClock;
    for(u32DivN1 = 1; u32DivN1<=8; u32DivN1=u32DivN1+1){
        for(u32DivN0 = 1; u32DivN0<=8; u32DivN0=u32DivN0+1){
            if(u32DramDiv==(u32DivN1*u32DivN0))
                break;
        }
        if(u32DivN0>=9)
            continue;
        if(u32DramDiv==(u32DivN1*u32DivN0))
            break;
    }
    if(u32DivN0>=9){//Over 8*8 range or prime number (19, 17, 13, 15..)
        sysprintf("Can not set the clock due to divider is %d\n", u32DramDiv);
        return WB_INVALID_CLOCK;
    }
    if(u32DivN0!=0)
        u32DivN0 = u32DivN0-1;
    if(u32DivN1!=0)
        u32DivN1 = u32DivN1-1;
    u32DramClockReg = ((u32DivN1<<5) | (eSrcClk<<3) | u32DivN0);
    u32DramClockReg = (inp32(REG_CLKDIV7) & ~(DRAM_N1 | DRAM_S | DRAM_N0)) | u32DramClockReg;

    u32FinHz = sysGetExternalClock();
    g_i32REG_MPLL = _sysGetPLLControlRegister((u32FinHz/1000), u32PLLClockHz);

    //sysprintf("Want to set DRAM DIV REG = 0x%x\n",u32DramClockReg);
    _dramClockSwitchStart(eSrcClk,          /* PLL clock source */
                        g_i32REG_MPLL,  /* From MLL */
                        u32DdrClock/2,  /* SDIC clock to judge high or low frequency */
                        u32DramClockReg);   /* DRAM clock divider */
    //sysprintf("MPLL REG = 0x%x\n", inp32(REG_MPLLCON));
    //sysprintf("DRAM DIV REG = 0x%x\n", inp32(REG_CLKDIV7));
    return  Successful;
}
/*
    Here return is DDR clock.
    MCLK should be DDR/2
*/
UINT32 sysGetDramClock(void)
{
    UINT32 u32DramClock, u32RegData;
    UINT32 u32SrcClock, u32Div, u32ExtFreq;

    u32RegData = inp32(REG_CLKDIV7);
    u32SrcClock = (u32RegData&DRAM_S)>>3;
    u32ExtFreq = sysGetExternalClock();

    u32Div = ((u32RegData & DRAM_N0)+1) * (((u32RegData & DRAM_N1)>>5)+1);
    if(u32SrcClock==0)
        u32SrcClock = sysGetExternalClock();
    else
        u32SrcClock = sysGetPLLOutputHz((E_SYS_SRC_CLK)u32SrcClock, u32ExtFreq);
    u32DramClock = (u32SrcClock/u32Div);
    return u32DramClock;

}
