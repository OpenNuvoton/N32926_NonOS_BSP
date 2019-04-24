#include <stdio.h>
#include <string.h>
#include "h264.h"
#include "wblib.h"
#include "w55fa92_reg.h"  
#include "h264_reg.h"

#include "AVCdec.h"

#include "h264api.h"
#include "encoder.h"
//#include "test.h"
#include "userdef.h"

#define __MAX_INSTANCE_NUM__  4

//#define	MAX_DEFAULT_WIDTH	1280
//#define MAX_DEFAULT_HEIGHT	720

//static int InstanceNum=0;
//static int InstanceArray[__MAX_INSTANCE_NUM__] = {0, 0, 0, 0};
//CodecInst codecInstPool[__MAX_INSTANCE_NUM__];

#define ERR_DEC_OVER_MAX_INSTANCE   -3
#define ERR_DEC_NO_EXIST_INSTANCE   -4

//static _H264D_Wptr;
//static 	_H264_Wptr=0;
//static FAVC_DEC_RESULT _tResult;
//static h264_encoder _tAVCEncHandle;	
h264_encoder *_pAVCEncHandle;	

extern struct enc_private      enc_data[];


int H264EncInit(void* handle)
{
	//_pAVCEncHandle = &_tAVCEncHandle;
	
	favc_encoder_open();
	
	
//    h264_encoder_init(&_pAVCEncHandle->mEncParam);

    
    return 0;
    
}



int H264EncOpen(void* handle, FAVC_ENC_PARAM * param)
{
    int ret=0;
    
//    if (h264e_init(param, handle) < 0) {

    if (h264e_init(param, 0) < 0) {    
       printk("FAVC_ENCODE_INIT error\n");
        ret = -1;
        return ret;
    }	
    
    _pAVCEncHandle = enc_data[0].enc_handle;
    
    return 0;    
}

int H264EncClose(DecHandle *handle)
{

          
    favc_encoder_release();
    return 0;
}




//-----------------------------------
// pRdptr : Buffer Read pointer
// pWrptr : Buffer Writer pointer
// *size  : Free buffer size
//-------------------------------------
int H264EncGetBitstreamBuffer(void* pHandle, PhysicalAddress *pWrptr, Uint32 *size)
{
	/*
//	DECODER * dec = (DECODER *)pHandle;
    DECODER* dec = (DECODER*)_pAVCDecHandle;	
		
	*size = AVCDec_QueryEmptyBuffer(dec);
	
	*pWrptr = (PhysicalAddress)(dec->pu8BS_start_phy + dec->u32BS_sw_offset);

	*/
	return 0;
}



