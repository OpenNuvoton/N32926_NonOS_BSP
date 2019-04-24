/* 
    Assume the bitstream size is known in file "test.info", 720x480 resolution, 10 frames
    sample code for H264 for bitstream input and YUV output
    This sample code is to do decode 100 rounds,10 frames/round, named "/tmp/dev0.yuv"
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"	
#include "nvtfat.h"
#include "w55fa92_sic.h"
#include "w55fa92_vpost.h"
#include "w55fa92_vpe.h"

#include "favc_avcodec.h"
#include "favc_version.h"
#include "h264.h"

#define	MAX_INSTANCE	4	// availabe value 1 ~ 4

#define  BS_FOLDER  "../pattern/"
#define  INFO_FOLDER "../info/"
#define MAX_FILE_NAME_LEN       	514

#define DECODE_OUTPUT_PACKET_YUV422 0
#define MAX_IMG_WIDTH                   1280
#define MAX_IMG_HEIGHT                  720

#define DISPLAY_MODE_CBYCRY	4
#define DISPLAY_MODE_YCBYCR	5
#define DISPLAY_MODE_CRYCBY	6

#define IOCTL_LCD_GET_DMA_BASE      _IOR('v', 32, unsigned int *)
#define VIDEO_FORMAT_CHANGE			_IOW('v', 50, unsigned int)	//frame buffer format change

#define IOCTL_LCD_ENABLE_INT		_IO('v', 28)
#define IOCTL_LCD_DISABLE_INT		_IO('v', 29)

#define dout_name         ".//dev0.yuv"

#define	OUTPUT_FILE		0

#define	INPUT_PATTERN_FOLDER	"C:\\h264\\"
#define	Console_Printf	sysprintf
#define printf sysprintf

#if DECODE_OUTPUT_PACKET_YUV422
#define LCM_WIDTH 	640
#define LCM_HEIGHT 	480
#else
#define LCM_WIDTH 	320
#define LCM_HEIGHT 	240
#endif

int fb_fd=0;
UINT32 g_u32VpostWidth, g_u32VpostHeight,fb_bpp;
unsigned int fb_paddress;


char fileName[4][256];
char infoFileName[4][256]; 
char file_info[256],filename[256];

int frame_info[4][FRAME_COUNT+1];

INT            din[4]={0,0,0,0},din_info[4]={0,0,0,0};
INT            dout;
int             favc_dec_fd[4]={0,0,0,0};
int             dec_mmap_addr;
unsigned int    BS_buffer[4];


typedef struct {
    unsigned int size;
    unsigned int addr;
} _BUF_INFO;

_BUF_INFO outputbuf[4];

unsigned char *pVBitStreamBuffer = NULL,*pVBitStartBuffer;
int bitstreamsize;
int decoded_img_width, decoded_img_height;
int output_vir_buf=0;


// VPE
extern int vpe_fd;
extern int InitVPE(void);
extern int FormatConversion(void* data, char* pDstBuf, int SrcWidth, int SrcHeight);
extern INT32 VPE_trigger(void);
extern INT32 VPE_entry(void);


int InitFB()
{
	LCDFORMATEX lcdFormat;
	
    dec_mmap_addr=(int)malloc(LCM_WIDTH*LCM_HEIGHT*2); 
    
    if(dec_mmap_addr <= 0)
    {
        printf("Fail to allocate mmap address\n");
        return -1;
    }    
    
	lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;	
	lcdFormat.nScreenWidth = LCM_WIDTH;
	lcdFormat.nScreenHeight = LCM_HEIGHT;
	vpostLCMInit(&lcdFormat, (UINT32*)dec_mmap_addr);
    return 0;
}


int h264_init(video_profile *video_setting, int instance)
{

    FAVC_DEC_PARAM      tDecParam;
    int                 ret_value,i;
    
    for(i=0;i<instance;i++)
    {
    	if (din[i] != 0)
    	{
	        if(favc_dec_fd[i]==0)
	          favc_dec_fd[i]=H264Dec_Open();
	    	
	        if(favc_dec_fd[i] < 0)
	        {
	            printf("Fail to open at instance %d\n", i);
	            return -1;
	        }
	        
	    	
	        memset(&tDecParam, 0, sizeof(FAVC_DEC_PARAM));
	        tDecParam.u32API_version = H264VER;
	        tDecParam.u32MaxWidth = video_setting->width;
	        tDecParam.u32MaxHeight = video_setting->height;
	   
	    	// For file output or VPE convert planr to packet format
	        tDecParam.u32FrameBufferWidth = video_setting->width;
	        tDecParam.u32FrameBufferHeight = video_setting->height;
	    	tDecParam.u32OutputFmt = 0; // 1->Packet YUV422 format, 0-> planar YUV420 foramt
	    
	        ret_value = H264_ioctl_ex(favc_dec_fd[i],FAVC_IOCTL_DECODE_INIT,&tDecParam);		
	        if(ret_value < 0)
	        {
	            printf("FAVC_IOCTL_DECODE_INIT: memory allocation failed\n");
	            return -1;
	        }

	        // allocated output buffer for each instance
	    	outputbuf[i].addr =(int)malloc(MAX_IMG_WIDTH*MAX_IMG_HEIGHT*3); 
			if (outputbuf[i].addr ==0)
		    	return -1;
			outputbuf[i].size = MAX_IMG_WIDTH*MAX_IMG_HEIGHT*3 / 2; 
		}
    }


    return 0;
}


int h264_close(video_profile *video_setting)
{
	int i;
	
	for(i=0;i<MAX_INSTANCE;i++)
    if(favc_dec_fd[i]) {
		H264Dec_Close_ex(favc_dec_fd[i]);
		favc_dec_fd[i] = 0;
    }
    
     
    return 0;
}


int favc_decode(video_profile *video_setting, unsigned char *frame, void *data, int size, int instance)
{

    AVFrame             *pict=(AVFrame *)data;
    FAVC_DEC_PARAM      tDecParam;
    int  ret_value;
    
    // set display virtual addr (YUV or RGB)
//YUV422 needn't assign U,V
    memset(&tDecParam, 0, sizeof(FAVC_DEC_PARAM));
    tDecParam.pu8Display_addr[0] = (unsigned int)pict->data[0];
    tDecParam.pu8Display_addr[1] = (unsigned int)pict->data[1];
    tDecParam.pu8Display_addr[2] = (unsigned int)pict->data[2];
    tDecParam.u32Pkt_size =	(unsigned int)size;
    tDecParam.pu8Pkt_buf = frame;

    tDecParam.crop_x = 0;
    tDecParam.crop_y = 0;
    
	tDecParam.u32OutputFmt = 1; // 1->Packet YUV422 format, 0-> planar YUV420 foramt
	

    if((ret_value = H264_ioctl_ex(favc_dec_fd[instance], FAVC_IOCTL_DECODE_FRAME, &tDecParam)) != 0)    
    {
        printf("FAVC_IOCTL_DECODE_FRAME: Failed.ret=%x\n",ret_value);
        return -1;
    }


    decoded_img_width = tDecParam.tResult.u32Width;
    decoded_img_height = tDecParam.tResult.u32Height;    

	if (tDecParam.tResult.isDisplayOut ==0)
		return 0;
	else
    	return tDecParam.got_picture;
}

INT openFile(char *filename, UINT32 op )
{
	char srcFile[256],suDirName[256];
	INT openFP=0;
	
    strcpy(srcFile,INPUT_PATTERN_FOLDER);   
    strcat(srcFile, filename);     
    fsAsciiToUnicode(srcFile,suDirName,1);
    openFP = fsOpenFile(suDirName, srcFile, op);	
    
    if (openFP >0 )
    	fsFileSeek(openFP,0,SEEK_SET);
    
    return openFP;	
}

int scanFile(INT opFP)
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


int DecodeH264(void)
{
    int  i,k;
    video_profile  video_setting;
    AVFrame  pict[4][2], *pict_ptr;
    int result,flushCount=0;
    int toggle_flag[4]={0,0,0,0};
    int readByte;
    int vpost_offset;    

    sysFlushCache(D_CACHE);
      
    // Open Bitstream 1 ~ 4   
    for(i=0;i<MAX_INSTANCE;i++)
    {
    	if (fileName[i] != "")
    	{
	        strcpy(filename,fileName[i]);  
	        din[i] = openFile(filename, O_RDONLY);
	        
	        if (din[i] < 0)
	        	break;
	        	
	        strcpy(filename,infoFileName[i]);
	        din_info[i] = openFile(filename,O_RDONLY);    
	        
	        if ((din[i]<=0) || (din_info[i]<=0))
	            printf("Open bitstream1 %s fail\n", fileName[i]);
	        else    
	            printf("Bistream %d open =%d, inform=%d\n",i, din[i],din_info[i]);
		}            
    } 

    //set the default value
    video_setting.width = (unsigned int)-1;
    video_setting.height = (unsigned int)-1;
   
    memset(frame_info, 0, sizeof(frame_info));
    
    // Open Bitstream info 1 ~ 4 
    for(k=0;k<MAX_INSTANCE;k++)
    {
        for(i=0; i< FRAME_COUNT; i++)
        {
        	if (din_info[k] > 0)
        	{
	            frame_info[k][i] = scanFile(din_info[k]);
	            if (frame_info[k][i] == 0)
	                break;
            }    
        }
    }
       
   	// Assume to allocate 4 bistream buffer size for it.
    for(i=0;i<MAX_INSTANCE;i++)
    {
        BS_buffer[i] = (unsigned int)nv_malloc(MAX_IMG_WIDTH*MAX_IMG_HEIGHT*3/2,32);      
        if (!BS_buffer[i])
        {
            printf("memory allocate fail\n");
            goto file_close;
        }    
    }    

    if (h264_init(&video_setting,MAX_INSTANCE) < 0)
        goto fail_h264_init;

  
    for(k=0;k<MAX_INSTANCE;k++)
    {
       fsFileSeek(din[k], 0, SEEK_SET);
       
       pict[k][0].data[0] = (unsigned char *)outputbuf[k].addr;
       pict[k][0].data[1] = (unsigned char *)(outputbuf[k].addr+ (MAX_IMG_WIDTH*MAX_IMG_HEIGHT));
       pict[k][0].data[2] = (unsigned char *)(outputbuf[k].addr+ (MAX_IMG_WIDTH*MAX_IMG_HEIGHT*5/4));
                
       pict[k][1].data[0] = (unsigned char *)outputbuf[k].addr + (outputbuf[i].size/2) ;
       pict[k][1].data[1] = (unsigned char *)(outputbuf[k].addr+ (outputbuf[i].size/2) + (MAX_IMG_WIDTH*MAX_IMG_HEIGHT));
       pict[k][1].data[2] = (unsigned char *)(outputbuf[k].addr+ (outputbuf[i].size/2) + (MAX_IMG_WIDTH*MAX_IMG_HEIGHT*5/4));        

    }

    for (i=0; i<FRAME_COUNT; i++)
    //for (i=0; i<30; i++)    
    {

        
        for(k=0;k<MAX_INSTANCE;k++)
        {
            if (frame_info[k][i] == 0)
            {
                continue;
            }     
               
           //read bitstream to memory  
            fsReadFile( din[k], (UINT8 *)BS_buffer[k], frame_info[k][i], &readByte);
               
        }                     
          
         
        for (k=0;k<MAX_INSTANCE;k++)
        {
	        // Decode Bitstream 1 ~ 4
	        if (frame_info[k][i] > 0)
	        {
	            if (toggle_flag[k])
	            {
	                pict_ptr = &pict[k][1];
	                toggle_flag[k] = 0;
	            }            
	            else
	            {
	                 pict_ptr = &pict[k][0];     
	                 toggle_flag[k] = 1;               
	            }            

	            result = favc_decode(&video_setting, (unsigned char *)(BS_buffer[k]), (void *)pict_ptr, frame_info[k][i],k);        
	            if (result < 0)
	            {
	               printf("frame %d/(0~%d) decode FAIL!\n", i, FRAME_COUNT-1);
	               goto fail_h264_init;
	            }
	            else if (result == 0)
	               	flushCount++;
	                    
	            switch(k)
	            {
	             	case 0:
	             		vpost_offset = 0;
	             		break;
	             	case 1:
		             	vpost_offset = LCM_WIDTH;
	             		break;
	             	case 2:
		             	vpost_offset = LCM_WIDTH * LCM_HEIGHT;
	             		break;
	             	case 3:
		             	vpost_offset = LCM_WIDTH * LCM_HEIGHT + LCM_WIDTH;
	             		break;			
	            }
	                    
	            FormatConversion((void *)pict_ptr, (char *)dec_mmap_addr+vpost_offset, decoded_img_width, decoded_img_height);  
	    
	        }        
        }  
    }


    printf("Total frame %d decode OK!\n", i);

fail_h264_init:

    h264_close(&video_setting);

    for(i=0;i<MAX_INSTANCE;i++)
    {
    	if (BS_buffer[i] != 0)
    	{
        	free((void *)BS_buffer[i]);
        	BS_buffer[i]=0;
		}        	
	}        
    
file_close:
    for(k=0;k<MAX_INSTANCE;k++)
        if (din[k])
            fsCloseFile(din[k]);
            
    for(k=0;k<MAX_INSTANCE;k++) 
        if (din_info[k])
            fsCloseFile(din_info[k]);            
            
    return 0;    
}
 

int GetBSFileName(void)
{
   
    int file_length;    
    int filecount=0;
    
    INT         nStatus;
	CHAR		szLongName[MAX_FILE_NAME_LEN];	
	CHAR		suDirName[256],fullPathName[256];	    
	FILE_FIND_T tFileInfo;    
    
	memset((CHAR *)&tFileInfo, 0, sizeof(FILE_FIND_T));    
    
    strcpy(fullPathName,INPUT_PATTERN_FOLDER); 
    strcat(fullPathName,"pattern\\");
	fsAsciiToUnicode(fullPathName, suDirName, 1);  
	nStatus = fsFindFirst(suDirName,NULL,&tFileInfo);    
	
	if (nStatus < 0)
	{
		sysprintf("No file found in %s folder\n", fullPathName);
		return -1;
	}    
    
	do 
	{
		if (tFileInfo.ucAttrib & FA_ARCHIVE)  
        {		
        	// Found 4 C:\h264\patter\*.264 or *.jsv bitstream
    		fsUnicodeToAscii(tFileInfo.suLongName, szLongName, 1);
            strcpy(fullPathName,"pattern\\");    		
            strcat(fullPathName,szLongName); 
            
            strcpy(infoFileName[filecount],"info\\");
            strcat(infoFileName[filecount],szLongName);
	        file_length = strlen(infoFileName[filecount]);                         
            
            if ((strcmp(&infoFileName[filecount][file_length-3],"264") ==0) || (strcmp(&infoFileName[filecount][file_length-3],"jsv") ==0))
            {
	            strcpy(&infoFileName[filecount][file_length-3],"txt");
	            strcpy(fileName[filecount],fullPathName);
 
            	filecount++;
            	
            	if (filecount >= MAX_INSTANCE)
            		break;   		
            }
		}			
	} while (!fsFindNext(&tFileInfo));	
	
   return 0; 
}

int main(void)
{
	UINT32 u32ExtFreq;
	int SD_clock; 
	WB_UART_T uart;		   
    
#if 1
    sysInvalidCache();    
	sysEnableCache(CACHE_WRITE_BACK);	
#endif	    

    if (InitFB())
        exit(-1);

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
		
	SD_clock = sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock())/1000;;
    sicIoctl(SIC_SET_CLOCK, SD_clock, 0, 0);       	


	fsInitFileSystem();
	fmiInitDevice(); 
	if (sicSdOpen0() <=0)
	{
		Console_Printf("Error in initialize SD card !!\n");
		while(1);
	}

    GetBSFileName();
    
	
	VPE_entry();

	//do {
		DecodeH264();
	//} while(1);
    printf("All decode done\n");
    
    while(1);
        
}


