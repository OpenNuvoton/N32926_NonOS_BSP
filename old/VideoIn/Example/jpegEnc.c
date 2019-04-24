#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wbtypes.h"
#include "wblib.h"

#include "jpegcodec.h"
#include "nvtfat.h"
#include "w55fa92_sic.h"
#include "jpegSample.h"
#include "demo.h"
/*          
extern CHAR g_u8String[100];          
extern UINT32 g_u32StringIndex;           
*/          
PUINT8 g_pu8EncFrameBuffer;								/* Source image data for encoding */ 

UINT8 __align(32) g_au8BitstreamBuffer[0x100000];		/* The buffer for encoding output */
                  
VOID JpegEncTest (VINDEV_T* pVin, BOOL bIsEncodePacket, UINT32 u32FrameAddr)
{
	//INT		nWriteLen, nStatus, nReadLen;
	//INT		hFile;
	//CHAR	encodePath[50];
	CHAR	szFileName[100];	
	UINT16 	u16Width = ENC_WIDTH, u16Height = ENC_HEIGHT;
	JPEG_INFO_T jpegInfo;
	INT len;
	static INT uEncodeFrame=0;
	//UINT32 starttime,endtime;
	
	if(bIsEncodePacket)	{
		u16Width = OPT_LCM_WIDTH;
		u16Height = OPT_LCM_HEIGHT;		
	}else{
		u16Width = OPT_ENCODE_WIDTH;
		u16Height = OPT_ENCODE_HEIGHT;
	}	
	
	sysprintf ("\nStart JPEG Encode Testing...\n");		
#if 0	
/*******************************************************************/	
/* 							Read Raw Data file					   */
/*******************************************************************/			
	sysprintf("Input Source file name\n");
	
	GetString();
		
	strcpy(encodePath, g_u8String);
				
	fsAsciiToUnicode(encodePath, suFileName1, TRUE);
	
    	hFile = fsOpenFile(suFileName1, NULL, O_RDONLY);
	if (hFile > 0)
		sysprintf("\tOpen file:[%s], file handle:%d\n", encodePath, hFile);
	else
	{
		sysprintf("\tFailed to open file: %s (%x)\n", encodePath, hFile);
		return;
	}	
		
	len = fsGetFileSize(hFile);
	
	sysprintf("\tRaw data size for Encode is %d\n", len);
	
	/* Allocate the Raw Data Buffer for Encode Operation */	
	g_pu8EncFrameBuffer = (PUINT8)malloc(sizeof(CHAR) * len);
	
	sysprintf("\tJPEG Bitstream is located 0x%X\n", (UINT32)g_au8BitstreamBuffer);
	
	nStatus = fsReadFile(hFile, (UINT8 *)((UINT32)g_pu8EncFrameBuffer | 0x80000000), len, &nReadLen);
	if (nStatus < 0)
		sysprintf("\tRead error!!\n");		
	
	fsCloseFile(hFile);	    
	    
#endif	    
/*******************************************************************/	
/* 						Encode JPEG Bitstream					   */
/*******************************************************************/	    
	memset((char*)g_au8BitstreamBuffer, 0, 0x80000);
	/* JPEG Init */
	jpegInit();	
				
	/* Set Source Y/U/V Stride */	       
    	jpegIoctl(JPEG_IOCTL_SET_YSTRIDE, u16Width, 0);
    	jpegIoctl(JPEG_IOCTL_SET_USTRIDE, u16Width/2, 0);
    	jpegIoctl(JPEG_IOCTL_SET_VSTRIDE, u16Width/2, 0);
     														
    	/* Set Bit stream Address */   
    	jpegIoctl(JPEG_IOCTL_SET_BITSTREAM_ADDR, (UINT32)g_au8BitstreamBuffer, 0);

	/* Encode mode, encoding primary image, YUV 4:2:2/4:2:0 */	
	if(bIsEncodePacket==TRUE){
    		jpegIoctl(JPEG_IOCTL_SET_ENCODE_MODE, JPEG_ENC_SOURCE_PACKET, JPEG_ENC_PRIMARY_YUV422);     		
		jpegIoctl(JPEG_IOCTL_SET_YADDR, (UINT32)u32FrameAddr, 0);	/* Set Source Address */	
    	}else{	
	    	jpegIoctl(JPEG_IOCTL_SET_ENCODE_MODE, JPEG_ENC_SOURCE_PLANAR, JPEG_ENC_PRIMARY_YUV422); 
		jpegIoctl(JPEG_IOCTL_SET_YADDR, (UINT32)u32FrameAddr, 0);	/* Set Source Address */	
	    	jpegIoctl(JPEG_IOCTL_SET_UADDR, (UINT32)u32FrameAddr+u16Width*u16Height, 0);	    		
	    	jpegIoctl(JPEG_IOCTL_SET_VADDR, (UINT32)u32FrameAddr+u16Width*u16Height+u16Width*u16Height/2 , 0);
 	}
    
    
    	/* Primary Encode Image Width / Height */    
    	jpegIoctl(JPEG_IOCTL_SET_DIMENSION, u16Height, u16Width);
#if 0       
	if(g_bEncSwReserveTest){  
		/* Reserve memory space for user application 
		   # Reserve memory space Start address is Bit stream Address + 6 and driver will add the app marker (FF E0 xx xx)for user automatically. 
		   # User can set the data before or after trigger JPEG (Engine will not write the reserved memory space).	
		   # The size parameter is the actual size that can used by user and it must be multiple of 2 but not be multiple of 4 (Max is 65530). 	   
		   # User can get the marker length (reserved length + 2) from byte 4 and byte 5 in the bitstream. 
		   		Byte 0 & 1 :  FF D8
				Byte 2 & 3 :  FF E0
				Byte 4 & 5 :  [High-byte of Marker Length][Low-byte of Marker Length]
		   		Byte 6 ~ (Length + 4)  :  [(Marker Length - 2)-byte Data] for user application
		   	   
		   	 Ex : jpegIoctl(JPEG_IOCTL_ENC_RESERVED_FOR_SOFTWARE, 1024,0); 
		          FF D8 FF E0 04 02 [1024 bytes]	   	   
		*/
		jpegIoctl(JPEG_IOCTL_ENC_RESERVED_FOR_SOFTWARE, 1024,0);     
	}
	     
	if(g_bEncUpTest){       
	    	/* Primary Encode Image Width / Height */        
		/* Encode upscale 2x */
	    	jpegIoctl(JPEG_IOCTL_SET_ENCODE_UPSCALE, u16Height * 2, u16Width * 2);       
    	}
#endif
           
	//Set Encode Source Image Height        
	jpegIoctl(JPEG_IOCTL_SET_SOURCE_IMAGE_HEIGHT, u16Height, 0);

	/* Include Quantization-Table and Huffman-Table */	
    	jpegIoctl(JPEG_IOCTL_ENC_SET_HEADER_CONTROL, JPEG_ENC_PRIMARY_QTAB | JPEG_ENC_PRIMARY_HTAB, 0);
       
	/* Use the default Quantization-table 0, Quantization-table 1 */
	jpegIoctl(JPEG_IOCTL_SET_DEFAULT_QTAB, 0, 0);

//	starttime = sysGetTicks(TIMER0);
//	sysprintf("Encode planar 300f =%d\n");	
//	sysprintf("Start tick =%d\n", starttime);
		
	/* Trigger JPEG encoder */		
    	jpegIoctl(JPEG_IOCTL_ENCODE_TRIGGER, 0, 0);  	       
    	/* Wait for complete */
	if(jpegWait())
	{
		//endtime = sysGetTicks(TIMER0);	
		jpegGetInfo(&jpegInfo);
		sysprintf(".");
		//sysprintf("\tJPEG Encode Complete!!\n");
		//sysprintf("\tJpeg Image Size = %d\n",jpegInfo.image_size[0]);	
		len = jpegInfo.image_size[0];		
    	}
    	else
    	{
    		sysprintf("\tJPEG Encode Error!!\n");
    		len = 0;
    		return;
    	}	    	  
    	uEncodeFrame = uEncodeFrame+1;
    	szFileName[0]=0;		
    	if(bIsEncodePacket)	
		sprintf(szFileName, "C:\\EncodePacket_%d.jpg", uEncodeFrame);
	else
		sprintf(szFileName, "C:\\EncodePlanar_%d.jpg", uEncodeFrame);
    	WriteFile(szFileName, (PUINT16)g_au8BitstreamBuffer, len);
//	sysprintf("\n");
//	starttime = sysGetTicks(TIMER0);	
//	sysprintf("End tick =%d\n", starttime);
	


	return;   
}
