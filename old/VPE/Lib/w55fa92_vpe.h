#ifndef _w55fa92_VPE_H__
#define _w55fa92_VPE_H__
#ifdef  __cplusplus
extern "C"
{
#endif

#define ERR_VPE_OPEN 		(VPE_ERR_ID|0x01)
#define ERR_VPE_CLOSE		(VPE_ERR_ID|0x02)
#define ERR_VPE_SRC_FMT		(VPE_ERR_ID|0x03)
#define ERR_VPE_DST_FMT		(VPE_ERR_ID|0x04)
#define ERR_VPE_OP			(VPE_ERR_ID|0x05)
#define ERR_VPE_IOCTL		(VPE_ERR_ID|0x06)
#define E_VPE_INVALID_INT	(VPE_ERR_ID|0x07)

typedef void (*PFN_VPE_CALLBACK)(void);

typedef enum
{
	VPE_INT_COMP=0,
	VPE_INT_PAGE_FAULT,
	VPE_INT_PAGE_MISS,		//After resume, still page fault. 
	VPE_INT_MB_COMP,
	VPE_INT_MB_ERR,
	VPE_INT_DMA_ERR
}E_VPE_INT_TYPE;


/* VPE source format */
#if 0
typedef enum
{
	VPE_SRC_PLANAR_YUV420 =0,	 
	VPE_SRC_PLANAR_YUV422 =1,
	VPE_SRC_PLANAR_YUV444,		
	VPE_SRC_PACKET_YUV422,
	VPE_SRC_PLANAR_YUV422T,
	VPE_SRC_PACKET_RGB555=5,
	VPE_SRC_PACKET_RGB565,
	VPE_SRC_PACKET_RGB888,
	VPE_SRC_PLANAR_YUV400 //=== Not meet SPEC.
}E_VPE_SRC_FMT;
#else
typedef enum
{
	VPE_SRC_PLANAR_YONLY  =0,	 
	VPE_SRC_PLANAR_YUV420 =1,
	VPE_SRC_PLANAR_YUV411 =2,
	VPE_SRC_PLANAR_YUV422 =3,		
	VPE_SRC_PLANAR_YUV422T=5,	
	VPE_SRC_PLANAR_YUV444 = 9,
	
	VPE_SRC_PACKET_YUV422 = 12,
	VPE_SRC_PACKET_RGB555 = 13,
	VPE_SRC_PACKET_RGB565 = 14,
	VPE_SRC_PACKET_RGB888 = 15
}E_VPE_SRC_FMT;
#endif
typedef enum 
{
	VPE_DST_PACKET_YUV422=0,
	VPE_DST_PACKET_RGB555,
	VPE_DST_PACKET_RGB565,
	VPE_DST_PACKET_RGB888
}E_VPE_DST_FMT;

typedef enum 
{
	VPE_OP_NORMAL=0x0,
	VPE_OP_RIGHT=0x1,
	VPE_OP_LEFT,
	VPE_OP_UPSIDEDOWN,
	VPE_OP_FLIP,
	VPE_OP_FLOP,
	VPE_DDA_SCALE
}E_VPE_CMD_OP;

typedef enum
{
	VPE_SCALE_DDA = 0,				/* 3x3 and Bilinear are disabled */
	VPE_SCALE_3X3 = 1,				/* Only enable 3x3, Not support now. It has to be approached by 2 steps*/
	VPE_SCALE_BILINEAR = 2,			/* Only enable Bilinear */
	VPE_SCALE_3X3_BILINEAR = 3		/* Both downscale are enabled, Not support now */
}E_VPE_SCALE_MODE;

typedef enum
{
	VPE_HOST_FRAME  =0,	 	// Block base (8x8 or 16x16)
	VPE_HOST_VDEC_LINE =1,	// Line base. for H264, H.263 annex-j. (4x4 block) ()
	VPE_HOST_JPEG =2,		// 
	VPE_HOST_VDEC_SW =3,		// Software, Block base 
	VPE_HOST_FRAME_TURBO =3		// Turbo Mode of "HOST=00"
}E_VPE_HOST;


#define VPE_IOCTL_SET_SRCBUF_ADDR			0
#define VPE_IOCTL_SET_DSTBUF_ADDR			(VPE_IOCTL_SET_SRCBUF_ADDR+2)
#define VPE_IOCTL_SET_FMT					(VPE_IOCTL_SET_DSTBUF_ADDR+2)
#define VPE_IOCTL_SET_SRC_OFFSET			(VPE_IOCTL_SET_FMT+2)
#define VPE_IOCTL_SET_DST_OFFSET			(VPE_IOCTL_SET_SRC_OFFSET+2)
#define VPE_IOCTL_SET_SRC_DIMENSION			(VPE_IOCTL_SET_DST_OFFSET+2)
#define VPE_IOCTL_SET_DST_DIMENSION			(VPE_IOCTL_SET_SRC_DIMENSION+2)
#define VPE_IOCTL_SET_MACRO_BLOCK			(VPE_IOCTL_SET_DST_DIMENSION+2)
#define VPE_IOCTL_SET_COLOR_RANGE			(VPE_IOCTL_SET_MACRO_BLOCK+2)
#define VPE_IOCTL_SET_FILTER				(VPE_IOCTL_SET_COLOR_RANGE+2)
#define VPE_IOCTL_SET_3X3_COEF				(VPE_IOCTL_SET_FILTER+2)
#define VPE_IOCTL_HOST_OP					(VPE_IOCTL_SET_3X3_COEF+2)
#define VPE_IOCTL_TRIGGER					(VPE_IOCTL_HOST_OP+2)
#define VPE_IOCTL_CHECK_TRIGGER				(VPE_IOCTL_TRIGGER+2)
ERRCODE vpeOpen(void);

ERRCODE vpeClose(void);

ERRCODE 
vpeInstallCallback(
	E_VPE_INT_TYPE eIntType, 
	PFN_VPE_CALLBACK pfnCallback,
	PFN_VPE_CALLBACK *pfnOldCallback
);

ERRCODE 
vpeEnableInt(
	E_VPE_INT_TYPE eIntType
);

ERRCODE 
vpeDisableInt(
	E_VPE_INT_TYPE eIntType
);

ERRCODE 
vpeIoctl(
	UINT32 u32Cmd, 
	UINT32 u32Arg0, 
	UINT32 u32Arg1, 
	UINT32 u32Arg2
); 

BOOL vpeCheckTrigger(void);

void vpeGetReferenceVirtualAddress(
	PUINT32 p32YBufAddr,
	PUINT32 p32UBufAddr,
	PUINT32 p32VBufAddr,
	PUINT32 p32DstBufAddr
);	

void vpemmuSetTTBEntry (int idx, UINT32 entryValue);	// BLT MMU Level-One Page Table.
void vpemmuSetTTB (UINT32 ttb);	// BLT MMU Translation Table Base Address. 16KB aligned.
void vpemmuEnableMainTLB (BOOL enabled);	// Turn On/Off BLT MMU Main TLB (SRAM Buffers)
void vpemmuEnableMainTLBSrcCh (BOOL enabled);	// BLT MMU Main TLB Service Channels.
void vpemmuEnableMMU (BOOL enabled);	// Enable or Disable BLT MMU Virtual Address Translation.
void vpemmuFlushMainTLB (void);	// Flush BLT MMU Main TLB Entries of Main TLB SRAM.
void vpemmuInvalidateMicroTLB (void);	// Invalidate BLT MMU Micro TLB Entries.
void vpemmuTeardown (void);	// Tear down BLT MMU.
void vpemmuSetup (UINT32 ttb, BOOL mainTLB, BOOL mainTLBSrcCh);	// Set up BLT MMU.
UINT32 vpemmuGetPageFaultVA (void);	// BLT MMU Page Fault Virtual Address.
UINT32 vpemmuGetTTB (void);
void vpemmuResumeMMU (void);	// Resume BLT MMU Transaction.

#ifdef __cplusplus
}
#endif
#endif /* _w55fa92_VPE_H */
