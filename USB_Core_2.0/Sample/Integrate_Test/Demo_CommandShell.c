/*************************************************************************
 * Nuvoton Electronics Corporation confidential
 *
 * Copyright (c) 2008 by Nuvoton Electronics Corporation
 * All rights reserved
 * 
 * FILENAME
 *     Demo_CommandShell.c
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     Command shell like demo program
 *
 * HISTORY
 *     2008.06.25		Created
 *
 * REMARK
 *     None
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wbtypes.h"
#include "wbio.h"
#include "wblib.h"

#include "nvtfat.h"
//#include "usb_lib.h"
#include "w55fa92_reg.h"

//#define SYS_CLOCK			15000000
#define SYS_CLOCK			2700000
#define UART_BAUD_RATE		115200

#define ENABLE_USB_HOST
#define ENABLE_FMI
#define ENABLE_GNAND
//#define TEST_PWOER_DOWN


#define get_timer_ticks()	sysGetTicks(TIMER0)

//#define DIRECT_MODE

extern void  fsGetErrorDescription(INT nErrCode, CHAR *szDescription, INT bIsPrint);
extern INT   fsBIG5toUnicode(void *bstr, void *ustr);


/* imported from WBFILE_DISK.C */
extern PDISK_T		*_fs_ptPDiskList;

#define DUMMY_BUFFER_SIZE		(64 * 1024)

__align(32) UINT8  _pucDummy[DUMMY_BUFFER_SIZE];

static CHAR *_pcFileCommads[] = 
{
	"?",    "HELP", "DIR",    "TYPE",  "CD",    "DEL", 
	"COPY", "MKDIR","MD",     "RMDIR", "RD",	"RENAME",
	"CMP",	"DS", 	"RDTST",  "WRTST", "APTST", "RAWRD",
	"DISK",	"DF", 	"FORMAT", "TEST",  "DELA",  "DIRS",
	"RAWWD","USB",
	NULL
};

enum  EnumerateCommandList 
{
	QQ=0,   HELP,   DIR,    TYPE,   CD,     DEL,
	COPY,   MKDIR,  MD,     RMDIR,  RD,		RENAME,
	CMP,	DUMP,	RDTST,  WRTST,	APTST,	RAWRD,
	DISK,	DF,		FORMAT, TEST,	DELA,   DIRS,
	RAWWD,	USB,
	CHANGE
};

static CHAR _szCommandLine[512];

static CHAR _pszCommandShellHelp[] = 
{
	"WBFile Command Shell Command List -\n"
	"    <HELP/?>    command help\n"
	"    <DIR>       list files under the current directory\n"
	"    <TYPE>      print file contents on console\n"
	"    <CD>        change directory\n"
	"    <DEL>       delete a file\n"
	"    <COPY>      copy a file\n"
	"    <MKDIR/MD>  build an empty directory\n"
	"    <RMDIR/RD>  remove an empty directory\n"
	"    <RENAME>    rename a file\n"
	"    <CMP>       compare two file\n"
	"    <DS>        dump current disk sector\n"
	"    <RDTST>     read speed test\n"
	"    <WRTST>     write speed test\n"
	"    <APTST>     file append test\n"
	"    <DISK>      print disk information\n"
	"    <x:>        change working drive, where x is the drive character\n"
	"\n\n"
};


static INT   _nCurrentDrive;
static CHAR  _pszWorkingDirectory[10][300] =
{
	"C:",
	"D:",
	"E:",
	"F:",
	"G:",
	"H:",
	"I:",
	"J:",
	"K:",
	"L:",
};



extern void FAT_dump_sector_cache(void);

static INT  get_command_code(CHAR *szCmd)
{
	INT     nIdx = 0;
	
	if ((szCmd[1] == ':') && (szCmd[2] == 0))
		return CHANGE;		/* change drive */
	
	while (1)
	{
		if (_pcFileCommads[nIdx] == NULL)
			return -1;      /* command not found */
					
		if (!strcmp(_pcFileCommads[nIdx], szCmd))
			return nIdx;
			
		nIdx++;
	}
}


static CHAR  *get_token(CHAR *szString, CHAR *pcSepCharList, INT nMaxLen)
{
	CHAR    *pcParsePtr = szString;
	CHAR    *pcPtr; 
	INT     nLen = 0;

	if (pcParsePtr == NULL)
		return NULL;
	
	while (*pcParsePtr)
	{
		pcPtr = pcSepCharList;
		while (*pcPtr)
		{
			if (*pcParsePtr == *pcPtr)
			{
				*pcParsePtr++ = 0;
				return pcParsePtr;
			}
			pcPtr++;
		}
		pcParsePtr++;
		if (++nLen >= nMaxLen)
			return NULL;        /* string too long!! */
	}
	
	return pcParsePtr;        /* reach end of the input string */
}



