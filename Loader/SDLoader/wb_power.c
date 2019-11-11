/***************************************************************************
 *                                                                         *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              *
 *                                                                         *
 ***************************************************************************/

/****************************************************************************
* FILENAME
*   wb_power.c
*
* VERSION
*   1.0
*
* DESCRIPTION
*   The power managemnet related function of Nuvoton ARM9 MCU
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*   sysDisableAllPM_IRQ
*   sysEnablePM_IRQ
*   sysPMStart
*
* HISTORY
*   2008-07-24  Ver 1.0 Modified by Min-Nan Cheng
*
* REMARK
*   When enter PD or MIDLE mode, the sysPMStart function will disable cache
*   (in order to access SRAM) and then recovery it after wake up.
****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "wblib.h"

#define REG_DLLMODE_R	0xb0003058	/* Not fix in B version. still read in 3058 */
#define DBG_PRINTF(...)

#if defined (__GNUC__)
UINT8  _tmp_buf[PD_RAM_SIZE] __attribute__((aligned (32)));
#else
__align(32) UINT8  _tmp_buf[PD_RAM_SIZE];
#endif

#if defined (__GNUC__) && !defined (__CC_ARM)
#pragma GCC push_options
#pragma GCC optimize ("O0")
#endif
//#define PA0_H  outp32(REG_GPIOC_DOUT, inp32(REG_GPIOC_DOUT) | 0x1);
//#define PA0_L  outp32(REG_GPIOC_DOUT, inp32(REG_GPIOC_DOUT) & ~0x1);

