#include <stdio.h>
#include <string.h>
#include "h264dec.h"
#include "wblib.h"
#include "w55fa92_reg.h"  
//#include "h264_reg.h"
#include "userdef.h"
#include "avcdec.h"
//#include "slice.h"

#include "favc_module.h"

#include "decoder.h"

UINT32 BITPicRunOK,BITBufferEmptyOK;
//extern FAVC_DEC_RESULT _tResult;
volatile int trans_done_flag, bus_error_flag, slice_end_flag, frame_end_flag, MV_overRang_flag, BSM_empty_flag, Enc_FrameDone_flag;
extern unsigned int    h264_max_width,h264_max_height;
extern unsigned int    mb_info_phy_buffer;
extern unsigned int    intra_pred_phy_buffer;

// AVC interrupt at position 29 (vector[31:0])
#define IRQ_AVC H264D_FTMCP200_IRQ 

#define Console_Printf	sysprintf

extern volatile PFN_DRVH264_INT_CALLBACK g_pfnH264Callback[2];

DECODER *cur_dec;
int slice_done_occur;

void mcp200_sync_bs(DECODER *dec)
{
   	uint32_t u32BS_hw_cur_offset = 0;  // current HW offset after decode	
	// christie '05/10/24 bitstream sync between CPU and HW
	// only control circular buffer for now
	u32BS_hw_cur_offset = inp32(REG_264_BSM_COUNT) >>16;	// BSM_COUNT[31:16], the unit is 16 word
	u32BS_hw_cur_offset = u32BS_hw_cur_offset << 6;		// translate 16 word to byte

	if( dec->u32BS_hw_prev_offset > u32BS_hw_cur_offset) // if circular
  		dec->u32UsedBytes = u32BS_hw_cur_offset + (dec->u32BS_buf_sz - dec->u32BS_hw_prev_offset);
	else
  		dec->u32UsedBytes += (u32BS_hw_cur_offset - dec->u32BS_hw_prev_offset);

	dec->u32BS_hw_prev_offset = u32BS_hw_cur_offset;
	// remaining buffer size
	if(dec->u32UsedBytes > dec->u32BS_buf_sz_remain)
  		dec->u32BS_buf_sz_remain = 0;
	else
  		dec->u32BS_buf_sz_remain -= dec->u32UsedBytes;

	// check HW internal buffer(not SDRAM buffer) to know if decoding finished
	if(m_isEndOfDec(inp32(REG_264_STREAM_STATUS)) && dec->bBS_end_of_data)	
  		dec->bEndOfDec = TRUE;
	else
  		dec->bEndOfDec = FALSE;
}