static void  accept_string(CHAR *pcString, INT nLength)
{
	CHAR    chr;
	INT		nCount;

	nCount = 0;

	while (1)
	{
		chr = sysGetChar();
		if (chr == 0)               /* control character pressed */
		{
			chr = sysGetChar();
			if (chr == 0x4b)        /* left arrow key */
			{
				if (nCount > 0)      /* have characters in pcString */
				{
					nCount--;        /* delete a character */
					sysprintf("%c%c%c", 0x08, 0x20, 0x08);
				}
			}
		}
		else if (chr == 0x08)       /* backspace */
		{
			if (nCount > 0)          /* have characters in pcString */
			{
				nCount--;            /* delete a character */
				sysprintf("%c%c%c", 0x08, 0x20, 0x08);
			}
		}
		else if (chr == 10)
			;
		else
		{
			if ((chr == 0xd) || (chr == '!'))
			{
				if (nCount == 0)
					sysprintf("\n");
				pcString[nCount] = 0;
				return;
			}
		  
			sysprintf("%c", chr);
			pcString[nCount] = chr;            // read in a character
			nCount++;
			if (nCount >= nLength)
			{
				nCount--;
				sysprintf("%c", 0x08);            // backspace
			}
		}
	}
}



static INT  Action_DIR(CHAR *suDirName)
{
	INT				i, nStatus;
	CHAR			szMainName[12], szExtName[8], *pcPtr;
	CHAR			szLongName[MAX_FILE_NAME_LEN/2];
	FILE_FIND_T  	tFileInfo;
	CHAR			szLabel[16];
	UINT32 			uBlockSize, uFreeSize, uDiskSize;

	fsGetVolumeLabel(suDirName[0], szLabel, 16);
	sysprintf("[%s]\n", szLabel);

	memset((UINT8 *)&tFileInfo, 0, sizeof(tFileInfo));
	nStatus = fsFindFirst(suDirName, NULL, &tFileInfo);
	if (nStatus < 0)
		return nStatus;

	do 
	{
		pcPtr = tFileInfo.szShortName;
		if ((tFileInfo.ucAttrib & A_DIR) && 
			(!strcmp(pcPtr, ".") || !strcmp(pcPtr, "..")))
			strcat(tFileInfo.szShortName, ".");

		memset(szMainName, 0x20, 9);
		szMainName[8] = 0;
		memset(szExtName, 0x20, 4);
		szExtName[3] = 0;
		i = 0;
		while (*pcPtr && (*pcPtr != '.'))
			szMainName[i++] = *pcPtr++;
		if (*pcPtr++)
		{
			i = 0;
			while (*pcPtr)
				szExtName[i++] = *pcPtr++;
		}

		fsUnicodeToAscii(tFileInfo.suLongName, szLongName, 1);
		
		if (tFileInfo.ucAttrib & A_DIR)
		{
			sysprintf("%s %s      <DIR>  %02d-%02d-%04d  %02d:%02d",
						szMainName, szExtName, 
						tFileInfo.ucWDateMonth, tFileInfo.ucWDateDay, (tFileInfo.ucWDateYear+80)%100 ,
						tFileInfo.ucWTimeHour, tFileInfo.ucWTimeMin);
			sysprintf("  %s\n", fsDebugUniStr(tFileInfo.suLongName));
		}
		else
		{
			sysprintf("%s %s %10d  %02d-%02d-%04d  %02d:%02d",
						szMainName, szExtName, tFileInfo.n64FileSize,
						tFileInfo.ucWDateMonth, tFileInfo.ucWDateDay, (tFileInfo.ucWDateYear+80)%100 ,
						tFileInfo.ucWTimeHour, tFileInfo.ucWTimeMin);
			sysprintf("  %s\n", fsDebugUniStr(tFileInfo.suLongName));
		}

	} while (!fsFindNext(&tFileInfo));
	
	fsFindClose(&tFileInfo);
	
	fsDiskFreeSpace(suDirName[0], &uBlockSize, &uFreeSize, &uDiskSize);
	
	sysprintf("\nDisk Size: %d Kbytes, Free Space: %d KBytes\n", (INT)uDiskSize, (INT)uFreeSize);

	return 0;
}


static INT  Action_DIRS(CHAR *suDirName)
{
	INT		nFileCnt, nDirCnt; 
	UINT64 	uTotalSize;
	INT   	nStatus;
	
	nStatus = fsGetDirectoryInfo(suDirName, NULL, &nFileCnt, &nDirCnt, &uTotalSize, TRUE);
	sysprintf("File = %d, Dir = %d, Total Size = %d\n", nFileCnt, nDirCnt, (INT)uTotalSize);
	return nStatus;
}


