/* 
    sample code for H264 for pattern 720x480 input and bitstream output
    This sample code is to do encode 1000 stream frames named "/tmp/dev0.264"
    #./h264_main test.yuv
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"	
#include "nvtfat.h"
#include "w55fa92_sic.h"
#include "w55fa92_vpost.h"
#include "ratecontrol.h"

#include "favc_avcodec.h"
#include "favc_version.h"
#include "h264.h"

//#include "Misc.h"
//#include "V4L.h"

#define     READ_YUV    1
#define     WRITE_FILE  1


#define dout_name1         "encQcif.264"
#define dinfo_name        "Qcifframe_info.txt"

#define	INPUT_PATTERN_FOLDER	"C:\\h264\\"
#define	printf	sysprintf
#define	Console_Printf sysprintf

FAVC_ENC_PARAM     enc_param;


#define VIDEO_PALETTE_YUV420P_MACRO		50		/* YUV 420 Planar Macro */

INT            din, dout, dout_info;
int             favc_enc_fd=0;
int             favc_dec_fd=0;
H264RateControl h264_ratec;
static int  favc_quant1=0;
int             enc_mmap_addr;
static unsigned int    out_virt_buffer1;
int dec_BS_buf_size;


typedef unsigned long long uint64;


int h264_init(video_profile *video_setting)
{

    favc_enc_fd=H264Enc_Open();
	
    if(favc_enc_fd <= 0)
    {
        printf("Fail to open Encoder\n");
        return -1;
    }
    

    enc_mmap_addr=(int)malloc(video_setting->width*video_setting->height*3/2);
    if(enc_mmap_addr <= 0)
        return -1;
    printf("mmap addr=0x%x\n",enc_mmap_addr);    

    memset(&enc_param, 0, sizeof(FAVC_ENC_PARAM));

    enc_param.u32API_version = H264VER;
  
    enc_param.u32FrameWidth=video_setting->width;
    enc_param.u32FrameHeight=video_setting->height;
  
    enc_param.fFrameRate = video_setting->framerate;
    enc_param.u32IPInterval = video_setting->gop_size; //60, IPPPP.... I, next I frame interval
    enc_param.u32MaxQuant       =video_setting->qmax;
    enc_param.u32MinQuant       =video_setting->qmin;
    enc_param.u32Quant = video_setting->quant; //32
    enc_param.u32BitRate = video_setting->bit_rate;          
    
    enc_param.ssp_output = -1;
    enc_param.intra = -1;

    enc_param.bROIEnable = 0;
    enc_param.u32ROIX = 0;
    enc_param.u32ROIY = 0;
    enc_param.u32ROIWidth = 0;
    enc_param.u32ROIHeight = 0;
    
#ifdef RATE_CTL
    memset(&h264_ratec, 0, sizeof(H264RateControl));
    H264RateControlInit(&h264_ratec, enc_param.u32BitRate,
        RC_DELAY_FACTOR,RC_AVERAGING_PERIOD, RC_BUFFER_SIZE_BITRATE, 
        (int)enc_param.fFrameRate,
        (int) enc_param.u32MaxQuant, 
        (int)enc_param.u32MinQuant,
        (unsigned int)enc_param.u32Quant, 
        enc_param.u32IPInterval);
#endif      

    favc_quant1 = video_setting->quant;


    if (H264_ioctl(FAVC_IOCTL_ENCODE_INIT, &enc_param) < 0)
    {
		H264Enc_Close();
        printf("Handler_1 Error to set FAVC_IOCTL_ENCODE_INIT\n");
        return -1;
    }
    

    return 0;
}


int printFile(INT opFP)
{
	UINT32 length=0,i;
	INT nByteCnt;
	UINT8 cptr;
	
	for(i=0;i<6;i++)
	{
		fsReadFile(opFP, &cptr, 1, &nByteCnt);
		if ((cptr >= 0x30) && (cptr <= 0x39))
		{
			length = length * 10 + (cptr-0x30);
		}	
		else
		{
			if (cptr == 0x0D)
			{
				fsReadFile(opFP, &cptr, 1, &nByteCnt);	// flush 0x0A
			}
			break;
		}	
	}	
	return length;	
}

