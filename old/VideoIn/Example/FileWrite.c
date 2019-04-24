#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "nvtfat.h"
#include "demo.h"

INT32 WriteFile(char* szAsciiName, 
					PUINT16 pu16BufAddr, 
					UINT32 u32Length)
{
	INT32 i32Ret = Successful;
	INT hFile, i32WriteLen;
	char suFileName[512];
	
	fsAsciiToUnicode(szAsciiName, suFileName, TRUE);
	hFile = fsOpenFile(suFileName, szAsciiName, O_CREATE);
	sysprintf("Open Dst File 0x%x\n", (UINT32)hFile);
	if (hFile < 0)
	{
		sysprintf("Fail to creating file 0x%x\n", (UINT32)hFile);					
		while(1);
//		return hFile;
	}	
	else
		sysprintf("Succeed to creating file \n");			
	i32Ret = fsWriteFile(hFile, 
							(UINT8 *)((UINT32)pu16BufAddr | E_NONCACHE_BIT), 
							u32Length, 
							&i32WriteLen);
	if (i32Ret < 0)	
	{
		sysprintf("Fail to write dst file\n");	
		while(1);
//		return i32Ret;	
	}
	i32Ret = fsCloseFile(hFile);
	if(i32Ret==FS_OK)
		sysprintf("Close Dst File ok\n");	
	else
	{
		sysprintf("Close Dst File fail\n");		
		while(1);
	}
	return i32Ret;	
}
