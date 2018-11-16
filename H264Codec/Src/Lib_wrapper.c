#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "memory.h"
#include "avcdec.h"
#include "decoder.h"
#include "encoder.h"
#include "wbtypes.h"
#include "W55FA92_reg.h"
#include "favc_module.h"

#include "h264dec.h"
#include "wblib.h"

#include "favc_avcodec.h"

extern void cleanup_favc(void);

volatile PFN_DRVH264_INT_CALLBACK g_pfnH264Callback[2] = {NULL, NULL};

static int    dec_idx=0, enc_idx=0;

extern struct dec_private      dec_data[];
extern struct enc_private      enc_data[];

#define	VDE_INT_NUM	33
#define	VEN_INT_NUM 34

int H264DecodeIntHandler(void);
int H264EncodeIntHandler(void);

extern DECODER *cur_dec;
extern int frame_end_flag,slice_end_flag,BITBufferEmptyOK,Enc_FrameDone_flag;

extern init_favc_dec(void);
extern int init_favc_dec_ex(int handle);
extern init_favc_enc(void);
extern int init_favc_enc_ex(int handle);
extern cleanup_favc_enc(void);
extern cleanup_favc_dec(void);


int H264Dec_Open(void)
{
	int idx;
	
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | VDE_CKE);
	outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) | VDEC_CKE);

    idx=0;	
    while( idx < MAX_DEC_NUM) {
        if( dec_idx&DEC_IDX_MASK_n(idx) ) {
            idx++;
        } else{
            dec_idx |= DEC_IDX_MASK_n(idx);
            break;
        }
    }	
    
    if (idx > MAX_DEC_NUM -1)
    	return -1;
	
	if (init_favc_dec_ex(idx) < 0)
		return -1;
		
	if (idx ==0)		
	{	// for First Decoder instance
		sysInstallISR(IRQ_LEVEL_1, IRQ_VDE, (PVOID)H264DecodeIntHandler);	
		//sysInstallISR(0, IRQ_VDE, (PVOID)H264DecodeIntHandler);	
		sysEnableInterrupt(IRQ_VDE);

		sysSetLocalInterrupt(ENABLE_IRQ);	// enable CPSR I bit		
	}
	return idx+1;	// remapping 0~ n-1 to 1 ~ n
}

int H264Enc_Open(void)
{
    int idx;
    
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | VDE_CKE);
	outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) | VENC_CKE);
	
    idx=0;	
    while( idx < MAX_ENC_NUM) {
        if( enc_idx&ENC_IDX_MASK_n(idx) ) {
            idx++;
        } else{
            enc_idx |= ENC_IDX_MASK_n(idx);
            break;
        }
    }	
    
    if (idx > MAX_ENC_NUM -1)
    	return -1;
    		
	if (init_favc_enc_ex(idx) <0)
		return -1;
	
	if (idx == 0)
	{
    	sysInstallISR(IRQ_LEVEL_1, IRQ_VEN, (PVOID)H264EncodeIntHandler);	
    	sysEnableInterrupt(IRQ_VEN);
    	outp32(REG_264_STS2, 0);			// Enable Encoder Frame done interrupt		
    
    	sysSetLocalInterrupt(ENABLE_IRQ);	// enable CPSR I bit
    }
	return idx+1;	// remapping 0~ n-1 to 1 ~ n	
}

int H264_ioctl_ex(int handle, int cmd, void* param)
{
	switch(cmd)
	{
		case FAVC_IOCTL_DECODE_INIT:
		case FAVC_IOCTL_DECODE_FRAME:
			handle--;	// remapping 1~ n to 0~ n-1
		    if(((1 << handle) & dec_idx) == 0) {
		        printk("No Support index %d for 0x%x\n",handle,dec_idx);
		        return -1;
		    } 		
			return favc_decoder_ioctl(&handle, cmd, param);
		//break;
		case FAVC_IOCTL_ENCODE_INIT:
		case FAVC_IOCTL_ENCODE_FRAME:
		case FAVC_IOCTL_GET_SPSPPS:
			handle--;	// remapping 1~ n to 0~ n-1
		    if(((1 << handle) & enc_idx) == 0) {
		        printk("No Support index %d for 0x%x\n",handle,enc_idx);
		        return -1;
		    } 		
			return 	favc_encoder_ioctl(&handle,cmd,param);
		default:
			sysprintf("Cmd %x is not supported\n",cmd);
			break;	
	}
	return -1;
}

