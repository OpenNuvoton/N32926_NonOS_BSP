#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wbtypes.h"
#include "wbio.h"
#include "wblib.h"
#include "W55fa92_reg.h"
#include "stdio.h"
#include "rotlib.h"
#include "nvtfat.h"
#include "ROT_demo.h"



INT32 FileSize(char* szAsciiName)
{
	INT32 i32FileSize = 0;
	char suFileName[512];
	INT32 hFile;
	fsAsciiToUnicode(szAsciiName, suFileName, TRUE);
	hFile = fsOpenFile(suFileName, NULL, O_RDONLY);
	if (hFile < 0) {
		sysprintf("Fail in open file \n");
		return hFile;
	}
	i32FileSize = fsGetFileSize(hFile);				
	fsCloseFile(hFile);
	return i32FileSize;
}

INT32 ReadFile(char* szAsciiName, 
				PUINT16 pu16BufAddr, 
				INT32 i32Length)
{
	INT32 i32Ret = Successful;
	char suFileName[512];
	INT32 hFile, i32ToTran;
	UINT32 u32Handle;
	UINT32 u32Tmp;
	
	u32Tmp = ((UINT32)pu16BufAddr) | E_NONCACHE_BIT;
	pu16BufAddr = (PUINT16) u32Tmp;
	
	fsAsciiToUnicode(szAsciiName, suFileName, TRUE);
	sysprintf("Open source file %s\n", szAsciiName);
	hFile = fsOpenFile(suFileName, szAsciiName, O_RDONLY);
	if (hFile < 0) {
		// error here
		u32Handle = hFile;
		sysprintf("Fail in open Src file %x\n", u32Handle);
		return hFile;
	}
	else
		sysprintf("Succeed in open Src file \n");		
		
	i32Ret = fsReadFile(hFile, (PUINT8)pu16BufAddr, i32Length, &i32ToTran);				
	if (i32Ret < 0){ 
		sysprintf ("Fail to read file \n");
		return i32Ret;
	}	
	i32Ret = fsCloseFile(hFile);
	if(i32Ret==FS_OK)
		sysprintf("Close source File\n");
	else
	{
		sysprintf("Fail to close source File\n");	
		return -1;
	}	
	return Successful;	
		
}


#ifdef __LCM_320x240__
//src pattern 240*320
#define src_w_step 	3
#define src_h_step 	4
//dst pattern 320*240
#define dst_w_step 	4
#define dst_h_step 	3
#endif 


