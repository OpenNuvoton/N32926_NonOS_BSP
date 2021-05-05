/**************************************************************************//**
 * @file     Writer.c
 * @brief    SpiWriter source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "W55FA92_reg.h"
#include "wblib.h"
#include "W55FA92_SIC.h"
#include "NVTFAT.h"
#ifndef __NoLCM__
#include "Font.h"
#endif
#include "Writer.h"
#ifdef __Security__
#include "Gneiss.h"
#endif

#ifdef __Security__
void RPMC_CreateRootKey(unsigned char *u8uid, unsigned int id_len, unsigned char *rootkey);
#endif

VOID Reset(void);
#ifndef __NoLCM__

extern S_DEMO_FONT s_sDemo_Font;
#endif
extern INI_INFO_T Ini_Writer;
extern BOOL volatile g_4byte_adderss;
void Exit4ByteMode(void);
int font_x=0, font_y=16;
UINT32 u32SkipX;

UINT32 volatile SPI_BLOCK_SIZE =64*1024;
UINT32 volatile IMAGE_LIST_SIZE  = 1024;

#define LOADER_ADDR        0x0900000
#define LOGO_ADDR        0x0500000
#define EXECUTE_ADDR    0x0800000
#define DATA_ADDR        0x0000000
#if 1
#define dbgprintf sysprintf
#else
#define dbgprintf(...)
#endif

extern PDISK_T *pDisk_SD0;

/**********************************/
#if defined (__GNUC__)
UINT8 infoBufArray[0x40000] __attribute__((aligned(32)));
UINT8 StorageBufferArray[0x40000] __attribute__((aligned(32)));
UINT8 CompareBufferArray[0x40000] __attribute__((aligned(32)));
#else
__align(32) UINT8 infoBufArray[0x40000];
__align(32) UINT8 StorageBufferArray[0x40000];
__align(32) UINT8 CompareBufferArray[0x40000];
#endif
UINT32 infoBuf, StorageBuffer, CompareBuffer, BufferSize=0;
UINT8 *pInfo;
CHAR suNvtFullName[2048], suNvtTargetFullName[2048];
static INT hNvtFile = -1;
INT32 gCurBlock=0;
FW_UPDATE_INFO_T FWInfo[21];
BOOL volatile bIsAbort = FALSE;
IBR_BOOT_STRUCT_T BootCodeMark;
INT32 gSpiLoaderSize;
UINT8 *pu8SoundPlayBuf;
UINT32 u32pu8SoundPlayBuf;
FILE_STAT_T FileStatus;
/*-----------------------------------------------------------------------------
 * Write StorageBuffer to sector gCurBlock in Spi flash.
 *---------------------------------------------------------------------------*/
void nvtWriteSpi(UINT32 len)
{
    int status = 0;
    int volatile block_count;
    if(len == 0)
        return;
    block_count = len / SPI_BLOCK_SIZE;
    if ((len % SPI_BLOCK_SIZE) != 0)
        block_count++;
        
    if(!Ini_Writer.ChipErase)    
        usiEraseSector(gCurBlock*SPI_BLOCK_SIZE, block_count);
    status = usiWrite(gCurBlock*SPI_BLOCK_SIZE, block_count * SPI_BLOCK_SIZE, (UINT8 *)StorageBuffer);
    if (status < 0)
    {
        sysprintf("ERROR in nvtWriteSpi(): Write Spi error ! Return = 0x%x.\n", status);
    }

#ifdef __DEBUG__
    // read data back to comfirm it
    status = usiRead(gCurBlock*SPI_BLOCK_SIZE, block_count  * SPI_BLOCK_SIZE, (UINT8 *)CompareBuffer);
    if (memcmp((UINT8 *)StorageBuffer, (UINT8 *)CompareBuffer, len)==0)
        sysprintf("Spi write OK from block %d, block %d.\n", gCurBlock, block_count);
    else
        sysprintf("Spi write FAIL from block %d, block %d.\n", gCurBlock, block_count);
#endif

    gCurBlock += block_count;
}

void initClock(void)
{
    WB_UART_T uart;
    UINT32 u32ExtFreq;
    UINT32 reg_tmp;

    u32ExtFreq = sysGetExternalClock();     // Hz unit
    if(u32ExtFreq==12000000)
    {
        outp32(REG_SDREF, 0x805A);
    }
    else
    {
        outp32(REG_SDREF, 0x80C0);
    }

    /**********************************************************************************************
     * Clock Constraints:
     * (a) If Memory Clock > System Clock, the source clock of Memory and System can come from
     *     different clock source. Suggestion MPLL for Memory Clock, UPLL for System Clock
     * (b) For Memory Clock = System Clock, the source clock of Memory and System must come from
     *     same clock source
     *********************************************************************************************/
#if 0
    /**********************************************************************************************
     * Slower down system and memory clock procedures:
     * If current working clock fast than desired working clock, Please follow the procedure below
     * 1. Change System Clock first
     * 2. Then change Memory Clock
     *
     * Following example shows the Memory Clock = System Clock case. User can specify different
     * Memory Clock and System Clock depends on DRAM bandwidth or power consumption requirement.
     *********************************************************************************************/
    sysSetSystemClock(eSYS_EXT, 12000000, 12000000);
    sysSetDramClock(eSYS_EXT, 12000000, 12000000);
#else
    /**********************************************************************************************
     * Speed up system and memory clock procedures:
     * If current working clock slower than desired working clock, Please follow the procedure below
     * 1. Change Memory Clock first
     * 2. Then change System Clock
     *
     * Following example shows to speed up clock case. User can specify different
     * Memory Clock and System Clock depends on DRAM bandwidth or power consumption requirement.
     *********************************************************************************************/
    sysSetDramClock(eSYS_MPLL, 360000000, 360000000);
    sysSetSystemClock(eSYS_UPLL,            //E_SYS_SRC_CLK eSrcClk,
                      240000000,            //UINT32 u32PllKHz,
                      240000000);           //UINT32 u32SysKHz,
    sysSetCPUClock(240000000/2);
#endif

    u32ExtFreq = sysGetExternalClock();        /* Hz unit */    
    uart.uiFreq = u32ExtFreq;
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uart_no=WB_UART_1;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    sysInitializeUART(&uart);
}