int H264_ioctl(int cmd, void* param)
{


	switch(cmd)
	{
		case FAVC_IOCTL_DECODE_INIT:
		case FAVC_IOCTL_DECODE_FRAME:
			return favc_decoder_ioctl((void*)NULL, cmd, param);
		//break;
		case FAVC_IOCTL_ENCODE_INIT:
		case FAVC_IOCTL_ENCODE_FRAME:
		case FAVC_IOCTL_GET_SPSPPS:
			return 	favc_encoder_ioctl((void *)NULL,cmd,param);
		default:
			sysprintf("Cmd %x is not supported\n",cmd);
			break;	
	}
	return -1;
}

void H264Enc_Close_ex(int handle)
{
		handle--;	// remapping 1~ n to 0~ n-1
		    
		h264_encoder_destroy(enc_data[handle].enc_handle);
		enc_data[handle].enc_handle = 0;
		favc_encoder_release_ex(handle);
		cleanup_favc_enc_ex();
		
		enc_idx &= (~(ENC_IDX_MASK_n(handle)) );			
}

void H264Enc_Close(void)
{
		h264_encoder_destroy(enc_data[0].enc_handle);
		enc_data[0].enc_handle = 0;
		favc_encoder_release();
		cleanup_favc_enc();

		enc_idx &= (~(ENC_IDX_MASK_n(0)) );		
}

void H264Dec_Close_ex(int handle)
{
		handle--;	// remapping 1~ n to 0~ n-1
		
		favc_decoder_release_ex(handle);
		cleanup_favc_dec();
		
		dec_idx &= (~(DEC_IDX_MASK_n(handle)) );
}

void H264Dec_Close(void)
{
		favc_decoder_release();
		cleanup_favc_dec();
		
		dec_idx &= (~(DEC_IDX_MASK_n(0)) );
}


int H264DecodeIntHandler(void)
{
	UINT32 INT_status;
	UINT32 INT_enable;
	
	INT_enable = inp32(REG_264_INTS)>>16;	
	INT_status = inp32(REG_264_INTS) & 0x3F;
	
	INT_status = INT_status & ~INT_enable;
		
	
	if (INT_status & INT_TRANS_DONE)
	{
		outp32(REG_264_INTS,inp32(REG_264_INTS) | MASK_TRANS_DONE_INT);	// Avoid the interrupt again	
		outp32(REG_264_INTS,inp32(REG_264_INTS) & ~INT_TRANS_DONE);			
	}
	if(INT_status & INT_BUS_ERROR)
	{
		outp32(REG_264_INTS,inp32(REG_264_INTS) & ~INT_BUS_ERROR);			
	} 
	if(INT_status & INT_MV_OVER_RANGE)
	{
		outp32(REG_264_INTS,inp32(REG_264_INTS) & ~INT_MV_OVER_RANGE);		
	}
	if(INT_status & INT_BSM_BUF_EMPTY)
	{
		BITBufferEmptyOK = 1;
	}
	
	refresh_int_sts(cur_dec);
	
	return 0;
	
}

int H264EncodeIntHandler(void)
{
	
	if (inp32(REG_264_STS2) & S2_FD_INT_STS)
	{
		Enc_FrameDone_flag = 1;
		outp32(REG_264_STS2,inp32(REG_264_STS2) & ~S2_FD_INT_STS);
	}
	
	return 0;
}

int H264InstallCallBack(
	E_DRVH264_INT eIntSource,
	PFN_DRVH264_INT_CALLBACK	pfnCallback,
	PFN_DRVH264_INT_CALLBACK 	*pfnOldCallback
)
{
	switch(eIntSource)
	{
		case eDRVH264_DEC_SLICE_COMPLETE:
			*pfnOldCallback = g_pfnH264Callback[0];		// return previous installed callback function pointer
			g_pfnH264Callback[0] = pfnCallback;       		// install current callback function
			break;
			
		case eDRVH264_DEC_FRAME_COMPLETE:
			*pfnOldCallback = g_pfnH264Callback[1];	
			g_pfnH264Callback[1] = pfnCallback;       
			break;
			
		default:
			return 1;
	}
	return 0;
}

