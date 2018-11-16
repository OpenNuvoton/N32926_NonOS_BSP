#include <stdio.h>
#include "wblib.h"

#include "AVCdec.h"
#include "favc_version.h"
#include "user_define.h"

#include "favc_module.h"
#include "decoder.h"
#include "encoder.h"


unsigned int h264_max_width = MAX_DEFAULT_WIDTH;
unsigned int h264_max_height = MAX_DEFAULT_HEIGHT;


unsigned int support_decoder = SUPPORT_DECODER_DEFAULT_YES;
unsigned int support_encoder = SUPPORT_ENCODER_DEFAULT_YES;

int    favc_enc_mutex;
int    favc_dec_mutex;
unsigned int    enc_raw_yuv_phy=0;

unsigned int    dec_bs_phy_buffer=0,dec_bs_buf_size = 0;
unsigned int    out_phy_buffer=0;

unsigned int    mb_info_phy_buffer;
unsigned int    intra_pred_phy_buffer; 


int decoder_int_handler(int irq, void *dev_id);
int encoder_int_handler(int irq, void *dev_id);

extern struct dec_private dec_data[];
extern struct enc_private enc_data[];

int init_favc_dec_ex(int handle)
{
    unsigned int size, bs_size, mb_info_size,intra_pred_size;

    // decoder part
    if (support_decoder) {
       
        bs_size = DIV_16((h264_max_width*h264_max_height*3/2));
        mb_info_size = intra_pred_size = DIV_16( (h264_max_width>>4)*8*4 );	
        dec_bs_buf_size = size = bs_size + mb_info_size + intra_pred_size;

        if((void*)(dec_bs_phy_buffer = (unsigned int)nv_malloc(size,32))==NULL)     
        {
            printk("Memory dec_bs_phy_buffer allocation error!\n");
            goto fail_allocate_d;
        }

        mb_info_phy_buffer = dec_bs_phy_buffer + bs_size;
        intra_pred_phy_buffer = mb_info_phy_buffer + mb_info_size;
        
		dec_data[handle].idx_ex = (DEC_IDX_EX *)nv_malloc(sizeof(DEC_IDX_EX),32);
		if   (dec_data[handle].idx_ex == NULL)
		{
			goto fail_allocate_d;
		}
		else 
		{
			dec_data[handle].idx_ex->signature = SIGNATURE;
			dec_data[handle].idx_ex->decoder_idx = handle;
			dec_data[handle].idx_ex->dec_bs_phy_buffer = dec_bs_phy_buffer;
		}

    }

    return 0;
    
    
fail_allocate_d:

    if(dec_bs_phy_buffer) {
		nv_free((void *)dec_bs_phy_buffer);    
		dec_bs_phy_buffer = 0;    
    }

    return -1;
}

int init_favc_dec(void)
{
    unsigned int size, bs_size, mb_info_size,intra_pred_size;

    // decoder part
    if (support_decoder) {
       
        bs_size = DIV_16((h264_max_width*h264_max_height*3/2));
        mb_info_size = intra_pred_size = DIV_16( (h264_max_width>>4)*8*4 );	
        dec_bs_buf_size = size = bs_size + mb_info_size + intra_pred_size;

        if((void*)(dec_bs_phy_buffer = (unsigned int)nv_malloc(size,32))==NULL)     
        {
            printk("Memory dec_bs_phy_buffer allocation error!\n");
            goto fail_allocate_d;
        }

        mb_info_phy_buffer = dec_bs_phy_buffer + bs_size;
        intra_pred_phy_buffer = mb_info_phy_buffer + mb_info_size;

    }

    return 0;
    
    
fail_allocate_d:

    if(dec_bs_phy_buffer) {
		nv_free((void *)dec_bs_phy_buffer);    
		dec_bs_phy_buffer = 0;    
    }

    return -1;
}

int init_favc_enc_ex(int handle)
{

    // encoder part
    if (support_encoder) {    

		enc_data[handle].idx_ex = (ENC_IDX_EX *)nv_malloc(sizeof(ENC_IDX_EX),32);
		if   (enc_data[handle].idx_ex == NULL)
		{
			goto fail_allocate_e;
		}
		else 
		{
			enc_data[handle].idx_ex->signature = SIGNATURE_E;
			enc_data[handle].idx_ex->encoder_idx = handle;
		}	    
	}

    return 0;

fail_allocate_e:

    return -1;
}

int init_favc_enc(void)
{

    // encoder part
    if (support_encoder) {    
		//Put encoder bitstream memory in system memory.
    	if((void*)(out_phy_buffer = (unsigned int)nv_malloc((h264_max_width*h264_max_height*3/2),32)) == NULL)
	    {
	        printk("Memory out_phy_buffer allocation error!\n");
	        goto fail_allocate_e;
	    }
	}

    return 0;

fail_allocate_e:

    if(out_phy_buffer) {
		nv_free((void *)out_phy_buffer);
		out_phy_buffer = 0;
    }

    return -1;
}

void cleanup_favc_dec_ex(int handle)
{

    // decoder part
    if (support_decoder) {   
    
     	if (dec_data[handle].idx_ex)
     	{
	        if(dec_data[handle].idx_ex->dec_bs_phy_buffer) {
				nv_free((void *)dec_data[handle].idx_ex->dec_bs_phy_buffer);
				dec_data[handle].idx_ex->dec_bs_phy_buffer=0;
	        }
	        nv_free((void *)dec_data[handle].idx_ex);
        }
    }
}

void cleanup_favc_dec(void)
{

    // decoder part
    if (support_decoder) {    
     
        if(dec_bs_phy_buffer) {
			nv_free((void *)dec_bs_phy_buffer);
			dec_bs_phy_buffer=0;
        }
    }
}

void cleanup_favc_enc_ex(int handle)
{
    // encoder part
     	if (enc_data[handle].idx_ex)
     	{
	        if(enc_data[handle].idx_ex->out_phy_buffer) {
				nv_free((void *)enc_data[handle].idx_ex->out_phy_buffer);
				enc_data[handle].idx_ex->out_phy_buffer=0;
	        }	        
	        nv_free((void *)enc_data[handle].idx_ex);
        }
        
    // encoder part
}

void cleanup_favc_enc(void)
{

    // encoder part

    if(out_phy_buffer) {
		nv_free((void *)out_phy_buffer);      
		out_phy_buffer = 0;
    }
}
