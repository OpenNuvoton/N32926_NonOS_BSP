#ifndef _FAVCAVCODEC_H_
#define _FAVCAVCODEC_H_

#include "port.h"
#include "wbtypes.h"


#define P_SLICE 0x00
#define B_SLICE 0x01
#define I_SLICE 0x02

// Constant value, don't change

#define OUTPUT_FMT_YUV420	0
#define OUTPUT_FMT_YUV422	1

//------------------------------

	// ioctl flag, should be consistent with driver definition
#define FAVC_IOCTL_DECODE_INIT    	0x4170
#define FAVC_IOCTL_DECODE_FRAME   	0x4172
	
#define FAVC_IOCTL_ENCODE_INIT    	0x4173
#define FAVC_IOCTL_ENCODE_FRAME   	0x4175
#define FAVC_IOCTL_GET_SPSPPS     	0x4179
	
	
/*****************************************************************************
 * Decoder structures
 ****************************************************************************/
typedef struct
{
	UINT32 bEndOfDec;					// Used by library only
	UINT32 u32Width;					// Decoded bitstream width
	UINT32 u32Height;					// Decoded bitstream height
	UINT32 u32UsedBytes;				// Reported used bitstream byte in buffer. It is not accurate. The inaccuracy is less than 256 bytes
	UINT32 u32FrameNum;					// Decoded frame number
	UINT32 isDisplayOut;				// 0 -> Buffer in reorder buffer, 1 -> available buffer, -1 -> last flush frame
	UINT32 isISlice;					// 1-> I Slice, 0 -> P slice
	UINT32 Reserved0;
} FAVC_DEC_RESULT; 
 
typedef struct
{
	UINT32 u32API_version;				// API version
	UINT32 u32MaxWidth;					// Not used now
	UINT32 u32MaxHeight;				// Not used now
	UINT32 u32FrameBufferWidth;			// if (u32FrameBufferWidth != -1), decoded image width is cropped with u32FrameBufferWidth
										// if (u32FrameBufferWidth == -1), decoded image width is continued on memory
	UINT32 u32FrameBufferHeight;		// if (u32FrameBufferHeight != -1), decoded image height is cropped with u32FrameBufferHeight
										// if (u32FrameBufferHeight == -1), decoded image height is continued on memory
	UINT32 u32Pkt_size;					// Current decoding bitstream length ( the exact bitstream length for one frame)
	UINT8  *pu8Pkt_buf;					// Current decoding bitstream buffer address (application ready bitstream here)
	UINT32 pu8Display_addr[3];			// Buffer address for decoded data
	UINT32 got_picture;					// 0 -> Decoding has someting error. 1 -> decoding is OK in current bitstream
	UINT8  *pu8BitStream_phy;			// physical address. buffer for bitstream (allocated and used by library only)
	UINT32 u32OutputFmt;				// Decoded output format, 0-> Planar YUV420 format, 1-> Packet YUV422 foramt
	UINT32 crop_x;						// pixel unit: crop x start point at decoded-frame (not supported now)
	UINT32 crop_y;						// pixel unit: crop y start point at decoded-frame (not supported now)
	
	FAVC_DEC_RESULT tResult;			// Return decoding result by library
		
} FAVC_DEC_PARAM; 
 
 	/* API return values */
typedef enum {
	RETCODE_OK = 0, 					// Operation succeed.
	RETCODE_ERR_MEMORY = 1,				// Operation failed (out of memory).
	RETCODE_ERR_API = 2,				// Operation failed (API version error).
	RETCODE_ERR_HEADER = 3,
	RETCODE_ERR_FILL_BUFFER = 4,
	RETCODE_ERR_FILE_OPEN = 5,
	RETCODE_HEADER_READY =6,
	RETCODE_BS_EMPTY =7,
	RETCODE_WAITING =8,
	RETCODE_DEC_OVERFLOW=9,
	RETCODE_HEADER_FINISH=10,
	RETCODE_DEC_TIMEOUT=11,
	RETCODE_PARSING_TIMEOUT=12,
	RETCODE_ERR_GENERAL=13,
	RETCODE_NOT_SUPPORT=14,
										// Below is added but not used
	RETCODE_FAILURE=15,
	RETCODE_FRAME_NOT_COMPLETE=16	
} AVC_RET;
 

