/**************************************************************************//**
 * @file     uvc_main.c
 * @brief    UVC Flow Control sample code
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "W55FA92_reg.h"
#include "demo.h"
#include "usbd.h"
#include "videoclass.h"
#ifndef __UVC_VIN__	
	#include "VGA.h"
	#include "QVGA.h"
	#include "QQVGA.h"
#endif
#include "jpegcodec.h"
#include "W55FA92_VideoIn.h"

/*********************************************************************************************************************/ 
/* 1. The Sample code sends image to PC for preview & shapshot (toggle two fixed image patterns or data from VideoIN)*/
/* 2. User can get the following information from uvcStatus                                 	   					 */
/*      #MaxVideoFrameSize				-> Maximum Video Frame size	for Preview										 */
/*		#snapshotMaxVideoFrameSize		-> Maximum Video Frame size for Snapshot								 	 */	
/*		#FormatIndex					-> Current Preview Format index												 */
/*		#FrameIndex						-> Current Preview Frame index												 */
/*		#StillImage						-> Snapshot flag (It will be set to TRUE when application sends Snapshot CMD)*/
/*		#snapshotFormatIndex			-> Snapshot Format Index													 */
/*		#snapshotFrameIndex				-> Snapshot Frame Index														 */	
/*		#bReady							-> UVC Status																 */ 
/* 3. Preview & Snapshot Image Format Index							  												 */ 
/*		#UVC_Format_YUY2		1 (Fixed Pattern)					  												 */
/*		#UVC_Foramt_MJPEG		2 (Fixed Pattern or Data from VideoIN)												 */
/* 4. Preview Image Frame Index									  													 */
/*		#UVC_VGA				1									  												 */
/*		#UVC_QVGA				2								  													 */
/*		#UVC_QQVGA				3								  													 */		
/* 5. Still (Snapshot) Image Frame Index									  										 */
/*		#UVC_STILL_VGA			1								  													 */
/*		#UVC_STILL_QVGA			2								  													 */
/*		#UVC_STILL_QQVGA		3									  										 		 */
/* 6. uavcdSendImage																								 	 */	
/*		#u32Addr			Image data Buffer																		 */
/*		#u32transferSize	Image data Size																	  	 	 */
/*		#bStillImage		User can use the value of uvcStatus.StillImage or send TRUE when he wnats to do snapshot */
/*							bStillImage will be clear after Snapshot image transfer is complete						 */
/* 7. uavcdIsReady return value																				 	 	 */
/*		#TRUE				Ready to send image data														 		 */
/*		#FALSE				Busy (The previous image transfer isn't complete)					 			  		 */
/* 8. ProcessUnitControl Value																				 	 	 */
/*		# the maximum, minimum, and default values of each item are defined in "videoclass.h, user can modify them 	 */
/*		  to change the values																						 */ 	
/*																											 		 */
/*********************************************************************************************************************/ 

extern INT32 InitializeUAC(UINT32 u32SampleRate);
extern void StartUAC(void);
extern void StopUAC(void);
extern void Send_AudioOneMSPacket(PUINT32 pu32Address, PUINT32 pu32Length);

/* Skip frame when Changing frame */
#define UVC_SKIP_FRAME 5

#if defined (__GNUC__)
/* Buffer for Packet & Planar format */
UINT8 u8PacketFrameBuffer0[640*480*2] __attribute__((aligned(32)));
UINT8 u8PacketFrameBuffer1[640*480*2] __attribute__((aligned(32)));
UINT8 u8PacketFrameBuffer2[640*480*2] __attribute__((aligned(32)));
UINT8 u8PlanarFrameBuffer0[640*480*2] __attribute__((aligned(32)));
UINT8 u8PlanarFrameBuffer1[640*480*2] __attribute__((aligned(32)));
UINT8 u8PlanarFrameBuffer2[640*480*2] __attribute__((aligned(32)));

/* JEPG Bitstream Buffer */
UINT8 u8jpegBitstream[500*1024] __attribute__((aligned(32)));
#else
/* Buffer for Packet & Planar format */
UINT8 __align(32) u8PacketFrameBuffer0[640*480*2];
UINT8 __align(32) u8PacketFrameBuffer1[640*480*2];
UINT8 __align(32) u8PacketFrameBuffer2[640*480*2];
UINT8 __align(32) u8PlanarFrameBuffer0[640*480*2];
UINT8 __align(32) u8PlanarFrameBuffer1[640*480*2];
UINT8 __align(32) u8PlanarFrameBuffer2[640*480*2];