/**********************************/
int main()
{
    DateTime_T ltime;

    int status=0, nReadLen;
    CHAR szNvtFullName[64];

    int  BootCodeMarkFlag, i,j;

    char Array1[64];
    UINT Next_Font_Height;
    INT  FileInfoIdx=0;
    int optional_ini_size = 0;
    extern IBR_BOOT_OPTIONAL_STRUCT_T optional_ini_file;
  
#ifdef __Security__
    UINT8     u8UID[8];
    unsigned char ROOTKey[32];    // Rootkey array    
    unsigned char RPMCStatus;
#endif  
    
    initClock();

    /* enable cache */
    sysDisableCache();
    sysInvalidCache();
    sysEnableCache(CACHE_WRITE_BACK);

    /* check SDRAM size and buffer address */

    infoBuf = (UINT32) &infoBufArray[0] | 0x80000000;
    StorageBuffer = (UINT32)&StorageBufferArray[0] | 0x80000000;
    CompareBuffer = (UINT32)&CompareBufferArray[0] | 0x80000000;

    pInfo = (UINT8 *)((UINT32)infoBuf | 0x80000000);

    /* configure Timer0 for FMI library */
    sysSetTimerReferenceClock(TIMER0, 12000000);    // External Crystal
    sysStartTimer(TIMER0, 100, PERIODIC_MODE);      // 100 ticks/per sec ==> 1tick/10ms

    sysprintf("=====> W55FA92 SpiWriter (v%d.%d) Begin [%d] <=====\n", MAJOR_VERSION_NUM, MINOR_VERSION_NUM, sysGetTicks(0));

    ltime.year = 2013;
    ltime.mon  = 8;
    ltime.day  = 29;
    ltime.hour = 8;
    ltime.min  = 40;
    ltime.sec  = 0;
    sysSetLocalTime(ltime);

    fsInitFileSystem();

    // date by CJChen1@nuvoton.com, the NVTFAT limitation, don't need to assign drive number to SD.
    //      The NVTFAT will auto assign 'C' to SD port 0 and 'D' to SD port 1
    /* initialize FMI (Flash memory interface controller) */
    sysSetTimerReferenceClock(TIMER0, 12000000);                //External Crystal

    //--- initial panel
#ifndef __NoLCM__    
    Draw_Init();
    font_y = g_Font_Height;
    Next_Font_Height = g_Font_Height-6;
#endif
    sicIoctl(SIC_SET_CLOCK, 192000, 0, 0);
    sicOpen();
#ifndef __NoLCM__   
    //--- mount SD card on port 0
    Draw_Font(COLOR_RGB16_WHITE,  &s_sDemo_Font, font_x, font_y, "Mount SD Card 0:");
    u32SkipX = 16;
#endif   
    status = sicSdOpen0();
    if (status < 0)
    {
#ifndef __NoLCM__       
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
#endif        
        sysprintf("===> 1.0 (No SD Card)\n");
        bIsAbort = TRUE;
        goto _end_;
    }
    // Get the SpiWriter setting from INI file (SpiWriter.ini)
    status = ProcessINI("C:\\SpiWriter.ini");
    if (status < 0)
    {
#ifndef __NoLCM__
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
#endif        
        sysprintf("===> 1.1 (Wrong INI file)\n");
        bIsAbort = TRUE;
        goto _end_;
    }
    else if(status == 1)
    {
        sprintf(Array1, "Too many image (Maximum is 21)."); 
        sysprintf("===> 1.1 (Too many images)\n");

#ifndef __NoLCM__   
        Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    
#endif     
        bIsAbort = TRUE;
        goto _end_;
    }
#ifndef __NoLCM__
    Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
    font_y += Next_Font_Height;
#endif  
    // Get the Boot Code Optional Setting from INI file (TurboWriter.ini) to optional_ini_file
    status = ProcessOptionalINI("C:\\TurboWriter.ini");
    
    if (status >= 0)
    {
        if (optional_ini_file.Counter == 0)
            optional_ini_size = 0;
        else
        {
            optional_ini_size = 8 * (optional_ini_file.Counter + 1);
            if (optional_ini_file.Counter % 2 == 0)
                optional_ini_size += 8;     // for dummy pair to make sure 16 bytes alignment.
        } 
        sysprintf("   Optional size is %d\n",optional_ini_size);   
    }

#ifndef __NoLCM__                         
       Draw_Font(COLOR_RGB16_WHITE,  &s_sDemo_Font, font_x, font_y, "Mount SPI Flash:");
#endif                 
    //--- mount SD card on port 1 or 2
    if (usiInit() < 0)
    {
        sysprintf("===> 1.2 (May not have Spi flash)\n");
#ifndef __NoLCM__
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
#endif        
        bIsAbort = TRUE;
        goto _end_;
    }
    else
    {
        sysprintf("===> 1.2 (Get Spi flash)\n");
        
        if(g_SPI_SIZE == 0)
            sprintf(Array1, "Spiflash size can't calculated"); 
        else
        {
            if( SPI_ID[2]  == SPIFLASH_WINBOND)    //Winbond
                sprintf(Array1, "Winbond Spiflash %dMB",g_SPI_SIZE>>20);
            else if (SPI_ID[2] == SPIFLASH_SST)
                sprintf(Array1, "SST Spiflash %dMB",g_SPI_SIZE>>20);
            else if(SPI_ID[2]  == SPIFLASH_GIGADEVICE)
                sprintf(Array1, "GigaDevice Spiflash %dMB",g_SPI_SIZE>>20); 
            else if (SPI_ID[2] == SPIFLASH_MXIC)
                sprintf(Array1, "MXIC Spiflash %dMB",g_SPI_SIZE>>20);
            else
                sprintf(Array1, "Spiflash %dMB",g_SPI_SIZE>>20);
        }
#ifndef __NoLCM__
        Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);
        Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);
#endif
    }
#ifndef __NoLCM__
    Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
    font_y += Next_Font_Height;
#endif
    if(Ini_Writer.SoundPlay)
    {
        if (strlen(Ini_Writer.SoundPlayStart.FileName) !=0)
        {
            sprintf(szNvtFullName, "C:\\%s",Ini_Writer.SoundPlayStart.FileName);
            fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);
            hNvtFile = fsOpenFile(suNvtFullName, NULL, O_RDONLY);

           
            if (hNvtFile < 0)
            {      
                sysprintf("===> 1.2.1 open file name [%s], return = 0x%x\n", suNvtFullName, hNvtFile);
#ifndef __NoLCM__
                 sprintf(Array1, "Open Sound File <Start> Fail");
                Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);
                Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);
