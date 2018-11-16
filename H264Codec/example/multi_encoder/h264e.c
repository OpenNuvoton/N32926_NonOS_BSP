/* 
    sample code for H264 encoder to encode multi-resolution in the same time and bitstream output
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


#define     WRITE_FILE  1


#define	INPUT_PATTERN_FOLDER	"C:\\h264\\pattern\\"
#define	OUTPUT_PATTERN_FOLDER	"C:\\h264\\"

#define	printf	sysprintf
#define	Console_Printf sysprintf

FAVC_ENC_PARAM     enc_param;

// Some field of below structure must be filled by programmer
// The others are filled by driver and keep the default value as 0
typedef struct ENCODE_INFO {
	int DrvHandle;	
	int YUVFileHandle;
	int BSFileHandle;
	int ImgWidth;				// must
	int ImgHeight;				// must
	int FrameRate;				// must
	int GOPSize;				// must
	int MaxQuant;				// must : Max is 51 and can be changed by programmer
	int MinQuant;				// must : Min is 0 and can be changed by programmer
	int CurrntQuant;			// must : Quant for first frame and can be changed by programmer
	int BitRate;				// must 
	int YUVRawBuffer;
	int BitStreamBuf;
	H264RateControl *rate_control;
} enode_info;

// Below encode information must match with the file declared in fileName[4][]
enode_info Enc_File[MAX_INSTANCE] = {
	{0, 0, 0, 176, 144, 30,  15, 51, 0, 25, 128000, 0, 0, 0},
	{0, 0, 0, 320, 240, 30,  20, 51, 0, 25, 256000, 0, 0, 0},		
//	{0, 0, 0, 352, 288, 30,  10, 51, 0, 25, 256000, 0, 0, 0},		
//	{0, 0, 0, 640, 480, 30,  30, 51, 0, 25, 512000, 0, 0, 0},					
};
	
// Below H.264_MacroMode raw data file must exist in C:\h264\Pattern folder of SD card
// The encoded bitstream will output to SD card C:\h264 folder with the file name xxx.h264
// For example, QVGA_2d.h264 is the encoded bitstream of QVGA_2d.yuv file.
char fileName[MAX_INSTANCE][256] = {
	{"foreman_qcif_2d.yuv"},	// resolution size: 176*144 --> match with Enc_File[0]
	{"QVGA_2D.yuv"},			// resolution size: 320*240 --> match with Enc_File[1]
//	{"CIF_2d.yuv"},				// resolution size: 352*288 --> match with Enc_File[2]	
//	{"VGA_2D.yuv"},				// resolution size: 640*480 --> match with Enc_File[3]
	
};


typedef unsigned long long uint64;


int h264_init(int instance)
{
	int i;
	int YUVRawDataBuf;	
	
    for(i=0;i<instance;i++)
    {
    
    	if ( Enc_File[i].DrvHandle == 0)
	    	Enc_File[i].DrvHandle=H264Enc_Open();
		
	    if(Enc_File[i].DrvHandle < 0)
	    {
	        printf("Fail to open Encoder\n");
	        return -1;
	    }

		// Allocate YUV raw Data Buffer
	    YUVRawDataBuf=(int)malloc(Enc_File[i].ImgWidth * Enc_File[i].ImgHeight *3/2);
	    if(YUVRawDataBuf <= 0)
	        return -1;
	    printf("Raw data Buffer addr=0x%x\n",YUVRawDataBuf);  
	    
	    Enc_File[i].YUVRawBuffer =  YUVRawDataBuf;
	     
		// Allocate Bit Straem Buffer
	    YUVRawDataBuf=(int)malloc(Enc_File[i].ImgWidth * Enc_File[i].ImgHeight *3/2);
	    if(YUVRawDataBuf <= 0)
	        return -1;
	    printf("Bit Stream Buf addr=0x%x\n",YUVRawDataBuf);  
	    
	    Enc_File[i].BitStreamBuf =  YUVRawDataBuf;

	    memset(&enc_param, 0, sizeof(FAVC_ENC_PARAM));

	    enc_param.u32API_version = H264VER;
	  
	    enc_param.u32FrameWidth =	Enc_File[i].ImgWidth;
	    enc_param.u32FrameHeight =	Enc_File[i].ImgHeight;
	  
	    enc_param.fFrameRate = 		Enc_File[i].FrameRate;
	    enc_param.u32IPInterval =  	Enc_File[i].GOPSize;
	    enc_param.u32MaxQuant =		Enc_File[i].MaxQuant;
	    enc_param.u32MinQuant  =	Enc_File[i].MinQuant;
	    enc_param.u32Quant = 		Enc_File[i].CurrntQuant; 
	    enc_param.u32BitRate = 		Enc_File[i].BitRate;          
	    
	    enc_param.ssp_output = -1;
	    enc_param.intra = -1;

	    enc_param.bROIEnable = 0;
	    enc_param.u32ROIX = 0;
	    enc_param.u32ROIY = 0;
	    enc_param.u32ROIWidth = 0;
	    enc_param.u32ROIHeight = 0;
	    
    
#ifdef RATE_CTL
	    Enc_File[i].rate_control =(H264RateControl *)malloc(sizeof(H264RateControl));
	    
	    if ((int)(Enc_File[i].rate_control) <= 0)
	    {
	    	printf("Memory allocate fail\n");
	    	return -1;
	    }
	    	
	    memset(Enc_File[i].rate_control, 0, sizeof(H264RateControl));
	    H264RateControlInit(Enc_File[i].rate_control, enc_param.u32BitRate,
	        RC_DELAY_FACTOR,RC_AVERAGING_PERIOD, RC_BUFFER_SIZE_BITRATE, 
	        (int)enc_param.fFrameRate,
	        (int) enc_param.u32MaxQuant, 
	        (int)enc_param.u32MinQuant,
	        (unsigned int)enc_param.u32Quant, 
	        enc_param.u32IPInterval);
#endif      

	    if (H264_ioctl_ex(Enc_File[i].DrvHandle, FAVC_IOCTL_ENCODE_INIT, &enc_param) < 0)
	    {
			H264Enc_Close_ex(Enc_File[i].DrvHandle);
	        printf("Handler_1 Error to set FAVC_IOCTL_ENCODE_INIT\n");
	        return -1;
	    }
    
	}
    return 0;
}


int enc_close(void)
{
	int i;
	
	for(i=0;i<MAX_INSTANCE;i++) {

		if(Enc_File[i].YUVFileHandle)
			fsCloseFile(Enc_File[i].YUVFileHandle);
			
		if(Enc_File[i].BSFileHandle)
			fsCloseFile(Enc_File[i].BSFileHandle);
						
	   	if(Enc_File[i].YUVRawBuffer)
	   	{
	    	free((void *) Enc_File[i].YUVRawBuffer);  
	    	Enc_File[i].YUVRawBuffer = 0;
	    }	
	   	if(Enc_File[i].BitStreamBuf)
	   	{
	    	free((void *) Enc_File[i].BitStreamBuf);  
	    	Enc_File[i].BitStreamBuf = 0;
	    }		    
	   	if(Enc_File[i].rate_control)
	   	{
	    	free((void *) Enc_File[i].rate_control);  
	    	Enc_File[i].rate_control = 0;
	    }	    	
	    		    
	    if( Enc_File[i].DrvHandle) {
			H264Enc_Close_ex(Enc_File[i].DrvHandle);
	      
	        Enc_File[i].DrvHandle = 0;        
	     }
 
    }
   
    return 0;
}

int favc_encode(int bRateControl)
{
    int total_image_size, readbyte, y_image_size;
    int fcount, i,j, writebyte;
    

    fcount = TEST_ROUND;
    for (j=0; j < fcount; j++)
    {
	    for(i=0;i<MAX_INSTANCE;i++)
	    {
	        y_image_size = Enc_File[i].ImgWidth *  Enc_File[i].ImgHeight;
	    	total_image_size =  y_image_size * 3 / 2;
	    
		    fsReadFile(Enc_File[i].YUVFileHandle, (UINT8 *)( Enc_File[i].YUVRawBuffer | CACHE_BIT31), total_image_size,&readbyte);   
	    
	    	if (readbyte == total_image_size)
	    	{
			    enc_param.pu8YFrameBaseAddr = (unsigned char *) Enc_File[i].YUVRawBuffer;   //input user continued virtual address (Y), Y=0 when NVOP
    			enc_param.pu8UFrameBaseAddr = (unsigned char *)(Enc_File[i].YUVRawBuffer + y_image_size);   // (U)
			    enc_param.pu8VFrameBaseAddr = (unsigned char *)(Enc_File[i].YUVRawBuffer + y_image_size +(y_image_size >> 2) );  // (V)			    

			    enc_param.bitstream =  (void *)Enc_File[i].BitStreamBuf;   
			    enc_param.ssp_output = -1;
			    enc_param.intra = -1;
			    enc_param.u32IPInterval = 0; 


			    enc_param.u32Quant = Enc_File[i].CurrntQuant;    
			        
			    enc_param.bitstream_size = 0;

			    if (H264_ioctl_ex(Enc_File[i].DrvHandle, FAVC_IOCTL_ENCODE_FRAME, &enc_param) < 0)
			    {
			        printf("Dev =%d Error to set FAVC_IOCTL_ENCODE_FRAME\n", Enc_File[i].DrvHandle);
			        return 0;
			    }


			    if (bRateControl)
			    {
			        if (enc_param.keyframe == 0) {
			            //printf("%d %d %d\n", enc_param.u32Quant, enc_param.bitstream_size, 0);
			            H264RateControlUpdate(Enc_File[i].rate_control, enc_param.u32Quant, enc_param.bitstream_size , 0);
			        } else  {
			            //printf("%d %d %d\n", enc_param.u32Quant, enc_param.bitstream_size, 1);
			            H264RateControlUpdate(Enc_File[i].rate_control, enc_param.u32Quant, enc_param.bitstream_size , 1);
			        }
			         Enc_File[i].CurrntQuant = Enc_File[i].rate_control->rtn_quant;
			        
			        printf("Index =%d, Frame = %d, Quant= %d\n", i,j, Enc_File[i].CurrntQuant);
			    }

	            fsWriteFile(Enc_File[i].BSFileHandle, (UINT8 *)Enc_File[i].BitStreamBuf,  enc_param.bitstream_size, &writebyte);        

			}	// if (readbyte == total_image_size)	    
	    }	// for(i=0;i<instance;i++)
    }	//for (i=0; i < fcount; i++)
    
    return 0;
}

INT openFile(char *filename, UINT32 op )
{
	char srcFile[512],suDirName[512];
	INT openFP=0;
	
	 if (op == O_CREATE )
		 strcpy(srcFile,OUTPUT_PATTERN_FOLDER);  	 
	 else
		 strcpy(srcFile,INPUT_PATTERN_FOLDER);   
	 strcat(srcFile, filename);     
	 fsAsciiToUnicode(srcFile,suDirName,1);
	 openFP = fsOpenFile(suDirName, srcFile, op);	
	    
	 if (openFP >0 )
	   fsFileSeek(openFP,0,SEEK_SET);
	    
    return openFP;	
}



int GetFileHandle(void)
{
	char OpenFile[512];
   int i, file_length;
   
    // Open File   
    for(i=0;i<MAX_INSTANCE;i++)
    {
    	if (fileName[i] != "")
    	{
	    	file_length = strlen(fileName[i]);
	        strcpy(OpenFile,fileName[i]);  
	        Enc_File[i].YUVFileHandle = openFile(OpenFile, O_RDONLY);
	        
   	
 	        if (Enc_File[i].YUVFileHandle<=0) 
 	        {
	            printf("Open YUV Raw data file %s fail\n", fileName[i]);
	            return -1;
			}	            
		            
            strcpy(&OpenFile[file_length-3],"h264");
            Enc_File[i].BSFileHandle = openFile(OpenFile, O_CREATE);

 	        if (Enc_File[i].BSFileHandle<=0) 
 	        {
	            printf("Open Bitstream file %s fail\n", OpenFile);            
	            return -1;
			}	            
	            
		}            
    } 

return 0;    
}

int main(void)
{
	UINT32 u32ExtFreq;
	int SD_clock; 	
	WB_UART_T uart;	
	
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
    sicIoctl(SIC_SET_CLOCK, SD_clock, 0, 0);       	


	fsInitFileSystem();
	fmiInitDevice(); 
	if (sicSdOpen0() <=0)
	{
		Console_Printf("Error in initialize SD card !!\n");
		while(1);
	}	

    printf("h264enc main\n");

	// Open YUV and Bitstream file handle
	if (GetFileHandle() < 0)
		goto fail_h264_init;

	// Initialize H.264
    if (h264_init(MAX_INSTANCE) < 0) 
    {
        printf("H.264 init fail\n");
        goto fail_h264_init;
    }        

	// Encode Bitstream
    favc_encode(1);

fail_h264_init:
       
         
    enc_close();
    
    sysprintf("H264 Enc done\n");
            
	while(1);            

}