static INT  Action_DIR2(CHAR *suDirName)
{
	INT				i, nStatus;
	CHAR			szMainName[12], szExtName[8], *pcPtr;
	CHAR			szLongName[MAX_FILE_NAME_LEN/2];
	CHAR			suLongName[MAX_FILE_NAME_LEN];
	CHAR			suSlash[6] = { '\\', 0, 0, 0 };
	FILE_FIND_T  	tFileInfo;
	UINT32 			uBlockSize, uFreeSize, uDiskSize;

	memset((UINT8 *)&tFileInfo, 0, sizeof(tFileInfo));
	sysprintf("DIR => %s\n", fsDebugUniStr(suDirName));
	nStatus = fsFindFirst(suDirName, NULL, &tFileInfo);
	if (nStatus < 0)
	{
		sysprintf("fsFindFirst <%s> error = %x\n\n\n", fsDebugUniStr(suDirName), nStatus);
		return nStatus;
	}

	do 
	{
		pcPtr = tFileInfo.szShortName;
		if ((tFileInfo.ucAttrib & A_DIR) && 
			(!strcmp(pcPtr, ".") || !strcmp(pcPtr, "..")))
			strcat(tFileInfo.szShortName, ".");

		memset(szMainName, 0x20, 9);
		szMainName[8] = 0;
		memset(szExtName, 0x20, 4);
		szExtName[3] = 0;
		i = 0;
		while (*pcPtr && (*pcPtr != '.'))
			szMainName[i++] = *pcPtr++;
		if (*pcPtr++)
		{
			i = 0;
			while (*pcPtr)
				szExtName[i++] = *pcPtr++;
		}

		fsUnicodeToAscii(tFileInfo.suLongName, szLongName, 1);
		
		if (tFileInfo.ucAttrib & A_DIR)
		{
			sysprintf("%s %s      <DIR>  %02d-%02d-%04d  %02d:%02d  %s\n",
						szMainName, szExtName, 
						tFileInfo.ucWDateMonth, tFileInfo.ucWDateDay, (tFileInfo.ucWDateYear+80)%100 ,
						tFileInfo.ucWTimeHour, tFileInfo.ucWTimeMin, szLongName);
			fsUnicodeCopyStr(suLongName, suDirName);
			if (fsUnicodeStrLen(suDirName) > 6)
				fsUnicodeStrCat(suLongName, suSlash);	/* not C:\ */
			fsUnicodeStrCat(suLongName, tFileInfo.suLongName);
			if ((tFileInfo.szShortName[0] != '.') &&
				(strncmp(tFileInfo.szShortName, "RECYCLED", 8)))
				Action_DIR2(suLongName);
		}
#if 0		
		else
			sysprintf("%s %s %10d  %02d-%02d-%04d  %02d:%02d  %s\n",
						szMainName, szExtName, tFileInfo.n64FileSize,
						tFileInfo.ucWDateMonth, tFileInfo.ucWDateDay, (tFileInfo.ucWDateYear+80)%100 ,
						tFileInfo.ucWTimeHour, tFileInfo.ucWTimeMin, szLongName);
#endif						
	} while (!fsFindNext(&tFileInfo));
	
	fsFindClose(&tFileInfo);
	
	fsDiskFreeSpace('C', &uBlockSize, &uFreeSize, &uDiskSize);
	
	sysprintf("Disk Size: %d Kbytes, Free Space: %d KBytes\n", (INT)uDiskSize, (INT)uFreeSize);

	return 0;
}


static	UINT8  _ucLine[128];
static INT  Action_TYPE(CHAR *suFileName, CHAR *szAsciiName)
{
	INT	 	hFile;
	INT     nLen, nIdx, nStatus;

	hFile = fsOpenFile(suFileName, szAsciiName, O_RDONLY);
	if (hFile < 0)
		return hFile;

	nStatus = 0;
	while (1)
	{
		nStatus = fsReadFile(hFile, _ucLine, 128, &nLen);
		if (nStatus < 0)
			break;
			
		for (nIdx = 0; nIdx < nLen; nIdx++)
		{
			if (_ucLine[nIdx] < 127)
				sysprintf("%c", _ucLine[nIdx]);
		}
	}

	fsCloseFile(hFile);

	return nStatus;
}



static INT  Action_ReadSpeedTest(CHAR *suFileName, CHAR *szAsciiName)
{
	INT	 	 hFile;
	INT      nReadLen, nTime0, nStatus;
	UINT32   uKBCnt;

	uKBCnt = 0;
#ifdef DIRECT_MODE
	hFile = fsOpenFile(suFileName, szAsciiName, O_RDONLY | O_FSEEK | O_DIRECT_READ);
#else
	hFile = fsOpenFile(suFileName, szAsciiName, O_RDONLY);
#endif	
	if (hFile < 0)
		return hFile;
		
	nStatus = 0;
	nTime0 = get_timer_ticks();
	while (1)
	{
		nReadLen = 0;
		nStatus = fsReadFile(hFile, (UINT8 *)_pucDummy, DUMMY_BUFFER_SIZE, &nReadLen);
		if ((nStatus < 0) || (nReadLen == 0))
			break;
		
		uKBCnt += nReadLen / 1024;
		
		if (uKBCnt % 8192 == 0)
			sysprintf("%d MB\n", uKBCnt/1024);
	}
	nTime0 = get_timer_ticks() - nTime0;
	if (nTime0 == 0) nTime0 = 1;

	sysprintf("Read speed: %d KB/sec\n", (INT)(uKBCnt * 100) / nTime0);
	
	fsCloseFile(hFile);
	return nStatus;
}



static INT  Action_WriteSpeedTest(CHAR *suFileName, CHAR *szAsciiName)
{
	INT	 	 hFile;
	INT      i, j, nWriteLen, nTime0, nStatus;
	UINT32   uKBCnt = 0;

#ifdef DIRECT_MODE
	hFile = fsOpenFile(suFileName, szAsciiName, O_CREATE | O_TRUNC | O_DIRECT_WRITE);
#else
	hFile = fsOpenFile(suFileName, szAsciiName, O_CREATE | O_TRUNC);
#endif	
	if (hFile < 0)
		return hFile;
		
	for (i = 0, j = 0; i < DUMMY_BUFFER_SIZE; i += 2, j++)
	{
		_pucDummy[i] = (j >> 8) & 0xff;
		_pucDummy[i + 1] = j & 0xff;
	}
	memset((UINT8 *)_pucDummy + 0x7e0, 0x97, 32);
		
	nStatus = 0;
	nTime0 = get_timer_ticks();

	while (1)
	{
		nStatus = fsWriteFile(hFile, (UINT8 *)_pucDummy, DUMMY_BUFFER_SIZE, &nWriteLen);
		if (nStatus < 0)
			break;
		
		uKBCnt += nWriteLen / 1024;
		
		if (uKBCnt % 1024 == 0)
			sysprintf("%d MB\r", uKBCnt);
		
		if (uKBCnt >= 40*1024)
			break;
	}
	nTime0 = get_timer_ticks() - nTime0;
	if (nTime0 == 0) nTime0 = 1;

	sysprintf("Write speed: %d KB/sec\n", (INT)(uKBCnt * 100) / nTime0);
	
	fsCloseFile(hFile);

	return nStatus;
}