void refresh_int_sts(DECODER *dec)
{
	uint32_t ADDR_STATUS0;
	uint32_t INTS;

		
	//ADDR_STATUS0 = inp32(REG_264_ADDR_STATUS0)>>16;	
	INTS = inp32(REG_264_INTS);	
//Console_Printf("INTS=0x%08x\n", INTS);

	if(INTS & INTS_END_SLICE) {
	
		ADDR_STATUS0 = inp32(REG_264_ADDR_STATUS0)>>16;
//Console_Printf("STATUS0=0x%x\n", ADDR_STATUS0);
		
		slice_end_flag = 1;
		
		if ( g_pfnH264Callback[0] != NULL)
				g_pfnH264Callback[0]();
		
		if(ADDR_STATUS0 == 0) {
			slice_done_occur = 1;
			dec->current_status = SLICE_DONE;
			outp32(REG_264_INTS, ((INTS | 0xFFFF) & INTS_CLEAR_SLICE));			
    	} else { //start next slice
			uint32_t MBx,MBy;
			
			outp32(REG_264_INTS, ((INTS | 0xFFFF) & INTS_CLEAR_SLICE));
			dec->current_status = PREPARE_DECODE;
			MBx=((ADDR_STATUS0>>8) & 0xff);
			MBy=((ADDR_STATUS0) & 0xff);
			dec->SliceDoneMBNum=(MBy*((int)(dec->video_width>>4)))+MBx;
			//Console_Printf ("MBx = %d, MBy=%d, dec->SliceDoneMBNum=%d\n",MBx, MBy,dec->SliceDoneMBNum );
		}
	}
    
	if((INTS & INTS_END_FRAME)) 	{
		// clear INT
		frame_end_flag = 1;
		BITPicRunOK = TRUE;
		dec->current_status = FRAME_DONE;
		
		if ( g_pfnH264Callback[1] != NULL)
				g_pfnH264Callback[1]();		
		
		outp32(REG_264_INTS, ((INTS | 0xFFFF) & INTS_CLEAR_FRAME));		
	} 
	
		
	if(INTS & INTS_BSM_BUF_EMPTY) {
		//Console_Printf("BSM_BUF_EMPTY at %d state\n", dec->current_status);	
				// Mark to skip the BSM buffer empty because only one frame bistram in buffer
				// And, it will issue BS_EMPTY before frame done in some bitstream
		//dec->current_status |= HW_BS_EMPTY;	
		
		outp32(REG_264_INTS, ((INTS | 0xFFFF) & INTS_CLEAR_BS_EMPTY));
	}

	if(INTS & (INTS_END_FRAME | INTS_BSM_BUF_EMPTY))
		mcp200_sync_bs(dec);

	if ((dec->current_status & FRAME_DONE) && (slice_done_occur != 1)) {
		// error bitstream, reset whole decoder
		Console_Printf ("Reset IP in refresh_int_sts\n");		
		outp32(REG_264_SOFT_RST, 1);
		dec->current_status = ERROR_BS;
	}
}


int decoder_int_handler(int irq, void *dev_id)
{

	if (cur_dec == NULL) {
		Console_Printf ("NULL irq for favc_decoder\n");
		// need to reset whole decoder
	}
	else
		refresh_int_sts(cur_dec);

	return 0;	
}