#endif            
                goto _SoundOpenEnd01_;
            }
            else
            {
#ifndef __NoLCM__
                 sprintf(Array1, "Open %s", szNvtFullName);
                Draw_CurrentOperation(Array1,hNvtFile);
#endif
                fsGetFileStatus(hNvtFile, suNvtFullName, szNvtFullName, &FileStatus);
                
                BufferSize = 0x40000;
                pu8SoundPlayBuf = (PUINT8)malloc(sizeof(CHAR) * FileStatus.n64FileSize);
                u32pu8SoundPlayBuf = (UINT32) pu8SoundPlayBuf | 0x80000000;
                if(pu8SoundPlayBuf == NULL)
                {
                    sysprintf("\tCan't allocate the buffer for user sound file (size 0x%X)\n",FileStatus.n64FileSize);
#ifndef __NoLCM__
                     sprintf(Array1, "Memory allocate for Sound Fail");
                    Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);
                    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);
#endif
                    goto _SoundOpenEnd00_;
                }
                while(1)
                {
                    status = fsReadFile(hNvtFile, (UINT8 *)u32pu8SoundPlayBuf, BufferSize, &nReadLen);
                    
                    u32pu8SoundPlayBuf += nReadLen;
                    if (status == ERR_FILE_EOF)
                        break;
                    else if (status < 0)
                    {
                        goto _SoundOpenEnd00_;
                    }
                }
_SoundOpenEnd00_:
                fsCloseFile(hNvtFile);
_SoundOpenEnd01_:
                hNvtFile = -1;
                if(pu8SoundPlayBuf != NULL)
                {
                    sound_init(Ini_Writer.SoundPlayVolume);
                    sound_play(START_BURN,(UINT32)pu8SoundPlayBuf | 0x80000000,FileStatus.n64FileSize);
                    free(pu8SoundPlayBuf);
                }
            }
        }
        else
        {
            sound_init(Ini_Writer.SoundPlayVolume);
            sound_play(START_BURN,0,0);
        }
    }
    
    if(u32GpioPort_Start)
    {
        if(u32GpioLevel_Start)
            outp32(u32GpioPort_Start,inp32(u32GpioPort_Start) | u32GpioPin_Start);
        else
            outp32(u32GpioPort_Start,inp32(u32GpioPort_Start) & ~u32GpioPin_Start);    
    }
    
#ifdef __Security__
    if(Ini_Writer.RootKey >= 1 && Ini_Writer.RootKey <= 4)
    {
        if ((RPMC_ReadUID(u8UID)) == -1)
        {
            sysprintf("read id error !!\n");
#ifndef __NoLCM__
               Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
            font_y += Next_Font_Height;
#endif
            return -1;
        }
        
        sprintf(szNvtFullName, "Write Root Key(%d):",Ini_Writer.RootKey);
#ifndef __NoLCM__
        Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, szNvtFullName);
        u32SkipX = 18;     
#endif
        sysprintf("SPI flash uid [0x%02X%02X%02X%02X%02X%02X%02X%02X]\n",u8UID[0], u8UID[1],u8UID[2], u8UID[3],u8UID[4], u8UID[5],u8UID[6], u8UID[7]);
      
          for(i=0;i<8;i++)
          {              
              if(u8UID[i] != 0xFF)
                  break;
          }
          
          if(i == 8)
          {
            sysprintf("read uid error !!\n");
            
#ifndef __NoLCM__
               Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
            font_y += Next_Font_Height;
#endif
            return -1;
          }
        /* first stage, initial rootkey */
        RPMC_CreateRootKey((unsigned char *)u8UID,8, ROOTKey);    // caculate ROOTKey with UID & ROOTKeyTag by SHA256

        RPMCStatus = RPMC_WrRootKey(Ini_Writer.RootKey, ROOTKey);        // initial Rootkey, use first rootkey/counter pair

        if(RPMCStatus == 0x80)
        {
            // Write rootkey success
            sysprintf("RPMC_WrRootKey Success - 0x%02X!!\n",RPMCStatus );
#ifndef __NoLCM__
               Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
            font_y += Next_Font_Height;
#endif            
        }
        else
        {
            // write rootkey fail, check datasheet for the error bit
            sysprintf("RPMC_WrRootKey Fail - 0x%02X!!\n",RPMCStatus );
#ifndef __NoLCM__
               Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
            font_y += Next_Font_Height;
#endif
            return -1;
        }
    }
    else if(Ini_Writer.RootKey != 0)
    {
        sysprintf("Wrong Root Key Index - %d!!\n",Ini_Writer.RootKey);
        sprintf(szNvtFullName, "Write Root Key(%d):",Ini_Writer.RootKey);
#ifndef __NoLCM__
        Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, szNvtFullName);
        u32SkipX = 18;
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
        font_y += Next_Font_Height;
#endif
        return -1;
    }
#endif

    if(Ini_Writer.ChipErase)
    {
        sysprintf("=====> 1.3 Spi Chip Erase [%d] <=====\n", sysGetTicks(0));
#ifndef __NoLCM__
        Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, "Spi Chip Erase:");
        u32SkipX = 15; 
#endif
        sysDelay(10);
        usiEraseAll();
        sysDelay(10);
        while(usiWaitEraseReady())
        {
#ifndef __NoLCM__
            Draw_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
#endif
            sysDelay(30);
        }
#ifndef __NoLCM__
        Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
           
           Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
        font_y += Next_Font_Height;
#endif        
    }
/***************************************RAW DATA MODE**************************************************/    
    if (strlen(Ini_Writer.RawData.FileName) != 0)
    {
            gCurBlock = 0;
            BufferSize = 0x40000;
           sysprintf("=====> 2.0 Copy Raw data [%d] <=====\n", sysGetTicks(0));
        #ifndef __NoLCM__
            Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, "Writing Raw Data:");
            u32SkipX = 17;
        #endif
            sprintf(szNvtFullName, "C:\\%s", Ini_Writer.RawData.FileName);
            fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);
            hNvtFile = fsOpenFile(suNvtFullName, NULL, O_RDONLY);

            sprintf(Array1, "Open %s", szNvtFullName);
        #ifndef __NoLCM__
            Draw_CurrentOperation(Array1,hNvtFile);
        #endif    
            if (hNvtFile < 0)
            {
        #ifndef __NoLCM__
                Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
                sprintf(Array1, "Open Raw Data File Fail");
                Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);   
                Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    

        #endif
                sysprintf("===> 2.1 File Open Fail\n");
                bIsAbort = TRUE;
                goto _end_;
            }

            while(1)
            {
                status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, BufferSize, &nReadLen);
        #ifndef __NoLCM__
                Draw_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
        #endif
                nvtWriteSpi(nReadLen);   // write StorageBuffer to SD card
                if (status == ERR_FILE_EOF)
                    break;
                else if (status < 0)
                {
        #ifndef __NoLCM__
                    Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);  
                    Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
        #endif
                    sysprintf("===> 2.1 File Read Fail\n");
                    bIsAbort = TRUE;
                    goto _end_;
                }
            }
            if(Ini_Writer.DataVerify)
            {
                
                sysprintf("=====> 2.1 Verify Raw Data [%d] <=====\n", sysGetTicks(0));
                /* verify raw data */
                fsFileSeek(hNvtFile, 0, SEEK_SET);
                i = 0;
                while(1)
                {
                    status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, SPI_BLOCK_SIZE, &nReadLen);
        #ifndef __NoLCM__
                    Draw_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
        #endif
                    usiRead(i*SPI_BLOCK_SIZE, SPI_BLOCK_SIZE, (UINT8 *)CompareBuffer);
                    i++;

                    if (memcmp((UINT8 *)StorageBuffer, (UINT8 *)CompareBuffer, nReadLen))
                    {
                        sysprintf("===> 2.1.1 Verify Fail\n");                        
        #ifndef __NoLCM__
                        Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);  
                        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
                        sprintf(Array1, "Raw Data:Verify Fail");     
                        Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);
                        Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    
        #endif
                        bIsAbort = TRUE;
                        goto _end_;
                    }
                    if (status == ERR_FILE_EOF)
                        break;
                    else if (status < 0)
                    {
        #ifndef __NoLCM__
                        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
        #endif            
                        sysprintf("===> 2.1.2 File Read Fail\n");
                        bIsAbort = TRUE;
                        goto _end_;
                    }
                }
            }

            fsCloseFile(hNvtFile);
            hNvtFile = -1;        

