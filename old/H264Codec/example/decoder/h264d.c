/* 
    Assume the bitstream size is known in file "test.info", 720x480 resolution, 10 frames
    sample code for H264 for bitstream input and YUV output
    This sample code is to do decode 100 rounds,10 frames/round, named "/tmp/dev0.yuv"
    #./h264d test.264 test.info
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

// VPOST related
//static struct fb_var_screeninfo var;
int fb_fd=0;
UINT32 g_u32VpostWidth, g_u32VpostHeight,fb_bpp;
unsigned int fb_paddress;

int frame_info[FRAME_COUNT+1];

INT            din,din_info;
INT            dout;
int             favc_dec_fd=0;
int             dec_mmap_addr;
unsigned int    in_virt_buffer;


typedef struct {
    unsigned int size;
    unsigned int addr;
} _BUF_INFO;

_BUF_INFO outputbuf;

unsigned char *pVBitStreamBuffer = NULL,*pVBitStartBuffer;
int bitstreamsize;
int decoded_img_width, decoded_img_height;
int output_vir_buf=0;


// VPE
extern int vpe_fd;
extern int InitVPE(void);
extern int FormatConversion(void* data, char* pDstBuf, int SrcWidth, int SrcHeight, int Tarwidth, int Tarheight);
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


int h264_init(video_profile *video_setting)
{

    FAVC_DEC_PARAM      tDecParam;
    int                 ret_value;

    if(favc_dec_fd==0)

    favc_dec_fd=H264Dec_Open();

    if(favc_dec_fd < 0)
    {
        printf("Fail to open Decoder\n");
        return -1;
    }
    
	
#if (DECODE_OUTPUT_PACKET_YUV422 ==0)
	outputbuf.addr =(int)malloc(MAX_IMG_WIDTH*MAX_IMG_HEIGHT*3); 
	if (outputbuf.addr ==0)
	    return -1;
	outputbuf.size = MAX_IMG_WIDTH*MAX_IMG_HEIGHT*3 / 2; 	    
#endif	    
    
    memset(&tDecParam, 0, sizeof(FAVC_DEC_PARAM));
    tDecParam.u32API_version = H264VER;
    tDecParam.u32MaxWidth = video_setting->width;
    tDecParam.u32MaxHeight = video_setting->height;
#if (DECODE_OUTPUT_PACKET_YUV422 ==0)
	// For file output or VPE convert planr to packet format
    tDecParam.u32FrameBufferWidth = video_setting->width;
    tDecParam.u32FrameBufferHeight = video_setting->height;
#else
	// For TV output. H.264 output packet to FB off-screen buffer
    tDecParam.u32FrameBufferWidth = 640;
    tDecParam.u32FrameBufferHeight = 480;
#endif   

#if DECODE_OUTPUT_PACKET_YUV422
	tDecParam.u32OutputFmt = 1; // 1->Packet YUV422 format, 0-> planar YUV420 foramt
#else	
	tDecParam.u32OutputFmt = 0; // 1->Packet YUV422 format, 0-> planar YUV420 foramt
#endif	

    ret_value = H264_ioctl(FAVC_IOCTL_DECODE_INIT,&tDecParam);		// Output : Packet YUV422 or Planar YUV420
    if(ret_value < 0)
    {
        printf("FAVC_IOCTL_DECODE_INIT: memory allocation failed\n");
        return -1;
    }
    
    return 0;
}


int h264_close(video_profile *video_setting)
{
    if(favc_dec_fd) {
		H264Dec_Close();
    }
    
    favc_dec_fd = 0;
        
    return 0;
}

int favc_decode(video_profile *video_setting, unsigned char *frame, void *data, int size)
{

    AVFrame             *pict=(AVFrame *)data;
    FAVC_DEC_PARAM      tDecParam;
    int  ret_value;
    
    memset(&tDecParam, 0, sizeof(FAVC_DEC_PARAM));
    tDecParam.pu8Display_addr[0] = (unsigned int)pict->data[0];
    tDecParam.pu8Display_addr[1] = (unsigned int)pict->data[1];
    tDecParam.pu8Display_addr[2] = (unsigned int)pict->data[2];
    tDecParam.u32Pkt_size =	(unsigned int)size;
    tDecParam.pu8Pkt_buf = (UINT8 *)((UINT32)frame | CACHE_BIT31);

    tDecParam.crop_x = 0;
    tDecParam.crop_y = 0;
    
	tDecParam.u32OutputFmt = 1; // 1->Packet YUV422 format, 0-> planar YUV420 foramt
	
    if((ret_value = H264_ioctl(FAVC_IOCTL_DECODE_FRAME, &tDecParam)) != 0)    
    {
        printf("FAVC_IOCTL_DECODE_FRAME: Failed.ret=%x\n",ret_value);
        return -1;
    }

#if OUTPUT_FILE
	if (tDecParam.tResult.isDisplayOut != 0)
	{
    	fwrite((void *)pict->data[0],video_setting->width*video_setting->height,1,dout);
		if (tDecParam.u32OutputFmt == 0)
		{
		    fwrite((void *)pict->data[1],video_setting->width*video_setting->height/4,1,dout);
		    fwrite((void *)pict->data[2],video_setting->width*video_setting->height/4,1,dout);
	    }
	}
#endif
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


int DecodeH264(char *src264File, char *src264info)
{
    int  i,totalsz = 0;
    video_profile  video_setting;
    AVFrame  pict[2], *pict_ptr;
    int runsz = 0; 
    int result,flushCount=0;
    int totaloffset=0;
    int toggle_flag=0;
    int nByteCnt;

    sysFlushCache(D_CACHE);
      

    din = openFile(src264File, O_RDONLY);
    din_info = openFile(src264info, O_RDONLY);    

    //set the default value
    video_setting.width = (unsigned int)-1;
    video_setting.height = (unsigned int)-1;
   

    if (din==0)
    {
        printf("Open file %s fail\n",src264File);
        goto file_close;
    }       
    printf("Open file %s is OK\n",src264File); 
    
    if (din_info ==0)
    {
        printf("Open file %s fail\n",src264info);
        goto file_close;        
    }
    
    
    memset(frame_info, 0, sizeof(frame_info));
    
    for(i=0; i< FRAME_COUNT; i++)
    {
        frame_info[i] = scanFile(din_info);
        totalsz += frame_info[i];
        //sysprintf("BS %d length = %d\n",i,frame_info[i]);

        if (frame_info[i] == 0)
            break;
    }
    
    bitstreamsize = MAX_IMG_WIDTH * MAX_IMG_HEIGHT * 2; 
    in_virt_buffer = (unsigned int)nv_malloc(bitstreamsize,32);          
    if (!in_virt_buffer)
        goto file_close;
        

    if (h264_init(&video_setting) < 0)
        goto fail_h264_init;


    //YUV422 needn't assign U,V
#if DECODE_OUTPUT_PACKET_YUV422
    pict[0].data[0] = (unsigned char *)dec_mmap_addr;
    pict[0].data[1] = (unsigned char *)(dec_mmap_addr+ (video_setting.width*video_setting.height));
    pict[0].data[2] = (unsigned char *)(dec_mmap_addr+ (video_setting.width*video_setting.height*5/4));
#else    
    pict[0].data[0] = (unsigned char *)outputbuf.addr;
    pict[0].data[1] = (unsigned char *)(outputbuf.addr+ (MAX_IMG_WIDTH*MAX_IMG_HEIGHT));
    pict[0].data[2] = (unsigned char *)(outputbuf.addr+ (MAX_IMG_WIDTH*MAX_IMG_HEIGHT*5/4));
    
    pict[1].data[0] = (unsigned char *)outputbuf.addr + (outputbuf.size/2) ;
    pict[1].data[1] = (unsigned char *)(outputbuf.addr+ (outputbuf.size/2) + (MAX_IMG_WIDTH*MAX_IMG_HEIGHT));
    pict[1].data[2] = (unsigned char *)(outputbuf.addr+ (outputbuf.size/2) + (MAX_IMG_WIDTH*MAX_IMG_HEIGHT*5/4));    
#endif    

    for (i=0; i<FRAME_COUNT; i++)
    {
        if ((frame_info[i] > bitstreamsize) || (frame_info[i] == 0))
        {
            if (frame_info[i] != 0)
                printf("Warning : Bitstream size is larger than buffer size\n");
            
            goto fail_h264_init;
        }     
           
		fsReadFile(din,(UINT8 *)(in_virt_buffer | CACHE_BIT31), frame_info[i], &nByteCnt);                  
        
        totaloffset += frame_info[i];
        
#if DECODE_OUTPUT_PACKET_YUV422        
		pict_ptr = &pict[0];
#else
        if (toggle_flag)
        {
            pict_ptr = &pict[1];
            toggle_flag = 0;
        }            
        else
        {
            pict_ptr = &pict[0];     
            toggle_flag = 1;               
        }        
#endif
		
        result = favc_decode(&video_setting, (unsigned char *)(in_virt_buffer), (void *)pict_ptr, frame_info[i]);        
        if (result < 0)
        {
            printf("frame %d(size=%d)/(0~%d) decode FAIL!\n", i, frame_info[i],FRAME_COUNT-1);
            goto fail_h264_init;
        }
        else if (result == 0)
        {
        	flushCount++;
		}     
		else
		{
		        
			vpeIoctl(VPE_IOCTL_SET_SRC_DIMENSION,						
						decoded_img_width,
						decoded_img_height,
						NULL);
						
			{
			 int width,height;
			 			
			 if (decoded_img_width	> 	LCM_WIDTH)
			 	width = LCM_WIDTH;
			 else
			 	width = decoded_img_width;
			 	
			 if (decoded_img_height	> 	LCM_HEIGHT)
			 	height = LCM_HEIGHT;
			 else
			 	height = decoded_img_height;
			 				 					
			vpeIoctl(VPE_IOCTL_SET_DST_DIMENSION,	
						width,
						height,
						NULL);		
			}			
						
			if (decoded_img_width <	LCM_WIDTH)
			{
				vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
							(UINT32)0,				/* Dst Left offset */
							(UINT32)LCM_WIDTH - decoded_img_width ,	/* Dst right offset */
							NULL);												
			}
			else
			{
				vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
							(UINT32)0,				/* Dst Left offset */
							(UINT32)0,				/* Dst right offset */
							NULL);			
			}						
										        
			vpeIoctl(VPE_IOCTL_SET_SRCBUF_ADDR,
					(UINT32)pict_ptr->data[0],				
					(UINT32)pict_ptr->data[1],	
					(UINT32)pict_ptr->data[2]);
					
		    VPE_trigger();	
		    
			do
			{
				ERRCODE errcode;
				errcode = vpeIoctl(VPE_IOCTL_CHECK_TRIGGER,	//TRUE==>Not complete, FALSE==>Complete
									NULL,					
									NULL,
									NULL);
				if(errcode==0)
					break;								
			}while(1);		    		
		}   	
        	
        if (i==0)
        	sysprintf("	Img width = %d, height =%d\n",decoded_img_width, decoded_img_height);
        runsz += frame_info[i];
    }

    printf("Total frame %d decode OK!\n", i);

