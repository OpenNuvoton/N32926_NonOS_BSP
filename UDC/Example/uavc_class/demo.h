
#include "W55FA92_VideoIn.h"

#define OPT_UART
#ifdef OPT_UART
#define DBG_PRINTF		sysprintf
#else
#define DBG_PRINTF		printf
#endif

#define	OPT_ENCODE_WIDTH		640			//35K pixel
#define	OPT_ENCODE_HEIGHT		480	
#define OPT_STRIDE				640
#define OPT_LCM_WIDTH			640
#define OPT_LCM_HEIGHT			480

#define OPT_CROP_WIDTH			640
#define OPT_CROP_HEIGHT			480
#define OPT_PREVIEW_WIDTH		640
#define OPT_PREVIEW_HEIGHT		480

void VideoIn_InterruptHandler(void);

UINT32 Smpl_OV7725_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1);

/* Buffer for Packet & Planar format */
extern UINT8 u8PlanarFrameBuffer0[];
extern UINT8 u8PlanarFrameBuffer1[];
extern UINT8 u8PlanarFrameBuffer2[];
extern UINT8 u8PacketFrameBuffer0[];
extern UINT8 u8PacketFrameBuffer1[];
extern UINT8 u8PacketFrameBuffer2[];

/* Current Width & Height */
extern UINT16 u16CurWidth, u16CurHeight;

extern VINDEV_T Vin;
extern VINDEV_T* pVin;

void Delay(UINT32 nCount);

UINT32 Smpl_OV9660(void);
UINT32 Smpl_OV7670(void);
UINT32 Smpl_OV7725(void);
UINT32 Smpl_NT99141(void);
UINT32 Smpl_NT99050(void);
UINT32 Smpl_NT99160(void);

/* UVC Main */
VOID uvc_main(void);
/* UVC event */
VOID uvcdEvent(void);
/* Change VideoIN Setting for Frame size or Buffer Address */
VOID ChangeFrame(BOOL bChangeSize, UINT32 u32Address, UINT16 u16Width,UINT16 u16Height);
/* VideoIN Buffer Address Control when Frame End */
VOID VideoInFrameEnd_InterruptHandler(void);
/* Get Image Buffer for USB transfer */
INT GetImageBuffer(VOID);
/* JPEG Encode function */
UINT32 jpegEncode(UINT32 u32YAddress,UINT32 u32BitstreamAddress, UINT16 u16Width,UINT16 u16Height);
/* Process Unit Control */
UINT32 ProcessUnitControl(UINT32 u32ItemSelect,UINT32 u32Value);
/* Get Image Size and Address (Image data control for Foramt and Frame)*/
INT GetImage(PUINT32 pu32Addr, PUINT32 pu32transferSize);

//Smpl_I2C.C
BOOL 
I2C_Write_8bitSlaveAddr_8bitReg_8bitData(
	UINT8 uAddr, 
	UINT8 uRegAddr, 
	UINT8 uData	
);
UINT8 
I2C_Read_8bitSlaveAddr_8bitReg_8bitData(
	UINT8 uAddr, 
	UINT8 uRegAddr
);