int enc_close(video_profile *video_setting)
{

   	if(enc_mmap_addr)
   	{
    		free((void *)enc_mmap_addr);  
    		enc_mmap_addr = 0;
    }		
    		    
    if(favc_enc_fd) {
		H264Enc_Close();
      
        favc_enc_fd = 0;        
     }
    
    if (favc_dec_fd)
    {
        //close(favc_dec_fd);
        favc_dec_fd = 0;
        
    }  
   
    return 0;
}

int favc_encode(int fileDecriptor, video_profile *video_setting, unsigned char *frame, void * data, int bRateControl)
{
    AVFrame             *pict=(AVFrame *)data;

    enc_param.pu8YFrameBaseAddr = (unsigned char *)pict->data[0];   //input user continued virtual address (Y), Y=0 when NVOP
    enc_param.pu8UFrameBaseAddr = (unsigned char *)pict->data[1];   //input user continued virtual address (U)
    enc_param.pu8VFrameBaseAddr = (unsigned char *)pict->data[2];   //input user continued virtual address (V)

    enc_param.bitstream = frame;  //output User Virtual address   
    enc_param.ssp_output = -1;
    enc_param.intra = -1;
    enc_param.u32IPInterval = 0; // use default IPInterval that set in INIT


    enc_param.u32Quant = favc_quant1;    
        
    enc_param.bitstream_size = 0;

    if (H264_ioctl(FAVC_IOCTL_ENCODE_FRAME, &enc_param) < 0)
    {
        printf("Dev =%d Error to set FAVC_IOCTL_ENCODE_FRAME\n", fileDecriptor);
        return 0;
    }

#ifdef RATE_CTL
    if (bRateControl)
    {
        if (enc_param.keyframe == 0) {
            //printf("%d %d %d\n", enc_param.u32Quant, enc_param.bitstream_size, 0);
            H264RateControlUpdate(&h264_ratec, enc_param.u32Quant, enc_param.bitstream_size , 0);
        } else  {
            //printf("%d %d %d\n", enc_param.u32Quant, enc_param.bitstream_size, 1);
            H264RateControlUpdate(&h264_ratec, enc_param.u32Quant, enc_param.bitstream_size , 1);
        }
        favc_quant1 = h264_ratec.rtn_quant;
        
        printf("Quant= %d\n",favc_quant1);
    }
#endif

    video_setting->intra = enc_param.keyframe;

    return enc_param.bitstream_size;
}

INT openFile(char *filename, UINT32 op )
{
	char srcFile[512],suDirName[512];
	INT openFP=0;
	
    strcpy(srcFile,INPUT_PATTERN_FOLDER);   
    strcat(srcFile, filename);     
    fsAsciiToUnicode(srcFile,suDirName,1);
    openFP = fsOpenFile(suDirName, srcFile, op);	
    
    if (openFP >0 )
    	fsFileSeek(openFP,0,SEEK_SET);
    
    return openFP;	
}

int main(void)
{
	UINT32 u32ExtFreq;
    int tlength=0;
    int i, length, fcount=0;
    video_profile   video_setting;
    AVFrame pict;
    unsigned int y_image_size, uv_image_size, total_image_size;
    int readbyte, writebyte; 
	int bitrate;
	
	int SD_clock; 	
	WB_UART_T uart;	
	
	char ascii[256];	
	
	u32ExtFreq = sysGetExternalClock();
		
	uart.uart_no = WB_UART_1;
	uart.uiFreq = u32ExtFreq;				
	uart.uiBaudrate = 115200;
	uart.uiDataBits = WB_DATA_BITS_8;
	uart.uiStopBits = WB_STOP_BITS_1;
	uart.uiParity = WB_PARITY_NONE;
	uart.uiRxTriggerLevel = LEVEL_1_BYTE; 

	sysInitializeUART(&uart);
	
	sysSetTimerReferenceClock (TIMER0, u32ExtFreq);
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);	
		
	SD_clock = sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock())/1000;
    sicIoctl(SIC_SET_CLOCK, SD_clock, 0, 0);       // clock from FPGA clock in		
    //fmiSD_Set_clock(200);

	fsInitFileSystem();
	fmiInitDevice(); 
	if (sicSdOpen0() <=0)
	{
		Console_Printf("Error in initialize SD card !!\n");
		while(1);
	}	

    printf("h264enc main\n");