int32_t
decoder_create(FAVC_DEC_PARAM * ptParam, void ** pptDecHandle, unsigned char ndev)
{
	DECODER *dec;
	int idx;
	
	if ((dec = nv_malloc(sizeof(DECODER),32)) == NULL){
		printk("Can't allocate dec size 0x%x\n",sizeof(DECODER));
		return RETCODE_ERR_MEMORY;
	}

	memset(dec, 0, sizeof(DECODER));

	//sps_list
	for(idx=0; idx<MAX_SPS_NUM; idx++)
		dec->sps[idx] = NULL;
	dec->active_sps_idx = 0;
	
	//pps_list
	for(idx=0; idx<MAX_PPS_NUM; idx++)
		dec->pps[idx] = NULL;
	dec->active_pps_idx = 0;
	
	if((dec->ssh = (struct_slice_header *)nv_malloc(sizeof(struct_slice_header),32))==NULL){	
		Console_Printf("Can't allocate ssh size 0x%x\n",sizeof(struct_slice_header));
		return RETCODE_ERR_MEMORY;
	}

	memset(dec->ssh, 0, sizeof(struct_slice_header));
	if((dec->dsd = (decode_slice_data *)nv_malloc(sizeof(decode_slice_data),32))==NULL){	
		Console_Printf("Can't allocate dsd size 0x%x\n",sizeof(decode_slice_data));
		return RETCODE_ERR_MEMORY;
	}	
	memset(dec->dsd, 0, sizeof(decode_slice_data));		
	if((dec->decode_para = (decode_parameter *)nv_malloc(sizeof(decode_parameter),32))==NULL){	
		Console_Printf("Can't allocate decode_para size 0x%x\n",sizeof(decode_parameter));
		return RETCODE_ERR_MEMORY;
	}
	memset(dec->decode_para, 0, sizeof(decode_parameter));	
	
	*pptDecHandle = dec;
	dec->output_base_phy = NULL;
	dec->output_base_u_phy = NULL;
	dec->output_base_v_phy = NULL;
	
	// dedicate parameters and constraints
//	dec->pu32BaseAddr = ptParam->pu32BaseAddr;
	dec->u32MaxWidth = ptParam->u32MaxWidth;
	dec->u32MaxHeight = ptParam->u32MaxHeight;
	
	dec->u32FrameBufferWidth = ptParam->u32FrameBufferWidth;
	dec->u32FrameBufferHeight = ptParam->u32FrameBufferHeight;		
		
	dec->video_width = 0;
	dec->video_height = 0;
	//dec->u32BS_buf_sz = ptParam->u32BS_buf_sz;
	dec->u32BS_buf_sz = (h264_max_width*h264_max_height*3)/2;    //need as same as driver allocated bitstream size;	
	
	if(dec->u32BS_buf_sz > 0x400000)
		dec->u32BS_buf_sz = 0x3FFFC0; 

	// bitstream sync utility
	dec->u32BS_hw_prev_offset = 0;	
	dec->u32BS_sw_offset = 0;
	dec->u32BS_buf_sz_remain = 0;
	dec->u32BS_sw_cur_fill_size = 0;	
	dec->bBS_end_of_data = FALSE;
	dec->u32UsedBytes = 0;
	dec->bEndOfDec = FALSE;	
	
	dec->decoded_frame_num = 0;
	dec->prev_frame_num = 0;
	dec->current_status = PREPARE_DECODE;
	
	// init
	dec->forbidden_zero_bit = 0;
	dec->nal_ref_idc = 0;
	dec->nal_unit_type = 0;	
		
	// internal local memory addr
	//dec->decode_para->MB_INFO_BASE = (uint32_t)ptParam->mb_info_phy;
	//dec->decode_para->INTRA_PRED_BASE = (uint32_t)ptParam->intra_pred_phy;
	dec->decode_para->MB_INFO_BASE = (uint32_t)mb_info_phy_buffer;
    dec->decode_para->INTRA_PRED_BASE = (uint32_t)intra_pred_phy_buffer;	
		
	dec->rec_frame_yaddr = NULL;

	for(idx=MAX_REF_FRAME_NUM; idx>=0; idx--) {
		dec->ref_pic_data[idx] = NULL;
		dec->mmco_ref_is_long_term[idx] = 0;
		dec->mmco_used_for_ref[idx] = -1;
		dec->mmco_ref_pic_num[idx] = -0xFFFF;
		dec->mmco_ref_lt_pic_num[idx] = 0xFFFF;		
	}
	
#ifdef DISPLAY_REORDER_CTRL
	printk("Display Reorder is enabled\n");
	
	for(idx=MAX_DISP_FRAME_NUM-1; idx>=0; idx--)
		dec->disp_pic_data[idx] = NULL;
#endif
		
	dec->prev_rec_frame = dec->ref_pic_data[MAX_REF_FRAME_NUM];
	dec->output_fmt = ptParam->u32OutputFmt;//output_fmt;	
	dec->output_base_phy = NULL;
	dec->output_base_u_phy = NULL;
	dec->output_base_v_phy = NULL;
	
	// input bitstream addr
	if(ptParam->pu8BitStream_phy == NULL) {	
		printk("Not allocate pu8BitStream_phy size\n");
		return RETCODE_ERR_MEMORY;
	} else {
		dec->pu8BS_start_phy = ptParam->pu8BitStream_phy;
//		dec->pu8BS_start_virt = ptParam->pu8BitStream;		
	}	
		
	#ifdef DISPLAY_REORDER_CTRL
	dec->disp_pos = 0;
	dec->dpb_used_size = 1;
	dec->is_dpb_full = 0;	
	#endif
	
	cur_dec = dec;
		
	outp32(REG_264_INTS, INT_MASK_BIT);	   // Mask interrupt  
	while(inp32(REG_264_INTS)!=(unsigned int)INT_MASK_BIT) { 	    
		outp32(REG_264_INTS, INT_MASK_BIT);
	}

	// bitstream base address and length
	outp32(REG_264_BSM_BASE, (uint32_t)dec->pu8BS_start_phy);
	outp32(REG_264_BSM_BUF_LENGTH, dec->u32BS_buf_sz >> 6);	//unit:16 word

	dec->reinit_very_first_time = 1;  

	return RETCODE_OK;
}


