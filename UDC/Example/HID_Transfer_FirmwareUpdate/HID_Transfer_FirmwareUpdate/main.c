#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "W55FA92_reg.h"
#include "HID.h"
#include "Common.h"   
#include "W55FA92_VPOST.h"

void HIDStart(void);

#if defined (__GNUC__)
CHAR g_pu8FrameBuffer[PANEL_WIDTH * PANEL_HEIGHT * 2] __attribute__((aligned(32)));
#else
__align(32) CHAR g_pu8FrameBuffer[PANEL_WIDTH * PANEL_HEIGHT * 2];
#endif
LCDFORMATEX lcdInfo;
UINT32 jpeg_decode = 0;

void delay(int delay)
{
    int volatile count = 100 * delay;
    int volatile i;
    for(i=0;i<count;i++)
        ;
}
int main(void)
{
    WB_UART_T uart;
    int status = 0;
    UINT32 u32ExtFreq;

    u32ExtFreq = sysGetExternalClock();    	/* Hz unit */	
    uart.uiFreq = u32ExtFreq;
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uart_no=WB_UART_1;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    sysInitializeUART(&uart);
  
    sysInvalidCache();
    sysEnableCache(CACHE_WRITE_THROUGH);
    sysFlushCache(I_D_CACHE);
    sysSetDramClock(eSYS_MPLL, 360000000, 360000000);
    sysSetSystemClock(eSYS_UPLL,
                      240000000,
                      240000000);

    sysprintf("Demo code Start\n");

    /* Init Panel */
    lcdInfo.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
    lcdInfo.nScreenWidth = PANEL_WIDTH;
    lcdInfo.nScreenHeight = PANEL_HEIGHT;

    vpostLCMInit(&lcdInfo, (UINT32*)((UINT32)g_pu8FrameBuffer | 0x80000000));

    jpegOpen();

    HIDStart();

    while(1)
    {
        switch(status & 0x03)
        {
            case 0:
                sysprintf("\r-");
                break;
            case 1:
                sysprintf("\r\\");
                break;
            case 2:
                sysprintf("\r|");
                break;
            case 3:
                sysprintf("\r/");
                break;
        }
        status++;
        delay(10000);
    }
}

