#ifndef _VIDEOIN_DEMO
#define _VIDEOIN_DEMO
#include "W55FA92_VideoIn.h"

//#define _SIM_

#define OPT_UART
#ifdef OPT_UART
#define DBG_PRINTF		sysprintf
#else
#define DBG_PRINTF(...)
#endif

#define __ENABLE_CACHE__
#ifdef __ENABLE_CACHE__
#define E_NONCACHE_BIT			0x80000000
#else
#define E_NONCACHE_BIT			0x00000000
#endif

//#define _MACRO_BLOCK_			/* if want to enable H.264 encoder. Example code only support  JPEG encoder */

//Choice sensor type. It deteminate the cropping size. 
//#define OV9660_VGA	
//#define OV9660_SXGA
//#define OV7670_VGA
//#define OV2640_SVGA
//#define OV2640_UXGA
//#define OV3642_QXGA
//#define VPG_QXGA
//#define SA71113
//#define WT8861
//#define TW9912
//#define OV7725_VGA
#define NT99141_VGA
//#define NT99141_HD
//#define NT99142_VGA
//#define NT99142_SVGA
//#define NT99142_HD
//#define NT99050_VGA
//#define NT99160_VGA
//#define NT99160_SVGA
//#define NT99160_HD
//#define HM1375_VGA
//#define HM1375_HD
//#define HM2056_HD
//#define HM2056_VGA
//#define HM1246_VGA
//#define HM1246_HD
//#define HM1246_SXGA
//#define SC1046_HD

//#define NT99340_HD
//#define NT99340_FULLHD
//#define NT99340_QXGA

//#define NT99252_SVGA
//#define NT99252_UXGA
//#define GC0308_VGA
 
//#define OV10633_VGA
//#define OV10633_HD

//#define GM7150_ONE_FIELD
//#define GM7150_TWO_FIELDS

//#define TVP5150_ONE_FIELD
//#define TVP5150_TWO_FIELDS

#ifdef GM7150_ONE_FIELD
	#define OPT_CROP_WIDTH		640
	#define OPT_CROP_HEIGHT		240
	#define ENCODE_HALF_VGA_DIMENSION
#elif defined(TVP5150_ONE_FIELD)
	#define OPT_CROP_WIDTH		640
	#define OPT_CROP_HEIGHT		240
	#define ENCODE_HALF_VGA_DIMENSION	
#elif defined(OV9660_VGA)||defined(OV7670_VGA)||defined(SA71113)||defined(WT8861)\
	|| defined(OV7725_VGA) || defined(NT99050_VGA) || defined(NT99160_VGA) || defined(HM1375_VGA)||defined(GC0308_VGA)||defined(TW9912)\
	|| defined(NT99141_VGA) || defined(NT99142_VGA) || defined(OV10633_VGA) || defined(GM7150_TWO_FIELDS)  || defined(TVP5150_TWO_FIELDS)\
	|| defined(HM1246_VGA) ||  defined(HM2056_VGA)
	#define OPT_CROP_WIDTH		640
	#define OPT_CROP_HEIGHT		480
	#define ENCODE_VGA_DIMENSION
#elif defined(OV2640_SVGA) || defined (NT99160_SVGA) || defined (NT99141_SVGA) || defined (NT99142_SVGA) || defined(NT99252_SVGA)
	#define OPT_CROP_WIDTH		800
	#define OPT_CROP_HEIGHT		600	
	#define ENCODE_SVGA_DIMENSION
#elif defined(OV2640_UXGA)  || defined(NT99252_UXGA)
	#define OPT_CROP_WIDTH		1600
	#define OPT_CROP_HEIGHT		1200	
	#define ENCODE_UXGA_DIMENSION
#elif defined(OV9660_SXGA) || defined(HM1246_SXGA)
	#define ENCODE_SXGA_DIMENSION
	#define OPT_CROP_WIDTH		1280
	#define OPT_CROP_HEIGHT		960
#elif defined(OV3642_QXGA)
	#define ENCODE_QXGA_DIMENSION
	#define OPT_CROP_WIDTH		2048
	#define OPT_CROP_HEIGHT		1536