/* JEPG Bitstream Buffer */
UINT8 __align(32) u8jpegBitstream[500*1024];
#endif

/* Toggle index for Fixed Pattern */
UINT8 volatile bufIndex = 0, u32PreviousBufIdx = 4;

/* Current Frame Index */
UINT32 u32CurFrameIndex = 1;

/* Current Format Index */
UINT32 u32CurFormatIndex = 1;

/* Frame Buffer Index for VideoIN and JPEG Encode Buffer Change */
UINT32 frameBufferIndex = 0;

/* Current Buffer Address for JPEG Encode */
UINT32 u32CurFrameAddress;

/* Current Width & Height */
UINT16 u16CurWidth = 640, u16CurHeight = 480;

/* Buffer index for USB transfer */
static UINT32 u32usbBufIdx = 2;

/* Buffer Status */
BOOL bIsFrameBuffer0=0,  bIsFrameBuffer1=0, bIsFrameBuffer2=0; /* 0 means buffer is clean */

/* VideoIN Buffer index */
UINT32 u32VideoInIdx = 0;

/* Skip Frame number */
UINT32 u32SkipFrame = 0;


VOID VideoPipe(PUINT32 pu32Address, PUINT32 pu32Length)
{

}
/* UVC Main */
VOID uvc_main(void)
{
#ifdef __UVC_VIN__	
	/* Skip frames */
	u32SkipFrame = UVC_SKIP_FRAME;	
	while(u32SkipFrame)
		GetImageBuffer();
#endif	
	/* Open USB Device */
	udcOpen();
	/* Init USB Device Setting as Video Class Device */
	uavcdInit(ProcessUnitControl,VideoPipe, Send_AudioOneMSPacket);		
	/* Init USB Device */
	udcInit();
	/* Deal with UVC event */	
	uvcdEvent();
	/* Deinit USB Device */
	udcDeinit();	
	/* Cloase USB Device */
	udcClose();
}

/* UVC event */
VOID uvcdEvent(void)
{
	UINT8 u8OpenMic,result;
    UINT32 u32Addr,u32transferSize;   
    
	InitializeUAC(16000);  // Sample rate = 16000KHz
	u8OpenMic = 0;
	while (1)
	{
		if (usbdStatus.appConnected == 1)
	    {
	    	/* Get Image Data */
			result = GetImage(&u32Addr, &u32transferSize);
			
			if(result)	/* If frame isn't updated, Do nothing */
			{
	 			/* Send Image */	
		  		uavcdSendImage(u32Addr, u32transferSize, uvcStatus.StillImage);
			 	/* Wait for Complete */ 	
		  		while(!uavcdIsReady());	  	
		  	}			  	
		}	   
		if(usbdStatus.appConnected_Audio == 1)   // audio start
		{
			if ( u8OpenMic == 0 )
			{
				u8OpenMic = 1;
				StartUAC();
			}
		}
		else if ( usbdStatus.appConnected_Audio == 0 )
		{
			if (u8OpenMic == 1)
			{
				u8OpenMic = 0;
				StopUAC();
			}
		}			
	}
}