static INT  Action_FileAppendTest(CHAR *suFileName, CHAR *szAsciiName)
{
	INT	 	hFile;
	CHAR	szPattern[128];
	INT		i, nWriteLen, nStatus;
	INT  	nWriteCnt = 15 * 1024;

	hFile = fsOpenFile(suFileName, szAsciiName, O_APPEND);
	if (hFile < 0)
		return hFile;
		
	for (i =0 ; i < 26; i++)
		szPattern[i] = 'A' + i;
		
	while (nWriteCnt > 0)
	{
		nStatus = fsWriteFile(hFile, (UINT8 *)szPattern, 26, &nWriteLen);
		if (nStatus < 0)
		{
			fsGetErrorDescription(nStatus, NULL, 1);
			break;
		}
		nWriteCnt -= nWriteLen;
	}
	nStatus = fsCloseFile(hFile);
	return nStatus;
}



static INT  Action_Compare(CHAR *suFileName1, CHAR *szAsciiName1, 
							CHAR *suFileName2, CHAR *szAsciiName2)
{
	INT		hFile1, hFile2;
	INT		nLen1, nLen2, nCount, nStatus1, nStatus2;
	UINT8	buffer1[8192], buffer2[8192];
	
	hFile1 = fsOpenFile(suFileName1, szAsciiName1, O_RDONLY);
	if (hFile1 < 0)
		return hFile1;
	
	hFile2 = fsOpenFile(suFileName2, szAsciiName2, O_RDONLY);
	if (hFile2 < 0)
		return hFile2;
	
	sysprintf("\nComparing file ...\n");
	nCount = 0;
	while (1)
	{
		nStatus1 = fsReadFile(hFile1, buffer1, 1024, &nLen1);
		nStatus2 = fsReadFile(hFile2, buffer2, 1024, &nLen2);
		
		if ((nStatus1 == ERR_FILE_EOF) && (nStatus2 == ERR_FILE_EOF))
		{
			sysprintf("\ncompare ok!\n");
			fsCloseFile(hFile1);
			fsCloseFile(hFile2);
			return 0;
		}
		
		if (nLen1 != nLen2)
			break;
			
		if (memcmp(buffer1, buffer2, 1024))
			break;
		
		nCount++;
		//if ((nCount % 1024) == 0)
		//	sysprintf("%d KB    \r", nCount);
	}
	
	sysprintf("\nFile Compare failed at offset %x\n", nCount * 1024);
	
	fsCloseFile(hFile1);
	fsCloseFile(hFile2);
	return -1;
}



static INT  Action_PrintDiskInfo()
{
	PDISK_T		*pDiskList, *ptPDiskPtr;
	PARTITION_T	*ptPartition;
	INT			nDiskIdx = 0;
	INT			nPartIdx;
	
	ptPDiskPtr = pDiskList = fsGetFullDiskInfomation();
	
	while (ptPDiskPtr != NULL)
	{
		sysprintf("\n\n=== Disk %d (%s) ======================\n", nDiskIdx++, (ptPDiskPtr->nDiskType & DISK_TYPE_USB_DEVICE) ? "USB" : "IDE");
		sysprintf("    name:     [%s%s]\n", ptPDiskPtr->szManufacture, ptPDiskPtr->szProduct);
		sysprintf("    head:     [%d]\n", ptPDiskPtr->nHeadNum);
		sysprintf("    sector:   [%d]\n", ptPDiskPtr->nSectorNum);
		sysprintf("    cylinder: [%d]\n", ptPDiskPtr->nCylinderNum);
		sysprintf("    size:     [%d MB]\n", (INT)ptPDiskPtr->uDiskSize / 1024);
		
		ptPartition = ptPDiskPtr->ptPartList;
		nPartIdx = 1;
		while (ptPartition != NULL)
		{
			sysprintf("\n    --- Partition %d --------------------\n", nPartIdx++);
			sysprintf("        active: [%s]\n", (ptPartition->ucState & 0x80) ? "Yes" : "No");
			sysprintf("        size:   [%d MB]\n", (INT)(ptPartition->uTotalSecN / 1024) / 2);
			sysprintf("        start:  [%d]\n", (INT)ptPartition->uStartSecN);
			sysprintf("        type:   ");
			if (ptPartition->ptLDisk == NULL)
				sysprintf("[Unknown]\n");
			else 
			{
				switch (ptPartition->ptLDisk->ucFileSysType)
				{
					case FILE_SYSTEM_FAT12:
						sysprintf("[FAT12]\n");
						break;
					case FILE_SYSTEM_FAT16:
						sysprintf("[FAT16]\n");
						break;
					case FILE_SYSTEM_FAT32:
						sysprintf("[FAT32]\n");
						break;
					case FILE_SYSTEM_NTFS:
						sysprintf("[NTFS]\n");
						break;
					default:
						sysprintf("[???????]\n");
						break;
				}
				sysprintf("        drive:  [%c]\n", ptPartition->ptLDisk->nDriveNo);
			}
			ptPartition = ptPartition->ptNextPart;
		}
		ptPDiskPtr = ptPDiskPtr->ptPDiskAllLink;
	}
	fsReleaseDiskInformation(pDiskList);
	return 0;
}