void decoder_destroy(void * ptDecHandle)
{
	int idx;
	DECODER * dec = (DECODER *)ptDecHandle;
	struct_sps **sps_list = dec->sps;
	struct_sps *sps;	
	struct_pps **pps_list = dec->pps;
	struct_pps *pps;	
	StorablePicture **ref_pic_data = dec->ref_pic_data;
	StorablePicture *tmp_pic;
#ifdef DISPLAY_REORDER_CTRL
	DisplayPicture **disp_pic_data = dec->disp_pic_data;
	DisplayPicture *disp_pic;
#endif
	
	//pps
	for(idx=0; idx<MAX_PPS_NUM; idx++){
		pps = *(pps_list+idx);
		if(pps)
			//dec->pfnFree(pps);
			nv_free(pps);
		pps=0;
	}
	
	//sps, <init_parameters>


	for(idx=0; idx<=MAX_REF_FRAME_NUM; idx++) {
		tmp_pic = *(ref_pic_data+idx);
		if(tmp_pic) {
			if(tmp_pic->L0_ref_idx_yaddr_phy) {
				nv_free(tmp_pic->L0_ref_idx_yaddr_phy);		
				tmp_pic->L0_ref_idx_yaddr_phy = 0;		
			}		
			nv_free(tmp_pic);
			tmp_pic=0;
		}
	} //for

#ifdef DISPLAY_REORDER_CTRL
	for(idx=MAX_DISP_FRAME_NUM-1; idx>=0; idx--) {
		disp_pic = *(disp_pic_data+idx);
		if(disp_pic) {	
			if(disp_pic->disp_idx_yaddr)
				nv_free(disp_pic->disp_idx_yaddr_phy);
				
			disp_pic->disp_idx_yaddr=disp_pic->disp_idx_yaddr_phy=0;
			nv_free(disp_pic);
			disp_pic=0;
		}	
	}
#endif
	
	for(idx=0; idx<MAX_SPS_NUM; idx++) {
		sps = *(sps_list+idx);
		if(sps)
			nv_free(sps);
		sps=0;
	}
	
	dec->pu8BS_start_virt=dec->pu8BS_start_phy=0;
	nv_free(dec->decode_para);
	dec->decode_para=0;
	nv_free(dec->dsd);
	dec->dsd=0;
	nv_free(dec->ssh);
	dec->ssh=0;
	nv_free(dec);
	dec=0;

	return;
}

void decoder_reinit(void * ptDecHandle)
{
	DECODER * dec = (DECODER *)ptDecHandle;

//#ifdef _RESET_IP_
//	outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) & ~0x200);
//	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) | VDE_RST);
//	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) & ~VDE_RST);	
//	outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) | 0x200);	
//#endif
	outp32(REG_264_SOFT_RST, inp32(REG_264_SOFT_RST) | SRESET);
		

	outp32(REG_264_INTS, INT_MASK_BIT);

#ifdef _RESET_IP_	
	outp32(REG_264_BSM_BASE, (uint32_t)dec->pu8BS_start_phy);
	outp32(REG_264_BSM_BUF_LENGTH,dec->u32BS_buf_sz >> 6);	//unit:16 word	
#endif	
	// restore the saved register content for bitstream context switch
	
#ifdef _RESET_IP_	
	if(dec->reinit_very_first_time) {
		dec->reinit_very_first_time = 0;
	} else {
	    outp32(REG_264_FRAMESIZE,   dec->register_FRAMESIZE);
	    outp32(REG_264_MBINFO_BASE, dec->register_MBINFO_BASE);
	    outp32(REG_264_INTRA_BASE,  dec->register_INTRA_BASE);
	    outp32(REG_264_OFFSET_REG0, dec->register_OFFSET_REG0);
	    outp32(REG_264_OFFSET_REG1, dec->register_OFFSET_REG1);
	    outp32(REG_264_RECON_LINEOFFS, dec->register_RECON_LINEOFFS);
	    outp32(REG_264_DP_LINEOFFS, dec->register_DP_LINEOFFS);
	    outp32(REG_264_LCD_PARAM, dec->register_LCD_PARAM);	    
	    outp32(REG_264_LCD_FRAME_LINE_OFF, dec->register_LCD_FRAME_LINE_OFF);	   
	    outp32(REG_264_MISC, dec->register_MISC);	   
	    outp32(REG_264_RECON_BASE, dec->register_RECON_BASE);	   	    	    	    	    	    	    	    	    	    
	}
#endif	
	

	dec->u32BS_hw_prev_offset = 0;	
	dec->u32BS_sw_offset = 0;
	dec->u32BS_buf_sz_remain = 0;
	dec->u32BS_sw_cur_fill_size = 0;	
	dec->bBS_end_of_data = FALSE;
	dec->u32UsedBytes = 0;
	dec->bEndOfDec = FALSE;	

	return;
}