/*
	Constraint : Width/height need to be multiple of 2
	Destinationn Offset : YUV422 format need to be multiple of 4.	
*/
INT32 Emu_DestinationLineOffsetFineTune(UINT8* puDstAddr0, UINT8* puDstAddr1)
{
	UINT32 u32TestIdx, u32Offset; 
	INT32 i32FileSize;
	CHAR szFileName[256];
	T_ROT_CONF tRotConf;
	BOOL bIsReadFile;
	UINT32 u32Width, u32Height;	
	UINT32 Buf=0;
	
	
	typedef struct tagRot
	{
		UINT32 u32SrcImgHW;
		UINT32 u32RotFormat;	//E_PACKET_RGB565 = 0,
							//E_PACKET_RGB888,
							//E_PACKET_YUV422
						
		UINT32 u32RotDir;		//E_ROT_LEFT_90
							//E_ROT_RIGHT_90					
		UINT32 u32SrcLineOffset;
		UINT32 u32DstLineOffset;
		
		UINT32 u32BufSize;		//8, 16. 32
		char  *pszFileName;
	}T_ROT_FMT;
	
	T_ROT_FMT tRotFmt[] = {	

	0x014000F0,				E_ROT_PACKET_RGB565, E_ROT_ROT_R90, 	0, 0,	E_LBUF_16,		"SkyDivin_PACKET_RGB565_size240x320.dat",
	((320-8)<<16| (240-6)),		E_ROT_PACKET_RGB565, E_ROT_ROT_R90, 	6, 8,	E_LBUF_16,		"SkyDivin_PACKET_RGB565_size240x320.dat",
	((320-8*2)<<16| (240-6*2)),	E_ROT_PACKET_RGB565, E_ROT_ROT_R90, 	6*2, 8*2,	E_LBUF_16,	"SkyDivin_PACKET_RGB565_size240x320.dat",
	((320-8*3)<<16| (240-6*3)),	E_ROT_PACKET_RGB565, E_ROT_ROT_R90, 	6*3, 8*3,	E_LBUF_16,	"SkyDivin_PACKET_RGB565_size240x320.dat",
	((320-8*4)<<16| (240-6*4)),	E_ROT_PACKET_RGB565, E_ROT_ROT_R90, 	6*4, 8*4,	E_LBUF_16,	"SkyDivin_PACKET_RGB565_size240x320.dat",
	
	
	0x014000F0,				E_ROT_PACKET_RGB565, E_ROT_ROT_L90, 	0, 0,	E_LBUF_16,		"SkyDivin_PACKET_RGB565_size240x320.dat",
	((320-8)<<16| (240-6)),		E_ROT_PACKET_RGB565, E_ROT_ROT_L90, 	6, 8,	E_LBUF_16,		"SkyDivin_PACKET_RGB565_size240x320.dat",
	((320-8*2)<<16| (240-6*2)),	E_ROT_PACKET_RGB565, E_ROT_ROT_L90, 	6*2, 8*2,	E_LBUF_16,	"SkyDivin_PACKET_RGB565_size240x320.dat",
	((320-8*3)<<16| (240-6*3)),	E_ROT_PACKET_RGB565, E_ROT_ROT_L90, 	6*3, 8*3,	E_LBUF_16,	"SkyDivin_PACKET_RGB565_size240x320.dat",
	((320-8*4)<<16| (240-6*4)),	E_ROT_PACKET_RGB565, E_ROT_ROT_L90, 	6*4, 8*4,	E_LBUF_16,	"SkyDivin_PACKET_RGB565_size240x320.dat",
	
				};	
   	rotOpen();
   	rotInstallISR(E_ROT_COMP_INT, (PVOID)rotDoneHandler);
   	rotInstallISR(E_ROT_ABORT_INT, (PVOID)rotAbortHandler);
   
	bIsReadFile = TRUE;
	
	sysprintf("Totatl Test item = %d\n", sizeof(tRotFmt)/sizeof(tRotFmt[0]));
	while(1)
	{
		for(u32TestIdx=0; u32TestIdx<sizeof(tRotFmt)/sizeof(tRotFmt[0]); u32TestIdx=u32TestIdx+1)
		{	
			
			sysprintf("Test item = %d\n", u32TestIdx);
			//for(u32Offset=0; u32Offset< 32; u32Offset=u32Offset+1)
			for(u32Offset=0; u32Offset< 1; u32Offset=u32Offset+1)
			{//u32Offset: Pixel unit

				tRotConf.eBufSize = (E_ROTENG_BUFSIZE)tRotFmt[u32TestIdx].u32BufSize;				//Assigned the buffer size for on the fly rotation
				tRotConf.eRotDir = (E_ROTENG_DIR)tRotFmt[u32TestIdx].u32RotDir;					//Left/Right
			
				u32Height = tRotFmt[u32TestIdx].u32SrcImgHW>>16;
				u32Width = tRotFmt[u32TestIdx].u32SrcImgHW&0x0FFFF;
				tRotConf.eRotFormat = (E_ROTENG_FMT)tRotFmt[u32TestIdx].u32RotFormat;
				#if 0				
				switch(tRotConf.eRotFormat)
				{
					case E_ROT_PACKET_RGB565:	sysprintf("Test RGB565\n"); 	break;
					case E_ROT_PACKET_YUV422:	sysprintf("Test YUV422\n"); 	break;
					case E_ROT_PACKET_RGB888:	sysprintf("Test RGB888\n"); 	break;				
				}
				#endif
				
				tRotConf.u32RotDimHW = (u32Height <<16 | u32Width);	
				tRotConf.u32SrcLineOffset = tRotFmt[u32TestIdx].u32SrcLineOffset;	//Source line offset
				tRotConf.u32DstLineOffset= tRotFmt[u32TestIdx].u32DstLineOffset;		//Destination  line offset				
				
				/* Left-Top Corner */
				tRotConf.u32SrcAddr = ADDR_ROT_SRC_ADDR+ (u32TestIdx%5)*src_h_step*OPT_LCM_HEIGHT*2+ (u32TestIdx%5)*src_w_step*2;		//Source buffer start address of rotated image	
				/* Left-Top Corner */
				if(VpostUseBuf==1){/* If VPOST shows buffer 1, ROT should use buffer 0 */
					tRotConf.u32DstAddr = (UINT32)puDstAddr0 + (u32TestIdx%5)*dst_h_step*OPT_LCM_WIDTH*2 + (u32TestIdx%5)*dst_w_step*2;		//Turn Left ok
					Buf = 0;
				}	
				else if(VpostUseBuf==0){		
					tRotConf.u32DstAddr = (UINT32)puDstAddr1 + (u32TestIdx%5)*dst_h_step*OPT_LCM_WIDTH*2 + (u32TestIdx%5)*dst_w_step*2;		//Turn Left ok
					Buf = 1;
				}
				/* Read source pattern from SD */				
				if(bIsReadFile==TRUE)
				{	
					szFileName[0] = 0;
					sprintf(szFileName, "C:\\pattern\\");
					strcat(szFileName, tRotFmt[u32TestIdx].pszFileName);
					i32FileSize= FileSize(szFileName);    				
					ReadFile(szFileName,(PUINT16)ADDR_ROT_SRC_ADDR, i32FileSize);
					bIsReadFile = FALSE;
							
				}	
				/* Clear ROT destination buffer */		
				if(VpostUseBuf==1){
					memset((char*)(((UINT32)puDstAddr0) | E_NONCACHE_BIT), 0x0, 240*320*2); //Offset need to extra space	
				}	
				else{
					memset((char*)(((UINT32)puDstAddr1) | E_NONCACHE_BIT), 0x0, 240*320*2); //Offset need to extra space	
				}	
									
				/* Set parameter then trtigger ROT */
				rotImageConfig(&tRotConf);
				rotClearDoneFlag();
				rotTrigger();		
				while(rotGetDoneFlag()==FALSE);	
				if(Buf==0)
					bIsBuffer0Dirty = TRUE;
				else
					bIsBuffer1Dirty = TRUE;
				sysprintf("Delay 1 second\n");	
				sysDelay(100);								

			}//for(u32Offset=0; u32Offset< 1; u32Offset=u32Offset+1)	
		}//for(u32TestIdx=0; u32TestIdx<2; u32TestIdx=u32TestIdx+1)		
	}

}