/***************************************RAW DATA MODE**************************************************/
        goto _end_;
    }

    /*******************/
    /* copy SpiLoader   */
    /*******************/
    if (strlen(Ini_Writer.SpiLoader.FileName) ==0)
        goto WriteLogo;
       
    sysprintf("=====> 2.0 Copy SpiLoader [%d] <=====\n", sysGetTicks(0));
#ifndef __NoLCM__   
    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, "Writing SpiLoader:");
    u32SkipX = 18;
#endif
    sprintf(szNvtFullName, "C:\\%s",Ini_Writer.SpiLoader.FileName);
    fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);
    hNvtFile = fsOpenFile(suNvtFullName, NULL, O_RDONLY);

    sprintf(Array1, "Open %s", szNvtFullName);
#ifndef __NoLCM__
    Draw_CurrentOperation(Array1,hNvtFile);
#endif

    if (hNvtFile < 0)
    {
#ifndef __NoLCM__      
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail); 
        sprintf(Array1, "Open Loader File Fail");
        Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);
        Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);
        
#endif        
        sysprintf("===> 2.1, open file name [%s], return = 0x%x\n", suNvtFullName, hNvtFile);
        bIsAbort = TRUE;
        goto _end_;
    }

    /* image information */

    FWInfo[FileInfoIdx].imageNo = FileInfoIdx;
    FWInfo[FileInfoIdx].imageFlag = 3;      // image type: system image
    FWInfo[FileInfoIdx].startBlock = 0;
    FWInfo[FileInfoIdx].endBlock = 0;
    FWInfo[FileInfoIdx].executeAddr = Ini_Writer.SpiLoader.address;
    FWInfo[FileInfoIdx].fileLen = (UINT32)fsGetFileSize(hNvtFile);
    gSpiLoaderSize = FWInfo[FileInfoIdx].fileLen;
    memcpy(&FWInfo[FileInfoIdx].imageName[0], Ini_Writer.SpiLoader.FileName, 32);

    // initial Boot Code Mark for SpiLoader
    BootCodeMark.BootCodeMarker = 0x57425AA5;
    BootCodeMark.ExeAddr = Ini_Writer.SpiLoader.address;
    BootCodeMark.ImageSize = FWInfo[FileInfoIdx].fileLen;
    BootCodeMark.Reserved = 0x0000000;

    gCurBlock = 0;         // write SpiLoader.bin to Block 0
    BootCodeMarkFlag = 1;
    BufferSize = 0x40000;
    fsFileSeek(hNvtFile, 0, SEEK_SET);    
    
    if(optional_ini_size ==0)    
    {
        while(1)
        {
            // write 1st sector with Boot Code Mark for SpiFlash        
            memcpy((UINT8 *)StorageBuffer, (UINT8 *)&BootCodeMark, sizeof(IBR_BOOT_STRUCT_T));          
            status = fsReadFile(hNvtFile, (UINT8 *)(StorageBuffer+sizeof(IBR_BOOT_STRUCT_T)),
                                BufferSize  - sizeof(IBR_BOOT_STRUCT_T), &nReadLen); 
            if(nReadLen < BufferSize  - sizeof(IBR_BOOT_STRUCT_T))                   
                   memset((UINT8 *)(StorageBuffer + nReadLen+sizeof(IBR_BOOT_STRUCT_T)) , 0xff,  (BufferSize  - sizeof(IBR_BOOT_STRUCT_T) - nReadLen));
                                              
            nvtWriteSpi(nReadLen);  // write StorageBuffer to SD card
            BootCodeMarkFlag = 0;   // boot code mark had written

            if (status == ERR_FILE_EOF)
                break;
            else if (status < 0)
            {
    #ifndef __NoLCM__           
                Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
    #endif            
                sysprintf("===> 2.2 File Read Fail\n");
                bIsAbort = TRUE;
                goto _end_;
            }
        }
    }
    else
    {
        while(1)
        {
      // write 1st sector with Boot Code Mark for SpiFlash        
            memcpy((UINT8 *)StorageBuffer, (UINT8 *)&BootCodeMark, sizeof(IBR_BOOT_STRUCT_T));    
            memcpy((UINT8 *)(StorageBuffer+sizeof(IBR_BOOT_STRUCT_T)), (UINT8 *)&optional_ini_file, optional_ini_size);      
            status = fsReadFile(hNvtFile, (UINT8 *)(StorageBuffer+sizeof(IBR_BOOT_STRUCT_T)+optional_ini_size),
                                BufferSize  - sizeof(IBR_BOOT_STRUCT_T) - optional_ini_size, &nReadLen); 
            if(nReadLen < BufferSize  - sizeof(IBR_BOOT_STRUCT_T) - optional_ini_size)                   
                   memset((UINT8 *)(StorageBuffer + nReadLen+sizeof(IBR_BOOT_STRUCT_T))+optional_ini_size , 0xff,  (BufferSize  - sizeof(IBR_BOOT_STRUCT_T) - optional_ini_size - nReadLen));
                                              
            nvtWriteSpi(nReadLen);  // write StorageBuffer to SD card
            BootCodeMarkFlag = 0;   // boot code mark had written

            if (status == ERR_FILE_EOF)
                break;
            else if (status < 0)
            {
#ifndef __NoLCM__           
                Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
#endif            
                sysprintf("===> 2.2 File Read Fail\n");
                bIsAbort = TRUE;
                goto _end_;
            }
        }
    }    
        
    
    /* verify loader */
    if(Ini_Writer.DataVerify)
    {
        sysprintf("=====> 2.3 verify SpiLoader [%d] <=====\n", sysGetTicks(0));
        fsFileSeek(hNvtFile, 0, SEEK_SET);
        i = FWInfo[FileInfoIdx].startBlock;
        while(1)
        {
            status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, SPI_BLOCK_SIZE, &nReadLen);
            usiRead(i*SPI_BLOCK_SIZE, SPI_BLOCK_SIZE, (UINT8 *)CompareBuffer);
            i++;
            
            if (status == ERR_FILE_EOF)
                break;
            else if (status < 0)
            {           
                sysprintf("===> 2.4 File Read Fail\n");
                bIsAbort = TRUE;    
#ifndef __NoLCM__   
                Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
#endif                     
                goto _end_;
            }
            
            if (memcmp((UINT8 *)StorageBuffer, (UINT8 *)(CompareBuffer +  sizeof(IBR_BOOT_STRUCT_T) + optional_ini_size), nReadLen))
            {  
                sysprintf("===> 2.4 Verify Fail 0\n");                        
#ifndef __NoLCM__   
                Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
                sprintf(Array1, "Loader:Verify Fail");     
                Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);
                Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    
#endif                          
                bIsAbort = TRUE;
                goto _end_;
            }
            if (memcmp((UINT8 *)&BootCodeMark, (UINT8 *)(CompareBuffer),  sizeof(IBR_BOOT_STRUCT_T)))
            {
                sysprintf("===> 2.4 Verify Fail 1\n");
#ifndef __NoLCM__   
                Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
                sprintf(Array1, "Loader:Verify Header Fail");     
                Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);
                Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    
