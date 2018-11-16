#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "w55fa92_sic.h"
#include "W55FA92_AudioRec.h"
#include "nvtfat.h"
#include "AurExample.h"

//#define DBG_PRINTF		sysprintf

INT32 WriteFile(char* szAsciiName, 
					PUINT16 pu16BufAddr, 
					UINT32 u32Length)
{
	INT hFile, i32Errcode = Successful, u32WriteLen;
	char suFileName[512];
	
	fsAsciiToUnicode(szAsciiName, suFileName, TRUE);
	hFile = fsOpenFile(suFileName, NULL, O_CREATE);
	if (hFile < 0)
	{
		DBG_PRINTF("Fail to creating file \n");					
		return hFile;
	}	
	else
		DBG_PRINTF("Succeed to creating file \n");			
	i32Errcode = fsWriteFile(hFile, 
							(UINT8 *)pu16BufAddr, 
							u32Length, 
							&u32WriteLen);
	if (i32Errcode < 0)
		return i32Errcode;	
		
	fsCloseFile(hFile);
	return Successful;	
}
char WaveHeader[]= {'R', 'I', 'F', 'F', 0x00, 0x00, 0x00, 0x00,	   //ForthCC code+(RAW-data-size+0x24)	
					'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',			
					0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,//Chunk-size, audio format, and NUMChannel
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//Sample-Rate and Byte-Count-Per-Sec 
					0x02, 0x00, 0x10, 0x00,						   //Align and Bits-per-sample.
					'd', 'a', 't', 'a', 0x00, 0x00, 0x00, 0x00};   //"data"and RAW-data-size		
						
INT32 AudioWriteFile(char* szAsciiName, 
					PUINT16 pu16BufAddr, 
					UINT32 u32Length,
					UINT32 u32SampleRate)
{
	INT hFile;//, i32Errcode = Successful, u32WriteLen;
	char suFileName[512];
//	UINT32 *pu32ptr; 
	
	strcat(szAsciiName, ".wav");
	fsAsciiToUnicode(szAsciiName, suFileName, TRUE);
	hFile = fsOpenFile(suFileName, NULL, O_CREATE);
	if (hFile < 0)
	{
		DBG_PRINTF("Fail to creating file \n");					
		return hFile;
	}	
	else
		DBG_PRINTF("Succeed to creating file \n");
	return Successful;		
#if 0
	pu32ptr =  (UINT32*)(WaveHeader+4);
	*pu32ptr = u32Length+0x24;		
	pu32ptr = (UINT32*)(WaveHeader+0x28); //(UINT32*)WaveHeader[0x28];
	*pu32ptr = u32Length;
	pu32ptr = (UINT32*)(WaveHeader+0x18);//(UINT32*)WaveHeader[0x10];
	*pu32ptr = u32SampleRate;
	pu32ptr = (UINT32*)(WaveHeader+0x1C);//(UINT32*)WaveHeader[0x14];
	*pu32ptr = u32SampleRate*2;				//16bits 
	i32Errcode = fsWriteFile(hFile, 
							(UINT8 *)WaveHeader, 
							0x2C, 
							&u32WriteLen);	
	if (i32Errcode < 0)
		return i32Errcode;											
	i32Errcode = fsWriteFile(hFile, 
							(UINT8 *)pu16BufAddr, 
							u32Length, 
							&u32WriteLen);
	if (i32Errcode < 0)
		return i32Errcode;	
		
	fsCloseFile(hFile);
	return Successful;	
#endif
}


INT32 ReadFile(char* szAsciiName, 
				PUINT16 pu16BufAddr, 
				UINT32 u32Length)
{
	INT32 i32Ret = Successful;
	char suFileName[512];
	INT32 hFile, i32ToTran;
	
	fsAsciiToUnicode(szAsciiName, suFileName, TRUE);
	hFile = fsOpenFile(suFileName, NULL, O_RDONLY);
	if (hFile < 0) {
		// error here
		sysprintf("Fail in creating Src file \n");
		return hFile;
	}
	else
		sysprintf("Succeed in creating Src file \n");		
		
	i32Ret = fsReadFile(hFile, (PUINT8)pu16BufAddr, u32Length, &i32ToTran);				
	if (i32Ret < 0){ 
		DBG_PRINTF ("Fail to read file \n");
		return i32Ret;
	}	
	fsCloseFile(hFile);
	return Successful;	
		
}

