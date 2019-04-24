/********************************************/
/* Faraday avc encoder driver operations    */
/********************************************/

#include <stdio.h>
#include <string.h>
#include "w55fa92_reg.h"
#include "h264dec.h"
#include "wblib.h"
#include "avcdec.h"

#include "port.h"
#include "register.h"
#include "encoder.h"
#include "common.h"
#include "slice.h"
#include "sequence.h"
#include "bs.h"


#include "favc_avcodec.h"
//#include "h264_enc_api.h"
#include "favc_version.h"
#include "encoder.h"


#define ALLO_NONINDEPEND    // used for mem alloc testing


// following global variables need modified for duplex
extern unsigned int     h264_max_width,h264_max_height;
extern unsigned int     out_phy_buffer;
extern unsigned int     enc_raw_yuv_phy;
extern struct semaphore favc_enc_mutex;

static int              enc_idx=0;


struct enc_private      enc_data[MAX_ENC_NUM]={0,0,0,0};

extern int favc_check_continued(unsigned int addr, int size);
extern unsigned int favc_user_va_to_pa(unsigned int addr);
extern int h264_encoder_spspps(void *handle,FAVC_ENC_PARAM *mEncParam);
extern int h264_encoder_nvop_nal(void *handle,FAVC_ENC_PARAM *mEncParam);


