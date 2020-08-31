/**************************************************************************//**
 * @file     W55FA92_VPOST.c
 * @version  V3.00
 * @brief    N329xx series VPOST driver source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "wblib.h"
#include "W55FA92_VPOST.h"
#include "W55FA92_reg.h"


#include <stdio.h>

VOID * g_VAFrameBuf = NULL;
VOID * g_VAOrigFrameBuf = NULL;

extern INT vpostLCMInit_GIANTPLUS_GPM1006D0(PLCDFORMATEX plcdformatex, UINT32 *pFramebuf);

INT32 vpostLCMInit(PLCDFORMATEX plcdformatex, UINT32 *pFramebuf)
{

#ifdef __HAVE_SHARP_LQ035Q1DH02__
	return vpostLCMInit_SHARP_LQ035Q1DH02(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_WINTEK_WMF3324__
	return vpostLCMInit_WINTEK_WMF3324(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_HANNSTAR_HSD043I9W1__
	return vpostLCMInit_HANNSTAR_HSD043I9W1(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_HANNSTAR_HSD043I9W1_16B__
	return vpostLCMInit_HANNSTAR_HSD043I9W1_16B(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_HANNSTAR_HSD043I9W1_18B__
	return vpostLCMInit_HANNSTAR_HSD043I9W1_18B(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_GOWORLD_GWMTF9406A__
	return vpostLCMInit_GOWORLD_GWMTF9406A(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_GOWORLD_GWMTF9360A__
	return vpostLCMInit_GOWORLD_GWMTF9360A(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_GOWORLD_GWMTF9615A__
	return vpostLCMInit_GOWORLD_GWMTF9615A(plcdformatex, pFramebuf);
#endif
#ifdef __HAVE_GOWORLD_GWMTF9360A_MODIFY__
	return vpostLCMInit_GOWORLD_GWMTF9360A_modify(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_TOPPLY__
	return vpostLCMInit_TOPPLY(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_VG680__
	return vpostLCMInit_VG680(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_TVOUT_720x480__
	return vpostLCMInit_TVOUT_720x480(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_TVOUT_640x480__
	return vpostLCMInit_TVOUT_640x480(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_TVOUT_320x240__
	return vpostLCMInit_TVOUT_320x240(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_TVOUT_480x272_TO_640x480__
	return vpostLCMInit_TVOUT_480x272_TO_640x480();
#endif

#ifdef __HAVE_AMPIRE_800x600__
	return vpostLCMInit_AMPIRE_800x600(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_HANNSTAR_HSD070IDW1__
	return vpostLCMInit_HANNSTAR_HSD070IDW1(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_AMPIRE_800x480__
	return vpostLCMInit_AMPIRE_800x480(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_AMPIRE_800x480_24B__
	return vpostLCMInit_AMPIRE_800x480_24B(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_AMPIRE_800x480_18B__
	return vpostLCMInit_AMPIRE_800x480_18B(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_AMPIRE_800x480_16B__
	return vpostLCMInit_AMPIRE_800x480_16B(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_HIMAX_HX8346__
	return vpostLCMInit_HIMAX_HX8346(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_TOPPLY_320x240__
	return vpostLCMInit_TOPPLY_320x240(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_GIANTPLUS_GPM1006D0__
	return vpostLCMInit_GIANTPLUS_GPM1006D0(plcdformatex, pFramebuf);
#endif


#ifdef __HAVE_LVDS_1024x768_18B__
	return vpostLCMInit_LVDS_1024x768_18B(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_ILITEK_ILI9341__
	return vpostLCMInit_ILITEK_ILI9341(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_HIMAX_HX8379__
	return vpostLCMInit_HIMAX_HX8379(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_TS8350_480x854_24B__
	return vpostLCMInit_TS8350_480x854_24B(plcdformatex, pFramebuf);
#endif

#ifdef __HAVE_LMT043DN__
	return vpostLCMInit_LMT043DN();
#endif

#ifdef __HAVE_FW050TFT_800x480__
    return vpostLCMInit_FW050TFT_800x480(plcdformatex, pFramebuf);
#endif

}

extern INT32 vpostLCMDeinit_GIANTPLUS_GPM1006D0(void);

INT32 vpostLCMDeinit(void)
{
#ifdef __HAVE_SHARP_LQ035Q1DH02__
	return vpostLCMDeinit_SHARP_LQ035Q1DH02();
#endif

#ifdef __HAVE_WINTEK_WMF3324__
	return vpostLCMDeinit_WINTEK_WMF3324();
#endif

#ifdef __HAVE_HANNSTAR_HSD043I9W1__
	return vpostLCMDeinit_HANNSTAR_HSD043I9W1();
#endif

#ifdef __HAVE_HANNSTAR_HSD043I9W1_16B__
	return vpostLCMDeinit_HANNSTAR_HSD043I9W1_16B();
#endif

#ifdef __HAVE_HANNSTAR_HSD043I9W1_18B__
	return vpostLCMDeinit_HANNSTAR_HSD043I9W1_18B();
#endif

#ifdef __HAVE_TOPPLY__
	return vpostLCMDeinit_TOPPLY();
#endif

#ifdef __HAVE_GOWORLD_GWMTF9406A__
	return vpostLCMDeinit_GOWORLD_GWMTF9406A();
#endif

#ifdef __HAVE_GOWORLD_GWMTF9360A__
	return vpostLCMDeinit_GOWORLD_GWMTF9360A();
#endif

#ifdef __HAVE_GOWORLD_GWMTF9615A__
	return vpostLCMDeinit_GOWORLD_GWMTF9615A();
#endif
#ifdef __HAVE_GOWORLD_GWMTF9360A_MODIFY__
	return vpostLCMDeinit_GOWORLD_GWMTF9360A_modify();
#endif

#ifdef __HAVE_VG680__
	return vpostLCMDeinit_VG680();
#endif

#ifdef __HAVE_TVOUT_720x480__
	return vpostLCMDeinit_TVOUT_720x480();
#endif

#ifdef __HAVE_TVOUT_640x480__
	return vpostLCMDeinit_TVOUT_640x480();
#endif

#ifdef __HAVE_TVOUT_320x240__
	return vpostLCMDeinit_TVOUT_320x240();
#endif

#ifdef __HAVE_TVOUT_480x272_TO_640x480__
	return vpostLCMDeinit_TVOUT_480x272_TO_640x480();
#endif

#ifdef __HAVE_AMPIRE_800x600__
	return vpostLCMDeinit_AMPIRE_800x600();
#endif

#ifdef __HAVE_AMPIRE_800x480__
	return vpostLCMDeinit_AMPIRE_800x480();
#endif

#ifdef __HAVE_AMPIRE_800x480_24B__
	return vpostLCMDeinit_AMPIRE_800x480_24B();
#endif

#ifdef __HAVE_AMPIRE_800x480_18B__
	return vpostLCMDeinit_AMPIRE_800x480_18B();
#endif

#ifdef __HAVE_AMPIRE_800x480_16B__
	return vpostLCMDeinit_AMPIRE_800x480_16B();
#endif

#ifdef __HAVE_HANNSTAR_HSD070IDW1__
	return vpostLCMDeinit_HANNSTAR_HSD070IDW1();
#endif

#ifdef __HAVE_HIMAX_HX8346__
	return vpostLCMDeinit_HIMAX_HX8346();
#endif

#ifdef __HAVE_TOPPLY_320x240__
	return vpostLCMDeinit_TOPPLY_320x240();
#endif

#ifdef __HAVE_GIANTPLUS_GPM1006D0__
	return vpostLCMDeinit_GIANTPLUS_GPM1006D0();
#endif

#ifdef __HAVE_LVDS_1024x768_18B__
	return vpostLCMDeinit_LVDS_1024x768_18B();
#endif
	
#ifdef __HAVE_HIMAX_HX8379__
	return vpostLCMDeinit_HIMAX_HX8379();
#endif

#ifdef __HAVE_TS8350_480x854_24B__
	return vpostLCMDeinit_TS8350_480x854_24B();
#endif

#ifdef __HAVE_LMT043DN__
	return vpostLCMDeinit_LMT043DN();
#endif

#ifdef __HAVE_FW050TFT_800x480__
    return vpostLCMDeinit_FW050TFT_800x480();
#endif
}

VOID* vpostGetFrameBuffer(void)
{
    return g_VAFrameBuf;
}

VOID vpostSetFrameBuffer(UINT32 pFramebuf)
{ 
	g_VAFrameBuf = (VOID *)pFramebuf;
	g_VAFrameBuf = (VOID*)((UINT32)g_VAFrameBuf | 0x80000000);
    outpw(REG_LCM_FSADDR, (UINT32)pFramebuf);
}


void LCDDelay(unsigned int nCount)
{
	unsigned volatile int i;
		
	for(;nCount!=0;nCount--)
//		for(i=0;i<100;i++);
		for(i=0;i<10;i++);
}