#elif defined(NT99141_VGA)
	#define ENCODE_VGA_DIMENSION
	#define OPT_CROP_WIDTH		640
	#define OPT_CROP_HEIGHT		480
#elif defined(NT99141_HD) || defined(NT99142_HD) || defined(NT99160_HD) || defined(HM1375_HD) ||\
      defined(NT99340_HD) || defined(OV10633_HD) || defined(SC1046_HD) || defined(HM2056_HD) ||\
      defined(HM1246_HD)
	#define ENCODE_HD_DIMENSION
	#define OPT_CROP_WIDTH		1280
	#define OPT_CROP_HEIGHT		720		
#elif defined(NT99340_FULLHD)
	#define ENCODE_FULLHD_DIMENSION
	#define OPT_CROP_WIDTH		1920
	#define OPT_CROP_HEIGHT		1088
#elif defined(NT99340_QXGA)
	#define ENCODE_QXGA_DIMENSION
	#define OPT_CROP_WIDTH		2048
	#define OPT_CROP_HEIGHT		1536	
#elif defined(VPG_QXGA)
	#define ENCODE_QXGA_DIMENSION
	#define OPT_CROP_WIDTH		2048
	#define OPT_CROP_HEIGHT		1536
#else
	#error "Please select one sensor"
#endif 

/* Choice one */

#ifdef ENCODE_QVGA_DIMENSION	
	#define	OPT_ENCODE_WIDTH		320			
	#define	OPT_ENCODE_HEIGHT		240	
#endif
#ifdef ENCODE_HALF_VGA_DIMENSION
	#define	OPT_ENCODE_WIDTH		640			
	#define	OPT_ENCODE_HEIGHT		240	
#endif
#ifdef ENCODE_VGA_DIMENSION
	#define	OPT_ENCODE_WIDTH		640			
	#define	OPT_ENCODE_HEIGHT		480	
#endif
#ifdef ENCODE_SVGA_DIMENSION
	#define	OPT_ENCODE_WIDTH		800			//
	#define	OPT_ENCODE_HEIGHT		600	
#endif
#ifdef ENCODE_SXGA_DIMENSION
	#define	OPT_ENCODE_WIDTH		1280		//1.3M pixel 
	#define	OPT_ENCODE_HEIGHT		960	
#endif
#ifdef ENCODE_UXGA_DIMENSION
	#define	OPT_ENCODE_WIDTH		1600		//2M pixel
	#define	OPT_ENCODE_HEIGHT		1200
#endif
#ifdef ENCODE_FULLHD_DIMENSION
	#define	OPT_ENCODE_WIDTH		1920		//Full HD pixel
	#define	OPT_ENCODE_HEIGHT		1080
#endif
#ifdef ENCODE_QXGA_DIMENSION
	#define	OPT_ENCODE_WIDTH		2048		//3M pixel
	#define	OPT_ENCODE_HEIGHT		1536		
#endif
#ifdef ENCODE_HD_DIMENSION
	#define	OPT_ENCODE_WIDTH		1280		//1280x720 pixel
	#define	OPT_ENCODE_HEIGHT		720					
#endif

#define 	OPT_ENCODE_WIDTH_2		640			//Shared sensor architecture,  Dimension for the 2nd port	
#define 	OPT_ENCODE_HEIGHT_2		480	
	
/* Choice one */
#ifdef __TV__
#define OPT_STRIDE				640
#define OPT_LCM_WIDTH			640
#define OPT_LCM_HEIGHT		480
#define OPT_PREVIEW_WIDTH		640
#define OPT_PREVIEW_HEIGHT		480
#elif defined(__LCM_320x240__) || defined(__LCM_QVGA__)
#define OPT_STRIDE				320
#define OPT_LCM_WIDTH			320
#define OPT_LCM_HEIGHT		240
#define OPT_PREVIEW_WIDTH		320
#define OPT_PREVIEW_HEIGHT		240
#elif defined(__LCM_480x272__)
#define OPT_STRIDE				480
#define OPT_LCM_WIDTH			480
#define OPT_LCM_HEIGHT		272
#define OPT_PREVIEW_WIDTH		364
#define OPT_PREVIEW_HEIGHT		272
#elif defined(__LCM_WVGA__) || defined(__LCM_800x480__)
#define OPT_STRIDE				800
#define OPT_LCM_WIDTH			800
#define OPT_LCM_HEIGHT		480
#define OPT_PREVIEW_WIDTH		640
#define OPT_PREVIEW_HEIGHT		480
#else
	#error "Please select one sensor"