int favc_encoder_open(void)
{
    int dev;
    int i;
    if((enc_idx&ENC_IDX_FULL)==ENC_IDX_FULL)
    {
        printk("Encoder Device Service Full,0x%x!\n",enc_idx);
        return -EFAULT;
    }
	
//	down(&favc_enc_mutex);
	
    dev=0;	
    while( dev < MAX_ENC_NUM) {
	if( enc_idx&ENC_IDX_MASK_n(dev) ) {
		dev++;
	} else {
		enc_idx |= ENC_IDX_MASK_n(dev);	
		break;
	}
    }	
	
    //printk("idx=%d enc_idx=0x%x\n",idx,enc_idx);
//    filp->private_data=kmalloc(sizeof(unsigned int),GFP_KERNEL);
//  if(filp->private_data==0)    {
//        printk("Can't allocate memory!\n");
//        up(&favc_enc_mutex);
//        return -EFAULT;
//    }
//    *(unsigned int *)filp->private_data=dev;
//    memset(&enc_data[dev],0,sizeof(struct enc_private));
    enc_data[dev].enc_handle=0;		
    for (i=0; i<SUPPORT_MAX_MSLIC_NUM; i++) {
        
        enc_data[dev].slicebuf[i].recon_phy_buffer = 0;
        enc_data[dev].slicebuf[i].refer_phy_buffer = 0;
        enc_data[dev].slicebuf[i].sysinfo_phy_buffer = 0;
        enc_data[dev].slicebuf[i].dma_phy_buffer = 0;        
        
        enc_data[dev].slicebuf[i].mslice_mb=0;
        enc_data[dev].slicebuf[i].mslice_height = enc_data[dev].slicebuf[i].mslice_width=0;
    }

//    up(&favc_enc_mutex);
    
    return 0; /* success */
}
int h264e_init(FAVC_ENC_PARAM * param, int dev)
{
    unsigned int tmp;
    unsigned int mslice_height;
    int i;
    unsigned int size=0;
    unsigned int roi_x, roi_y, roi_width, roi_height;	
    unsigned int recon_size, refer_size, sysinfo_size, dma_size;	

	// Clear setting for next bitstream encode
	if (dev < MAX_ENC_NUM)
	{
		if (enc_data[dev].enc_handle != 0x0)
		memset(enc_data[dev].enc_handle, 0, sizeof(h264_encoder));
	}	
		
    //if(param->u32API_version != H264VER)  {
    //    printk("Fail API Version v%d.%d (Current Driver v%d.%d)\n",
    //    param->u32API_version>>16,param->u32API_version&0xffff, H264VER_MAJOR, H264VER_MINOR);
    //    return -EFAULT;
    //}
    if (param->u32FrameWidth & 0xF){
        tmp = param->u32FrameWidth;
        param->u32FrameWidth = ((tmp + 15) >> 4) << 4;
        printk("REPLACE width %d with %d\n", tmp, param->u32FrameWidth);
    }
    if (param->u32FrameHeight & 0xF){
        tmp = param->u32FrameHeight;
        param->u32FrameHeight = ((tmp + 15) >> 4) << 4;
        printk("REPLACE height %d with %d\n", tmp, param->u32FrameHeight);
    }
    if (param->u32FrameWidth <= (8 * 16)){
        // the width of H.264 should larger than 8 macro width
        printk("The width is too SMALL ( width<=8x16) for H.264 encoder\n");
        return -EFAULT;
    }

    #define IMG_FMT_SUPPORT 0	// only H.264 2D format is supported.

    if (param->img_fmt > IMG_FMT_SUPPORT) {
        printk("img_fmt error (cur: %d)\n", param->img_fmt);
        return -EFAULT;
    }
    /*
    else if (param->img_fmt == 1) { // 2D mp4 mode
        int distance_uv = -15 + (param->u32FrameWidth * param->u32FrameHeight/4)/4;
        if ((distance_uv >= 0x8000) || (distance_uv < -0x8000)) {
            printk ("[Error] Not support such high resolution (%dx%d) at MP4-2D mode\n", param->u32FrameWidth, param->u32FrameHeight);
            return -EFAULT;
        }
    }
    */


    tmp = param->u32FrameWidth * param->u32FrameHeight >> 8;

    if ((param->control & 1) || (tmp < LIMIT_MAX_MB)) {
        enc_data[dev].mslice_num = 1;
        enc_data[dev].mslice_enable = 0;
        //printk("SingleSlice encoding, One frame %d slices max mb %d\n", enc_data[dev].mslice_num, LIMIT_MAX_MB);
    }
    else{
        enc_data[dev].mslice_num = (tmp + LIMIT_MAX_MB - 1) / LIMIT_MAX_MB;
        enc_data[dev].mslice_enable = 1;
        //printk("MultiSlice encoding, One frame %d slices total %d max mb %d\n", enc_data[dev].mslice_num, mb_num, LIMIT_MAX_MB);
    }
    enc_data[dev].frame_width = param->u32FrameWidth;
    enc_data[dev].frame_height = param->u32FrameHeight;

    if (param->bROIEnable){
        if ((param->u32ROIWidth == 0) || (param->u32ROIHeight == 0)){
            printk("Error: ROIwidth, ROIHeight (%d, %d) should not be 0\n", param->u32ROIWidth, param->u32ROIHeight);
            return -EFAULT;
        }
        if (param->u32ROIWidth & 0xF){
            tmp = param->u32ROIWidth;
            param->u32ROIWidth = ((tmp + 0xF) >> 4) << 4;
            printk("REPLACE ROI width %d with %d\n", tmp, param->u32ROIWidth);
        }
        if (param->u32ROIHeight & 0xF){
            tmp = param->u32ROIHeight;
            param->u32ROIHeight = ((tmp + 0xF) >> 4) << 4;
            printk("REPLACE ROI height %d with %d\n", tmp, param->u32ROIHeight);
        }
        if (param->u32ROIX & 0xF){
            tmp = param->u32ROIX;
            param->u32ROIX = ((tmp + 0xF) >> 4) << 4;
            printk("REPLACE ROI x %d with %d\n", tmp, param->u32ROIX);
        }
        if (param->u32ROIY & 0xF){
            tmp = param->u32ROIY;
            param->u32ROIY = ((tmp + 0xF) >> 4) << 4;
            printk("REPLACE ROI y %d with %d\n", tmp, param->u32ROIY);
        }

        if (((param->u32ROIX + param->u32ROIWidth) > param->u32FrameWidth) || 
           ((param->u32ROIY + param->u32ROIHeight) > param->u32FrameHeight)){
            printk("Error: Setting ROI, X Y = (%d, %d) , W H = (%d, %d), FW FH = (%d, %d)\n", 
           param->u32ROIX, param->u32ROIY, 
           param->u32ROIWidth, param->u32ROIHeight, 
           param->u32FrameWidth, param->u32FrameHeight);
            return -EFAULT;
        }
        enc_data[dev].roi_width = param->u32ROIWidth;
        enc_data[dev].roi_height = param->u32ROIHeight;
        param->pic_height = enc_data[dev].roi_height;
        param->pic_width = enc_data[dev].roi_width;
        roi_x = param->u32ROIX;
        roi_y = param->u32ROIY;
        roi_width = param->u32ROIWidth;
        roi_height = param->u32ROIHeight;
    } //if (param->bROIEnable)

    else {
        param->u32ROIX = 0;
        param->pic_height = enc_data[dev].frame_height;
        param->pic_width = enc_data[dev].frame_width;
        roi_x = 0;
        roi_y = 0;
        roi_width = 0;
        roi_height = 0;
    }
    enc_data[dev].roi_enable = param->bROIEnable;

    //========================================================================================
	tmp = enc_data[dev].frame_height;
	mslice_height = (((tmp / enc_data[dev].mslice_num) + (PIXEL_Y - 1)) / PIXEL_Y) * PIXEL_Y;

	// MultiSlice
	for (i = 0; i < enc_data[dev].mslice_num; i++){
		sbuf * pslc = &enc_data[dev].slicebuf[i];
		pslc->mslice_width = enc_data[dev].frame_width;
		if (tmp >= mslice_height) pslc->mslice_height = mslice_height;
		else pslc->mslice_height = tmp;
		tmp -= pslc->mslice_height;
		pslc->mslice_mb = pslc->mslice_width * pslc->mslice_height / SIZE_Y;
		if (i >= (enc_data[dev].mslice_num - 1))
			pslc->mslice_last = 1;
		else
			pslc->mslice_last = 0;

		// ROI
		if (param->bROIEnable){
			if (roi_height > 0){
				if (roi_y <= enc_data[dev].slicebuf[i].mslice_height){  // KC: Set msslice_ROIHeight for each slice
					pslc->mslice_ROI = 1;
					pslc->mslice_ROIX = roi_x;
					pslc->mslice_ROIY = roi_y;
					pslc->mslice_ROIWidth = enc_data[dev].roi_width;
					if (roi_height > pslc->mslice_height)
						pslc->mslice_ROIHeight = pslc->mslice_height - pslc->mslice_ROIY;
					else
						pslc->mslice_ROIHeight = roi_height;
					roi_height -= enc_data[dev].slicebuf[i].mslice_ROIHeight;
					roi_y = 0;
					pslc->mslice_ROILast = (roi_height == 0)? 1: 0;
				}
				else{                                                   
					pslc->mslice_ROI = 1;
					pslc->mslice_ROIX = 0;
					pslc->mslice_ROIY = 0;
					pslc->mslice_ROIWidth = 0;
					pslc->mslice_ROIHeight = 0;
					pslc->mslice_ROILast = 0;
					roi_y -= pslc->mslice_height;
				}
			}
			else{
				pslc->mslice_ROI = 1;
				pslc->mslice_ROIX = 0;
				pslc->mslice_ROIY = 0;
				pslc->mslice_ROIWidth = 0;
				pslc->mslice_ROIHeight = 0;
				pslc->mslice_ROILast = 0;
			}
			pslc->mslice_ROIMB = pslc->mslice_ROIWidth * pslc->mslice_ROIHeight / SIZE_Y;
			recon_size = refer_size = DIV_16(pslc->mslice_ROIWidth* pslc->mslice_ROIHeight* 3 / 2);
			sysinfo_size = DIV_16(SYSBUF_SIZE(pslc->mslice_ROIWidth, pslc->mslice_ROIHeight));
		}
		else{
			pslc->mslice_ROI = 0;
			pslc->mslice_ROIX = 0;
			pslc->mslice_ROIY = 0;
			pslc->mslice_ROIWidth = 0;
			pslc->mslice_ROIHeight = 0;
			pslc->mslice_ROILast = 0;
			pslc->mslice_ROIMB = 0;
			recon_size = refer_size = DIV_16(pslc->mslice_width * pslc->mslice_height * 3 / 2);
			sysinfo_size = DIV_16(SYSBUF_SIZE(pslc->mslice_width, pslc->mslice_height));
		}

		dma_size = DIV_16(DMA_BUF_SIZE + DMA_BUF_SIZE);

		/* allocate recontructed */
		if ((pslc->recon_phy_buffer = (unsigned int)nv_malloc(recon_size,32)) == NULL){			
			printk("Memory recon_virt_buffer allocation error!\n");
			return -EFAULT;
		}

		size = refer_size + sysinfo_size + dma_size;
		/* allocate refer */
		if ((pslc->refer_phy_buffer = (unsigned int)nv_malloc(size,32)) == NULL){			
			printk("Memory refer_virt_buffer allocation error!\n");
			return -EFAULT;
		}
		
	  	if((void*)(out_phy_buffer = (unsigned int)nv_malloc((enc_data[dev].frame_width*enc_data[dev].frame_height*3/2),32)) == NULL)
	    {
	        printk("Memory out_phy_buffer allocation error!\n");
	        return -EFAULT;
	    }
		enc_data[dev].idx_ex->out_phy_buffer = out_phy_buffer;

		/* allocate sysinfo buffer*/
		pslc->sysinfo_phy_buffer = pslc->refer_phy_buffer + refer_size;

		/* allocate dma buffer */
		pslc->dma_phy_buffer = pslc->sysinfo_phy_buffer + sysinfo_size;

		// init for first slice encoding
		param->multi_slice = enc_data[dev].mslice_enable;
		param->u32FrameWidth = pslc->mslice_width;
		param->u32FrameHeight = pslc->mslice_height;
		param->pu8ReConstructFrame = (unsigned char*) pslc->recon_phy_buffer;
		param->pu8ReferenceFrame = (unsigned char*) pslc->refer_phy_buffer;
		param->pu8SysInfoBuffer = (unsigned char*) pslc->sysinfo_phy_buffer;
		param->pu8DMABuffer_phy = (unsigned char*) pslc->dma_phy_buffer;

		param->bROIEnable = pslc->mslice_ROI;
		param->u32ROIX = pslc->mslice_ROIX;
		param->u32ROIY = pslc->mslice_ROIY;
		param->u32ROIWidth = pslc->mslice_ROIWidth;
		param->u32ROIHeight = pslc->mslice_ROIHeight;
		param->pu8BitstreamAddr = (unsigned char*)out_phy_buffer;
		param->threshold_disable = 0;
		param->chroma_threshold = 4;
		param->luma_threshold = 4;
		param->beta_offset = 0;
		param->alpha_offset = 0;
		param->chroma_qp_offset = 0;
		param->disable_ilf = 0;
		param->no_frames = 300; // not used if IGNORE_FRAME_NO is enabled
//		param->pfnDmaMalloc = hconsistent_alloc;
//		param->pfnDmaFree = hconsistent_free;
//		param->pfnMalloc = hkmalloc;
//		param->pfnFree = hkfree;

		if (enc_data[dev].enc_handle == NULL){
			// first time to allocate h264_encoder memory
			enc_data[dev].enc_handle = h264_encoder_init(param);
			if (enc_data[dev].enc_handle == NULL){
				printk("Error to create encoder structure!\n");
				return - EFAULT;
			}
		}
		// init every default DMA command
		h264_encoder_reinit(enc_data[dev].enc_handle, param, param->img_fmt);
	}
	return 0;
}