INT  build_full_path(CHAR *szFullPath, CHAR *szPath)
{
	INT		nDriveNo;
	CHAR	*pcPtr;
	
	if (szPath[1] == ':')
	{
		nDriveNo = szPath[0];
		if ((nDriveNo >= 'a') && (nDriveNo <= 'l'))
			nDriveNo -= ('a' - 'A');
		pcPtr = szPath + 2;
	}
	else
	{
		nDriveNo = _nCurrentDrive;
		pcPtr = szPath;
	}
	
	if ((nDriveNo < 'C') || (nDriveNo > 'L'))
		return -1;
		
	if (pcPtr[0] == '\\')
		strcpy(szFullPath, szPath);
	else
	{
		strcpy(szFullPath, _pszWorkingDirectory[nDriveNo - 'C']);
		strcat(szFullPath, "\\");
		strcat(szFullPath, pcPtr);
	}
	
	return 0;
}


INT Test()
{
	INT     nStatus;
	//CHAR	szSrcA[24] = "C:\\tape001.mpg";
	CHAR	szSrcA[24] = "C:\\1.mp4";
	CHAR	szDstA[24] = "C:\\copya";
	CHAR	suFileName1[64], suFileName2[64];
	INT		t0, tmp;

	t0 = get_timer_ticks();

	while (1)
	{
		sysprintf("Delete file: %s\n", szDstA);
		fsAsciiToUnicode(szDstA, suFileName1, TRUE);
		nStatus = fsDeleteFile(suFileName1, NULL);
		if (nStatus < 0)
			sysprintf("Failed, status = %x\n", nStatus);

		sysprintf("Copy file: %s\n", szSrcA);
		fsAsciiToUnicode(szSrcA, suFileName1, TRUE);
		fsAsciiToUnicode(szDstA, suFileName2, TRUE);
		nStatus = fsCopyFile(suFileName1, NULL, suFileName2, NULL);
		if (nStatus < 0)
		{
			sysprintf("Failed, status = %x\n", nStatus);
			exit(0);
		}

		tmp = (get_timer_ticks() - t0) / 100;
		sysprintf("%02d:%02d:%02d\n",	tmp / 3600, (tmp % 3600) / 60, tmp % 60);

		sysprintf("Compare file: %s and %s\n", szSrcA, szDstA);
		fsAsciiToUnicode(szSrcA, suFileName1, TRUE);
		fsAsciiToUnicode(szDstA, suFileName2, TRUE);
		
		if (Action_Compare(suFileName1, NULL, suFileName2, NULL) < 0)
			break;

		tmp = (get_timer_ticks() - t0) / 100;
		sysprintf("%02d:%02d:%02d\n",	tmp / 3600, (tmp % 3600) / 60, tmp % 60);
	}	
	return 0;
}


extern INT  _fsIocCacheHit, _fsIocCacheMiss, _fsIocDirectWrite, _fsIocDirectRead;

void Create_File_Test(void)
{
	CHAR  	szAsciiStr[128], suFileName[512];
	INT		nIdx, hFile, nWriteLen;
	INT		t0, t_create, t_write, t_close;

	memset(szAsciiStr, 0, sizeof(szAsciiStr));
	sprintf(szAsciiStr, "C:\\Test");
	fsAsciiToUnicode(szAsciiStr, suFileName, TRUE);
	fsMakeDirectory(suFileName, NULL);
	
	for (nIdx = 1; nIdx < 1000; nIdx++)
	{
		sprintf(szAsciiStr, "C:\\Test\\%04d", nIdx);
		fsAsciiToUnicode(szAsciiStr, suFileName, TRUE);
		
		t0 = get_timer_ticks();
		hFile = fsOpenFile(suFileName, NULL, O_CREATE | O_IOC_VER2);
		t_create = get_timer_ticks() - t0;
		
		t0 = get_timer_ticks();
		fsWriteFile(hFile, (UINT8 *)_pucDummy, 1024, &nWriteLen);	
		t_write = get_timer_ticks() - t0;
		
		t0 = get_timer_ticks();
		fsCloseFile(hFile);
		t_close = get_timer_ticks() - t0;

		//sysprintf("%04d --- %d, %d, %d\n", nIdx, t_create, t_write, t_close);
	}
	fsFlushIOCache();
}


