#ifndef _ROTLIB_H_
#define _ROTLIB_H_
#include "wbtypes.h"

typedef void (*PFN_ROT)(void);
typedef void* PVOID;
#ifndef NULL
#define NULL 0
#endif
#ifndef CALLBACK
#define CALLBACK
#endif

#define ROT_ERR_ID				0xFFFF2000	/* ROT library ID */
#define ERR_ROT_BUSY       		(ROT_ERR_ID + 1) 	

typedef enum tagRotEngFmt
{
	E_ROT_PACKET_RGB565 = 0,
	E_ROT_PACKET_RGB888,
	E_ROT_PACKET_YUV422
}E_ROTENG_FMT;
typedef enum tagRotIntNum
{
	E_ROT_COMP_INT =0,
	E_ROT_ABORT_INT=1,
	E_ROT_OVERFLOW_INT=2
}E_ROTENG_INT_NUM;

typedef enum tagRotEngBufSize
{
	E_LBUF_4 = 0,
	E_LBUF_8,
	E_LBUF_16
}E_ROTENG_BUFSIZE;
typedef enum tagRotEngDir
{
	E_ROT_ROT_R90 = 0,
	E_ROT_ROT_L90
}E_ROTENG_DIR;

typedef struct tagRotationEng
{
	E_ROTENG_FMT eRotFormat;		// Assigned the rotation format RGB888/RGB565/YUV422
	E_ROTENG_BUFSIZE eBufSize;		// Assigned the buffer size for on the fly rotation
	E_ROTENG_DIR eRotDir;			// Left/Right
	
	UINT32 u32RotDimHW;			// Rotation Dimension
	UINT32 u32SrcLineOffset;		// Source line offset, pixel unit
	UINT32 u32DstLineOffset;	// Destination  line offset, pixel unit
	UINT32 u32SrcAddr;				// Source buffer start address of rotated image
	UINT32 u32DstAddr;				//
}T_ROT_CONF;
void rotOpen(void);
void rotClose(void);


INT32 rotImageConfig(T_ROT_CONF* ptRotConf);
INT32 rotTrigger(void);
INT32 rotGetPacketPixelWidth(E_ROTENG_FMT ePacFormat);
//void rotPacketResetDstBufferAddr(T_ROT_CONF* ptRotConf);
//void CALLBACK rotIntHandler(void);
//void rotInit(BOOL bIsRotEngMode, BOOL bIsRotIntEnable);
void rotInstallISR(UINT32 u32IntNum,PVOID pvIsr);
//INT32 rotSrcLineOffset(UINT32 uSrcFmt, UINT uSrcPacLineOffset);
//INT32 rotDstLineOffset(UINT32 uSrcFmt, UINT uDstPacLineOffset);

#endif	/*_ROTLIB_H_*/