/* Change VideoIN Buffer when Frame End */
void VideoIn_InterruptHandler(void)
{
	switch(u32VideoInIdx)
	{
		case 0:		
				if(bIsFrameBuffer1==0)
				{
					if(uvcStatus.StillImage)
					{				
						/* Change frame buffer 1 if Frame Buffer 1 is clean, Otherwise, do nothing */
						if(uvcStatus.snapshotFormatIndex == UVC_Format_YUY2)
							ChangeFrame(FALSE, (UINT32)u8PacketFrameBuffer1, u16CurWidth,u16CurHeight);
						else
							ChangeFrame(FALSE, (UINT32)u8PlanarFrameBuffer1, u16CurWidth,u16CurHeight);		
					
					 	bIsFrameBuffer0 = 1; u32VideoInIdx = 1; 
					}
					else
					{
						/* Change frame buffer 1 if Frame Buffer 1 is clean, Otherwise, do nothing */
						if(uvcStatus.FormatIndex == UVC_Format_YUY2)
							ChangeFrame(FALSE, (UINT32)u8PacketFrameBuffer1, u16CurWidth,u16CurHeight);
						else
							ChangeFrame(FALSE, (UINT32)u8PlanarFrameBuffer1, u16CurWidth,u16CurHeight);		
					
					 	bIsFrameBuffer0 = 1; u32VideoInIdx = 1; 				
					
					}
				}				
				break; 	 				
		case 1:		
				if(bIsFrameBuffer2==0)
				{
					if(uvcStatus.StillImage)
					{					
						/* Change frame buffer 2 if Frame Buffer 2 is clean, Otherwise, do nothing */			
						if(uvcStatus.snapshotFormatIndex == UVC_Format_YUY2)
							ChangeFrame(FALSE, (UINT32)u8PacketFrameBuffer2, u16CurWidth,u16CurHeight);
						else
							ChangeFrame(FALSE, (UINT32)u8PlanarFrameBuffer2, u16CurWidth,u16CurHeight);								
		 				bIsFrameBuffer1 = 1; u32VideoInIdx = 2;
		 			}
		 			else
		 			{
						/* Change frame buffer 2 if Frame Buffer 2 is clean, Otherwise, do nothing */			
						if(uvcStatus.FormatIndex == UVC_Format_YUY2)
							ChangeFrame(FALSE, (UINT32)u8PacketFrameBuffer2, u16CurWidth,u16CurHeight);
						else
							ChangeFrame(FALSE, (UINT32)u8PlanarFrameBuffer2, u16CurWidth,u16CurHeight);								
		 				bIsFrameBuffer1 = 1; u32VideoInIdx = 2;		 			
		 			
		 			}	 			
				}	
				break; 	 				
		case 2:		
				if(bIsFrameBuffer0==0)
				{
					if(uvcStatus.StillImage)
					{					
						/* Change frame buffer 0 if Frame Buffer 0 is clean, Otherwise, do nothing */
						if(uvcStatus.snapshotFormatIndex == UVC_Format_YUY2)
							ChangeFrame(FALSE, (UINT32)u8PacketFrameBuffer0, u16CurWidth,u16CurHeight);
						else
							ChangeFrame(FALSE, (UINT32)u8PlanarFrameBuffer0, u16CurWidth,u16CurHeight);	
						bIsFrameBuffer2 = 1; u32VideoInIdx = 0; 
					}					
					else
					{
						/* Change frame buffer 0 if Frame Buffer 0 is clean, Otherwise, do nothing */
						if(uvcStatus.FormatIndex == UVC_Format_YUY2)
							ChangeFrame(FALSE, (UINT32)u8PacketFrameBuffer0, u16CurWidth,u16CurHeight);
						else
							ChangeFrame(FALSE, (UINT32)u8PlanarFrameBuffer0, u16CurWidth,u16CurHeight);	
						bIsFrameBuffer2 = 1; u32VideoInIdx = 0; 					
					}
				}	
				break; 	 
	}

					
	if(u32SkipFrame != 0)
		u32SkipFrame--;
}

