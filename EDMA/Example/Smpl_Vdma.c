/**************************************************************************//**
 * @file     Smpl_Vdma.c
 * @brief    Use VDMA to transfer data from memory to memory.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wblib.h"
#include "W55FA92_EDMA.h"
#include "W55FA92_VPOST.h"


#if defined(__GNUC__)
__attribute__((aligned(32))) UINT8 DestBuffer[320*240*2];
#else
__align(32) UINT8 DestBuffer[320*240*2];
#endif
#define	DEST_ADDR	((UINT32)DestBuffer | NON_CACHE_BIT)

#if defined(__GNUC__)
extern __attribute__((aligned(32))) UINT8 LoadAddr[];
#else
extern __align(32) UINT8 LoadAddr[];
#endif
extern LCDFORMATEX lcdFormat;

INT32 g_VdmaCh = 0;
volatile BOOL g_bVdmaInt = FALSE;

void EdmaIrqHandler(unsigned int arg)
{ 	
	EDMA_Free(g_VdmaCh);  	
	g_bVdmaInt = TRUE;
}

void ColorSpaceTransformTest(void)
{
	UINT32 src_addr, dest_addr;

	src_addr = (UINT32)LoadAddr | NON_CACHE_BIT;
	dest_addr = DEST_ADDR;
	
	g_VdmaCh = VDMA_FindandRequest();
	
	if (g_VdmaCh < 0) 
	{
		sysprintf("Request VDMA fail.\n");
	       return;
	}
	
	EDMA_SetupCST(g_VdmaCh, eDRVEDMA_RGB565, eDRVEDMA_YCbCr422);
	EDMA_SetupSingle(g_VdmaCh, src_addr, dest_addr, 320*240*2);
	EDMA_SetupHandlers(g_VdmaCh, eDRVEDMA_BLKD_FLAG, EdmaIrqHandler, 0);
	
	EDMA_Trigger(g_VdmaCh);

	while(g_bVdmaInt == FALSE);

	EDMA_ClearCST(g_VdmaCh);
	g_bVdmaInt = FALSE;

	sysprintf("\nColor space transform test complete\n");

	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;
	vpostLCMInit(&lcdFormat, (UINT32*)dest_addr);
	
}

void TransferLengthTest(void)
{
	UINT32 src_addr, dest_addr;

	src_addr = (UINT32)LoadAddr | NON_CACHE_BIT;
	dest_addr = DEST_ADDR;

	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;	
	vpostLCMInit(&lcdFormat, (UINT32*)LoadAddr);
	
	g_VdmaCh = VDMA_FindandRequest();
	
	if (g_VdmaCh < 0) 
	{
		sysprintf("Request VDMA fail.\n");
	       return;
	}

	EDMA_SetupSingle(g_VdmaCh, src_addr, dest_addr, 320*240*2);
	EDMA_SetupHandlers(g_VdmaCh, eDRVEDMA_BLKD_FLAG, EdmaIrqHandler, 0);

	EDMA_Trigger(g_VdmaCh);

	while(g_bVdmaInt == FALSE);
	
	g_bVdmaInt = FALSE;

	if(memcmp((UINT8*)src_addr, (UINT8*)dest_addr, 320*240*2) != 0)
	{
		sysprintf("\nCompare error\n");
		while(1);
	}
	else
		sysprintf("\nTransfer length test complete\n");
	
}