#endif                    
                bIsAbort = TRUE;
                goto _end_;
            }
            if(optional_ini_size)
            {
                if (memcmp((UINT8 *)&optional_ini_file, (UINT8 *)(CompareBuffer) + sizeof(IBR_BOOT_STRUCT_T),  optional_ini_size))
                {
                    sysprintf("===> 2.4 Verify Fail 1\n");
#ifndef __NoLCM__   
                    Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
                    sprintf(Array1, "Loader:Verify Header Fail");     
                    Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);
                    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    
#endif                    
                    bIsAbort = TRUE;
                    goto _end_;
                }            
            }
        }
    }    
#ifndef __NoLCM__   
    Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
    font_y += Next_Font_Height;
#endif
    fsCloseFile(hNvtFile);
    hNvtFile = -1;

    FWInfo[FileInfoIdx].endBlock = gCurBlock - 1;
    gCurBlock = 1;      // 0x21 is the System Info sector, the next image MUST begin from 0x22
    FileInfoIdx++;
    BufferSize = 0x40000;
    /*************/
    /* copy logo */
    /*************/
WriteLogo:
    if (strlen(Ini_Writer.Logo.FileName) ==0)
        goto WriteExecute;

    sysprintf("=====> 3.0 copy logo [%d] <=====\n", sysGetTicks(0));
#ifndef __NoLCM__   
    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, "Writing Logo:");
    u32SkipX = 13;
#endif
    sprintf(szNvtFullName, "C:\\%s", Ini_Writer.Logo.FileName);
    fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);
    hNvtFile = fsOpenFile(suNvtFullName, NULL, O_RDONLY);

    sprintf(Array1, "Open %s", szNvtFullName);
#ifndef __NoLCM__       
    Draw_CurrentOperation(Array1,hNvtFile);
#endif    
    if (hNvtFile < 0)
    {
#ifndef __NoLCM__       
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
        sprintf(Array1, "Open Logo File Fail");
        Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);   
        Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    

#endif        
        sysprintf("===> 3.1 File Open Fail\n");
        bIsAbort = TRUE;
        goto _end_;
    }

    /* image information */
    FWInfo[FileInfoIdx].imageNo = FileInfoIdx;
    FWInfo[FileInfoIdx].imageFlag = 4;      // image type: logo
    if(Ini_Writer.Logo.StartBlock !=0)
    {
        if(Ini_Writer.Logo.StartBlock < gCurBlock)        
        {            
            sysprintf("===> 3.2 Logo Start Block is occupied (CurBlock %d Start Block %d)\n",gCurBlock,Ini_Writer.Logo.StartBlock);            
#ifndef __NoLCM__   
            sprintf(Array1, "Logo:Start Block is occupied"); 
            Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);
            Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    
#endif     
            bIsAbort = TRUE;
            goto _end_;                            
        }            

        gCurBlock = Ini_Writer.Logo.StartBlock;        
    }
    FWInfo[FileInfoIdx].startBlock = gCurBlock;    
    FWInfo[FileInfoIdx].executeAddr = Ini_Writer.Logo.address;
    FWInfo[FileInfoIdx].fileLen = (UINT32)fsGetFileSize(hNvtFile);
    memcpy(&FWInfo[FileInfoIdx].imageName[0], Ini_Writer.Logo.FileName, 32);

    while(1)
    {
        status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, BufferSize, &nReadLen);
#ifndef __NoLCM__        
        Draw_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
#endif        
        if(nReadLen < BufferSize)                   
               memset((UINT8 *)(StorageBuffer + nReadLen), 0xff,  (BufferSize - nReadLen));        
        nvtWriteSpi(nReadLen);   // write StorageBuffer to SD card
        if (status == ERR_FILE_EOF)
            break;
        else if (status < 0)
        {
#ifndef __NoLCM__           
            Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);  
            Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
#endif            
            sysprintf("===> 3.3 File Read Fail\n");
            bIsAbort = TRUE;
            goto _end_;
        }
    }
    if(Ini_Writer.DataVerify)
    {
        sysprintf("=====> 3.4 verify logo [%d] <=====\n", sysGetTicks(0));
        /* verify logo */
        fsFileSeek(hNvtFile, 0, SEEK_SET);
        i = FWInfo[FileInfoIdx].startBlock;
        while(1)
        {
            status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, SPI_BLOCK_SIZE, &nReadLen);
            usiRead(i*SPI_BLOCK_SIZE, SPI_BLOCK_SIZE, (UINT8 *)CompareBuffer);
            i++;

            if (memcmp((UINT8 *)StorageBuffer, (UINT8 *)CompareBuffer, nReadLen))
            {  
                sysprintf("===> 3.5 Verify Fail\n");                        
#ifndef __NoLCM__   
                Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);  
                Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
                sprintf(Array1, "Logo:Verify Fail");     
                Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);
                Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    
