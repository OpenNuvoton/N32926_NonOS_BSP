
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wbio.h"
#include "wbtypes.h"
#include "W55fa92_vpost.h"
#include "demo.h"

extern VOID vpostEnableInt(E_DRVVPOST_INT eInt);
extern int vpostInstallCallBack(E_DRVVPOST_INT eIntSource,
						PFN_DRVVPOST_INT_CALLBACK	pfnCallback,
						PFN_DRVVPOST_INT_CALLBACK 	*pfnOldCallback);
extern VOID vpostSetFrameBuffer(UINT32 u32BufferAddress);
	
extern BOOL bIsFrameBuffer0,  bIsFrameBuffer1, bIsFrameBuffer2; 
extern UINT8 u8PacketFrameBuffer[],  u8PacketFrameBuffer1[],  u8PacketFrameBuffer2[];
static UINT32 u32BufIdx=2;

void	VPOST_InterruptServiceRiuntine()
{
	switch(u32BufIdx)
	{
		case 0: 
			if(bIsFrameBuffer1==1)	
			{/* Check Frame Buffer 1 is dirty */
				bIsFrameBuffer0 = 0;	/* Frame Buffer 0 is clean */
				u32BufIdx = 1;			/* Now display buf 1 */
				vpostSetFrameBuffer((UINT32)u8PacketFrameBuffer1);
			}
			break;		
		case 1: 
			if(bIsFrameBuffer2==1)	
			{/* Check Frame Buffer 2 is dirty */
				bIsFrameBuffer1 = 0;	/* Frame Buffer 1 is clean */
				u32BufIdx = 2;			/* Now display buf 2 */	
				vpostSetFrameBuffer((UINT32)u8PacketFrameBuffer2);
			}
			break;
		case 	2:  	
			if(bIsFrameBuffer0==1)		
			{/* Check Frame Buffer 0 is dirty */
				bIsFrameBuffer2 = 0;	/* Frame Buffer 2 is clean */
				u32BufIdx = 0;			/* Now display buf 0 */
				vpostSetFrameBuffer((UINT32)u8PacketFrameBuffer);			
			}
			break;
	}	
}

void InitVPOST(UINT8* pu8FrameBuffer)
{		
	PFN_DRVVPOST_INT_CALLBACK fun_ptr;
	LCDFORMATEX lcdFormat;	

	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;//DRVVPOST_FRAME_YCBYCR;  //DRVVPOST_FRAME_RGB565;
	lcdFormat.nScreenWidth = OPT_LCM_WIDTH;
	lcdFormat.nScreenHeight = OPT_LCM_HEIGHT;	  
	vpostLCMInit(&lcdFormat, (UINT32*)pu8FrameBuffer);
	
	vpostInstallCallBack(eDRVVPOST_VINT, (PFN_DRVVPOST_INT_CALLBACK)VPOST_InterruptServiceRiuntine,  (PFN_DRVVPOST_INT_CALLBACK*)&fun_ptr);
//#ifdef __LCM_480x272__
	vpostEnableInt(eDRVVPOST_VINT);	
	sysEnableInterrupt(IRQ_VPOST);	
//#endif 	
}	
//////////////////////////////////////////////////////////////////////////////////////////////////////



extern BOOL bIsVin2FrameBuffer0,  bIsVin2FrameBuffer1, bIsVin2FrameBuffer2; 
extern UINT8 u8PacketFrameBuffer2_0[],  u8PacketFrameBuffer2_1[],  u8PacketFrameBuffer2_2[];
static UINT32 u32VPBufIdx=2;

void	VPOST_InterruptServiceRiuntine_2()
{
	switch(u32VPBufIdx)
	{
		case 0: 
			if(bIsVin2FrameBuffer1==1)	
			{/* Check Frame Buffer 1 is dirty */
				bIsFrameBuffer0 = 0;	/* Frame Buffer 0 is clean */
				u32VPBufIdx = 1;		/* Now display buf 1 */		
				vpostSetFrameBuffer((UINT32)u8PacketFrameBuffer2_1);
			}
			break;		
		case 1: 
			if(bIsVin2FrameBuffer2==1)	
			{/* Check Frame Buffer 2 is dirty */
				bIsFrameBuffer1 = 0;	/* Frame Buffer 1 is clean */
				u32VPBufIdx = 2;
				vpostSetFrameBuffer((UINT32)u8PacketFrameBuffer2_2);
			}
			break;
		case 	2:  	
			if(bIsVin2FrameBuffer0==1)		
			{/* Check Frame Buffer 0 is dirty */
				bIsFrameBuffer2 = 0;	/* Frame Buffer 2 is clean */
				u32VPBufIdx = 0;
				vpostSetFrameBuffer((UINT32)u8PacketFrameBuffer2_0);			
			}
			break;
	}	
}

void InitVPOST_2(UINT8* pu8FrameBuffer)
{		
	PFN_DRVVPOST_INT_CALLBACK fun_ptr;
	LCDFORMATEX lcdFormat;	
	
	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_YCBYCR;//DRVVPOST_FRAME_YCBYCR;  //DRVVPOST_FRAME_RGB565;
	lcdFormat.nScreenWidth = OPT_LCM_WIDTH;
	lcdFormat.nScreenHeight = OPT_LCM_HEIGHT;	  
	vpostLCMInit(&lcdFormat, (UINT32*)pu8FrameBuffer);
	
	vpostInstallCallBack(eDRVVPOST_VINT, (PFN_DRVVPOST_INT_CALLBACK)VPOST_InterruptServiceRiuntine_2,  (PFN_DRVVPOST_INT_CALLBACK*)&fun_ptr);
//#ifdef __LCM_480x272__
	vpostEnableInt(eDRVVPOST_VINT);	
	sysEnableInterrupt(IRQ_VPOST);	
//#endif 	
}	