//int H264EncStartOneFrame(DecHandle pHandle, FAVC_DEC_RESULT *ptResult, int* BSMsize)
int H264EncStartOneFrame(DecHandle pHandle, int newQuant, int* BSMsize, int* keyframe)
{
    h264_encoder*  pEnc = (h264_encoder *)enc_data[0].enc_handle;
    FAVC_ENC_PARAM* pEncParam = (FAVC_ENC_PARAM *)&pEnc->mEncParam;
    int ret = 0,i,mslice_len;
    int dev=0;     
    unsigned int YFrameBase, UFrameBase, VFrameBase;     
#if 1

	pEncParam->u32Quant = newQuant;

	pEncParam->bitstream_size = 0;
	
        //check Qp value
        if(pEncParam->u32Quant > MaxQp || (int)pEncParam->u32Quant < MinQp){
            printk("[Error]: Qp Error Qp:%d MaxQp:%d  MinQp:%d  \n",pEncParam->u32Quant,pEncParam->u32MaxQuant,pEncParam->u32MinQuant);
            ret=-EFAULT;
            goto encoder_ioctl_exit;    	            
        }
        
        enc_data[dev].mslice_first_mb=0;       // Don't forget reset mslice_first_mb to 0, it will influence pEnc->frame_num 		
        enc_data[dev].mslice_stride_yuv=0;   // Don't forget reset mslice_stride_yuv to 0, it will influence Y,U,V or UV base        
        
        YFrameBase = (UINT32)pEncParam->pu8YFrameBaseAddr;
        UFrameBase = (UINT32)pEncParam->pu8UFrameBaseAddr;
        VFrameBase = (UINT32)pEncParam->pu8VFrameBaseAddr;        

        if (h264_fmt(enc_data[dev].enc_handle) == 1) { // mp4-2D
            int distance_uv = -15 + (((int)VFrameBase - (int)UFrameBase))/4;
            if ((distance_uv >= 0x8000) || (distance_uv < -0x8000)) {
                printk ("[Error] Distance between U & V base address is too large (0x%08x, 0x%08x) at MP4-2D mode\n",
                  (int)pEncParam->pu8UFrameBaseAddr, (int)pEncParam->pu8VFrameBaseAddr);
                ret = -EFAULT;
                goto encoder_ioctl_exit;
            }
        }

        pEncParam->multi_slice = enc_data[dev].mslice_enable;        

	//h264_encoder_encode();
	       for ( i = 0; i<enc_data[dev].mslice_num; i++ ) {  
           // ROI parameter update
           // Multi-slice is not support dynamic ROI
           if ( enc_data[dev].mslice_num > 1 ) {
               pEncParam->u32ROIX = enc_data[dev].slicebuf[i].mslice_ROIX;
               pEncParam->u32ROIY = enc_data[dev].slicebuf[i].mslice_ROIY;
           } else {
               if ( pEncParam->bROIEnable == 0 ) {
                   pEncParam->u32ROIX = enc_data[dev].slicebuf[i].mslice_ROIX;
                   pEncParam->u32ROIY = enc_data[dev].slicebuf[i].mslice_ROIY;
               }
           }
           pEncParam->u32ROIWidth = enc_data[dev].slicebuf[i].mslice_ROIWidth;
           pEncParam->u32ROIHeight = enc_data[dev].slicebuf[i].mslice_ROIHeight;
           pEncParam->bROIEnable = enc_data[dev].slicebuf[i].mslice_ROI;
           // MultiSlice reference memory buffer update		  				  
           pEncParam->u32FrameWidth= enc_data[dev].slicebuf[i].mslice_width;
           pEncParam->u32FrameHeight = enc_data[dev].slicebuf[i].mslice_height;	
           pEncParam->pu8ReConstructFrame=(unsigned char *)enc_data[dev].slicebuf[i].recon_phy_buffer;
           pEncParam->pu8ReferenceFrame=(unsigned char *)enc_data[dev].slicebuf[i].refer_phy_buffer;
           pEncParam->pu8SysInfoBuffer=(unsigned char *)enc_data[dev].slicebuf[i].sysinfo_phy_buffer;
           pEncParam->pu8DMABuffer_phy=(unsigned char *)enc_data[dev].slicebuf[i].dma_phy_buffer;
//           tEncParam.pu8DMABuffer_virt=(unsigned char *)enc_data[dev].slicebuf[i].dma_virt_buffer;          
           // Multislice Y,U,V or UV base update
           switch (h264_fmt(enc_data[dev].enc_handle)) {
           case 3: // uyvy-1D
               pEncParam->pu8YFrameBaseAddr=(unsigned char *)YFrameBase + enc_data[dev].mslice_stride_yuv*SIZE_Y * 2;
               break;

           case 0: // h264-2D
               pEncParam->pu8YFrameBaseAddr = (unsigned char *)YFrameBase + enc_data[dev].mslice_stride_yuv*SIZE_Y ;
               pEncParam->pu8UVFrameBaseAddr = (unsigned char *)UFrameBase + enc_data[dev].mslice_stride_yuv*( SIZE_U + SIZE_V);	  
               break;

           default: // mp4-2D, 420YUV-1D
               pEncParam->pu8YFrameBaseAddr = (unsigned char *)YFrameBase + enc_data[dev].mslice_stride_yuv*SIZE_Y ;
               pEncParam->pu8UFrameBaseAddr = (unsigned char *)UFrameBase + enc_data[dev].mslice_stride_yuv*SIZE_U ;
               pEncParam->pu8VFrameBaseAddr = (unsigned char *)VFrameBase + enc_data[dev].mslice_stride_yuv*SIZE_V ;
               break;
           }

           mslice_len = 0;  // Don't forget reset mslice_len to 0  
           if ( enc_data[dev].roi_enable == 1) {
               //printk("\ndrv: %x %x %x %x\n", enc_data[dev].slicebuf[i].mslice_ROI,
               //      enc_data[dev].slicebuf[i].mslice_ROIWidth,
               //      enc_data[dev].slicebuf[i].mslice_ROIHeight,
               //      enc_data[dev].slicebuf[i].mslice_ROIMB);
               if ( (enc_data[dev].slicebuf[i].mslice_ROI ==1) 
                   && (	enc_data[dev].slicebuf[i].mslice_ROIWidth != 0 )
                   && ( enc_data[dev].slicebuf[i].mslice_ROIHeight != 0)
                   && ( 	enc_data[dev].slicebuf[i].mslice_ROIMB != 0) ) {	
                   mslice_len = h264_encoder_encode( enc_data[dev].enc_handle,
                                 pEncParam, 
                                 enc_data[dev].mslice_first_mb, 
                                 enc_data[dev].slicebuf[i].mslice_ROILast, 
                                 i);
                }
            } else {
                mslice_len = h264_encoder_encode(enc_data[dev].enc_handle,
                                 pEncParam,
                                 enc_data[dev].mslice_first_mb, 
                                 enc_data[dev].slicebuf[i].mslice_last, 
                                 i);
            }

            if ( mslice_len < 0) {
                ret = -EFAULT;
                goto encoder_ioctl_exit;
            }
            else {
#ifdef EVALUATION_PERFORMANCE
                get_cpy_start();
#endif
//                if (copy_to_user((void *)(pEncParam->bitstream + pEncParam->bitstream_size),(void *)out_virt_buffer, mslice_len))
//                {
//                    printk("Error copy_to_user() of FAVC_IOCTL_ENCODE_FRAME: out_virt_buffer\n");
//                    ret = -EFAULT;
//                    goto encoder_ioctl_exit;
//                }

#ifdef EVALUATION_PERFORMANCE
                get_cpy_stop();
#endif
                pEncParam->bitstream_size += mslice_len;
            }
            
            *BSMsize = pEncParam->bitstream_size;
            *keyframe = pEncParam->keyframe;

            //printk("STRIDE_YUV %d\n", enc_data[dev].mslice_stride_yuv);
            enc_data[dev].mslice_stride_yuv += enc_data[dev].slicebuf[i].mslice_mb; // update mslice_stride_yuv						 
	     	  
            if ( enc_data[dev].roi_enable == 1) { // update mb skip number
                enc_data[dev].mslice_first_mb += enc_data[dev].slicebuf[i].mslice_ROIMB;
            } else {
                enc_data[dev].mslice_first_mb += enc_data[dev].slicebuf[i].mslice_mb;
            }

            //printk("skip mb run %d stride %d\n", enc_data[dev].mslice_first_mb, enc_data[dev].mslice_stride_yuv);
        } //for

        //toggle DMA buffer
        for ( i = 0; i<enc_data[dev].mslice_num; i++ ) { 
            h264_toggle_DMA_buffer(enc_data[dev].enc_handle,i);
        }
#endif	

encoder_ioctl_exit:
	return 0;
}

int H264EncGetBSLength(int* bs_length)
{
	*bs_length = inp32(REG_264_STS1);
	outp32(REG_264_CMD1, C1_CLEAR_BITSTREAM_LEN);
	//CLEAR_BITSTREAM_LENGTH();
	
	//Console_Printf("Encode BS length = %d\n",*bs_length);
	
	return 0;

}