/* Get Image Size and Address (Image data control for Foramt and Frame)*/
INT GetImageBuffer(VOID)
{
	switch(u32usbBufIdx)
	{
		case 0: 
				if(bIsFrameBuffer1==1)	
				{
					if(uvcStatus.StillImage)
					{
						/* Check Frame Buffer 1 is dirty */
						bIsFrameBuffer0 = 0;	/* Frame Buffer 0 is clean */
						u32usbBufIdx = 1;
						if(uvcStatus.snapshotFormatIndex == UVC_Format_YUY2)	
							u32CurFrameAddress = (UINT32)u8PacketFrameBuffer1;
						else
							u32CurFrameAddress = (UINT32)u8PlanarFrameBuffer1;
					}
					else
					{
							/* Check Frame Buffer 1 is dirty */
						bIsFrameBuffer0 = 0;	/* Frame Buffer 0 is clean */
						u32usbBufIdx = 1;
						if(uvcStatus.FormatIndex == UVC_Format_YUY2)	
							u32CurFrameAddress = (UINT32)u8PacketFrameBuffer1;
						else
							u32CurFrameAddress = (UINT32)u8PlanarFrameBuffer1;				
					
					}
				}
				break;		
		case 1: 
				if(bIsFrameBuffer2==1)	
				{
					if(uvcStatus.StillImage)
					{				
						/* Check Frame Buffer 2 is dirty */
						bIsFrameBuffer1 = 0;	/* Frame Buffer 1 is clean */
						u32usbBufIdx = 2;
						if(uvcStatus.snapshotFormatIndex == UVC_Format_YUY2)	
							u32CurFrameAddress = (UINT32)u8PacketFrameBuffer2;
						else
							u32CurFrameAddress = (UINT32)u8PlanarFrameBuffer2;
					}
					else
					{
						/* Check Frame Buffer 2 is dirty */
						bIsFrameBuffer1 = 0;	/* Frame Buffer 1 is clean */
						u32usbBufIdx = 2;
						if(uvcStatus.FormatIndex == UVC_Format_YUY2)	
							u32CurFrameAddress = (UINT32)u8PacketFrameBuffer2;
						else
							u32CurFrameAddress = (UINT32)u8PlanarFrameBuffer2;					
					}
				}
				break;
		case 2:  	
				if(bIsFrameBuffer0==1)		
				{
					if(uvcStatus.StillImage)
					{					
						/* Check Frame Buffer 0 is dirty */
						bIsFrameBuffer2 = 0;	/* Frame Buffer 2 is clean */
						u32usbBufIdx = 0;
						if(uvcStatus.snapshotFormatIndex == UVC_Format_YUY2)	
							u32CurFrameAddress = (UINT32)u8PacketFrameBuffer0;
						else
							u32CurFrameAddress = (UINT32)u8PlanarFrameBuffer0;
					}
					else
					{
						/* Check Frame Buffer 0 is dirty */
						bIsFrameBuffer2 = 0;	/* Frame Buffer 2 is clean */
						u32usbBufIdx = 0;
						if(uvcStatus.FormatIndex == UVC_Format_YUY2)	
							u32CurFrameAddress = (UINT32)u8PacketFrameBuffer0;
						else
							u32CurFrameAddress = (UINT32)u8PlanarFrameBuffer0;	
					}					
				}
				break;
	}	
	if(u32usbBufIdx == u32PreviousBufIdx)	/* Skip the frame that had already been encoded */
		return 0;							
	else
		u32PreviousBufIdx = u32usbBufIdx;	/* Update u32PreviousBufIdx */
	return 1;	
}

/* Change VideoIN Setting for Frame size */
VOID ChangeFrame(BOOL bChangeSize, UINT32 u32Address, UINT16 u16Width,UINT16 u16Height)
{	
	if(bChangeSize)	/* Change Frame Size */
	{
		if(uvcStatus.StillImage)
		{
			if(uvcStatus.snapshotFormatIndex == UVC_Format_YUY2)
			{					 							 
				pVin->PreviewPipeSize(u16Height, u16Width);
			}
			else
			{					 							 
				pVin->EncodePipeSize(u16Height, u16Width);			
			}		
		}
		else		
		{
			if(uvcStatus.FormatIndex == UVC_Format_YUY2)
			{					 							 
				pVin->PreviewPipeSize(u16Height, u16Width);
			}
			else
			{					 							 
				pVin->EncodePipeSize(u16Height, u16Width);				
			}	
		}		
	}
	/* Set Buffer Address */
	if(uvcStatus.StillImage)
	{
		if(uvcStatus.snapshotFormatIndex == UVC_Format_YUY2)
		{
			pVin->SetBaseStartAddress(
						eVIDEOIN_PACKET,			
						(E_VIDEOIN_BUFFER)0, 							//Packet buffer addrress
						(UINT32)u32Address);	
						
			
		}
		else
		{
			pVin->SetBaseStartAddress(
						eVIDEOIN_PLANAR,			
						(E_VIDEOIN_BUFFER)0, 							/* Planar buffer Y addrress */
						(UINT32)((UINT32)u32Address) );
			pVin->SetBaseStartAddress(
						eVIDEOIN_PLANAR,			
						(E_VIDEOIN_BUFFER)1, 							/* Planar buffer U addrress */
						(UINT32)((UINT32)u32Address+u16Width*u16Height) );
			pVin->SetBaseStartAddress(
						eVIDEOIN_PLANAR,			
						(E_VIDEOIN_BUFFER)2, 							/* Planar buffer V addrress */
						(UINT32)((UINT32)u32Address+u16Width*u16Height+u16Width*u16Height/2) );							
		}
	
	}
	else
	{
		if(uvcStatus.FormatIndex == UVC_Format_YUY2)
		{
			pVin->SetBaseStartAddress(
						eVIDEOIN_PACKET,			
						(E_VIDEOIN_BUFFER)0, 							//Packet buffer addrress
						(UINT32)u32Address);	
						
		}
		else
		{
			pVin->SetBaseStartAddress(
						eVIDEOIN_PLANAR,			
						(E_VIDEOIN_BUFFER)0, 							/* Planar buffer Y addrress */
						(UINT32)((UINT32)u32Address) );
			pVin->SetBaseStartAddress(
						eVIDEOIN_PLANAR,			
						(E_VIDEOIN_BUFFER)1, 							/* Planar buffer U addrress */
						(UINT32)((UINT32)u32Address+u16Width*u16Height) );
			pVin->SetBaseStartAddress(
						eVIDEOIN_PLANAR,			
						(E_VIDEOIN_BUFFER)2, 							/* Planar buffer V addrress */
						(UINT32)((UINT32)u32Address+u16Width*u16Height+u16Width*u16Height/2) );							
		}			
	}	
	pVin->SetStride(u16Width, u16Width);	
	pVin->SetShadowRegister();										
}