#endif                          
                bIsAbort = TRUE;
                goto _end_;
            }
            if (status == ERR_FILE_EOF)
                break;
            else if (status < 0)
            {
#ifndef __NoLCM__           
                Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
#endif            
                sysprintf("===> 3.5 File Read Fail\n");
                bIsAbort = TRUE;
                goto _end_;
            }
        }
    }
#ifndef __NoLCM__       
    Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);  
    Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
    font_y += Next_Font_Height;
#endif
    fsCloseFile(hNvtFile);
    hNvtFile = -1;

    FWInfo[FileInfoIdx].endBlock = gCurBlock - 1;
    FileInfoIdx++;
    
    /******************/
    /* copy  Execute  */
    /******************/
WriteExecute:
    if (strlen(Ini_Writer.Execute.FileName) ==0)
        goto WriteSysteInfo;

    sysprintf("=====> 4.0 copy Execute [%d] <=====\n", sysGetTicks(0));
#ifndef __NoLCM__   
    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, "Writing Execute:");
    u32SkipX = 16;
#endif
    sprintf(szNvtFullName, "C:\\%s",Ini_Writer.Execute.FileName);
    fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);
    hNvtFile = fsOpenFile(suNvtFullName, NULL, O_RDONLY);

    sprintf(Array1, "Open %s", szNvtFullName);
#ifndef __NoLCM__       
    Draw_CurrentOperation(Array1,hNvtFile);
#endif    
    if (hNvtFile < 0)
    {
#ifndef __NoLCM__       
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
        sprintf(Array1, "Open Execute File Fail");
        Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);   
        Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    

#endif        
        sysprintf("===> 4.1 File Open Fail\n");
        bIsAbort = TRUE;
        goto _end_;
    }

    /* image information */
    FWInfo[FileInfoIdx].imageNo = FileInfoIdx;
    FWInfo[FileInfoIdx].imageFlag = 1;      // image type: execute
    if(Ini_Writer.Execute.StartBlock !=0)
    {
        if(Ini_Writer.Execute.StartBlock < gCurBlock)        
        {            
            sysprintf("===> 4.2 Execute Start Block is occupied (CurBlock %d Start Block %d)\n",gCurBlock,Ini_Writer.Execute.StartBlock);
            
#ifndef __NoLCM__   
            sprintf(Array1, "Execute:Start Block is occupied"); 
            Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);
            Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    
#endif     
            bIsAbort = TRUE;
            goto _end_;                            
        }            

        gCurBlock = Ini_Writer.Execute.StartBlock;        
    }    
    FWInfo[FileInfoIdx].startBlock = gCurBlock;
    FWInfo[FileInfoIdx].executeAddr = Ini_Writer.Execute.address;
    FWInfo[FileInfoIdx].fileLen = (UINT32)fsGetFileSize(hNvtFile);
    memcpy(&FWInfo[FileInfoIdx].imageName[0], Ini_Writer.Execute.FileName, 32);

    while(1)
    {
        status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, BufferSize, &nReadLen);
#ifndef __NoLCM__        
        Draw_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
#endif        
        if(nReadLen < BufferSize)                   
               memset((UINT8 *)(StorageBuffer + nReadLen), 0xff,  (BufferSize - nReadLen));       
        nvtWriteSpi(nReadLen);
        if (status == ERR_FILE_EOF)
            break;
        else if (status < 0)
        {
#ifndef __NoLCM__     
            Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);        
            Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
#endif                    
            sysprintf("===> 4.3 File Open Fail\n");
            bIsAbort = TRUE;
            goto _end_;
        }
    }
    if(Ini_Writer.DataVerify)
    {
        sysprintf("=====> 4.4 verify Execute [%d] <=====\n", sysGetTicks(0));
        /* verify execute */
        fsFileSeek(hNvtFile, 0, SEEK_SET);
        i = FWInfo[FileInfoIdx].startBlock;
        while(1)
        {
            status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, SPI_BLOCK_SIZE, &nReadLen);
#ifndef __NoLCM__            
            Draw_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
#endif            
            usiRead(i*SPI_BLOCK_SIZE, SPI_BLOCK_SIZE, (UINT8 *)CompareBuffer);
            i++;
            if (memcmp((UINT8 *)StorageBuffer, (UINT8 *)CompareBuffer, nReadLen))
            {        
                sysprintf("===> 4.5 Verify Fail\n");
                            
#ifndef __NoLCM__   
                sprintf(Array1, "Execute:Verify Fail"); 
                
                   Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
                Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
                
                Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);

                Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    
#endif                          
                bIsAbort = TRUE;
                goto _end_;
            }

            if (status == ERR_FILE_EOF)
                break;
            else if (status < 0)
            {
#ifndef __NoLCM__           
                Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
#endif            
                sysprintf("===> 4.5 File Read Fail\n");
                bIsAbort = TRUE;
                goto _end_;
            }
        }
    }
#ifndef __NoLCM__   
    Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);  
    Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
    font_y += Next_Font_Height;
#endif
    fsCloseFile(hNvtFile);
    hNvtFile = -1;

    FWInfo[FileInfoIdx].endBlock = gCurBlock - 1;
    FileInfoIdx++;

    if(u32UserImageCount)
    {    
        sysprintf("=====> 5.0 copy user image [%d] <=====\n", sysGetTicks(0));    
        sprintf(Array1, "Writing %d User Image(s):", u32UserImageCount); 
#ifndef __NoLCM__    
        Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, Array1);
#endif        
        u32SkipX = 24;    
        for(i=0;i<u32UserImageCount;i++)
        {                    
            sysprintf("=====> 5.1 copy image %d [%d] <=====\n",i, sysGetTicks(0));                
            if(Ini_Writer.UserImage[i].StartBlock !=0)
            {
                if(Ini_Writer.UserImage[i].StartBlock< gCurBlock)        
                {
                    
                    sysprintf("===> 5.2 Image %d:Start Block is occupied (CurBlock %d Start Block %d)\n",i,gCurBlock,Ini_Writer.UserImage[i].StartBlock);
                                
#ifndef __NoLCM__   
                    sprintf(Array1, "Image %d:Start Block is occupied",i); 
                    Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);
                    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    
#endif     
                    bIsAbort = TRUE;
                    goto _end_;                            
                }            

                gCurBlock = Ini_Writer.UserImage[i].StartBlock;        
            }        
            
            sprintf(szNvtFullName, "C:\\%s",Ini_Writer.UserImage[i].FileName);
            fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);
            hNvtFile = fsOpenFile(suNvtFullName, NULL, O_RDONLY);

            sprintf(Array1, "Open %s", szNvtFullName);
#ifndef __NoLCM__       
            Draw_CurrentOperation(Array1,hNvtFile);
