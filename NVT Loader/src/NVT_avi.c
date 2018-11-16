/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "w55fa92_vpost.h"
#include "AviLib.h"
#include "nvtfat.h"
#include "nvtloader.h"
#include "w55fa92_ts_adc.h"
#include "w55fa92_gpio.h"
#include "SPU.h"
#include "w55fa92_spu.h"

static int _complete = 0;
static int _offset = 0;
static int _fd;


extern unsigned char g_kbuf[];
extern BOOL bIsIceMode;
void initVPost(unsigned char*);




static BOOL bEscapeKeyPress=FALSE;
static BOOL bSetVolume=FALSE;
UINT16 u16Volume = 60;		// 60*63/100=; Linux driver default = 60 
UINT32 u32Saturation = 60;		// 60*63/100=; Linux driver default = 60 

// Detect Earphone plug in?
UINT32 u32EarphoneDetChannel = 0;
UINT32 u32Flag20ms=0;

//#define __OPT_SPEAKER_DECADE_1DB__

#ifdef __OPT_SPEAKER_DECADE_1DB__		
volatile UINT8 gb_IsFirstChkEarphone = TRUE;
volatile UINT8 gb_IsEarphoneState = TRUE;
extern void spuSwitchVolume(UINT8);
#endif
BOARD_S* ps_avi_boardinfo;

void Timer0_200msCallback(void)
{
	if(ps_avi_boardinfo ->earphone_detect()==FALSE)
	{
		DBG_PRINTF("Speaker High\n");
		ps_avi_boardinfo->spkpower_enable();
		
#ifdef __OPT_SPEAKER_DECADE_1DB__		
		if (gb_IsFirstChkEarphone == TRUE)
		{
			gb_IsFirstChkEarphone = FALSE;
			gb_IsEarphoneState = FALSE;
			spuSwitchVolume(gb_IsEarphoneState);
		}
		else
		{
			if (gb_IsEarphoneState == TRUE)
			{
				gb_IsEarphoneState = FALSE;
				spuSwitchVolume(gb_IsEarphoneState);			
			}
		}
#endif					
	}	
	else
	{
		DBG_PRINTF("Speaker Low\n");
		ps_avi_boardinfo->spkpower_disable();	

#ifdef __OPT_SPEAKER_DECADE_1DB__		
		if (gb_IsFirstChkEarphone == TRUE)
		{
			gb_IsFirstChkEarphone = FALSE;
			gb_IsEarphoneState = TRUE;
		}
		else
		{
			if (gb_IsEarphoneState == FALSE)
			{
				gb_IsEarphoneState = TRUE;
				spuSwitchVolume(gb_IsEarphoneState);			
			}
		}
#endif					
	}	
}

/* Volume config file locate in NAND disk */
void VolumeConfigFile(void)
{
	INT8 path[64];
	INT32 i32FileHandle;
	/* Check if volume config file exists */
	fsAsciiToUnicode(VOLUME_PATH, path, TRUE);
	i32FileHandle = fsOpenFile(path, 0, O_RDONLY);
	if(i32FileHandle > 0)
	{
		INT32		nStatus, nLen;
		UINT8  		u8VolCfg[2];
		nStatus = fsReadFile(i32FileHandle, u8VolCfg, 2, &nLen);
		if(nStatus>=0)
		{
			u16Volume = u8VolCfg[0]|(((UINT16)u8VolCfg[1])<<8);
			//u16Volume = u16Volume * 31/100;
			sysprintf("Volume = %d\n", u16Volume);
		}
		fsCloseFile(i32FileHandle);
	}
}