/*****************************************************************************
 * H.264 Encoder structures
 ****************************************************************************/

typedef struct {
   	UINT32 u32API_version;
	UINT32 u32BitRate;					// The encoded bitrate in bps.
  	UINT32 u32FrameWidth; 				// The width of encoded frame in pels.
  	UINT32 u32FrameHeight; 				// The height of encoded frame in pels.
  	UINT32 fFrameRate; 					// The base frame rate per second
  	UINT32 u32IPInterval; 				// The frame interval between I-frames.
  	UINT32 u32MaxQuant;					// The maximum quantization value. (max = 51)
  	UINT32 u32MinQuant;					// The minimum quantization value. (min=0)
  	UINT32 u32Quant; 					// The frame quantization value for initialization 
  		
    INT32 ssp_output;   				// This variable tells the H.264 must be encoded out sps + pps before slice data.
                                		// ->  1 : force the encoder to output sps+pps  
                                		// ->  0 : force the encoder to output sps+pps on any Slice I frame
                                		// -> -1: (default) only output SPS+PPS on first IDR frame.
    INT32 intra; 						// This variable tells the H.264 must be encoded out an I-Slice type frame.
                                		// ->  1 : forces the encoder  to create a keyframe.
                                		// ->  0 :  forces the  encoder not to  create a keyframe.
                                		// -> -1: (default) let   the  encoder   decide  (based   on   contents  and u32IPInterval)  		

  /**********************************************************************************************/
  // The additional parameters for Region Of Interest feature (Don't enable ROI)
  /**********************************************************************************************/
  	INT32 bROIEnable;  					// To enable the function of encoding rectangular region of interest(ROI) within captured frame. (0 : Disable, 1: Enable)
  	UINT32 u32ROIX;  					// The upper-left corner x coordinate of rectangular region of interest within captured frame if bROIEnable is enabled.
  	UINT32 u32ROIY;  					// The upper-left corner coordinate y of region of interest within captured frame if bROIEnable is enabled
  	UINT32 u32ROIWidth;  				// The width of user-defined rectangular region of interest within the captured frame in pixel units if bROIEnable is enabled
  	UINT32 u32ROIHeight; 				// The height of user-defined rectangular region of interest within the captured frame in pixel units if bROIEnable is enabled
  /**********************************************************************************************/
  // Buffer allocated by Application. 
  /**********************************************************************************************/  
  	UINT8 *pu8YFrameBaseAddr;  			// The base address for input Y frame buffer. (8-byte aligned)
  	UINT8 *pu8UVFrameBaseAddr; 			// The base address for input UV frame buffer in H.264 2D mode.(8-byte aligned)
  	UINT8 *pu8UFrameBaseAddr;  			// The base address for input U frame buffer.(8-byte aligned) (pu8UVFrameBaseAddr must be equal to pu8UFrameBaseAddr)
  	UINT8 *pu8VFrameBaseAddr;  			// The base address for input V frame buffer.(8-byte aligned)
  		
  	void *bitstream;					// Bitstream Buffer address for driver to write bitstream  (allocated by application)
  /**********************************************************************************************/
  // Below filed is updated by driver to application.
  /**********************************************************************************************/   		
 	UINT8 *pu8BitstreamAddr; 			// The bitstream buffer address while encoding one single frame allocated by library (16-byte aligned)
   
  	UINT32  bitstream_size;				// Bitstream length for current frame (Updated by driver)
  	INT32 keyframe;  					// This parameter is indicated the Slice type of frame. (Updated by Driver, 1-> I slice, 0-> P slice) 
  	INT32 frame_cost;  					// frame_cout is updated by driver.  		
  /**********************************************************************************************/
  // Below filed is used by driver internally. Application doesn' need to care it.
  /**********************************************************************************************/  		
  	UINT32 no_frames; 					// The number of frames to be encoded (Not used now)
	UINT32 threshold_disable; 			// The transform coefficients threshold.
  	UINT32 chroma_threshold;  			// The chroma coefficients threshold (0 ~ 7).
  	UINT32 luma_threshold;    			// The luma coefficients threshold (0 ~ 7).
  	UINT32 beta_offset;       			// The beta offset for in-loop filter.
  	UINT32 alpha_offset;      			// The alpha offset for in-loop filter.
  	UINT32 chroma_qp_offset;  			// The chroma qp offset (-12 to 12 inclusively).
  	UINT32 disable_ilf;       			// To disable in-loop filter or not
  	UINT32 watermark_enable;     		// To enable watermark function or not (Don't enable it now)
  	UINT32 watermark_interval;    		// To specify the watermark interval if watermark function is enabled 
  	UINT32 watermark_init_pattern;		// To specify the initial watermark pattern if watermark function is enabled   		
  
  	UINT8 *pu8ReConstructFrame;			// The address of reconstruct frame buffer. (16-byte aligned)(size = u32FrameWidth * @ref u32FrameHeight * 3/2)
  	UINT8 *pu8ReferenceFrame;  			// The address of reference frame buffer.(16-byte aligned)(size = u32FrameWidth * @ref u32FrameHeight * 3/2)
  	UINT8 *pu8SysInfoBuffer; 			// The address of system info buffer.(4-byte aligned)(size = MBs_Count_Width+1)/2) *64 *2)
//  	UINT8 *pu8DMABuffer_virt; 			// The virtual address of DMA buffer, which size is equal to ((4*19+2)*sizeof(unsigned int));(4-byte aligned)
  	UINT8 *pu8DMABuffer_phy;  			// The physical address of DMA buffer, which size is equal to ((4*19+2)*sizeof(unsigned int));(4-byte aligned)
    INT32 nvop_ioctl; 					// This parameter is valid only on FAVC_IOCTL_ENCODE_NVOP. (Not implemented)
   	UINT32 multi_slice;
    UINT32 pic_height; 					// This parameter is used to keep the frame height for sps and pps on Multi Slice mode
    UINT32 pic_width;	 				// This parameter is used to keep the frame width for sps and pps on Multi Slice mode
	UINT32 img_fmt;						// 0: 2D format, CbCr interleave, named H264_2D (VideoIn supported only)
	UINT32 control;						// 0 : Do NOT force one frame as one slice(default), 1 : Force one frame as one slice
} FAVC_ENC_PARAM;

