#include <stdio.h>
#include <string.h>
#include "h264.h"
#include "wblib.h"
#include "w55fa92_reg.h"  
#include "h264_reg.h"

#include "AVCdec.h"
#include "h264api.h"
#include "decoder.h"
#include "userdef.h"


#define __MAX_INSTANCE_NUM__  4

//#define	MAX_DEFAULT_WIDTH	1280
//#define MAX_DEFAULT_HEIGHT	720

static int InstanceNum=0;
CodecInst codecInstPool[__MAX_INSTANCE_NUM__];

#define ERR_DEC_OVER_MAX_INSTANCE   -3
#define ERR_DEC_NO_EXIST_INSTANCE   -4

static _H264D_Wptr;
static 	_H264_Wptr=0;
FAVC_DEC_RESULT _tResult;
static DECODER _tAVCDecHandle;	
DECODER *_pAVCDecHandle;	

extern unsigned int h264_max_width;
extern unsigned int h264_max_height;

extern volatile int BSM_empty_flag;

int H264DecInit(void* ptDecHandle)
{
	_pAVCDecHandle = &_tAVCDecHandle;
#if 0	
    return AVCDec_Init(_pAVCDecHandle);
#else
	return 0;
#endif    
    
}


int SeekPSC(void* ptDecHandle, unsigned char* nal)
{
	UINT8 read_data;
	
    DECODER * dec = (DECODER *)ptDecHandle;
    
    while((inp32(REG_264_STREAM_STATUS) & RBSP_Q_FULL) ==0);
    
	outp32(REG_264_ADDR_BSM_CTL,SYS_BSM_NSC <<8);
	
	do {
		if (inp32(REG_264_STREAM_STATUS) & BSB_EMPTY)
		{
			return -1;
		}
	}while((inp32(REG_264_ADDR_STATUS0) & PARSER_IDLE) ==0);
	
	read_data=read_u(8);
	
    dec->forbidden_zero_bit = read_data&0x00000080;
    dec->nal_ref_idc   = read_data&0x00000060;
    dec->disposable_flag = (dec->nal_ref_idc == 0); //NALU_PRIORITY_DISPOSABLE
    dec->nal_unit_type = read_data&0x0000001F;	
	
	*nal = dec->nal_unit_type;
	return 0;
}


int H264DecOpen(void *handle, DecOpenParam *decOP)
{
	CodecInst * pCodecInst;
	
	pCodecInst = &codecInstPool[0];

    
    _pAVCDecHandle->pu8BS_start_phy = (uint8_t *)decOP->bitstreamBuffer;
	_pAVCDecHandle->pu8BS_start_virt = _pAVCDecHandle->pu8BS_start_phy;
	_pAVCDecHandle->u32BS_buf_sz = decOP->bitstreamBufferSize;
    
#if 1	//Reset IP
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) | VDE_RST);
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) & ~VDE_RST);		
#else    
	outp32(REG_264_SOFT_RST, 1); 
#endif	
    outp32(REG_264_BSM_BASE, decOP->bitstreamBuffer);
    outp32(REG_264_BSM_BUF_LENGTH, decOP->bitstreamBufferSize/64);		// unit: 16 word = 64 byte
    _H264_Wptr = 0;
    _H264D_Wptr = 0;
    
    BSM_empty_flag=0;
    
    return 0;    
}

int H264DecClose(DecHandle *handle)
{
    DECODER *dec = (DECODER *)_pAVCDecHandle;

    InstanceNum--;  
    
    if (InstanceNum == 0)
    {
    // close clock here if no instance open here
    
    // free allocated memory        
    }
    
    AVCDec_Release(dec);
          
    
    return 0;
}

int H264DecGetInitialInfo(void* ptAVCDecHandle, DecInitialInfo *info, DecConfigParam* decConfig)
{
    DECODER * dec;
    AVC_RET ret;
    UINT8 NAL_value;
	int 			Y_stride,UV_stride=0,video_width,video_height; 
	struct_sps *sps;	   
    
//    dec = (DECODER *)ptAVCDecHandle;		
    dec = (DECODER *)_pAVCDecHandle;		  
    
    // search NAL type
    // Search SPS 
    do {
	    SeekPSC(dec,&NAL_value);
    } while (NAL_value != 7);
    //if (SeekPSC(dec,&NAL_value) <0)
    //	return -1;
    
    // Parsing SPS here to return image width, heght and so on here
   	if((ret = read_nal_sps(dec)) != RETCODE_OK)
		   return ret;
		   
	if (decConfig->OutputStrideEnable)
	{
		sps = dec->sps[dec->active_sps_idx];
		video_width = sps->pic_width_in_mbs <<4; //*16;
		video_height = sps->pic_height_in_map_units <<4; //*16;	
		
		outp32(REG_264_MISC,inp32(REG_264_MISC)|LCD_MODE_LARGE);
		if (decConfig->outputFmt == YUV420)
		{
			Y_stride = (decConfig->OutputStride/16 - video_width/16) * 4 + 1;
			UV_stride = (decConfig->OutputStride/16 - video_width/16) * 2 + 1;			
		}
		else
		{
			Y_stride = (decConfig->OutputStride/16 - video_width/16) * 8 + 1;		
		}
		
		outp32(REG_264_LCD_FRAME_LINE_OFF,(UV_stride<< 16) |Y_stride );		
	}
	else
	{
		outp32(REG_264_MISC,inp32(REG_264_MISC)&~LCD_MODE_LARGE);	
	}	
			   
    // memory issue, should be done only once a sequence
    if ((ret = init_parameters(dec)) != RETCODE_OK)
           return ret;
#ifdef _RESET_IP_	           
    dec->register_FRAMESIZE      = inp32(REG_264_FRAMESIZE);
    //dec->register_MBINFO_BASE    = inp32(REG_264_MBINFO_BASE);
    //dec->register_INTRA_BASE     = inp32(REG_264_INTRA_BASE);
    dec->register_OFFSET_REG0    = inp32(REG_264_OFFSET_REG0);
    dec->register_OFFSET_REG1    = inp32(REG_264_OFFSET_REG1);
    dec->register_RECON_LINEOFFS = inp32(REG_264_RECON_LINEOFFS);
    dec->register_DP_LINEOFFS    = inp32(REG_264_DP_LINEOFFS);	              
    dec->register_LCD_PARAM      = inp32(REG_264_LCD_PARAM);
    dec->register_LCD_FRAME_LINE_OFF = inp32(REG_264_LCD_FRAME_LINE_OFF);
    dec->register_MISC           = inp32(REG_264_MISC);
    dec->register_RECON_BASE     = inp32(REG_264_RECON_BASE);
    
    dec->pu8BS_start_phy 		 =  (UINT8 *)inp32(REG_264_BSM_BASE);
    dec->u32BS_buf_sz 			 =  (inp32(REG_264_BSM_BUF_LENGTH) << 6);
    
    
    dec->reinit_very_first_time = 0;	
#endif    

    info->picWidth = dec->video_width;
    info->picHeight = dec->video_height;    
    info->minFrameBufferCount = dec->sps[dec->active_sps_idx]->num_ref_frames+2;	// Another 1 is for Reconsturct frame buffer and Macro block + Intra data base
    return 0;
}