static void Sample_PowerDown(void)
{
	register int reg3, reg2, reg1, reg0;
    
	UINT32 u32RstDebounce = inp32(REG_EXTRST_DEBOUNCE);
	outp32(REG_EXTRST_DEBOUNCE, 0x0);					/* Disable Reset debounce as power down */
    reg3 = 0xB0000200;
	
#if defined (__GNUC__)
    __asm
	(
		"  mov 	%0, #300       \n"
		"  mov  %1, #0         \n"
		"  mov  %2, #1         \n"
		" loopz:	           \n"
		"  add  %1, %1, %2     \n"
		"  cmp 	%1, %0         \n"
		"  bne  loopz          \n"
		: : "r"(reg2), "r"(reg1), "r"(reg0) :"memory"
	);
#else
	__asm
	{/* Dealy */
		mov 	reg2, #100
		mov		reg1, #0
		mov		reg0, #1
	loopz:		add reg1, reg1, reg0
		cmp 	reg1, reg2
		bne		loopz
	}
#endif
	//outp32(REG_AHBCLK, inp32(REG_AHBCLK)|GVE_CKE);			/* The clock need to be enable if power down. Otherwise, pressing reset button will be useless */
	
	outp32(REG_SDOPM, inp32(REG_SDOPM) & ~AUTOPDN);			             
	outp32(REG_DLLMODE,  (inp32(REG_DLLMODE_R)&~0x8) | 0x10);	// Disable chip's DLL
	
	/*********** DRAM enter self refresh mode ************/	
	outp32(REG_SDCMD, (inp32(REG_SDCMD) & ~0x20) | 0x10);
    /*********** DRAM enter self refresh mode ************/	

#if defined (__GNUC__)
	__asm volatile
	(
		"  mov 	%0, #100       \n"
		"  mov  %1, #0         \n"
		"  mov  %2, #1         \n"
		" loopa:	           \n"
		"  add  %1, %1, %2     \n"
		"  cmp 	%1, %0         \n"
		"  bne  loopa          \n"
		: : "r"(reg2), "r"(reg1), "r"(reg0) :"memory"
	);
#else	
	__asm
	{/* Dealy */
		mov 	reg2, #100
		mov		reg1, #0
		mov		reg0, #1
	loopa:		add reg1, reg1, reg0
		cmp 	reg1, reg2
		bne		loopa
	}	
#endif
//	outp32(REG_DLLMODE,  (inp32(REG_DLLMODE_R)&~0x8) | 0x10);	// Disable chip's DLL

	/* Change the system clock souce to 12M crystal*/
	outp32(REG_CLKDIV0, (inp32(REG_CLKDIV0) & (~0x18)) );	

#if defined (__GNUC__)
	__asm volatile
	(
		"  mov 	%0, #100       \n"
		"  mov  %1, #0         \n"
		"  mov  %2, #1         \n"
		" loop0:	           \n"
		"  add  %1, %1, %2     \n"
		"  cmp 	%1, %0         \n"
		"  bne  loop0          \n"
		: : "r"(reg2), "r"(reg1), "r"(reg0) :"memory"
	);
#else	
	__asm
	{/* Delay */
		mov 	reg2, #100
		mov		reg1, #0
		mov		reg0, #1
	loop0:	add 		reg1, reg1, reg0
		cmp 		reg1, reg2
		bne		loop0
	}		
#endif

	outp32(REG_UPLLCON, inp32(REG_UPLLCON) | 0x4000);		/* Power down UPLL and APLL */
	outp32(REG_APLLCON, inp32(REG_APLLCON) | 0x4000);	

#if defined (__GNUC__)
	__asm volatile
	(
		"  mov 	%0, #100       \n"
		"  mov  %1, #0         \n"
		"  mov  %2, #1         \n"
		" loop1:	           \n"
		"  add  %1, %1, %2     \n"
		"  cmp 	%1, %0         \n"
		"  bne  loop1          \n"
		: : "r"(reg2), "r"(reg1), "r"(reg0) :"memory"
	);
#else	
	__asm
	{
			mov 	reg2, #100
			mov		reg1, #0
			mov		reg0, #1
	loop1:	add 	reg1, reg1, reg0
			cmp 	reg1, reg2
			bne		loop1
	}	
#endif	
	/* Fill and enable the pre-scale for power down wake up */
	outp32(REG_PWRCON, (inp32(REG_PWRCON)  & ~0xFFFF00) | 0xFF02);     // 25ms ~ 75ms depends on system power and PLL character

#if defined (__GNUC__)
	__asm volatile
	(
		"  mov 	%0, #10        \n"
		"  mov  %1, #0         \n"
		"  mov  %2, #1         \n"
		" loop2:	           \n"
		"  add  %1, %1, %2     \n"
		"  cmp 	%1, %0         \n"
		"  bne  loop2          \n"
		: : "r"(reg2), "r"(reg1), "r"(reg0) :"memory"
	);
#else		
	__asm
	{
		mov 	reg2, #10
		mov		reg1, #0
		mov		reg0, #1
	loop2:	add 	reg1, reg1, reg0
		cmp 		reg1, reg2
		bne		loop2
	}	
#endif	

	///////////////////////////////////////////////////////////////
	/*  Enter power down. (Stop the external clock */
#if defined (__GNUC__)
	__asm volatile
	(
		"MOV     %0,#0xb0000000 \n"
        "LDR     %0,[%0,#0x200] \n"
		"BIC     %0,%0,#0x1     \n"
		"MOV     %1,#0xb0000000 \n"
		"STR     %0,[%1,#0x200] \n"
		: : "r"(reg3), "r"(reg1), "r"(reg0): "memory"
    );
#else
	__asm 
	{/* Power down */
		MOV     reg3,#0xb0000000 
        LDR     reg3,[reg3,#0x200] 
		BIC     reg3,reg3,#0x1     
		MOV     reg2,#0xb0000000 
		STR     reg3,[reg2,#0x200] 
	}   
#endif
#if defined (__GNUC__)
	__asm volatile
	(
		"  mov 	%0, #300       \n"
		"  mov  %1, #0         \n"
		"  mov  %2, #1         \n"
		" loop3:	           \n"
		"  add  %1, %1, %2     \n"
		"  cmp 	%1, %0         \n"
		"  bne  loop3          \n"
		: : "r"(reg2), "r"(reg1), "r"(reg0) :"memory"
	);
#else
	__asm
	{/* Wake up*/ 
			mov 	reg2, #300
			mov		reg1, #0
			mov		reg0, #1
	loop3:	add 	reg1, reg1, reg0
			cmp 	reg1, reg2
			bne		loop3
	}
#endif
	
	 /* Force UPLL and APLL in normal mode */ 
	outp32(REG_UPLLCON, inp32(REG_UPLLCON) & (~0x4000));		
	outp32(REG_APLLCON, inp32(REG_APLLCON) & (~0x4000));	
	while((inp32(REG_POR_LVRD)&APLL_LKDT)==0);				// Wait PLL lock bit.
	{//Waitting for PLL stable if enable PLL again
#if defined (__GNUC__)
	__asm volatile
	(
		"  mov 	%0, #500       \n"
		"  mov  %1, #0         \n"
		"  mov  %2, #1         \n"
		" loop4:	           \n"
		"  add  %1, %1, %2     \n"
		"  cmp 	%1, %0         \n"
		"  bne  loop4          \n"
		: : "r"(reg2), "r"(reg1), "r"(reg0) :"memory"
	 );
#else
		__asm
		{/* Delay a moment for PLL stable */
				mov 	reg2, #500
				mov		reg1, #0
				mov		reg0, #1
		loop4:	add 	reg1, reg1, reg0
				cmp 	reg1, reg2
				bne		loop4
		}
#endif
	}	
	/* Change system clock to PLL and delay a moment.  Let DDR/SDRAM lock the frequency */
	outp32(REG_CLKDIV0, inp32(REG_CLKDIV0) | 0x18);	

#if defined (__GNUC__)
	__asm volatile
	(
		"  mov 	%0, #500       \n"
		"  mov  %1, #0         \n"
		"  mov  %2, #1         \n"
		" loop5:	           \n"
		"  add  %1, %1, %2     \n"
		"  cmp 	%1, %0         \n"
		"  bne  loop5          \n"
		: : "r"(reg2), "r"(reg1), "r"(reg0) :"memory"
	);
#else
	__asm
	{
			mov 	reg2, #500
			mov		reg1, #0
			mov		reg0, #1
	loop5:	add 	reg1, reg1, reg0
			cmp 	reg1, reg2
			bne		loop5
	}
#endif
	/*********** DRAM escape self refresh mode ************/
	outp32(REG_SDCMD,  0x20);
	/*********** DRAM escape self refresh mode ************/
	
#if defined (__GNUC__)
	__asm volatile
	(
		"  mov 	%0, #100       \n"
		"  mov  %1, #0         \n"
		"  mov  %2, #1         \n"
		" loop6:	           \n"
		"  add  %1, %1, %2     \n"
		"  cmp 	%1, %0         \n"
		"  bne  loop6          \n"
		: : "r"(reg2), "r"(reg1), "r"(reg0) :"memory"
	);
#else
	__asm
	{/*  Delay a moment until the escape self-refresh command reached to DDR/SDRAM */
			mov 	reg2, #100
			mov		reg1, #0
			mov		reg0, #1
	loop6:	add 	reg1, reg1, reg0
			cmp 	reg1, reg2
			bne		loop6
	}			
#endif
	
	outp32(REG_SDMR,  0x532);   								// RESET DLL(bit[8]) of DDR2 

#if defined (__GNUC__)
	__asm volatile
	(
		"  mov 	%0, #100       \n"
		"  mov  %1, #0         \n"
		"  mov  %2, #1         \n"
		" loop7:	           \n"
		"  add  %1, %1, %2     \n"
		"  cmp 	%1, %0         \n"
		"  bne  loop7          \n"
		: : "r"(reg2), "r"(reg1), "r"(reg0) :"memory"
	);
#else	
	__asm
	{/*  Delay a moment until the escape self-refresh command reached to DDR/SDRAM */
			mov 	reg2, #100
			mov		reg1, #0
			mov		reg0, #1
	loop7:	add 	reg1, reg1, reg0
			cmp 	reg1, reg2
			bne		loop7
	}	
#endif
	
	outp32(REG_SDMR,  0x432);  									// RESET DLL(bit[8]) of DDR2 		

#if defined (__GNUC__)
	outp32(REG_DLLMODE,   inp32(REG_DLLMODE_R)       | 0x18);	// Enable chip's DLL
	__asm volatile
	(
		"  mov 	%0, #4000     \n"
		"  mov  %1, #0         \n"
		"  mov  %2, #1         \n"
		" loop8:	           \n"
		"  add  %1, %1, %2     \n"
		"  cmp 	%1, %0         \n"
		"  bne  loop8          \n"
		: : "r"(reg2), "r"(reg1), "r"(reg0) :"memory"
	);
#else	
	outp32(REG_DLLMODE,   inp32(REG_DLLMODE_R)       | 0x18);	// Enable chip's DLL
	__asm														/*  Wait chip's DLL lock time > 100us */
	{
			mov 	reg2, #4000
			mov		reg1, #0
			mov		reg0, #1
	loop8:	add 	reg1, reg1, reg0
			cmp 	reg1, reg2
			bne		loop8
	}	
#endif

	outp32(REG_EXTRST_DEBOUNCE, u32RstDebounce);	
//	while (!(inpw(0xb8008118) & 0x400000));
//	outpb(0xb8008100, '6');		
			
}
#if defined (__GNUC__) && !defined (__CC_ARM)
#pragma GCC pop_options
#endif



static void Entry_PowerDown(UINT32 u32WakeUpSrc)
{
	UINT32 j;
	UINT32 u32IntEnable, u32IntEnableH, bIsCacheState, u32CacheMode;
	void (*wb_fun)(void);

	UINT32 u32RamBase = PD_RAM_BASE;
	//UINT32 u32RamSize = PD_RAM_SIZE;

	//memcpy((char*)((UINT32)_tmp_buf| 0x80000000), (char*)(u32RamBase | 0x80000000), u32RamSize);
	memcpy((VOID *)((UINT32)u32RamBase | 0x80000000),
			(VOID *)( ((UINT32)Sample_PowerDown -(PD_RAM_START-PD_RAM_BASE)) | 0x80000000),
			PD_RAM_SIZE);
#if 0
	if(memcmp((char*)(u32RamBase | 0x80000000), (char*)((UINT32)((UINT32)Sample_PowerDown -(PD_RAM_START-PD_RAM_BASE)) | 0x80000000), u32RamSize)!=0)
	{
		sysprintf("Memcpy copy wrong\n");
	}
#endif

	if(sysGetCacheState()==TRUE){
		DBG_PRINTF("Cache enable\n");
		bIsCacheState = TRUE;
		u32CacheMode = sysGetCacheMode();
		sysDisableCache();
		sysFlushCache(I_D_CACHE);
	}else{
		DBG_PRINTF("Cache disable\n");
	}
	
	wb_fun = (void(*)(void))(PD_RAM_START);
	sysprintf("Jump to SRAM (Suspend)\n");


	u32IntEnable = inp32(REG_AIC_IMR);
	u32IntEnableH = inp32(REG_AIC_IMRH);	
	
	outp32(REG_AIC_MDCR, 0xFFFFFFFE);
	outp32(REG_AIC_MDCRH, 0xFFFFFFFE);	
	outp32(REG_AIC_MECR, 0x00000000);	
	outp32(REG_AIC_MECRH, 0x00000000);
	
	j = 0x800;
	while(j--);
	if(u32WakeUpSrc>WE_UHC20)
		outp32(REG_MISSR, ((u32WakeUpSrc<<16)|(u32WakeUpSrc<<8)));	//Enable and Clear interrupt
	else	
		outp32(REG_MISSR, ((u32WakeUpSrc<<14)|(u32WakeUpSrc<<6)));
	wb_fun();

	
	if(u32WakeUpSrc>WE_EMAC)
		outp32(REG_MISSR, ((u32WakeUpSrc<<16)|(u32WakeUpSrc<<8)));	//Enable and Clear interrupt
	else	
		outp32(REG_MISSR, ((u32WakeUpSrc<<14)|(u32WakeUpSrc<<6)));
	
	//memcpy((VOID *)u32RamBase, (VOID *)_tmp_buf, PD_RAM_SIZE);
	DBG_PRINTF("Exit to SRAM (Suspend)\n");
	outp32(REG_AIC_MECR, u32IntEnable);								/*  Restore the interrupt channels */		
	outp32(REG_AIC_MECRH, u32IntEnableH);	
	if(bIsCacheState==TRUE)
		sysEnableCache(u32CacheMode);		
}
ERRCODE sysPowerDown(UINT32 u32WakeUpSrc)
{
	Entry_PowerDown(u32WakeUpSrc);
	return Successful;
}