int32_t
decoder_sync(void * ptDecHandle)
{
	DECODER * dec = (DECODER *)ptDecHandle;
	int32_t status;
	
	status = dec->current_status;

	return status;
}

void decoder_fill_bs_reg(void * ptDecHandle, int size)
{
	DECODER * dec = (DECODER *)ptDecHandle;

	uint32_t bs_fill_count = 0;
   	uint32_t u32BS_final_fill_residue = 0;
	
	// christie '05/10/24 bitstream sync between CPU and HW acc	
	//the unit is 16 word= 2^6=64 bytes
	bs_fill_count = dec->u32BS_sw_cur_fill_size >> 6;
	
	if (size & 0x3F)	// Not multiply of 64
		Console_Printf(" Warning : fill bitstream not multiple of 64\n");
			
	//if((dec->u32BS_sw_cur_fill_size >0) && dec->bBS_end_of_data)
	if(dec->bBS_end_of_data) {
		u32BS_final_fill_residue = dec->u32BS_sw_cur_fill_size & (64 - 1);
		if(u32BS_final_fill_residue)  // =dec->u32BS_sw_cur_fill_size % 64
			bs_fill_count ++;
		bs_fill_count++;  // HW internal buffer issue, reserve two more unit
	}

	
	outp32(REG_264_BSM_COUNT, bs_fill_count);	// BSM_COUNT[15:0], note->can't exceed 2^22 byte	
	dec->u32BS_sw_cur_fill_size = 0;		// reset sw fill size after write register

	if(dec->bBS_end_of_data)
        outp32(REG_264_SOFT_RST, END_OF_STREAM);		
	return;
}


void decoder_reset(void * ptDecHandle)
{
    outp32(REG_264_SOFT_RST, 1);
	return;
}

// this function is use to drain write buffer
void decoder_dummy_write(void * ptDecHandle)
{
	outp32(REG_264_INTS, INT_MASK_BIT);	
	return;
}


