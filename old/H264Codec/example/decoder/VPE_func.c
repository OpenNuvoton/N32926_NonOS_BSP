/*=======================================================
	VPE Emulation Code 
	1. Format conversion
	2. Scaling down				(Quality is important!)
	3. Scaling up				(Quality is important!)
	3. On the fly with C&M  		(Important!!!)
	4. On the fly with JPEG 
	5. Sorce Offset & dst offset 
	6. Rotation		  		(Important!!)
	7. Scatter gather 			(Important!!)
========================================================*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "w55fa92_reg.h"
#include "wblib.h"
#include "w55fa92_vpe.h"
#include "nvtfat.h"
#include "W55FA92_SIC.h"
#include "w55fa92_vpost.h"


#define DBG_PRINTF(...)

UINT32 u3210msFlag=0;
void TimerBase(void)
{
	u3210msFlag = u3210msFlag+1;
}

BOOL bIsVPECompleteInt = FALSE; 
BOOL bIsVPEScatterGatherOnePieceInt = FALSE;
BOOL bIsVPEBlockInt = FALSE;
BOOL bIsVPEBlockErrorInt = FALSE;
BOOL bIsVPEDMAErrorInt = FALSE;
UINT32 u32CompletCount = 0;

extern int dec_mmap_addr;


void vpeCompleteCallback(void)
{
//	sysprintf("I bit in InISR = %d\n", sysGetIBitState());
	u32CompletCount= u32CompletCount+1;
	bIsVPECompleteInt = TRUE;
}	
extern unsigned int _mmuSectionTable[];

void vpeMacroBlockCallback(void)
{
	bIsVPEBlockInt = TRUE;
}
void vpeMacroBlockErrorCallback(void)
{
	bIsVPEBlockErrorInt = TRUE;
}	
void vpeDmaErrorCallback(void)
{
	bIsVPEDMAErrorInt = TRUE;
}
void vpeInit(void)		
{
	PFN_VPE_CALLBACK OldVpeCallback;
	
	vpeOpen();	//Assigned VPE working clock to 48MHz. 
	vpeInstallCallback(VPE_INT_COMP,
						vpeCompleteCallback, 
						&OldVpeCallback);				
#if 0						
	vpeInstallCallback(VPE_INT_PAGE_FAULT,
						vpePageFaultCallback, 
						&OldVpeCallback);						
	vpeInstallCallback(VPE_INT_PAGE_MISS,
						vpePageMissCallback, 
						&OldVpeCallback);
#endif						
	/* For C&M and JPEG	*/			
	vpeInstallCallback(VPE_INT_MB_COMP,
						vpeMacroBlockCallback, 
						&OldVpeCallback);
				
	vpeInstallCallback(VPE_INT_MB_ERR,
						vpeMacroBlockErrorCallback, 
						&OldVpeCallback);										
	vpeInstallCallback(VPE_INT_DMA_ERR,
						vpeDmaErrorCallback, 
						&OldVpeCallback);
	vpeEnableInt(VPE_INT_COMP);				
	vpeEnableInt(VPE_INT_PAGE_FAULT);	
	vpeEnableInt(VPE_INT_PAGE_MISS);	
#if 0			
	vpeEnableInt(VPE_INT_MB_COMP);	
	vpeEnableInt(VPE_INT_MB_ERR);
#endif			
	vpeEnableInt(VPE_INT_DMA_ERR);		
						
}	

#define	SRC_PLANAR_YUV420 	1
#define	Packet_RGB565		2
//extern  UINT8 DecY[640*480];
//extern  UINT8 DecU[640*480/2];
//extern  UINT8 DecV[640*480/2];
	
INT32 NormalFormatConversionRotationDownscale_QVGA(void)
{
	//INT8* pi8Y=0;
	//INT8* pi8U=0;
	//INT8* pi8V=0; 
	//INT8* piDstAddr=0;
	//ERRCODE ErrCode;
	//UINT32 u32SrcLeftOffset=0, u32SrcRightOffset=0;
	//INT32 u32Idx=0, u32Idy;
	UINT32 u32Width=320, u32Height=240;
	
	//UINT32 u32TarW, u32TarH;
	
																																								
																	
				vpeIoctl(VPE_IOCTL_SET_FMT,
							SRC_PLANAR_YUV420,	/* Src Format */
							Packet_RGB565,					/* Dst Format */
							0);	
							
				vpeIoctl(VPE_IOCTL_SET_SRC_OFFSET,		
							(UINT32)0,	/* Src Left offset */
							(UINT32)0,	/* Src right offset */
							NULL);	
				vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
							(UINT32)0,				/* Dst Left offset */
							(UINT32)0,				/* Dst right offset */
							NULL);	
						
				vpeIoctl(VPE_IOCTL_SET_SRC_DIMENSION,						
							640,
							480,
							NULL);
																						
				vpeIoctl(VPE_IOCTL_SET_COLOR_RANGE,
							FALSE,
							FALSE,
							NULL);			
							
				vpeIoctl(VPE_IOCTL_SET_FILTER,
							//VPE_SCALE_3X3,			//Removed 
							//VPE_SCALE_DDA,			//OK
							VPE_SCALE_BILINEAR,		//
							NULL,
							NULL);		
				
				vpeIoctl(VPE_IOCTL_SET_3X3_COEF,
							0x0,						//Central weight =0 ==> Hardware bulid in coefficience. 
							0x0,
							0x0);										
		
					
/*		
				vpeIoctl(VPE_IOCTL_SET_SRCBUF_ADDR,
							(UINT32)DecY,				// MMU on, the is virtual address, MMU off, the is physical address. 
							(UINT32)DecU,	
							(UINT32)DecV);
*/												
					vpeIoctl(VPE_IOCTL_SET_DSTBUF_ADDR,
							//(UINT32)VPOSDISPLAYBUFADDR,
							(UINT32)dec_mmap_addr,
							NULL,
							NULL);	
								
					vpeIoctl(VPE_IOCTL_SET_DST_DIMENSION,	
								u32Width,
								u32Height,
								NULL);											
								
	return 0;
}	

INT32 VPE_trigger(void)
{
					vpeIoctl(VPE_IOCTL_TRIGGER,
								NULL,
								NULL,
								NULL);	
	return 0;
}

		
INT32 VPE_entry(void)
{

	vpeInit();	
	NormalFormatConversionRotationDownscale_QVGA();
	
										
	return 0;
} 