/* Get Image Size and Address (Image data control for Foramt and Frame) */
INT GetImage(PUINT32 pu32Addr, PUINT32 pu32transferSize)
{
	INT result;
#ifndef __UVC_VIN__	
	UINT32 u32Size0,u32Size1; 
    UINT32 u32BuffAddr1,u32BuffAddr0;    
#endif    
	/* Get Image Buffer (Return 0 if frame isn't updated) */
	result = GetImageBuffer();
	
	if(!result)		/* If frame isn't updated */
		return 0;	/* Skip the frame that had already been encoded */
	
	if(uvcStatus.StillImage)	/* Snapshot */
	{     
		//sysprintf("uvcStatus.snapshotFormatIndex %d\n",uvcStatus.snapshotFormatIndex);
		/* UVC_Foramt_YUY2 */	
        if(uvcStatus.snapshotFormatIndex == UVC_Format_YUY2)
        {    
    		*pu32transferSize = uvcStatus.snapshotMaxVideoFrameSize;   
#ifdef __UVC_VIN__	
			if(u32CurFrameIndex != uvcStatus.snapshotFrameIndex || u32CurFormatIndex != uvcStatus.snapshotFormatIndex)
			{				
				/* Frame Size Changed */
            	switch(uvcStatus.snapshotFrameIndex)
            	{										
					case UVC_STILL_VGA:							
						u16CurWidth = 640;
						u16CurHeight = 480;	
						break;										
					case UVC_STILL_QVGA:							
						u16CurWidth = 320;
						u16CurHeight = 240;
						break;			
					case UVC_STILL_QQVGA:									
						u16CurWidth = 160;
						u16CurHeight = 120;
						break;													
            	} 
            	/* Set VideoIn Setting for Frame Size */
            	ChangeFrame(TRUE, u32CurFrameAddress, u16CurWidth,u16CurHeight);
				u32CurFrameIndex = uvcStatus.snapshotFrameIndex;	
				u32CurFormatIndex = uvcStatus.snapshotFormatIndex; 		
				
				/* Skip frames */
				u32SkipFrame = UVC_SKIP_FRAME;		

				while(u32SkipFrame)
			   		GetImageBuffer();							
			}
			*pu32Addr = u32CurFrameAddress;
#else		
         	switch(uvcStatus.snapshotFrameIndex)
        	{										
				case UVC_STILL_VGA:
					u32BuffAddr1 = (UINT32)g_YUV_VGA_0;
					u32BuffAddr0 = (UINT32)g_YUV_VGA_1;	
					break;										
				case UVC_STILL_QVGA:
					u32BuffAddr1 = (UINT32)g_YUV_QVGA_0;
					u32BuffAddr0 = (UINT32)g_YUV_QVGA_1;										
					break;			
				case UVC_STILL_QQVGA:
					u32BuffAddr1 = (UINT32)g_YUV_QQVGA_0;
					u32BuffAddr0 = (UINT32)g_YUV_QQVGA_1;										
					break;													
        	}	 			 
			if(bufIndex)	/* Buffer address for Image 1 */
				*pu32Addr = u32BuffAddr1;
			else			/* Buffer address for Image 0 */
				*pu32Addr = u32BuffAddr0;        	      		       							            		       			
#endif				
        }        
       	else	/* UVC_Foramt_MJPEG */		
		{
#ifdef __UVC_VIN__			
			if(u32CurFrameIndex != uvcStatus.snapshotFrameIndex ||u32CurFormatIndex != uvcStatus.snapshotFormatIndex)
			{
				/* Frame Size Changed */
            	switch(uvcStatus.snapshotFrameIndex)
            	{										
					case UVC_STILL_VGA:							
						u16CurWidth = 640;
						u16CurHeight = 480;	
						break;										
					case UVC_STILL_QVGA:							
						u16CurWidth = 320;
						u16CurHeight = 240;
						break;			
					case UVC_STILL_QQVGA:									
						u16CurWidth = 160;
						u16CurHeight = 120;
						break;													
            	} 
            	/* Set VideoIn Setting for Frame Size */
            	ChangeFrame(TRUE, u32CurFrameAddress, u16CurWidth,u16CurHeight);
				u32CurFrameIndex = uvcStatus.snapshotFrameIndex;	
				u32CurFormatIndex = uvcStatus.snapshotFormatIndex;				
				/* Skip frames */
				u32SkipFrame = UVC_SKIP_FRAME;					
				while(u32SkipFrame)
			   		GetImageBuffer();					
			}	
			/* Encode JPEG */
			*pu32transferSize = (jpegEncode((UINT32)u32CurFrameAddress,(UINT32)u8jpegBitstream, u16CurWidth,u16CurHeight) + 3) & ~0x03;
			/* Set Address */
			*pu32Addr = (UINT32)u8jpegBitstream;	
#else
        	switch(uvcStatus.snapshotFrameIndex)
        	{											
				case UVC_VGA:
					u32BuffAddr1 = (UINT32)g_MJPEG_VGA_1;
					u32BuffAddr0 = (UINT32)g_MJPEG_VGA_0;	
					u32Size0 = 73728;	
					u32Size1 = 61440;							
					break;										
				case UVC_QVGA:
					u32BuffAddr1 = (UINT32)g_MJPEG_QVGA_1;
					u32BuffAddr0 = (UINT32)g_MJPEG_QVGA_0;	
					u32Size0 = 20480;	
					u32Size1 = 20480;																
					break;										
				case UVC_QQVGA:
					u32BuffAddr1 = (UINT32)g_MJPEG_QQVGA_1;
					u32BuffAddr0 = (UINT32)g_MJPEG_QQVGA_0;	
					u32Size0 = 8192;	
					u32Size1 = 8192;															
					break;						
        	}
        	
 			if(bufIndex)	/* Buffer address for Image 1 */
 			{
				*pu32Addr = u32BuffAddr1;
				*pu32transferSize = u32Size1;
			}
			else			/* Buffer address for Image 0 */
			{
				*pu32Addr = u32BuffAddr0;          	
     			*pu32transferSize = u32Size0; 
     		}				       	
#endif			
		}
		
	}
	else					/* Preview */
	{         						
        switch (uvcStatus.FormatIndex)
        {        
			case UVC_Format_YUY2:		/* UVC_Foramt_YUY2 */
        	{     	
	   			*pu32transferSize = uvcStatus.MaxVideoFrameSize;	   			
#ifdef __UVC_VIN__	 		
				if(u32CurFrameIndex != uvcStatus.FrameIndex || u32CurFormatIndex != uvcStatus.FormatIndex)
				{
					/* Frame Size Changed */
	            	switch(uvcStatus.FrameIndex)
	            	{										
						case UVC_VGA:							
							u16CurWidth = 640;
							u16CurHeight = 480;	
							break;										
						case UVC_QVGA:							
							u16CurWidth = 320;
							u16CurHeight = 240;
							break;			
						case UVC_QQVGA:									
							u16CurWidth = 160;
							u16CurHeight = 120;
							break;													
	            	} 
	            	/* Set VideoIn Setting for Frame Size */
	            	ChangeFrame(TRUE, u32CurFrameAddress, u16CurWidth,u16CurHeight);					
					u32CurFrameIndex = uvcStatus.FrameIndex;	
					u32CurFormatIndex = uvcStatus.FormatIndex; 		
					/* Skip frames */
					u32SkipFrame = UVC_SKIP_FRAME;	
					
					while(u32SkipFrame)
						GetImageBuffer();
				}	
				*pu32Addr = u32CurFrameAddress;
#else	
            	switch(uvcStatus.FrameIndex)
            	{										
					case UVC_VGA:
						u32BuffAddr1 = (UINT32)g_YUV_VGA_0;
						u32BuffAddr0 = (UINT32)g_YUV_VGA_1;	
						break;										
					case UVC_QVGA:
						u32BuffAddr1 = (UINT32)g_YUV_QVGA_0;
						u32BuffAddr0 = (UINT32)g_YUV_QVGA_1;										
						break;			
					case UVC_QQVGA:
						u32BuffAddr1 = (UINT32)g_YUV_QQVGA_0;
						u32BuffAddr0 = (UINT32)g_YUV_QQVGA_1;										
						break;													
            	}   
				if(bufIndex)	/* Buffer address for Image 1 */
					*pu32Addr = u32BuffAddr1;
				else			/* Buffer address for Image 0 */
					*pu32Addr = u32BuffAddr0; 
#endif
				break;			
            }            
        	case UVC_Foramt_MJPEG:     /* UVC_Foramt_MJPEG */
        	{		
#ifdef __UVC_VIN__        	
				if(u32CurFrameIndex != uvcStatus.FrameIndex || u32CurFormatIndex != uvcStatus.FormatIndex)
				{
					/* Frame Size Changed */
	            	switch(uvcStatus.FrameIndex)
	            	{										
						case UVC_VGA:							
							u16CurWidth = 640;
							u16CurHeight = 480;	
							break;										
						case UVC_QVGA:							
							u16CurWidth = 320;
							u16CurHeight = 240;
							break;			
						case UVC_QQVGA:									
							u16CurWidth = 160;
							u16CurHeight = 120;
							break;													
	            	} 
	            	/* Set VideoIn Setting for Frame Size */
	            	ChangeFrame(TRUE, u32CurFrameAddress, u16CurWidth,u16CurHeight);
					u32CurFrameIndex = uvcStatus.FrameIndex;	
					u32CurFormatIndex = uvcStatus.FormatIndex; 	
					/* Skip frames */
					u32SkipFrame = UVC_SKIP_FRAME;	
				}	
				/* Encode JPEG */
				*pu32transferSize = (jpegEncode((UINT32)u32CurFrameAddress,(UINT32)u8jpegBitstream, u16CurWidth,u16CurHeight) + 3) & ~0x03;
				/* Set Address */
				*pu32Addr = (UINT32)u8jpegBitstream;				             
#else

            	switch(uvcStatus.FrameIndex)
            	{											
					case UVC_VGA:
						u32BuffAddr1 = (UINT32)g_MJPEG_VGA_1;
						u32BuffAddr0 = (UINT32)g_MJPEG_VGA_0;	
						u32Size0 = 73728;	
						u32Size1 = 61440;							
						break;										
					case UVC_QVGA:
						u32BuffAddr1 = (UINT32)g_MJPEG_QVGA_1;
						u32BuffAddr0 = (UINT32)g_MJPEG_QVGA_0;	
						u32Size0 = 20480;	
						u32Size1 = 20480;																
						break;										
					case UVC_QQVGA:
						u32BuffAddr1 = (UINT32)g_MJPEG_QQVGA_1;
						u32BuffAddr0 = (UINT32)g_MJPEG_QQVGA_0;	
						u32Size0 = 8192;	
						u32Size1 = 8192;															
						break;					
            	}

	 			if(bufIndex)	/* Buffer address for Image 1 */
	 			{
					*pu32Addr = u32BuffAddr1;
					*pu32transferSize = u32Size1;
				}
				else			/* Buffer address for Image 0 */
				{
					*pu32Addr = u32BuffAddr0;          	
	     			*pu32transferSize = u32Size0; 
	     		}	     
				
#endif	
				break;			
            }
   		}       				      				   
	}
#ifndef __UVC_VIN__ 	
	/* The next preview image buffer for Fixed Pattern */
	bufIndex = (bufIndex + 1) % 2;	
#endif	
	return 1;
}

