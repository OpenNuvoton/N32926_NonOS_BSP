/********************************************/
/* Faraday avc decoder driver operations    */
/********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "w55fa92_reg.h"
#include "h264dec.h"
#include "AVCdec.h"
#include "decoder.h"

#include "favc_version.h"
#include "encoder.h"

#include "wblib.h"
#include "decoder.h"


extern unsigned int    h264_max_width,h264_max_height;
extern unsigned int    dec_bs_phy_buffer;
extern unsigned int    mb_info_phy_buffer;
extern unsigned int    intra_pred_phy_buffer; 

struct dec_private      dec_data[MAX_DEC_NUM]={0,0,0,0};

extern int favc_check_continued(unsigned int addr, int size);
extern unsigned int favc_user_va_to_pa(unsigned int addr);

static FAVC_DEC_RESULT     _tDecResult[MAX_DEC_NUM];

int favc_decoder_open(void)
{
    return 0; /* success */
}

int favc_decoder_OneFrame(void *ptDecHandle, FAVC_DEC_RESULT *result)
{
	int ret = 0;
 retry:
	ret = AVCDec_OneFrame(ptDecHandle, result);

	if(ret == RETCODE_HEADER_READY)
		goto retry;

		
	return ret;
}

int favc_decoder_ioctl(void *handle, unsigned int cmd, void *arg)
{
	int					dev=0;
    int                 ret = 0;
    int idx;
    FAVC_DEC_PARAM      tDecParam;
//    FAVC_DEC_RESULT     tDecResult;

    idx = *(int *)handle;
	
    if ((dec_data[idx].idx_ex->signature == SIGNATURE) && (dec_data[idx].idx_ex->decoder_idx == idx))
    {
    	dev=idx;
    }
    else
    	dev=0;
	

    switch(cmd){
        case FAVC_IOCTL_DECODE_INIT:
            memcpy((void *)&tDecParam, (void *)arg, sizeof(tDecParam));
           
            dec_data[dev].frame_width=tDecParam.u32FrameBufferWidth;
            dec_data[dev].frame_height=tDecParam.u32FrameBufferHeight;
            dec_data[dev].video_width=0;
            dec_data[dev].video_height=0;
            if(tDecParam.u32API_version!=H264VER) {
                 printk("Fail API Version v%d.%d (Current Driver v%d.%d)\n",
		        tDecParam.u32API_version>>16,tDecParam.u32API_version&0xffff,H264VER_MAJOR,H264VER_MINOR);
		        printk("Please upgrade your H264 driver and re-compiler AP.\n");
		        ret=-EFAULT;
		        goto decoder_ioctl_exit;
            }
            
     		tDecParam.pu8BitStream_phy = (unsigned char *)dec_bs_phy_buffer;
	
            if ((ret=AVCDec_Init(&tDecParam, (void **)&(dec_data[dev].dec_handle), dev)) != RETCODE_OK){
                printk("FAVC_Init decoder failure, Error Number %d\n",ret);
                ret=-EFAULT;
                goto decoder_ioctl_exit;
            }
            
            break;
			
 
        case FAVC_IOCTL_DECODE_FRAME:
            memcpy((char *)&tDecParam, (char *)arg, sizeof(tDecParam));

            AVCDec_ReInit(dec_data[dev].dec_handle);


            if ((ret = AVCDec_FillBuffer(dec_data[dev].dec_handle, tDecParam.pu8Pkt_buf, tDecParam.u32Pkt_size, 1)) != RETCODE_OK) {
                printk("AVCDec_FillBuffer() decoder failure, Error Number %d\n",ret);                
                ret=-1;
                goto decoder_ioctl_exit;
            }
            
            if (tDecParam.u32Pkt_size ==0)
	            AVCDec_EndOfData(dec_data[dev].dec_handle);


            decoder_dummy_write(dec_data[dev].dec_handle);
			
          AVCDec_SetOutputAddr(dec_data[dev].dec_handle,
                (unsigned char *)tDecParam.pu8Display_addr[0],
                (unsigned char *)tDecParam.pu8Display_addr[1],
                (unsigned char *)tDecParam.pu8Display_addr[2]);
            AVCDec_SetCrop(dec_data[dev].dec_handle, tDecParam.crop_x, tDecParam.crop_y);
            if((ret=favc_decoder_OneFrame(dec_data[dev].dec_handle, &_tDecResult[dev]) )< 0){

                tDecParam.got_picture=0;
            } else {
	            dec_data[dev].video_width=_tDecResult[dev].u32Width; 
	            dec_data[dev].video_height=_tDecResult[dev].u32Height;            
              	tDecParam.got_picture=1;
            }
            memcpy((unsigned char *)&tDecParam.tResult, (unsigned char *)&_tDecResult[dev],sizeof(FAVC_DEC_RESULT));
            
			memcpy((void *)arg,(void *)&tDecParam, sizeof(tDecParam));
         
            break;
			
        default:
            break;
    }

decoder_ioctl_exit:
    
    return ret;
}

//get continue output Y,U,V User address
int favc_decoder_mmap(void)
{
    return 0;
}

int favc_decoder_release_ex(int dev)
{
 
    if(dec_data[dev].dec_handle)
        AVCDec_Release(dec_data[dev].dec_handle);
        
   
    dec_data[dev].dec_handle=0;
    
    if(dec_data[dev].idx_ex)
    {
    	nv_free(dec_data[dev].idx_ex);
    	dec_data[dev].idx_ex = 0;
	}    	
   
    return 0;
}	

int favc_decoder_release(void)
{
    int dev=0;
 
    if(dec_data[dev].dec_handle)
        AVCDec_Release(dec_data[dev].dec_handle);
        
   
    dec_data[dev].dec_handle=0;
   
    return 0;
}	