void file_1000_test()
{
	int   hFile;
	int   i, t0, t1, nWriteLen;
	char  fname[64], suFullName[128];
	UINT32 		uBlockSize, uFreeSize, uDiskSize;

	fsDiskFreeSpace('C', &uBlockSize, &uFreeSize, &uDiskSize);
	sysprintf("\nDisk Size: %d Kbytes, Free Space: %d KBytes\n", (INT)uDiskSize, (INT)uFreeSize);

	fsAsciiToUnicode("C:\\TEST", suFullName, TRUE);
	fsMakeDirectory(suFullName, NULL);

	t0 = t1 = sysGetTicks(TIMER0);

while (1)
{
	for (i = 0; i < 10; i++)
	{
		sprintf(fname, "C:\\TEST\\%d.txt", i);
		sysprintf("Write file %s\n", fname); 
		fsAsciiToUnicode(fname, suFullName, TRUE);
	
		hFile = fsOpenFile(suFullName, NULL, O_CREATE);
		
		//fsWriteFile(hFile, (UINT8 *)suFullName, 20, &nWriteLen);		
		fsWriteFile(hFile, (UINT8 *)0x600000, 128*1024, &nWriteLen);		
		
		fsCloseFile(hFile);
		
		if (i % 10 == 0)
		{
		//	sysprintf("%d\n", sysGetTicks(TIMER0) - t1);
			t1 = sysGetTicks(TIMER0);
		}
	}
}	
			sysprintf("%d\n", sysGetTicks(TIMER0) - t1);
	
	sysprintf("Ticks = %d\n", sysGetTicks(TIMER0) - t0);
}
void  CommandShell()
{
	INT     	nCmdCode;
	CHAR    	*szCmd, *szArgum1, *szArgum2, *szArgum3, *pcPtr;
	CHAR		szPath1[MAX_PATH_LEN], szPath2[MAX_PATH_LEN];
	CHAR 		suFullName1[MAX_PATH_LEN], suFullName2[MAX_PATH_LEN];
	INT			hFile, nStatus;
	volatile int i;
	
	//Create_File_Test();

	_nCurrentDrive = 'C';
	
	while (1)
	{
		dump_ehci_ports();
		usb_work_thread();

		//sysprintf("memory: %d\n", USB_available_memory());
		
		strcpy(szPath1, _pszWorkingDirectory[_nCurrentDrive - 'C']);
		if (strlen(_pszWorkingDirectory[_nCurrentDrive - 'C']) == 2)
			sysprintf("%s\\>", szPath1);
		else
			sysprintf("%s>", szPath1);
			
		
		accept_string(_szCommandLine, 256);
		
		if (strlen(_szCommandLine) == 0)
			continue;
	
		sysprintf("\n");
		
		/*- get command tokens */
		szCmd = &_szCommandLine[0];
		szArgum1 = get_token(szCmd, " \t\r\n", 8);
		szArgum2 = get_token(szArgum1, " \t\r\n", 128);
		szArgum3 = get_token(szArgum2, " \t\r\n", 128);
			
		//sysprintf("Command:<%s> <%s> <%s>\n", szCmd, szArgum1, szArgum2);
		
		fsAsciiToUpperCase(szCmd);
		
		nCmdCode = get_command_code(szCmd);
		
		if ((szArgum1 == NULL) && (szCmd[1] == ':') && (szCmd[2] == 0))
			nCmdCode = CHANGE;
		
		if (nCmdCode < 0)
		{
			sysprintf("Command not found!\n");
			continue;
		}
		
		build_full_path(szPath1, szArgum1);
		fsAsciiToUnicode(szPath1, suFullName1, 1);
		build_full_path(szPath2, szArgum2);
		fsAsciiToUnicode(szPath2, suFullName2, 1);
		
		nStatus = 0;

		switch (nCmdCode)
		{
			case QQ:
			case HELP:
				sysprintf("%s", _pszCommandShellHelp);
				break;
				
			case DIR:
				nStatus = Action_DIR(suFullName1);
				//nStatus = Action_DIR2(suFullName1);
				break;

			case DIRS:
				nStatus = Action_DIRS(suFullName1);
				break;
		  
			case TYPE:
				nStatus = Action_TYPE(suFullName1, szArgum1);
				break;
				
			case CD:
				if (!strcmp(szArgum1, "\\"))
				{
					_pszWorkingDirectory[_nCurrentDrive - 'C'][2] = '\0';
					break;
				}
			
				if (!strcmp(szArgum1, ".."))
				{
					if (strlen(_pszWorkingDirectory[_nCurrentDrive - 'C']) == 2)
						break;
						
					strcpy(szPath1, _pszWorkingDirectory[_nCurrentDrive - 'C']);
					
					nStatus = 0;
					pcPtr = szPath1 + strlen(szPath1);
					while (pcPtr > szPath1)
					{
						if (*pcPtr == '\\')
							break;
						pcPtr--;
					}
					*pcPtr = '\0';
					strcpy(_pszWorkingDirectory[_nCurrentDrive - 'C'], szPath1);
					break;
				}
				
				fsAsciiToUnicode(szPath1, suFullName1, 1);
				hFile = fsOpenFile(suFullName1, szArgum1, O_DIR);
				if (hFile < 0)
					nStatus = hFile;
				else
				{
					strcpy(_pszWorkingDirectory[_nCurrentDrive - 'C'], szPath1);
					fsCloseFile(hFile);
					nStatus = 0;
				}
				break;
				
			case DEL:
				nStatus = fsDeleteFile(suFullName1, szArgum1);
				break;
				
			case DELA:
				nStatus = fsDeleteDirTree(suFullName1, NULL);
				break;
				
			case COPY:
				nStatus = fsCopyFile(suFullName1, szArgum1, suFullName2, szArgum2);
				break;
				
			case MKDIR:
			case MD:
				nStatus = fsMakeDirectory(suFullName1, szArgum1);
				break;
				
			case RMDIR:
			case RD:
				nStatus = fsRemoveDirectory(suFullName1, szArgum1);
				break;
				
			case RENAME:
				if ((nStatus = fsRenameFile(suFullName1, szArgum1, suFullName2, szArgum2, 0)) == ERR_FILE_IS_DIRECTORY)
					nStatus = fsRenameFile(suFullName1, szArgum1, suFullName2, szArgum2, 1);
				break;
				
			case CMP:
				nStatus = Action_Compare(suFullName1, szArgum1, suFullName2, szArgum2);
				break;
				
			case CHANGE:
				if ((szCmd[0] >= 'C') && (szCmd[0] <= 'L'))
					_nCurrentDrive = szCmd[0];
				continue;
				
			//case DUMP:
			//	{
			//		UINT32	uSectorNo = 0;
			//		
			//		while (*szArgum1)
			//			uSectorNo = uSectorNo * 10 + (*szArgum1++ - '0');
			//		fsDumpDiskSector(uSectorNo, 1);
			//	}
			//	continue;
				
			case RDTST:
				//_fs_uReadSecCnt = _fs_uWriteSecCnt = 0;
				nStatus = Action_ReadSpeedTest(suFullName1, szArgum1);
				//sysPrintf("\n\n\nTotal: Read: %d, Write: %d\n", _fs_uReadSecCnt, _fs_uWriteSecCnt);
				break;

			case WRTST:
				//_fs_uReadSecCnt = _fs_uWriteSecCnt = 0;
				//nStatus = Action_WriteSpeedTest(suFullName1, szArgum1);
				nStatus = Action_WriteSpeedTest(suFullName1, NULL);
				sysprintf("wrtst [%x]\n", nStatus);
				//sysPrintf("\n\n\nTotal: Read: %d, Write: %d\n", _fs_uReadSecCnt, _fs_uWriteSecCnt);
				break;
				
			case APTST:
				nStatus = Action_FileAppendTest(suFullName1, szArgum1);
				break;
				
			case RAWRD:
				{
					LDISK_T	 *ptLDisk;
					PDISK_T  *ptPDisk;
					INT  nTime0;
					INT  nCount, nStatus;
					
					nStatus = get_vdisk('C', &ptLDisk);
					if (nStatus < 0)
						break;
					ptPDisk = ptLDisk->ptPDisk;
					
					nTime0 = get_timer_ticks();
					/* 40MB read test */
					for (nCount = 0; nCount < 40960; nCount += 64)
					{
						nStatus = ptPDisk->ptDriver->read(ptPDisk, 1024, 128, (UINT8 *)_pucDummy);
						if (nStatus < 0)
							break;
					}
					nTime0 = get_timer_ticks() - nTime0;
					if (nTime0 == 0) nTime0 = 1;
					sysprintf("Raw Read speed: %d KB/sec\n", (INT)(nCount * 100) / nTime0);
				}	
				break;

			case RAWWD:
				{
					LDISK_T	 *ptLDisk;
					PDISK_T  *ptPDisk;
					INT  nTime0;
					INT  nCount, nStatus;
					
					nStatus = get_vdisk('C', &ptLDisk);
					if (nStatus < 0)
						break;
					ptPDisk = ptLDisk->ptPDisk;
					
					nTime0 = get_timer_ticks();
					/* 40MB write test */
					for (nCount = 0; nCount < 40960; nCount += 64)
					{
						nStatus = ptPDisk->ptDriver->write(ptPDisk, 1024, 128, (UINT8 *)_pucDummy, BLOCKING_WRITE);
						if (nStatus < 0)
							break;
					}
					nTime0 = get_timer_ticks() - nTime0;
					if (nTime0 == 0) nTime0 = 1;
					sysprintf("Raw write speed: %d KB/sec\n", (INT)(nCount * 100) / nTime0);
				}	
				break;
				
			case DISK:
				Action_PrintDiskInfo();
				break;
				
			case DF:
				FAT_dump_sector_cache();
		  		break;

			case FORMAT:
				{
					PDISK_T		*pDiskList;
				
					pDiskList = fsGetFullDiskInfomation();
					fsFormatFlashMemoryCard(pDiskList);
					fsReleaseDiskInformation(pDiskList);
				}
				break;

			case TEST:
				nStatus = Test();
				break;
		  		
			case USB:

#define UCMDR			0xB0005020	/* USB Command Register */
#define UPSCR0			0xB0005064	/* USB Port0 Status and Control Register */
#define UPSCR1			0xB0005068	/* USB Port1 Status and Control Register */
#define CMD_RUN			(1<<0)		/* start/stop HC */
#define PORT_SUSPEND	(1<<7)		/* suspend port */
#define PORT_RESUME		(1<<6)		/* resume it */

				sysprintf("Before suspend: %x\n", inpw(UPSCR1));
				
				outpw(UPSCR1, inpw(UPSCR1) | PORT_SUSPEND);
				for (i = 0; i < 0x10000; i++);
				sysprintf("After set suspend: %x\n", inpw(UPSCR1));
				
				outpw(UCMDR, (inpw(UCMDR) & ~CMD_RUN));
				sysprintf("After clear RUN: %x\n", inpw(UPSCR1));
				sysprintf("After suspend completed: %x\n", inpw(UPSCR1));

#if 1	// remote wakeup
				while (1)
				{
					if (inpw(UPSCR1) & 0x40)
						break;
				}
				sysprintf("Wake up!!\n");
				outpw(UPSCR1, (inpw(UPSCR1) & ~0x40));

				outpw(UCMDR, (inpw(UCMDR) | CMD_RUN));
				for (i = 0; i < 0x10000; i++);
				sysprintf("After enable RUN: %x\n", inpw(UPSCR1));
				
#else	// resume			
				sysGetChar();

				outpw(UCMDR, (inpw(UCMDR) | CMD_RUN));
				for (i = 0; i < 0x10000; i++);
				sysprintf("After enable RUN: %x\n", inpw(UPSCR1));
				
				outpw(UPSCR1, inpw(UPSCR1) | PORT_RESUME);
				for (i = 0; i < 0x100000; i++);
				sysprintf("After set resume: %x\n", inpw(UPSCR1));
				
				outpw(UPSCR1, inpw(UPSCR1) &~ (PORT_RESUME | PORT_SUSPEND));
				for (i = 0; i < 0x10000; i++);
				sysprintf("After resume completed: %x\n", inpw(UPSCR1));
#endif				
				break;

			default:
			   sysprintf("Unknown command!\n");
			   break;
	   	} /* end of switch */
	   
	   	if (nStatus != ERR_FILE_EOF)
	  		fsGetErrorDescription(nStatus, NULL, 1);
		//fsFlushIOCache();
		//sysprintf("Hit=%d, Miss=%d, DW=%d, DR=%d\n", _fsIocCacheHit, _fsIocCacheMiss, _fsIocDirectWrite, _fsIocDirectRead);
	} 
}