#if 0
    if(argc < 2)
    {
        printf("usage : h264enc bitrate (in decimal)\n");
        exit(0);
    }    
        
     bitrate = atoi(argv[1]);
     printf("bitrate = %d\n",bitrate);
#else
	bitrate = 512000;
#endif     

#if READ_YUV
    din = openFile("pattern\\foreman_qcif_2d.yuv", O_RDONLY);
    printf("input file din=%d\n",din);
#endif    

    dout=openFile(dout_name1,O_CREATE);
    printf("Use encoder output name %s\n",dout_name1);
    
    dout_info = openFile(dinfo_name,O_CREATE);
    printf("Use encoder output info_name %s\n",dinfo_name);

    //set the default value
    video_setting.qmax = 51;
    video_setting.qmin = 0;
    video_setting.quant = 25;   
#ifdef RATE_CTL            
    video_setting.bit_rate = bitrate;
#else    
    video_setting.bit_rate = FIX_QUANT;
#endif  
    video_setting.width = 176;  
    video_setting.height = 144;
    video_setting.framerate = 30;
    video_setting.frame_rate_base = 1;
    video_setting.gop_size = IPInterval;

    y_image_size = video_setting.width*video_setting.height;
    uv_image_size = y_image_size >> 1;
    total_image_size = y_image_size + uv_image_size;
    
    out_virt_buffer1 = (unsigned int)malloc(total_image_size);
    if (!out_virt_buffer1)
        goto file_close;    
    

    if (h264_init(&video_setting) < 0) 
    {
        printf("H.264 init fail\n");
        goto fail_h264_init;
    }        

    
    pict.data[0]=(unsigned char *)enc_mmap_addr;
    pict.data[1]=(unsigned char *)(enc_mmap_addr + y_image_size);
    pict.data[2]=(unsigned char *)(enc_mmap_addr + y_image_size +(y_image_size >> 2));    


    fcount = TEST_ROUND;
    for (i=0; i < fcount; i++)
    {
        // Read next QCIF H.264_2D source format   
        fsReadFile(din, (UINT8 *)(enc_mmap_addr | CACHE_BIT31), total_image_size,&readbyte);             
        printf("Read byte =%d\n",readbyte);
        
        if (readbyte > 0)
        {
            length = favc_encode(favc_enc_fd, &video_setting,(unsigned char *)out_virt_buffer1,(void *)&pict, 1);
            //printf("encode length = %d\n",length); 
            if (length == 0)
                break;
#if WRITE_FILE      
            fsWriteFile(dout, (UINT8 *)out_virt_buffer1, length, &writebyte);        

			//itoa(length, ascii, 10);
			sprintf(ascii,"%d\n",length);
            fsWriteFile(dout_info, (UINT8 *)ascii, strlen(ascii), &writebyte);  			            
#endif            
			tlength += length;
        }
       
    }

    printf("Total Frame %d encode Done, total bitstream = %d \n", i, tlength);

fail_h264_init:
       
         
file_close:

	if (out_virt_buffer1)
	{
		free((void *)out_virt_buffer1);
		out_virt_buffer1 = 0;
	}		

	if (din > 0)
    fsCloseFile(din);
 
 	if (dout > 0)
	    fsCloseFile(dout);
	  
	if (dout_info > 0)  
	    fsCloseFile(dout_info);	 
    
    enc_close(&video_setting);
    
    sysprintf("H264 Enc done\n");
            
	while(1);            
    //return 0;
}