#endif

#if defined(NT99160_VGA) || defined(NT99160_SVGA) || defined(NT99160_HD)
	#define __PCLK_60MHZ__
	//#define __PCLK_48MHZ__	
	
	//#define __50HZ__
	#define __60HZ__
#endif
#if defined(NT99141_VGA) || defined(NT99141_SVGA) || defined(NT99141_HD)
	//#define __PCLK_60MHZ__
	//#define __PCLK_48MHZ__
	#define __PCLK_74MHZ__	
	
	//#define __50HZ__
	#define __60HZ__
#endif
#if defined(NT99142_VGA) || defined(NT99142_SVGA) || defined(NT99142_HD)
	#define __PCLK_60MHZ__
	//#define __PCLK_48MHZ__
	//#define __PCLK_74MHZ__	
	
	//#define __50HZ__
	#define __60HZ__
#endif
#if defined(NT99340_HD) || defined(NT99340_FULLHD) || defined(NT99340_QXGA)
	#define __PCLK_64MHZ__	
	
	//#define __50HZ__
	#define __60HZ__
#endif
#if defined(NT99050_VGA) || defined(GC0308_VGA) 	
	//#define __50HZ__
	#define __60HZ__
#endif
#if defined(TW9912) 
	#define __NTSC__
	//#define __PAL__
#endif 

#if defined(TVP5150_ONE_FIELD) || defined(TVP5150_TWO_FIELDS)
#define CONFIG_PAL_SYSTEM_DEV1
//#define CONFIG_NTSC_SYSTEM_DEV1
#endif

#if defined(NT99050_VGA) || defined(GC0308_VGA) 	
	//#define __50HZ__
	#define __60HZ__
#endif
#if defined(SC1046_HD)
	#define __PCLK_60MHZ__
	//#define __PCLK_48MHZ__
#endif 

#if defined(GM7150_ONE_FIELD) || defined(GM7150_TWO_FIELDS) 	
	#define __NTSC__
	//#define __PAL__
#endif

extern VINDEV_T Vin;
extern VINDEV_T* pVin;

extern VINDEV_T Vin2;
extern VINDEV_T* pVin2;
extern VINDEV_T* vin0;
extern VINDEV_T* vin1;

void VideoIn_InterruptHandler(void);
void MD_Handler(void);
void VideoIn2_InterruptHandler(void);

//GCD.c
UINT8 GCD(UINT16 m1, UINT16 m2);

extern UINT8 u8PacketFrameBuffer[];
extern UINT8 u8PlanarFrameBuffer[];
extern UINT8 u8PlanarFrameBuffer1[];
extern UINT8 u8PlanarFrameBuffer2_0[];
extern UINT8 u8PlanarFrameBuffer2_1[];