void loadKernel(AVI_INFO_T *aviInfo)
{
	int bytes, result;
	
	if(bSetVolume==FALSE)
	{
#ifdef __AVI_PLAYBACK__	
		aviSetPlayVolume(u16Volume);
#else 
		spuSetVolume(u16Volume, u16Volume);	
#endif		
		sysprintf("Volume = %d\n", u16Volume);	
		bSetVolume = TRUE;				
	}
	
	if(bEscapeKeyPress==TRUE)	
		return;
	//sysprintf("\n");
	if(!_complete) {
		if(_offset < CP_SIZE)
		{//1th, keep the original vector table in  SDRAM. 			
			result = fsReadFile(_fd, (g_kbuf + _offset), (CP_SIZE - _offset), &bytes);
		}	
		else
		{//2nd, 3rd, .... Copy the kernel content to address 16K, 32K, 	
			result = fsReadFile(_fd, (UINT8 *)_offset, CP_SIZE, &bytes);
		}	
		if(result == FS_OK)
			_offset += bytes;
		else
			_complete = 1;
	}
	if(bEscapeKeyPress==FALSE)
	{
		result = kpi_read(KEY_ADC_CHANNEL);		
		if(result == HOME_KEY)
		{//Stop AVI playback
			bEscapeKeyPress = TRUE;
			sysprintf("Key pressed %d\n", result);
#ifdef __AVI_PLAYBACK__						
			aviStopPlayFile();				
			aviSetPlayVolume(u16Volume);		
#endif						
			sysprintf("Volume = %d\n", u16Volume);
		}		
	}	
	return;
}

void loadKernelCont(int fd, int offset, unsigned char* pkBuf)
{	
	int bytes, result;
	
	while(1) {
		if(offset < CP_SIZE)		
			result = fsReadFile(fd, (pkBuf + offset), (CP_SIZE - offset), &bytes);
		else
			result = fsReadFile(fd, (UINT8 *)offset, CP_SIZE, &bytes);	
		if(result == FS_OK)
			offset += bytes;
		else
			return;	
	}
}
extern LCDFORMATEX lcdInfo;
void lcmFill2Dark(unsigned char* fb)
{
	if(lcdInfo.ucVASrcFormat == DRVVPOST_FRAME_YCBYCR)
	{	
		UINT32 i;
		UINT32* ptBufAddr=(UINT32*)((UINT32)fb | 0x80000000);
		for(i=0;i<(PANEL_WIDTH * PANEL_HEIGHT * PANEL_BPP);i=i+4)
		{
			outpw(ptBufAddr, 0x80108010);
			ptBufAddr = ptBufAddr+1;		//2 pixels 
		}
	}
	else if(lcdInfo.ucVASrcFormat == DRVVPOST_FRAME_RGB565)
	{
		memset((char*)fb, 0, PANEL_WIDTH * PANEL_HEIGHT * PANEL_BPP);
	} 
}
extern UINT8 g_kbuf[CP_SIZE];

void playAnimation(BOARD_S* ps_board, int kfd, char* pcString)
{
	char aniPath[64];	
	ps_avi_boardinfo = ps_board;
	if(ps_board ->earphone_detect()==FALSE)	/* Fixed the defaut speaker out while earphone attached */
		ps_board->spkpower_enable();
	else
		ps_board->spkpower_disable();		
						
	fsAsciiToUnicode(pcString, aniPath, TRUE);
	ps_board->mute_disable();
	_fd = kfd; // let callback function know the file descriptor
	bEscapeKeyPress = FALSE;
#ifdef __AVI_PLAYBACK__	
	if (aviPlayFile(aniPath, 0, 0, DIRECT_RGB565, (kfd > 0) ? loadKernel : NULL) < 0)
		DBG_PRINTF("Playback failed\n");
	else
		DBG_PRINTF("Playback done.\n");
#endif
	// If movie is too short for callback function to load kernel to SDRAM, keep working...
	if(kfd > 0 && _complete == 0) {
		loadKernelCont(kfd, _offset, g_kbuf);
	}
	
	if(kfd > 0) {
		fsCloseFile(_fd);
	}
	lcmFill2Dark((unsigned char *)FB_ADDR);
	return;

}