AVC_RET decoder_decode(void * ptDecHandle, FAVC_DEC_RESULT * ptResult)
{
    DECODER * dec = (DECODER *)ptDecHandle;
    AVC_RET ret;
    struct_sps *sps;
    struct_slice_header * ssh = dec->ssh;


    uint32_t read_data  = 0;
    #ifdef DISPLAY_REORDER_CTRL
    DisplayPicture **disp_pic_data = dec->disp_pic_data;
    DisplayPicture *disp_pic;
    #endif
    decode_slice_data *dsd = dec->dsd;

	unsigned int current_tick;

    cur_dec = dec;
    
   
DECODE_NEXT_FRAME:
   
    dec->current_status=PREPARE_DECODE;
    dec->SliceDoneMBNum=0;
    
	frame_end_flag=0;
	slice_end_flag=0;     

    do  {
    
    	// KC : if (BSM_BUF_LENGTH > BSM_COUNT[BSM_INC_VALID_COUNT]), it will has no buffer empty status
    	if (dec->bBS_end_of_data)
    	{
    		if ((BSM_empty_flag == 1) || (inp32(REG_264_STREAM_STATUS) & BSB_EMPTY))
    			dec->bEndOfDec = 1;
    	}
//        if(dec->current_status & HW_BS_EMPTY)    {
        if((dec->current_status & HW_BS_EMPTY) && ((dec->current_status & FRAME_DONE) ==0))   {        
            dec->current_status &= CLEAR_BS_EMPTY;
            if(dec->bBS_end_of_data==FALSE) {
                //printk("Bitstream is not enough\n");
                return RETCODE_BS_EMPTY;
            }
			
        } else if(dec->current_status & SLICE_DONE) {
            //return RETCODE_WAITING; 
            //waiting for current slice_done or frame_done
#ifndef _USE_INT_
            refresh_int_sts(dec);
#endif            
        } else if(dec->current_status & FRAME_DONE) {
            dec->current_status = PREPARE_DECODE;
            sps = dec->sps[dec->active_sps_idx];

#ifdef DISPLAY_REORDER_CTRL
            if(store_picture_in_dpb(dec)) {
                //cp display frame to LCD buffer
                if(sps->frame_cropping_flag)
                    cropping_frame(dec);
                ptResult->isDisplayOut = 1;
            } else {				
	            ptResult->isDisplayOut = 0;
            }
#else
            if(sps->frame_cropping_flag)
                cropping_frame(dec);		
            ptResult->isDisplayOut = 1;
#endif	
			
            ptResult->u32UsedBytes = dec->u32UsedBytes;
            ptResult->bEndOfDec = dec->bEndOfDec;
            ptResult->u32FrameNum = dec->decoded_frame_num;	

            // save the register content for bitstream context switch
            dec->register_FRAMESIZE      = inp32(REG_264_FRAMESIZE);
            dec->register_MBINFO_BASE    = inp32(REG_264_MBINFO_BASE);
            dec->register_INTRA_BASE     = inp32(REG_264_INTRA_BASE);
            dec->register_OFFSET_REG0    = inp32(REG_264_OFFSET_REG0);
            dec->register_OFFSET_REG1    = inp32(REG_264_OFFSET_REG1);
            dec->register_RECON_LINEOFFS = inp32(REG_264_RECON_LINEOFFS);
            dec->register_DP_LINEOFFS    = inp32(REG_264_DP_LINEOFFS);	              
            dec->register_LCD_PARAM      = inp32(REG_264_LCD_PARAM);
            dec->register_LCD_FRAME_LINE_OFF = inp32(REG_264_LCD_FRAME_LINE_OFF);
            dec->register_MISC           = inp32(REG_264_MISC);
            dec->register_RECON_BASE     = inp32(REG_264_RECON_BASE);	        
            return RETCODE_OK;		
        } 

        else if(dec->current_status & PREPARE_DECODE) {
            // trigger encode one slice
#ifdef DISPLAY_REORDER_CTRL
            //if(ptResult->bEndOfDec) {
            if(dec->bEndOfDec) {            
                sps = dec->sps[dec->active_sps_idx];
                ptResult->isDisplayOut = flush_one_pic_from_dpb(dec);
                if(sps->frame_cropping_flag)
            	    cropping_frame(dec);
                return RETCODE_OK;
            }
#else
            if(dec->bEndOfDec) {            
                if(sps->frame_cropping_flag)
            	    cropping_frame(dec);
                return RETCODE_BS_EMPTY;
            }            
#endif
		
            do {//while(dec->current_status & PREPARE_DECODE)
               	// prepare bitstream
               	
               	
#if 1               	
               	//dbg
               	//if (dec->bBS_end_of_data ==0) 	// Some pattern may hang here if no such check
               		do {
               			if ((BSM_empty_flag) || (inp32(REG_264_STREAM_STATUS) & BSB_EMPTY))
               				break;
               		} while((inp32(REG_264_STREAM_STATUS) & RBSP_Q_FULL) ==0);
#else
               	//dbg
               	if (dec->bBS_end_of_data ==0) 	// Some pattern may hang here if no such check
					 while((inp32(REG_264_STREAM_STATUS) & RBSP_Q_FULL) ==0);
#endif 
               	
               	outp32(REG_264_ADDR_BSM_CTL,m_sys_parser_ctl(SYS_BSM_NSC) );

				while (m_isParserBusy(inp32(REG_264_ADDR_STATUS0))) ;	


               	// Avoid the Buffer empty to get wrong value
               	
		    	if (dec->bBS_end_of_data)
		    	{
		    		if (inp32(REG_264_STREAM_STATUS) & BSB_EMPTY)
		    		{
		    			break;
		    		}
		    		else if ((inp32(REG_264_INTS) & INT_BSM_BUF_EMPTY) || (BSM_empty_flag))
		    		{
			    		//return RETCODE_BS_EMPTY;
		    			//return RETCODE_OK;	// fail for display_reorder_ctl disable
			    		break;
			    	}
		    	} 
		    	  

                // Start code was found
                read_data=read_u(8);
                
                //Console_Printf("Read =0x%x\n",read_data);

                dec->forbidden_zero_bit = read_data&0x00000080;
                dec->nal_ref_idc   = read_data&0x00000060;
                dec->disposable_flag = (dec->nal_ref_idc == 0); //NALU_PRIORITY_DISPOSABLE
                dec->nal_unit_type = read_data&0x0000001F;

                switch(dec->nal_unit_type)  {
                    case 1:
                    case 5:            	  
                       	if((ret = read_nal_slice(dec))!= RETCODE_OK)    // KC : slice_header( )
                            return ret;

                       	if(ssh->first_mb_in_slice == 0)  
                       	{
                            dsd->curr_slice_nr = 0;	
                      	    // Display base address
#ifdef DISPLAY_REORDER_CTRL
                            disp_pic = *(disp_pic_data + dec->disp_pos);	
                            outp32(REG_264_DISP_Y_BASE, (uint32_t)disp_pic->disp_idx_yaddr_phy);    // word align

                            if ( dec->output_fmt == OUTPUT_FMT_YUV420) {
                                outp32(REG_264_DISP_U_BASE, (uint32_t)(disp_pic->disp_idx_yaddr_phy + dec->video_width*dec->video_height)); // word align
                                outp32(REG_264_DISP_V_BASE, (uint32_t)(disp_pic->disp_idx_yaddr_phy + dec->video_width*dec->video_height*5/4)); // word align                                 
                            }
#else
                            outp32(REG_264_DISP_Y_BASE, (uint32_t)dec->output_base_phy);    // word align                            
						   	
                            if ( dec->output_fmt == OUTPUT_FMT_YUV420 /* && (ssh->first_mb_in_slice == 0)*/) {
                                outp32(REG_264_DISP_U_BASE, (uint32_t)dec->output_base_u_phy); // word align                                
                                outp32(REG_264_DISP_V_BASE, (uint32_t)dec->output_base_v_phy); // word align                                   
                            }
#endif	
                       	}
                       	else //(ssh->first_mb_in_slice != 0)  
                       	{        
                            if(ssh->first_mb_in_slice != dec->SliceDoneMBNum){
                                Console_Printf("Slice MB Number is not continue ssh->first_mb_in_slice= %d, dec->SliceDoneMBNum = %d\n", ssh->first_mb_in_slice, dec->SliceDoneMBNum);
                                return RETCODE_ERR_HEADER;
                       	    }
                       	}
                        if (cropcom_check (dec) < 0)
                            return RETCODE_ERR_GENERAL;

                       	dec->current_status = SLICE_TRIGGER;                            
#ifdef _USE_INT_
						slice_done_occur = 0;
						current_tick=sysGetTicks(TIMER0);

                        decode_slice(dec);	
                        
                        do {
                        	
                          if (frame_end_flag)                           
                           {
	                           	//BITPicRunOK = TRUE;
	                           	frame_end_flag=0;
	                           	break;
                           } else if (slice_end_flag)
                           {
                           		slice_end_flag=0;
                           	  	break;
                           }
                                                   	
                        } while (TimeOutCheck(current_tick) ==0);
                        
                        if (TimeOutCheck(current_tick))
                        {
                         	refresh_int_sts(dec);
                        	return RETCODE_DEC_TIMEOUT;
                        }
                       			
#else
                        #ifdef EVALUATION_PERFORMANCE
                        timeframe.dec_sw_stop  = get_counter();
                        performance_count();
                        timeframe.dec_hw_start = get_counter();
                        #endif

                        decode_slice(dec);
#ifndef _DATAFLOW_               
                        do{
                        	UINT32 INTS_value;
                        	INTS_value = inp32(REG_264_INTS);
                           if ((INTS_value & INT_END_FRAME ) || (INTS_value & INT_END_SLICE))
                           		break;
                        } while(1);
#endif                        
                        refresh_int_sts(dec);                        

                        #ifdef EVALUATION_PERFORMANCE
                        timeframe.dec_hw_stop = get_counter();
                        timeframe.dec_sw_start= timeframe.dec_hw_stop;
                        #endif

#endif 

                        break;
						
                    case 7: 	// SPS
                       	if((ret = read_nal_sps(dec)) != RETCODE_OK)
                            return ret;
                        // memory issue, should be done only once a sequence
                        if ((ret = init_parameters(dec)) != RETCODE_OK)
                            return ret;
                        dec->register_FRAMESIZE      = inp32(REG_264_FRAMESIZE);
                        dec->register_MBINFO_BASE    = inp32(REG_264_MBINFO_BASE);
                        dec->register_INTRA_BASE     = inp32(REG_264_INTRA_BASE);
                        dec->register_OFFSET_REG0    = inp32(REG_264_OFFSET_REG0);
                        dec->register_OFFSET_REG1    = inp32(REG_264_OFFSET_REG1);
                        dec->register_RECON_LINEOFFS = inp32(REG_264_RECON_LINEOFFS);
                        dec->register_DP_LINEOFFS    = inp32(REG_264_DP_LINEOFFS);	              
                        dec->register_LCD_PARAM      = inp32(REG_264_LCD_PARAM);
                        dec->register_LCD_FRAME_LINE_OFF = inp32(REG_264_LCD_FRAME_LINE_OFF);
                        dec->register_MISC           = inp32(REG_264_MISC);
                        dec->register_RECON_BASE     = inp32(REG_264_RECON_BASE);	
                                        
                        ptResult->u32Width = dec->video_width; 
                        ptResult->u32Height = dec->video_height;
                
                        #ifdef FLOW_VERIFY
                        sps = dec->sps[dec->active_sps_idx]; 
                        #endif
                        ptResult->u32UsedBytes = 0;
							
                        if(( (dec->current_status & HW_BS_EMPTY) 
                         || ((dec->u32BS_sw_offset - (inp32(REG_264_BSM_COUNT) >>10))< 0x50))                          
                         && (dec->bBS_end_of_data==FALSE)) {
                            Console_Printf("ERR: This frame is not including video Data\n");
                            return RETCODE_BS_EMPTY;
                        }
                        goto DECODE_NEXT_FRAME;                        
                        //break;
         
                    case 8:	// PPS
                       	//read_nal_pps(dec);
                       	if ((ret = read_nal_pps(dec)) != RETCODE_OK)
                       		return ret;
    	                break;
            			
                    case 6:
                        if ( !read_nal_sei(dec) ) {
                            return RETCODE_ERR_HEADER;
                        }
                        break;
            				
                    case 9:
                       	read_nal_delimiter();
                       	break;
						
                    default:
                        return RETCODE_ERR_HEADER;
                } // case
            } while(dec->current_status & PREPARE_DECODE);

        }
    }while(1);
	
    //return RETCODE_WAITING;
}


int isISlice(void * ptDecHandle)
{
	DECODER *dec = (DECODER *)ptDecHandle;
    
	if(dec->ssh->slice_type==P_SLICE)
		return 0;
	else if(dec->ssh->slice_type==I_SLICE)
		return 1;
	else 
		return -1;    
}