fail_h264_init:

    h264_close(&video_setting);

    nv_free((void *)in_virt_buffer);  
    
file_close:
	if (outputbuf.addr >0)
		free((void *)outputbuf.addr);
		
		
	if (din > 0)
		fsCloseFile(din);
	if (din_info >0)
		fsCloseFile(din_info);			
	
    return 0;    
}
 


#define  BS_FOLDER  "../pattern/"
#define  INFO_FOLDER "../info/"
#define MAX_FILE_NAME_LEN       	514

int main(void)
{
	UINT32 u32ExtFreq;
    char infoFile[256];
    int file_length;
	int SD_clock; 
	
	INT         nStatus;
	CHAR		szLongName[MAX_FILE_NAME_LEN];	
	CHAR		suDirName[256],fullPathName[256];	
	FILE_FIND_T tFileInfo;
	WB_UART_T uart;		   
    
#if 1
    sysInvalidCache();    
	sysEnableCache(CACHE_WRITE_BACK);	
#endif	    

    if (InitFB())
        exit(-1);
/*        
	if ( InitVPE())	
		exit(-1);
*/

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
    sicIoctl(SIC_SET_CLOCK, SD_clock, 0, 0);       // clock from FPGA clock in		
    //fmiSD_Set_clock(200);

	fsInitFileSystem();
	fmiInitDevice(); 
	if (sicSdOpen0() <=0)
	{
		Console_Printf("Error in initialize SD card !!\n");
		while(1);
	}

    
	memset((CHAR *)&tFileInfo, 0, sizeof(FILE_FIND_T));

    strcpy(fullPathName,INPUT_PATTERN_FOLDER); 
    strcat(fullPathName,"pattern\\");
	fsAsciiToUnicode(fullPathName, suDirName, 1);    
	nStatus = fsFindFirst(suDirName,NULL,&tFileInfo);
	
	VPE_entry();


	if (nStatus < 0)
	{
		sysprintf("No file found in %s folder\n", fullPathName);
		return -1;
	}		
		
	do 
	{
		if (tFileInfo.ucAttrib & FA_ARCHIVE)  
        {		
        	// Playback all C:\h264\patter\*.264 or *.jsv bitstream
    		fsUnicodeToAscii(tFileInfo.suLongName, szLongName, 1);
            strcpy(fullPathName,"pattern\\");    		
            strcat(fullPathName,szLongName); 
            
            strcpy(infoFile,"info\\");
            strcat(infoFile,szLongName);
	        file_length = strlen(infoFile);                         
            
            if ((strcmp(&infoFile[file_length-3],"264") ==0) || (strcmp(&infoFile[file_length-3],"jsv") ==0))
            {
	            strcpy(&infoFile[file_length-3],"txt");
            	DecodeH264(fullPathName ,infoFile);    		
            }
		}			
	} while (!fsFindNext(&tFileInfo));	
	
	fsFindClose(&tFileInfo);

    printf("All decode done\n");
    
    while(1);
        
}