int favc_encoder_ioctl(void* handle, unsigned int cmd, void* arg)
{
    FAVC_ENC_PARAM     tEncParam;
//    FAVC_VUI_PARAM     tVUIParam;
//    FAVC_CROP_PARAM    tCROPParam;
    int                 i, ret=0;
	int					dev=0;    
  
    int mslice_len;
//    unsigned int sysbuf_len=0;
    unsigned int YFrameBase, UFrameBase, VFrameBase;
    int idx;    

	if (handle != NULL)
	{
	    idx = *(int *)handle;
		
	    if ((enc_data[idx].idx_ex->signature == SIGNATURE_E) && (enc_data[idx].idx_ex->encoder_idx == idx))
	    {
	    	dev=idx;
	    }
    }
    else
    	dev=0;

    switch(cmd) {
    case FAVC_IOCTL_ENCODE_INIT:
		memcpy((char *)&tEncParam, (char *)arg, sizeof(tEncParam));
        
        tEncParam.img_fmt = 0; // 2D h264 format
        if (h264e_init(&tEncParam, dev) < 0) {
            printk("FAVC_IOCTL_ENCODE_INIT error\n");
            ret = -EFAULT;
        }
        break;
/*
    case FAVC_IOCTL_ENCODE_INIT_MP4:
	     printk("MP4 2D format is not supported now\n");
	     ret = -EFAULT;
       
        break;

    case FAVC_IOCTL_ENCODE_VUI:
		memcpy((char *)&tVUIParam, (char *)arg, sizeof(tVUIParam));
        
        if ( enc_data[dev].enc_handle == NULL ) {
            printk("Error: enc_handle %d is NULL\n", dev);
            ret=-EFAULT;
            goto encoder_ioctl_exit;
        }	
        h264_encoder_init_vui(enc_data[dev].enc_handle, &tVUIParam);

		memcpy((char *)arg,(char *)&tVUIParam, sizeof(tVUIParam));		
     
        break;

    case FAVC_IOCTL_ENCODE_CROP:
		memcpy((char *)&tCROPParam, (char *)arg, sizeof(tCROPParam));
       
        if ( enc_data[dev].enc_handle == NULL ) {	
            printk("Error: enc_handle %d is NULL\n", dev);
            ret=-EFAULT;
            goto encoder_ioctl_exit;
        }
        h264_encoder_init_crop(enc_data[dev].enc_handle, &tCROPParam);


		memcpy((char *)arg,(char *)&tCROPParam, sizeof(tCROPParam));
        
        break;

*/
    case FAVC_IOCTL_ENCODE_FRAME:

#ifdef EVALUATION_PERFORMANCE
        get_drv_start();
#endif

		memcpy((char *)&tEncParam, (char *)arg, sizeof(tEncParam));
     
        if ( enc_data[dev].enc_handle == NULL ) {	
            printk("Error: enc_handle %d is NULL\n", dev);
            ret=-EFAULT;
            goto encoder_ioctl_exit;
        }	

           
        //check Qp value
        if((int)tEncParam.u32Quant > MaxQp || (int)tEncParam.u32Quant < MinQp){
            printk("[Error]: Qp Error Qp:%d MaxQp:%d  MinQp:%d  \n",tEncParam.u32Quant,tEncParam.u32MaxQuant,tEncParam.u32MinQuant);
            ret=-EFAULT;
            goto encoder_ioctl_exit;    	            
        }
            
        enc_data[dev].mslice_first_mb=0;       // Don't forget reset mslice_first_mb to 0, it will influence pEnc->frame_num 		
        enc_data[dev].mslice_stride_yuv=0;   // Don't forget reset mslice_stride_yuv to 0, it will influence Y,U,V or UV base
			
        YFrameBase = (unsigned int)tEncParam.pu8YFrameBaseAddr;
        UFrameBase = (unsigned int)tEncParam.pu8UFrameBaseAddr;
        VFrameBase = (unsigned int)tEncParam.pu8VFrameBaseAddr;

        if (h264_fmt(enc_data[dev].enc_handle) == 1) { // mp4-2D
            int distance_uv = -15 + (((int)VFrameBase - (int)UFrameBase))/4;
            if ((distance_uv >= 0x8000) || (distance_uv < -0x8000)) {
                printk ("[Error] Distance between U & V base address is too large (0x%08x, 0x%08x) at MP4-2D mode\n",
                  (int)tEncParam.pu8UFrameBaseAddr, (int)tEncParam.pu8VFrameBaseAddr);
                ret = -EFAULT;
                goto encoder_ioctl_exit;
            }
        }

        tEncParam.multi_slice = enc_data[dev].mslice_enable;
        tEncParam.bitstream_size =0;
        //printk("\nmslice_num: %d, mslice_enable: %d\n", enc_data[dev].mslice_num, enc_data[dev].mslice_enable);

        for ( i = 0; i<enc_data[dev].mslice_num; i++ ) {  
           // ROI parameter update
           // Multi-slice is not support dynamic ROI
           if ( enc_data[dev].mslice_num > 1 ) {
               tEncParam.u32ROIX = enc_data[dev].slicebuf[i].mslice_ROIX;
               tEncParam.u32ROIY = enc_data[dev].slicebuf[i].mslice_ROIY;
           } else {
               if ( tEncParam.bROIEnable == 0 ) {
                   tEncParam.u32ROIX = enc_data[dev].slicebuf[i].mslice_ROIX;
                   tEncParam.u32ROIY = enc_data[dev].slicebuf[i].mslice_ROIY;
               }
           }
           tEncParam.u32ROIWidth = enc_data[dev].slicebuf[i].mslice_ROIWidth;
           tEncParam.u32ROIHeight = enc_data[dev].slicebuf[i].mslice_ROIHeight;
           tEncParam.bROIEnable = enc_data[dev].slicebuf[i].mslice_ROI;
           // MultiSlice reference memory buffer update		  				  
           tEncParam.u32FrameWidth= enc_data[dev].slicebuf[i].mslice_width;
           tEncParam.u32FrameHeight = enc_data[dev].slicebuf[i].mslice_height;	
           tEncParam.pu8ReConstructFrame=(unsigned char *)enc_data[dev].slicebuf[i].recon_phy_buffer;
           tEncParam.pu8ReferenceFrame=(unsigned char *)enc_data[dev].slicebuf[i].refer_phy_buffer;
           tEncParam.pu8SysInfoBuffer=(unsigned char *)enc_data[dev].slicebuf[i].sysinfo_phy_buffer;
           tEncParam.pu8DMABuffer_phy=(unsigned char *)enc_data[dev].slicebuf[i].dma_phy_buffer;
        
           // Multislice Y,U,V or UV base update
           switch (h264_fmt(enc_data[dev].enc_handle)) {
           case 3: // uyvy-1D
               tEncParam.pu8YFrameBaseAddr=(unsigned char *)YFrameBase + enc_data[dev].mslice_stride_yuv*SIZE_Y * 2;
               break;

           case 0: // h264-2D
               tEncParam.pu8YFrameBaseAddr = (unsigned char *)YFrameBase + enc_data[dev].mslice_stride_yuv*SIZE_Y ;
               tEncParam.pu8UVFrameBaseAddr = (unsigned char *)UFrameBase + enc_data[dev].mslice_stride_yuv*( SIZE_U + SIZE_V);	  
               break;

           default: // mp4-2D, 420YUV-1D
               tEncParam.pu8YFrameBaseAddr = (unsigned char *)YFrameBase + enc_data[dev].mslice_stride_yuv*SIZE_Y ;
               tEncParam.pu8UFrameBaseAddr = (unsigned char *)UFrameBase + enc_data[dev].mslice_stride_yuv*SIZE_U ;
               tEncParam.pu8VFrameBaseAddr = (unsigned char *)VFrameBase + enc_data[dev].mslice_stride_yuv*SIZE_V ;
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
                                 &tEncParam, 
                                 enc_data[dev].mslice_first_mb, 
                                 enc_data[dev].slicebuf[i].mslice_ROILast, 
                                 i);
                }
            } else {
                mslice_len = h264_encoder_encode(enc_data[dev].enc_handle,
                                 &tEncParam,
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

				memcpy((char *)((UINT32)tEncParam.bitstream + (UINT32)tEncParam.bitstream_size), 
								(char *)((UINT32)enc_data[dev].idx_ex->out_phy_buffer | 0x80000000), mslice_len);
	
#ifdef EVALUATION_PERFORMANCE
                get_cpy_stop();
#endif
                tEncParam.bitstream_size += mslice_len;
            }

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
		memcpy((char *)arg,(char *)&tEncParam, sizeof(tEncParam));
    
        break;

/*
    case FAVC_IOCTL_GET_SYSINFO:
        sysbuf_len=0;		
        for ( i = 0; i<enc_data[dev].mslice_num; i++ ) {   	
            int sys_inf_size=0;
            sbuf * pslc = &enc_data[dev].slicebuf[i];
            if(pslc->mslice_ROI){
                sys_inf_size = DIV_16(SYSBUF_SIZE(pslc->mslice_ROIWidth, pslc->mslice_ROIHeight));
            }
            else{
                sys_inf_size = DIV_16(SYSBUF_SIZE(pslc->mslice_width, pslc->mslice_height));
            }

			memcpy((char *)((UINT32)arg+sysbuf_len), (char *)(pslc->sysinfo_phy_buffer), sys_inf_size);
          
            sysbuf_len += sys_inf_size;	
        }
        break;
*/
    case FAVC_IOCTL_GET_SPSPPS: 
		memcpy((char *)&tEncParam, (char *)arg, sizeof(tEncParam));
       
        tEncParam.bitstream_size =  h264_encoder_spspps(enc_data[dev].enc_handle,&tEncParam);		


		memcpy((void *)tEncParam.bitstream,(void *)out_phy_buffer, tEncParam.bitstream_size);
		memcpy((char *)arg, (char *)&tEncParam, sizeof(tEncParam));
        
        break;

/*
    case FAVC_IOCTL_ENCODE_NVOP: // not implement OK, don't use it

		memcpy((char *)&tEncParam, (char *)arg, sizeof(tEncParam));
        
        tEncParam.nvop_ioctl = 1;
        tEncParam.pu8BitstreamAddr=(unsigned char *)out_phy_buffer;
        for ( i = 0; i<enc_data[dev].mslice_num; i++ ) {   			
            mslice_len =  h264_encoder_nvop_nal(enc_data[dev].enc_handle,&tEncParam);		
            tEncParam.pu8BitstreamAddr=(unsigned char *)out_phy_buffer;


			memcpy((char *)((UINT32)tEncParam.bitstream + (UINT32)tEncParam.bitstream_size), (char *)out_phy_buffer, mslice_len);
            
            tEncParam.bitstream_size += mslice_len;   
        }

		memcpy((char *)arg, (char *)&tEncParam, sizeof(tEncParam));
       
        break;
*/		
    default:
        printk("[Error] Not support such IOCTL 0x%x\n", cmd);
        ret = -EFAULT;
        break;
    }

encoder_ioctl_exit:
  
    return ret;
}