#endif                
            if (hNvtFile < 0)
            {
#ifndef __NoLCM__               
                   Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
                Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
                 sprintf(Array1, "Open Image %d File Fail",i);
                Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);   
                Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    

#endif        
                sysprintf("===> 5.3 File Open Fail\n");
                bIsAbort = TRUE;
                goto _end_;
            }

            /* image information */
            FWInfo[FileInfoIdx].imageNo = FileInfoIdx;
            FWInfo[FileInfoIdx].imageFlag = 0;      // image type: data
            FWInfo[FileInfoIdx].startBlock = gCurBlock;
            FWInfo[FileInfoIdx].executeAddr = DATA_ADDR;
            FWInfo[FileInfoIdx].fileLen = (UINT32)fsGetFileSize(hNvtFile);
            memcpy(&FWInfo[FileInfoIdx].imageName[0], Ini_Writer.UserImage[i].FileName, 32);            
        
            while(1)
            {
                status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, BufferSize, &nReadLen);        
#ifndef __NoLCM__                
                Draw_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
#endif                
                if(nReadLen < BufferSize)                   
                       memset((UINT8 *)(StorageBuffer + nReadLen), 0xff,  (BufferSize - nReadLen));            
                nvtWriteSpi(nReadLen);   // write StorageBuffer to Spiflash
                if (status == ERR_FILE_EOF)
                    break;
                else if (status < 0)
                {
#ifndef __NoLCM__       
                    Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);    
                    Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
#endif            
                    sysprintf("===> 5.4 File Read Fail\n");
                    bIsAbort = TRUE;
                    goto _end_;
                }
            }            
        
            if(Ini_Writer.DataVerify)
            {                
                sysprintf("=====> 5.5 verify image %d [%d] <=====\n",i, sysGetTicks(0));    
                fsFileSeek(hNvtFile, 0, SEEK_SET);
                j = FWInfo[FileInfoIdx].startBlock;
                while(1)
                {
                    status = fsReadFile(hNvtFile, (UINT8 *)StorageBuffer, SPI_BLOCK_SIZE, &nReadLen);
#ifndef __NoLCM__                    
                    Draw_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
#endif                    
                    usiRead(j*SPI_BLOCK_SIZE, SPI_BLOCK_SIZE, (UINT8 *)CompareBuffer);
                    j++;

                    if (memcmp((UINT8 *)StorageBuffer, (UINT8 *)CompareBuffer, nReadLen))
                    {
#ifndef __NoLCM__           
                        Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
                        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
                        sprintf(Array1, "User Image %d:Verify Fail",i);             
                        Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);
                        
                        Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);                            
#endif            
                        sysprintf("===> 5.6 Verify Fail\n");
                                      
                        bIsAbort = TRUE;
                        goto _end_;
                    }

                    if (status == ERR_FILE_EOF)
                        break;
                    else if (status < 0)
                    {
#ifndef __NoLCM__           
                        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Fail);
#endif            
                        sysprintf("===> 5.7 File Read Fail\n");
                        bIsAbort = TRUE;
                        goto _end_;
                    }
                   }            
            }        
            FWInfo[FileInfoIdx].endBlock = gCurBlock - 1;
            FileInfoIdx++;    

            fsCloseFile(hNvtFile);
            hNvtFile = -1;                                        
        }            
#ifndef __NoLCM__                   
        Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
           font_y += Next_Font_Height;
#endif        
    }

WriteSysteInfo:

    sysprintf("=====> 6.0 write image list [%d] <=====\n", sysGetTicks(0));    
#ifndef __NoLCM__    
    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, font_x, font_y, "Writing Image List:");
#endif    
    u32SkipX = 19; 
  
    /* set information to Block 63 */
    {
        unsigned int *ptr;
        pInfo = (UINT8 *)((UINT32)infoBuf | 0x80000000);
        ptr = (unsigned int *)((UINT32)infoBuf | 0x80000000);

        memset(pInfo, 0xff,  SPI_BLOCK_SIZE);

        /* update image information for Spi */
        *(ptr+0) = 0xAA554257;      // magic number
        *(ptr+1) = FileInfoIdx;     // image count
        *(ptr+2) = 0xFFFFFFFF;    // sector number for systeam area (Reserved)
        *(ptr+3) = 0x63594257;      // magic number
      
        usiRead(0 * SPI_BLOCK_SIZE, SPI_BLOCK_SIZE, (UINT8 *)StorageBuffer);                
        
        memcpy(pInfo+16, (char *)&FWInfo, IMAGE_LIST_SIZE - 16);
       
           memcpy((char *)(StorageBuffer + IMAGE_LIST_SIZE*63), (char *)pInfo, IMAGE_LIST_SIZE);
                       
        usiEraseSector(0*SPI_BLOCK_SIZE, 1);
        status = usiWrite(0*SPI_BLOCK_SIZE, SPI_BLOCK_SIZE, (UINT8 *)StorageBuffer);
        if (status != 0)
        {
            sysprintf("===> 6.1 ERROR in main(): Write System Info to Block 63 error ! Return = 0x%x.\n", status);
        }
        if(Ini_Writer.DataVerify)
        {    
            /* Verify information */        
            usiRead(0*SPI_BLOCK_SIZE, SPI_BLOCK_SIZE, (UINT8 *)CompareBuffer);
                    
            if (memcmp((UINT8 *)StorageBuffer, (UINT8 *)CompareBuffer, SPI_BLOCK_SIZE)==0)
                sysprintf("===> 6.2 System Info write OK on Block 0 .\n");
            else
            {
                sysprintf("===> 6.2 System Info write fail on Block 0.\n");
                sprintf(Array1, "Image List:Verify Fail");             
#ifndef __NoLCM__                 
                Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);
                Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);                                   
#endif                
                bIsAbort = TRUE;
                goto _end_;
            }
        }
    }
    