/* JPEG Encode function */
UINT32 jpegEncode(UINT32 u32YAddress,UINT32 u32BitstreamAddress, UINT16 u16Width,UINT16 u16Height)
{
		JPEG_INFO_T jpegInform;
		/* JPEG Init */
		jpegInit();	
						
		/* Set Source Y/U/V Stride */	       
	    jpegIoctl(JPEG_IOCTL_SET_YSTRIDE, u16Width, 0);	   
	    jpegIoctl(JPEG_IOCTL_SET_USTRIDE, u16Width/2, 0);
		jpegIoctl(JPEG_IOCTL_SET_VSTRIDE, u16Width/2, 0);			

	   	/* Set Source Address */	
		jpegIoctl(JPEG_IOCTL_SET_YADDR, u32YAddress, 0);
   		jpegIoctl(JPEG_IOCTL_SET_UADDR, (u32YAddress + u16Width * u16Height), 0);   		
		jpegIoctl(JPEG_IOCTL_SET_VADDR, (u32YAddress + u16Width*u16Height+u16Width*u16Height/2), 0);	
														
	    /* Set Bit stream Address */   
	    jpegIoctl(JPEG_IOCTL_SET_BITSTREAM_ADDR, u32BitstreamAddress, 0);
	    	
		/* Encode mode, encoding primary image, YUV 4:2:0 */
	    jpegIoctl(JPEG_IOCTL_SET_ENCODE_MODE, JPEG_ENC_SOURCE_PLANAR, JPEG_ENC_PRIMARY_YUV422);		
	    
	    /* Primary Encode Image Width / Height */    
	    jpegIoctl(JPEG_IOCTL_SET_DIMENSION, u16Height, u16Width);	       
	    
		/* Include Quantization-Table and Huffman-Table */	
	    jpegIoctl(JPEG_IOCTL_ENC_SET_HEADER_CONTROL, JPEG_ENC_PRIMARY_QTAB | JPEG_ENC_PRIMARY_HTAB, 0);
	       
		/* Use the default Quantization-table 0, Quantization-table 1 */
		jpegIoctl(JPEG_IOCTL_SET_DEFAULT_QTAB, 0, 0);
		
		/* Trigger JPEG encoder */
	    jpegIoctl(JPEG_IOCTL_ENCODE_TRIGGER, 0, 0);  
	    
	    /* Wait for complete */
		if(jpegWait())
		{
			jpegGetInfo(&jpegInform);
			return jpegInform.image_size[0];		
	    }
	    else
	    	return 0;
}