typedef struct {
	UINT32  video_format;
	UINT8 	colour_description_present_flag;
	UINT32  colour_primaries;
	UINT32  transfer_characteristics;
	UINT32  matrix_coefficients;
	UINT8 	chroma_location_info_present_flag;
	UINT32  chroma_sample_loc_type_top_field;
	UINT32  chroma_sample_loc_type_bottom_field;
}FAVC_VUI_PARAM ;

typedef struct {
	UINT32 left_offset;
	UINT32 right_offset;
	UINT32 top_offset;
	UINT32 buttom_offset;
}FAVC_CROP_PARAM;

typedef enum {
				eDRVH264_DEC_SLICE_COMPLETE = 0,		
				eDRVH264_DEC_FRAME_COMPLETE
} E_DRVH264_INT;

typedef int (*PFN_DRVH264_INT_CALLBACK)(void);

extern int H264InstallCallBack(
	E_DRVH264_INT eIntSource,
	PFN_DRVH264_INT_CALLBACK	pfnCallback,
	PFN_DRVH264_INT_CALLBACK 	*pfnOldCallback
);

extern int H264Dec_Open(void);
extern int H264Enc_Open(void);
extern int H264_ioctl(int cmd, void* param);
extern int H264_ioctl_ex(int handle, int cmd, void* param);
extern void H264Enc_Close(void);
extern void H264Enc_Close_ex(int handle);
extern void H264Dec_Close(void);
extern void H264Dec_Close_ex(int handle);
extern void* nv_malloc(INT32 size, INT32 alignment);
extern int nv_free(void* ptr);

#endif
