#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "wblib.h"
#include "w55fa92_vpe.h"
#include "w55fa92_vpost.h"

#define LCD_WIDTH   320
#define LCD_HEIGHT  240
#define VPE_SOURCE_PATTERN_WIDTH    468
#define VPE_SOURCE_PATTERN_HEIGHT   88

extern unsigned char g_au8VpeSrcPattern[];

static __align(256) UINT8  s_VpostFrameBufferPool[320*240*4];

void init(void)
{
    WB_UART_T uart;
    UINT32 u32ExtFreq;
    LCDFORMATEX lcdFormat;

    /* Init UART */
    u32ExtFreq = sysGetExternalClock();
    sysUartPort(1);
    uart.uiFreq = u32ExtFreq;   //use APB clock
    uart.uiBaudrate = 115200;
    uart.uiDataBits = WB_DATA_BITS_8;
    uart.uiStopBits = WB_STOP_BITS_1;
    uart.uiParity = WB_PARITY_NONE;
    uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    uart.uart_no = WB_UART_1;
    sysInitializeUART(&uart);

    lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGBx888;
    lcdFormat.nScreenWidth = LCD_WIDTH;
    lcdFormat.nScreenHeight = LCD_HEIGHT;
    vpostLCMInit(&lcdFormat, (UINT32*)s_VpostFrameBufferPool);

    sysSetLocalInterrupt(ENABLE_FIQ_IRQ);
}

void EnableCache(void)
{
    if (! sysGetCacheState ())
    {
        sysInvalidCache ();
///     sysEnableCache (CACHE_WRITE_THROUGH);
        sysEnableCache (CACHE_WRITE_BACK);
        sysFlushCache (I_D_CACHE);
    }
}

void vpe_demo(void)
{
    INT32 i32OpNormal, i, j, k;
    UINT32 u32Y, u32U, u32V;

    vpeOpen();

    u32Y = ((UINT32)g_au8VpeSrcPattern) | BIT31;
    u32U = u32Y+VPE_SOURCE_PATTERN_WIDTH*VPE_SOURCE_PATTERN_HEIGHT;     /* Planar YUV420 */
    u32V = u32U+VPE_SOURCE_PATTERN_WIDTH*VPE_SOURCE_PATTERN_HEIGHT/4;

    while(1)
    {
        for (i32OpNormal = 0; i32OpNormal < 6; i32OpNormal++)
        {
            if ((i32OpNormal == 1) || (i32OpNormal == 2))
                continue;

            if (j == 0)
            {
                j = 1;
                k = 320;
            }
            else
            {
                j = 0;
                k = 200;
            }

            vpeIoctl(VPE_IOCTL_HOST_OP,
                     VPE_HOST_FRAME_TURBO,
                     (E_VPE_CMD_OP)i32OpNormal,
                     NULL);

            vpeIoctl(VPE_IOCTL_SET_SRCBUF_ADDR,
                     u32Y,
                     u32U,
                     u32V);

            vpeIoctl(VPE_IOCTL_SET_DSTBUF_ADDR,
                     (((UINT32)s_VpostFrameBufferPool) | BIT31),
                     NULL,
                     NULL);

            vpeIoctl(VPE_IOCTL_SET_FMT,
                     VPE_SRC_PLANAR_YUV420, /* Src Format */
                     VPE_DST_PACKET_RGB888, /* Dst Format */
                     0);

            vpeIoctl(VPE_IOCTL_SET_SRC_OFFSET,
                     (UINT32)0, /* Src Left offset */
                     (UINT32)0, /* Src right offset */
                     NULL);

            vpeIoctl(VPE_IOCTL_SET_DST_OFFSET,
                     (UINT32)((LCD_WIDTH - k) / 2),   /* Dst Left offset */
                     (UINT32)((LCD_WIDTH - k) / 2),   /* Dst right offset */
                     NULL);

            vpeIoctl(VPE_IOCTL_SET_SRC_DIMENSION,
                     VPE_SOURCE_PATTERN_WIDTH,
                     VPE_SOURCE_PATTERN_HEIGHT,
                     NULL);

            vpeIoctl(VPE_IOCTL_SET_DST_DIMENSION,   /* Scale Down */
                     k,
                     230,
                     NULL);

            vpeIoctl(VPE_IOCTL_SET_FILTER,
                     VPE_SCALE_BILINEAR,
                     NULL,
                     NULL);

            vpeIoctl(VPE_IOCTL_TRIGGER,
                     NULL,
                     NULL,
                     NULL);
            do
            {
                ERRCODE errcode;
                //TRUE==>Not complete, FALSE==>Complete
                errcode = vpeIoctl(VPE_IOCTL_CHECK_TRIGGER,
                                   NULL,
                                   NULL,
                                   NULL);
                if(errcode==0)
                    break;
            }
            while(1);

            for (i = 0; i < 0x800000; i ++);
        }
    }
}

INT32 main(void)
{
    EnableCache();
    init();

    sysprintf("================================================================\n");
    sysprintf("						VPE demo\n");
    sysprintf("================================================================\n");

    vpe_demo();

    return 0;
}