_end_:
    if(g_4byte_adderss)
    {
        Exit4ByteMode();
        g_4byte_adderss = FALSE;
        sysprintf("SPI 4 Byte Address Mode Disable\n");    
        
        Reset();
    }
    sysprintf("=====> Finish [%d] <=====\n", sysGetTicks(0));
    if(bIsAbort)
    {
         if(u32GpioPort_Fail)
        {
            if(u32GpioLevel_Fail)
                outp32(u32GpioPort_Fail,inp32(u32GpioPort_Fail) | u32GpioPin_Fail);
            else
                outp32(u32GpioPort_Fail,inp32(u32GpioPort_Fail) & ~u32GpioPin_Fail);    
        }
        if(Ini_Writer.SoundPlay)
        {
            if (strlen(Ini_Writer.SoundPlayFail.FileName) !=0)    
            {
                sprintf(szNvtFullName, "C:\\%s",Ini_Writer.SoundPlayFail.FileName);
                fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);
                hNvtFile = fsOpenFile(suNvtFullName, NULL, O_RDONLY);

                sprintf(Array1, "Open %s", szNvtFullName);
                if (hNvtFile < 0)
                {      
                    sysprintf("===> 1.1.1 open file name [%s], return = 0x%x\n", suNvtFullName, hNvtFile);
#ifndef __NoLCM__
                     sprintf(Array1, "Open Sound File <Fail> Fail");
                    Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);   
                    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    
#endif                           
                    goto _SoundOpenEnd11_;
                }            
                else
                {        
#ifndef __NoLCM__   
                     sprintf(Array1, "Open %s", szNvtFullName);
                       Draw_CurrentOperation(Array1,hNvtFile);
#endif                        
                    fsGetFileStatus(hNvtFile, suNvtFullName, szNvtFullName, &FileStatus);
                    
                    BufferSize = 0x40000;
                    pu8SoundPlayBuf = (PUINT8)malloc(sizeof(CHAR) * FileStatus.n64FileSize);    
                    u32pu8SoundPlayBuf = (UINT32) pu8SoundPlayBuf | 0x80000000;
                    if(pu8SoundPlayBuf == NULL)
                    {
                        sysprintf("\tCan't allocate the buffer for user sound file (size 0x%X)\n",FileStatus.n64FileSize);
#ifndef __NoLCM__
                         sprintf(Array1, "Memory allocate for Sound Fail");
                        Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);   
                        Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    
#endif                                     
                        goto _SoundOpenEnd10_;
                    }                
                    while(1)
                    {
                        status = fsReadFile(hNvtFile, (UINT8 *)u32pu8SoundPlayBuf, BufferSize, &nReadLen);
                        
                        u32pu8SoundPlayBuf += nReadLen;
                        if (status == ERR_FILE_EOF)
                            break;
                        else if (status < 0)
                        {
                            goto _SoundOpenEnd10_;
                        }
                    }
_SoundOpenEnd10_:                
                    fsCloseFile(hNvtFile);
_SoundOpenEnd11_:                    
                    hNvtFile = -1;                
                    if(pu8SoundPlayBuf != NULL)
                    {
                        sound_init(Ini_Writer.SoundPlayVolume);
                        sound_play(BURN_FAIL,(UINT32)pu8SoundPlayBuf | 0x80000000,FileStatus.n64FileSize);                
                        free (pu8SoundPlayBuf);
                    }
                }            
            }
            else
            {
                sound_init(Ini_Writer.SoundPlayVolume);
                sound_play(BURN_FAIL,0,0);          
            }
        }
    }
    else
    {
#ifndef __NoLCM__    
        Draw_Clear_Wait_Status(font_x+ u32SkipX*g_Font_Step, font_y);  
        Draw_Status(font_x+ u32SkipX*g_Font_Step, font_y, Successful);
#endif        
        font_y += Next_Font_Height;
        if(u32GpioPort_Pass)
        {
            if(u32GpioLevel_Pass)
                outp32(u32GpioPort_Pass,inp32(u32GpioPort_Pass) | u32GpioPin_Pass);
            else
                outp32(u32GpioPort_Pass,inp32(u32GpioPort_Pass) & ~u32GpioPin_Pass);    
        }    
        if(Ini_Writer.SoundPlay)
        {
            if (strlen(Ini_Writer.SoundPlayPass.FileName) !=0)    
            {
                sprintf(szNvtFullName, "C:\\%s",Ini_Writer.SoundPlayPass.FileName);
                fsAsciiToUnicode(szNvtFullName, suNvtFullName, TRUE);
                hNvtFile = fsOpenFile(suNvtFullName, NULL, O_RDONLY);

                sprintf(Array1, "Open %s", szNvtFullName);
                if (hNvtFile < 0)
                {      
                    sysprintf("===> 1.1.1 open file name [%s], return = 0x%x\n", suNvtFullName, hNvtFile);
#ifndef __NoLCM__
                     sprintf(Array1, "Open Sound File <Pass> Fail");
                    Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);   
                    Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    
#endif                           
                    goto _SoundOpenEnd21_;
                }            
                else
                {        
#ifndef __NoLCM__   
                     sprintf(Array1, "Open %s", szNvtFullName);
                       Draw_CurrentOperation(Array1,hNvtFile);
#endif                        
                    fsGetFileStatus(hNvtFile, suNvtFullName, szNvtFullName, &FileStatus);
                    
                    BufferSize = 0x40000;
                    pu8SoundPlayBuf = (PUINT8)malloc(sizeof(CHAR) * FileStatus.n64FileSize);    
                    u32pu8SoundPlayBuf = (UINT32) pu8SoundPlayBuf | 0x80000000;
                    if(pu8SoundPlayBuf == NULL)
                    {
                        sysprintf("\tCan't allocate the buffer for user sound file (size 0x%X)\n",FileStatus.n64FileSize);
#ifndef __NoLCM__
                         sprintf(Array1, "Memory allocate for Sound Fail");
                        Draw_Clear(2,_LCM_HEIGHT_ - g_Font_Height+1,_LCM_WIDTH_-3, _LCM_HEIGHT_-3, 0);   
                        Draw_Font(COLOR_RGB16_WHITE, &s_sDemo_Font, 0,  _LCM_HEIGHT_ -1-g_Font_Height,Array1);    
#endif                                     
                        goto _SoundOpenEnd20_;
                    }                
                    while(1)
                    {
                        status = fsReadFile(hNvtFile, (UINT8 *)u32pu8SoundPlayBuf, BufferSize, &nReadLen);
                        
                        u32pu8SoundPlayBuf += nReadLen;
                        if (status == ERR_FILE_EOF)
                            break;
                        else if (status < 0)
                        {
                            goto _SoundOpenEnd20_;
                        }
                    }
_SoundOpenEnd20_:                
                    fsCloseFile(hNvtFile);
_SoundOpenEnd21_:                    
                    hNvtFile = -1;                
                    if(pu8SoundPlayBuf != NULL)
                    {
                        sound_init(Ini_Writer.SoundPlayVolume);
                        sound_play(BURN_PASS,(UINT32)pu8SoundPlayBuf | 0x80000000,FileStatus.n64FileSize);                
                        free (pu8SoundPlayBuf);
                    }
                }                
            }
            else
            {        
                sound_init(Ini_Writer.SoundPlayVolume);
                sound_play(BURN_PASS,0,0);        
            }
        }
    }
#ifndef __NoLCM__       
    Draw_FinalStatus(bIsAbort);
#endif
    while(1);
}