INT32 AudioOpenFile(char* szAsciiName)
{
	INT hFile;
	char suFileName[256];

	fsAsciiToUnicode(szAsciiName, suFileName, TRUE);
	hFile = fsOpenFile(suFileName, NULL, O_CREATE);
	if (hFile < 0)
	{
		DBG_PRINTF("Fail to creating file \n");					
		return hFile;
	}	
	else
		DBG_PRINTF("Succeed to creating file \n");	
	return hFile;						
}			

INT32 AudioWriteFileHead(char* szAsciiName, 
					UINT32 u32Length,
					UINT32 u32SampleRate)
{
	INT hFile, i32Errcode, u32WriteLen;
	char suFileName[256];
	UINT32 *pu32ptr; 
	
	strcat(szAsciiName, ".wav");
	fsAsciiToUnicode(szAsciiName, suFileName, TRUE);
	hFile = fsOpenFile(suFileName, NULL, O_CREATE);
	if (hFile < 0)
	{
		DBG_PRINTF("Fail to creating file \n");					
		return hFile;
	}	
	else
		DBG_PRINTF("Succeed to creating file \n");	
#if 0		
	pu32ptr =  (UINT32*)(WaveHeader+4);
	*pu32ptr = u32Length+0x24;		
	pu32ptr = (UINT32*)(WaveHeader+0x28); //(UINT32*)WaveHeader[0x28];
	*pu32ptr = u32Length;
	pu32ptr = (UINT32*)(WaveHeader+0x18);//(UINT32*)WaveHeader[0x10];
	*pu32ptr = u32SampleRate;
	pu32ptr = (UINT32*)(WaveHeader+0x1C);//(UINT32*)WaveHeader[0x14];
	*pu32ptr = u32SampleRate*2;				//16bits 
#else
	pu32ptr =  (UINT32*) ((UINT32)(WaveHeader+4) | 0x80000000);
	*pu32ptr = u32Length+0x24;		
	pu32ptr = (UINT32*)((UINT32)(WaveHeader+0x28) | 0x80000000) ; //(UINT32*)WaveHeader[0x28];
	*pu32ptr = u32Length;
	pu32ptr = (UINT32*)((UINT32)(WaveHeader+0x18) | 0x80000000);//(UINT32*)WaveHeader[0x10];
	*pu32ptr = u32SampleRate;
	pu32ptr = (UINT32*)((UINT32)(WaveHeader+0x1C) | 0x80000000);//(UINT32*)WaveHeader[0x14];
	*pu32ptr = u32SampleRate*2;				//16bits 
#endif	
	i32Errcode = fsWriteFile(hFile, 
							(UINT8 *)((UINT32)WaveHeader | 0x80000000), 
							0x2C, 
							&u32WriteLen);	
	return hFile;						
}				
INT32 AudioWriteFileData(INT hFile, UINT16* pu16BufAddr, UINT32 u32Length)
{
	INT i32Errcode, u32WriteLen;
	UINT32 u32Tmp;
	
	u32Tmp = ((UINT32)pu16BufAddr) | E_NONCACHE_BIT;
	pu16BufAddr = (PUINT16) u32Tmp;
	
	i32Errcode = fsWriteFile(hFile, 
						(UINT8 *)pu16BufAddr, 
						u32Length, 
						&u32WriteLen);
	if(i32Errcode<0)
		return i32Errcode;
	else
		return Successful;						
}
INT32 AudioWriteFileClose(INT32 hFile)
{
	fsCloseFile(hFile);
	return Successful;	
}
