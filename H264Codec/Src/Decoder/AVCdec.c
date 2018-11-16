#include <stdlib.h>
#include <string.h>
#include "AVCdec.h"
#include "decoder.h"

//#define		CACHE_BIT31 0x80000000

extern DECODER *_pAVCDecHandle;

int32_t
AVCDec_Init(FAVC_DEC_PARAM * ptParam, void ** pptDecHandle, unsigned char ndev)
{
	return decoder_create(ptParam, pptDecHandle, ndev);	

}

void
AVCDec_ReInit(void * ptDecHandle)
{
  decoder_reinit(ptDecHandle);
}


uint32_t
AVCDec_QueryEmptyBuffer (void * ptDecHandle)
{
	DECODER * dec = (DECODER *)ptDecHandle;

	return dec->u32BS_buf_sz - dec->u32BS_buf_sz_remain - 64;	// keep 64 byte gate	

}

AVC_RET                                 
AVCDec_FillBuffer(void * ptDecHandle, uint8_t * ptBuf, uint32_t u32BufSize, bool bfitFrameSize)
{
	DECODER * dec = (DECODER *)ptDecHandle;
	uint32_t size_after_fill = 0;
	uint32_t circular_size = 0;
	uint32_t resisual = 0;

	if (u32BufSize > (dec->u32BS_buf_sz - dec->u32BS_buf_sz_remain))
		return RETCODE_ERR_FILL_BUFFER;

	if (u32BufSize)
	{
		// Simply by no wrap around case due to IP is reset for each bitstream
		if (u32BufSize + 256 > dec->u32BS_buf_sz)
			return RETCODE_ERR_GENERAL;
		else
		{
			memcpy((unsigned char *)(CACHE_BIT31 | (unsigned int)dec->pu8BS_start_phy), (unsigned char *)ptBuf,  u32BufSize);		
			memset((unsigned char *)(CACHE_BIT31 | (unsigned int)(dec->pu8BS_start_phy +u32BufSize)), 0x0, 4);
			
			if(u32BufSize & 3) // multiple of word?
				u32BufSize = u32BufSize+(4-(u32BufSize & 3));	
				
			dec->u32BS_sw_offset += u32BufSize; 			
		}	
	/*
		memset((unsigned char *)(CACHE_BIT31 | (unsigned int)(ptBuf +u32BufSize)), 0x0, 4);
		
		if(u32BufSize & 3) // multiple of word?
			u32BufSize = u32BufSize+(4-(u32BufSize & 3));
		  
		size_after_fill = dec->u32BS_sw_offset + u32BufSize;
		
		// christie '05/10/24 bitstream sync between CPU and HW
		if(size_after_fill > dec->u32BS_buf_sz)
		{
			circular_size = size_after_fill - dec->u32BS_buf_sz;
			
			memcpy((unsigned char *)(CACHE_BIT31 | (unsigned int)(dec->pu8BS_start_phy + dec->u32BS_sw_offset)), (unsigned char *)ptBuf,  u32BufSize-circular_size);

			dec->u32BS_sw_offset = circular_size;
		}
		else
		{
			memcpy((unsigned char *)(CACHE_BIT31 | (unsigned int)(dec->pu8BS_start_phy + dec->u32BS_sw_offset)), (unsigned char *)ptBuf,  u32BufSize-circular_size);
			dec->u32BS_sw_offset += u32BufSize;  
		}
		*/		
	}

	if(bfitFrameSize)
	{
		// fill in dummy 0
		resisual = 64 - (u32BufSize & (64 - 1)) + (4*32) + 64;
		size_after_fill = dec->u32BS_sw_offset + resisual;
		if(size_after_fill > dec->u32BS_buf_sz)
		{		
		  circular_size = size_after_fill - dec->u32BS_buf_sz;
		  memset((unsigned char *)(CACHE_BIT31 | (unsigned int)(dec->pu8BS_start_phy + dec->u32BS_sw_offset)), 0, (resisual - circular_size));			  	
  		  memset((unsigned char *)(CACHE_BIT31 | (unsigned int)dec->pu8BS_start_phy), 0, circular_size);
		  dec->u32BS_sw_offset = circular_size;
		}	
		else
		{
		  memset((unsigned char *)(CACHE_BIT31 | (unsigned int)(dec->pu8BS_start_phy + dec->u32BS_sw_offset)), 0, resisual);		  
		  dec->u32BS_sw_offset += resisual;  
		}	
	}
	dec->u32BS_buf_sz_remain += (u32BufSize + resisual);
	dec->u32BS_sw_cur_fill_size += (u32BufSize + resisual);
	
	// HW setting
   	decoder_fill_bs_reg(ptDecHandle,(u32BufSize+63)/64 * 64);	
	
	return RETCODE_OK;
}

uint32_t
AVCDec_QueryFilledBuffer (void * ptDecHandle)
{
	DECODER * dec = (DECODER *)ptDecHandle;

	return dec->u32BS_buf_sz_remain;
}


void
AVCDec_InvalidBS (void * ptDecHandle)
{
	DECODER * dec = (DECODER *)ptDecHandle;

	if (dec->pfnSemWait) dec->pfnSemWait (dec->pvSemaphore);
    dec->u32BS_hw_prev_offset = 0;
	dec->u32BS_buf_sz_remain = 0;
	if (dec->pfnSemSignal) dec->pfnSemSignal(dec->pvSemaphore);
}

void
AVCDec_EndOfData (void * ptDecHandle)
{
    DECODER *dec = (DECODER *)ptDecHandle;	

	dec->bBS_end_of_data = TRUE;	 
}

void
AVCDec_SetOutputAddr (void * ptDecHandle,
		uint8_t * pu8output_phy, uint8_t * pu8output_u_phy, uint8_t * pu8output_v_phy)
{
	DECODER * dec = (DECODER *)ptDecHandle;	

	dec->output_base_phy = pu8output_phy;
	dec->output_base_u_phy = pu8output_u_phy;
	dec->output_base_v_phy = pu8output_v_phy;
}

AVC_RET
AVCDec_OneFrame(void * ptDecHandle, FAVC_DEC_RESULT * ptResult)
{	
	AVC_RET ret;
	ret = decoder_decode(ptDecHandle, ptResult);
	ptResult->isISlice =  isISlice(ptDecHandle);
	return ret;
}

void AVCDec_SetCrop(void * ptDecHandle, int x, int y)
{	
	DECODER * dec = (DECODER *)ptDecHandle;

	dec->crop_x = (x+15)/16*16;
	dec->crop_y = (y+15)/16*16;
}

int32_t
AVCDec_Sync_OneFrame(void * ptDecHandle)            // KC : not called
{
	int32_t status;
	status = decoder_sync(ptDecHandle);
	
  	return status;
}

void
AVCDec_Release(void * ptDecHandle)
{  	
  	decoder_destroy(ptDecHandle);
}