/* Process Unit Control */
UINT32 ProcessUnitControl(UINT32 u32ItemSelect,UINT32 u32Value)
{
	switch(u32ItemSelect)
	{
		case PU_BACKLIGHT_COMPENSATION_CONTROL:
				sysprintf("Set Backlight -> %d\n",u32Value);
				break;			
		case PU_BRIGHTNESS_CONTROL:
				sysprintf("Set Brightness -> %d\n",u32Value);
				break;			
		case PU_CONTRAST_CONTROL:
				sysprintf("Set Contrast -> %d\n",u32Value);	
			 	break;			
		case PU_HUE_CONTROL:
				sysprintf("Set Hue -> %d\n",u32Value);	
			 	break;			
		case PU_SATURATION_CONTROL:
				sysprintf("Set Saturation -> %d\n",u32Value);	
			 	break;			
		case PU_SHARPNESS_CONTROL:
				sysprintf("Set Sharpness -> %d\n",u32Value);	
			 	break;			
		case PU_GAMMA_CONTROL:		
				sysprintf("Set Gamma -> %d\n",u32Value);			
			 	break;
		case PU_POWER_LINE_FREQUENCY_CONTROL:	
				sysprintf("Set Power Line Frequency -> %d\n",u32Value);		
			 	break;		
/* audio function */			 	
		case PU_MUTE_CONTROL:	
				//sysprintf("Audio Mute function -> %d\n",u32Value);		
			 	break;		
		case PU_VOLUME_CONTROL:	
				sysprintf("Audio Volume function -> %d\n",u32Value);		
			 	break;				 		 	
	}
	return 0;
}
