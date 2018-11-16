
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wbio.h"
#include "wbtypes.h" 
#include "W55fa92_vpost.h"
#include "ROT_demo.h"

extern VOID vpostEnableInt(E_DRVVPOST_INT eInt);
extern int vpostInstallCallBack(E_DRVVPOST_INT eIntSource,
						PFN_DRVVPOST_INT_CALLBACK	pfnCallback,
						PFN_DRVVPOST_INT_CALLBACK 	*pfnOldCallback);
extern VOID vpostSetFrameBuffer_BaseAddress(UINT32 u32BufferAddress);



PFN_DRVVPOST_INT_CALLBACK fun_ptr;
LCDFORMATEX lcdFormat;	
void	VPOST_InterruptServiceRiuntine()
{
	if(bIsBuffer0Dirty==1){//change VPOST to show the buffer, otherwise, keep to show original buffer 		
	
		bIsBuffer0Dirty = 0;	
		VpostUseBuf = 0;
		vpostSetFrameBuffer_BaseAddress((UINT32)u8FrameBuffer0);
	}else if(bIsBuffer1Dirty==1){		
		bIsBuffer1Dirty = 0;
		VpostUseBuf =1;
		vpostSetFrameBuffer_BaseAddress((UINT32)u8FrameBuffer1);
	}
}

void InitVPOST(UINT8* pu8FrameBuffer)
{		
	
	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;//DRVVPOST_FRAME_YCBYCR;  //DRVVPOST_FRAME_RGB565;
	lcdFormat.nScreenWidth = OPT_LCM_WIDTH;
	lcdFormat.nScreenHeight = OPT_LCM_HEIGHT;	  
	vpostLCMInit(&lcdFormat, (UINT32*)pu8FrameBuffer);
	
	vpostInstallCallBack(eDRVVPOST_VINT, (PFN_DRVVPOST_INT_CALLBACK)VPOST_InterruptServiceRiuntine,  (PFN_DRVVPOST_INT_CALLBACK*)&fun_ptr);
	vpostEnableInt(eDRVVPOST_VINT);	
	sysEnableInterrupt(IRQ_VPOST);	
 	
}	