int H264DecRegisterFrameBuffer(void* pHandle, FrameBuffer *pBuffer, int num, int stride)
{
//    DECODER* dec = (DECODER*)pHandle;	
    DECODER* dec = (DECODER*)_pAVCDecHandle;	
	int i, halfsize;
	StorablePicture **ref_pic_data = dec->ref_pic_data;
	StorablePicture *tmp_pic;	
	struct_sps *sps = dec->sps[dec->active_sps_idx];
	decode_parameter     *decode_para = dec->decode_para;			
	
	if	(pBuffer == NULL)
	{
		Console_Printf("Wrong frame buffer register\n");
		return -1;
	}
	
	//-------------------------------------
	// The required frame buffer includes 
	// 1. Reconstruct frame 
	// 2. Intra data buffer 
	// 3. Macro block information 
	// 4. Output frame
	// 5. reference frame
	//-------------------------------------
	
	// Output frame
	
	for(i=0;i<sps->num_ref_frames+1;i++)	// last one for reconstruct frame buffer
	{
		tmp_pic = *(ref_pic_data+i);
        tmp_pic->L0_ref_idx_yaddr_phy = (uint8_t *)pBuffer[i].bufY;        
	}
	
	

	
	// Intra data @ last frame
	outp32(REG_264_INTRA_BASE, pBuffer[num-1].bufY);
	decode_para->INTRA_PRED_BASE = inp32(REG_264_INTRA_BASE);
#ifdef _RESET_IP_	
	dec->register_INTRA_BASE     = inp32(REG_264_INTRA_BASE);
#endif
	
	// Macro block info
	halfsize = DIV_16((h264_max_width >> 4) * 8 * 4);
	outp32(REG_264_MBINFO_BASE, pBuffer[num-1].bufY + halfsize );	
	decode_para->MB_INFO_BASE = inp32(REG_264_MBINFO_BASE);
#ifdef _RESET_IP_	
	dec->register_MBINFO_BASE    = inp32(REG_264_MBINFO_BASE);
#endif	
    return 0;
}

//-----------------------------------
// pRdptr : Buffer Read pointer
// pWrptr : Buffer Writer pointer
// *size  : Free buffer size
//-------------------------------------
int H264DecGetBitstreamBuffer(void* pHandle, PhysicalAddress *pWrptr, Uint32 *size)
{
    DECODER* dec = (DECODER*)_pAVCDecHandle;	
		
	*size = AVCDec_QueryEmptyBuffer(dec);
	
	*pWrptr = (PhysicalAddress)(dec->pu8BS_start_phy + dec->u32BS_sw_offset);
	
	return 0;
}


int H264DecUpdateBitstreamBuffer(void* pHandle, unsigned int size)
{
    DECODER* dec = (DECODER*)_pAVCDecHandle;
	if (size ==0)
	{
		AVCDec_UpdateInform(dec,size, 1);
		decoder_fill_bs_reg(dec,size);	// 1121_added
	}
	else
	{
		if (size % 64)
		{
			AVCDec_UpdateInform(dec,size, 1);	
			decoder_fill_bs_reg(dec,(size+63)/64*64);		
		}
		else
		{
			AVCDec_UpdateInform(dec,size, 0);	
			decoder_fill_bs_reg(dec,size);
		}	
	}
	return 0;
}


int H264DecStartOneFrame(DecHandle pHandle, FAVC_DEC_RESULT *ptResult)
{
    DECODER* dec = (DECODER*)_pAVCDecHandle;
    int result = 0;
	
	decoder_reinit(dec);
	result = decoder_decode(dec,ptResult);
	
	_tResult = *ptResult;
	
	return result;
}

int H264DecGetOutputInfo(void* pHandle, DecOutputInfo *pParam)
{
    DECODER* dec = (DECODER*)_pAVCDecHandle;	
        
    pParam->indexFrameDisplay = dec->framepoc;
    pParam->indexFrameDecoded = dec->ssh->frame_num;
#ifdef DISPLAY_REORDER_CTRL    
    pParam->isDisplayOut = _tResult.isDisplayOut;
#else
    pParam->isDisplayOut = 1;
#endif    
    pParam->picType = isISlice(_pAVCDecHandle);
	return 0;
}