void usb_raw_disk_test()
{
	LDISK_T	 	*ptLDiskC, *ptLDiskD;
	PDISK_T  	*ptPDisk;
	INT  		nCount, nStatus;
	UINT32		uSecNo;
	UINT32 		uBlockSize, uFreeSize, uDiskSize;
	
#ifdef TWO_DEVICE_TEST
	while ((get_vdisk('C', &ptLDiskC) < 0) || (get_vdisk('D', &ptLDiskD) < 0)) ;
#else
	while (get_vdisk('C', &ptLDiskC) < 0)
				usb_work_thread();
#endif
	
	fsDiskFreeSpace('C', &uBlockSize, &uFreeSize, &uDiskSize);
	printf("Disk size = %d KB\n", uDiskSize);

#ifdef TWO_DEVICE_TEST
	if (uDiskSize > 4 * 1024 * 1024)
		ptPDisk = ptLDiskC->ptPDisk;
	else
		ptPDisk = ptLDiskD->ptPDisk;
#else
	ptPDisk = ptLDiskC->ptPDisk;
#endif

	for (uSecNo = 1000000; ;uSecNo+=64)
	{
		printf("1");
		nStatus = ptPDisk->ptDriver->read(ptPDisk, uSecNo, 64, (UINT8 *)_pucDummy);
		if (nStatus < 0)
			break;
		printf("2");
		nStatus = ptPDisk->ptDriver->write(ptPDisk, uSecNo, 64, (UINT8 *)_pucDummy, 1);
		if (nStatus < 0)
			break;
			
		if (uSecNo > 10000000)
			uSecNo = 1000;
	}
	printf("usb_raw_disk_test - failed status = 0x%x\n", nStatus);
}