UINT32 Smpl_OV9660_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_OV9660_SXGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_OV7670_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_OV7725_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99141_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99141_HD(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99141_VGA_MotionDetection(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99141_DEV1_HD_DEV2_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99050(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99160_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99160_SVGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99160_HD(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_HM1375_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_HM1375_HD(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99340_HD(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99340_FULLHD(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99340_QXGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99252_SVGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99252_UXGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_SP1628_HD(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_TW9912_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_WT8861_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_SA71113_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_GC0308_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_OV10633_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_OV10633_HD(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_SC1046_HD(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99142_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99142_SVGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_NT99142_HD(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_GM7150_OneField(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_GM7150_TwoFields(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_TVP5150_OneField(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_TVP5150_TwoFields(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_TW9900_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_HM2056_HD(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_HM2056_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
void Smpl_HM2056_LightMode(void);
UINT32 Smpl_HM1246_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_HM1246_HD720P(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
UINT32 Smpl_HM1246_SXGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
void Delay(UINT32 nCount);

//Smpl_VPOST.C
void InitVPOST(UINT8* pu8FrameBuffer);
void InitVPOST_2(UINT8* pu8FrameBuffer);
//demo.C
void VideoIn_InterruptHandler(void);
void CoWork_VideoIn0_InterruptHandler(void);
void CoWork_VideoIn0_InterruptHandler(void);
void CoWork_VideoIn_InterruptHandler(void);
void CoWork_VideoIn1_InterruptHandler(void);
UINT32 VideoIn_GetCurrFrameCount(void);
void VideoIn_ClearFrameCount(void);
UINT32 Smpl_OV9660_VGA_SHARE_withMacroBlock(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2);
VOID JpegEncTest (VINDEV_T* pVin, BOOL bIsEncodePacket, UINT32 u32FrameAddr);

//FrameSyn.c
VOID pfnFSC_Ch0_ReadSwitchCallback(void);
VOID pfnFSC_Ch0_WriteSwitchCallback(void);
VOID pfnFSC_Ch0_ReadErrorCallback(void);
VOID pfnFSC_Ch0_WriteErrorCallback(void);

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

INT32 WriteFile(char* szAsciiName, 
					PUINT16 pu16BufAddr, 
					UINT32 u32Length);


typedef struct{
	INT32 (*IQ_GetBrightness)(void);
	INT32 (*IQ_SetBrightness)(INT16);
	INT32 (*IQ_GetSharpness)(void);	
	INT32 (*IQ_SetSharpness)(INT16);	
	INT32 (*IQ_GetContrast)(void);	
	INT32 (*IQ_SetContrast)(INT16);	
	INT32 (*IQ_GetHue)(void);
	INT32 (*IQ_SetHue)(INT16);	
}IQ_S;

#if 0//From Wiki
	1 Video graphics array 
		1.1 QQVGA (160กั120) 
		1.2 HQVGA (240กั160) 
		1.3 QVGA (320กั240) 
		1.4 WQVGA (432กั240) 
		1.5 HVGA (480กั320) 
		1.6 VGA (640กั480) 
		1.7 WVGA (800กั480) 
		1.8 FWVGA (854กั480) 
		1.9 SVGA (800กั600) 
		1.10 WSVGA (1024กั576/600) 

	2 Extended graphics array 
		2.1 XGA (1024กั768) 
		2.2 WXGA (1280กั768) 
		2.3 XGA+ (1152กั864) 
		2.4 WXGA+ (1440กั900) 
		2.5 SXGA (1280กั1024) 
		2.6 SXGA+ (1400กั1050) 
		2.7 WSXGA+ (1680กั1050) 
		2.8 UXGA (1600กั1200) 
		2.9 WUXGA (1920กั1200) 
	3 Quad-extended graphics array 
		3.1 QWXGA (2048กั1152) 
		3.2 QXGA (2048กั1536) 
		3.3 WQXGA (2560กั1600) 
		3.4 QSXGA (2560กั2048) 
		3.5 WQSXGA (3200กั2048) 
		3.6 QUXGA (3200กั2400) 
		3.7 WQUXGA (3840กั2400) 
	4 Hyper-extended graphics array 
		4.1 HXGA (4096กั3072) 
		4.2 WHXGA (5120กั3200) 
		4.3 HSXGA (5120กั4096) 
		4.4 WHSXGA (6400กั4096) 
		4.5 HUXGA (6400กั4800) 
		4.6 WHUXGA (7680กั4800) 
	5 Multiples of 720 and 1080 
		5.1 nHD (640กั360) 
		5.2 qHD (960กั540) 
		5.3 WQHD (2560กั1440) 
		5.4 QFHD (3840กั2160) 
	#endif

#endif /* !_VIDEOIN_DEMO */