//get continue input Y,U,V User address
extern unsigned int     enc_raw_yuv_phy;
unsigned int enc_raw_yuv_size = 0;


int favc_encoder_mmap(void)

{


    return 0;
}

int favc_encoder_release_ex(int dev)
{
   
    int i;
   
	
    for ( i = 0; i<enc_data[dev].mslice_num; i++ ) {
        if(enc_data[dev].slicebuf[i].recon_phy_buffer)
        { 
            nv_free((void *)enc_data[dev].slicebuf[i].recon_phy_buffer);  
            enc_data[dev].slicebuf[i].recon_phy_buffer = 0;          
		}
        if(enc_data[dev].slicebuf[i].refer_phy_buffer)
        {
            nv_free((void *)enc_data[dev].slicebuf[i].refer_phy_buffer);
            enc_data[dev].slicebuf[i].refer_phy_buffer=0;
        }               

        enc_data[dev].slicebuf[i].dma_phy_buffer=0;
        enc_data[dev].slicebuf[i].sysinfo_phy_buffer=0;	 
    }
    
    if(enc_data[dev].idx_ex)
    {
    	if(enc_data[dev].idx_ex->out_phy_buffer) {
			nv_free((void *)enc_data[dev].idx_ex->out_phy_buffer);
			enc_data[dev].idx_ex->out_phy_buffer=0;
	    }	
    	nv_free(enc_data[dev].idx_ex);
    	enc_data[dev].idx_ex = 0;
	}  
	
    enc_idx &= (~(ENC_IDX_MASK_n(dev)));
    
    return 0;
}

int favc_encoder_release(void)
{
   
int dev=0;
    int i;
   
	
    for ( i = 0; i<enc_data[dev].mslice_num; i++ ) {
        if(enc_data[dev].slicebuf[i].recon_phy_buffer)
        { 
            nv_free((void *)enc_data[dev].slicebuf[i].recon_phy_buffer);  
            enc_data[dev].slicebuf[i].recon_phy_buffer = 0;          
		}
        if(enc_data[dev].slicebuf[i].refer_phy_buffer)
        {
            nv_free((void *)enc_data[dev].slicebuf[i].refer_phy_buffer);
            enc_data[dev].slicebuf[i].refer_phy_buffer=0;
        }               

        enc_data[dev].slicebuf[i].dma_phy_buffer=0;
        enc_data[dev].slicebuf[i].sysinfo_phy_buffer=0;	 
    }
	
    enc_idx &= (~(ENC_IDX_MASK_n(dev)));
    
    return 0;
}		