int CommandShell_MODE()
{


	CommandShell();

	while(1);
	return 0;
}


#if 0		// ARM ADS
/*
 * standalone.c - minimal bootstrap for C library
 * Copyright (C) 2000 ARM Limited.
 * All rights reserved.
 */

/*
 * RCS $Revision: 1 $
 * Checkin $Date: 08/08/22 9:09p $ 0
 * Revising $Author: Mncheng $
 */

/*
 * This code defines a run-time environment for the C library.
 * Without this, the C startup code will attempt to use semi-hosting
 * calls to get environment information.
 */
 extern unsigned int Image$$ZI$$Limit;

void _sys_exit(int return_code)
{
label:  goto label; /* endless loop */
}

void _ttywrch(int ch)
{
    char tempch = (char)ch;
    (void)tempch;
}


__value_in_regs struct R0_R3 {unsigned heap_base, stack_base, heap_limit, stack_limit;} 
    __user_initial_stackheap(unsigned int R0, unsigned int SP, unsigned int R2, unsigned int SL)
{
    struct R0_R3 config;

    //config.heap_base = 0x00060000;
    config.heap_base = (unsigned int)&Image$$ZI$$Limit;
    config.stack_base = 0x1800000; //Stack base;

/*
To place heap_base directly above the ZI area, use:
    extern unsigned int Image$$ZI$$Limit;
    config.heap_base = (unsigned int)&Image$$ZI$$Limit;
(or &Image$$region_name$$ZI$$Limit for scatterloaded images)

To specify the limits for the heap & stack, use e.g:
    config.heap_limit = SL;
    config.stack_limit = SL;
*/

    return config;
}

/* end of file standalone.c */

/* end of file standalone.c */
#endif