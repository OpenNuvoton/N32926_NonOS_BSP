/**************************************************************************//**
 * @file     main.c
 * @brief    Demonstrate BLT functions
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

//#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "wblib.h"
#include "BLT.h"
#include "W55FA92_VPOST.h"

/* Color format
 *
 * RGB565           : 2-byte, no alpha
 * RGB888(xRGB8888) : 4-byte with MSB byte ignored, no alpha
 * ARGB8888         : 4-byte with MSB byte representing alpha
 */

static uint8_t src_pat[] = {
//#include "Pat_RGB888_size160x120.txt"
#include "Pat_ARGB888_size160x120.txt"
};

/* Source image spec. Must be consistent with source image data */
#define SRCIMG_HASALPHA     1
#define SRCIMG_WIDTH        160
#define SRCIMG_STRIDE       (SRCIMG_WIDTH * 4)
#define SRCIMG_HEIGHT       120

#define DISP_WIDTH          320
#define DISP_STRIDE         (DISP_WIDTH * 2)
#define DISP_HEIGHT         240

#define SIZE_SRCIMG         (SRCIMG_STRIDE * SRCIMG_HEIGHT)
#define SIZE_SRCIMG_BUF     ((SIZE_SRCIMG + 31) / 32 * 32)
#define SIZE_DISP           (DISP_STRIDE * DISP_HEIGHT)
#define SIZE_DISP_BUF       ((SIZE_DISP + 31) / 32 * 32)

#define SIZE_TXMEM          (SIZE_SRCIMG_BUF + SIZE_DISP_BUF)

#if defined (__GNUC__)
uint8_t txmem[SIZE_TXMEM] __attribute__((aligned (32)));
#else
__align (32) uint8_t txmem[SIZE_TXMEM];
#endif

/* To avoid error-prone cache synchronization, the txmem for CPU and BLT operation is always
 * non-cacheable. */
#define ADDR_SRCIMG         (sysGetCacheState() ? (((uint32_t) txmem) | 0x80000000) : (((uint32_t) txmem)))
#define ADDR_DISP           (ADDR_SRCIMG + SIZE_SRCIMG_BUF)

#define FMT_DST             eDRVBLT_DEST_RGB565

#define COLOR_WHITE         0xFFFFFFFF
#define COLOR_BLACK         0xFF000000
#define COLOR_GRAY          0xFF808080
#define COLOR_RED           0xFFFF0000
#define COLOR_GREEN         0xFF00FF00
#define COLOR_BLUE          0xFF0000FF



#define DELAY_INTER_FRAME   50

void clr_disp_buf(uint32_t fill_color)
{
    S_FI_FILLOP clr_op;
    
    // Fill color is non-premultiplied alpha.
       
    clr_op.sRect.i16Xmin = 0;
    clr_op.sRect.i16Ymin = 0;  
    clr_op.sRect.i16Xmax = DISP_WIDTH;
    clr_op.sRect.i16Ymax = DISP_HEIGHT;      
    
    clr_op.sARGB8.u8Blue = (fill_color & 0x000000FF);
    clr_op.sARGB8.u8Green = (fill_color & 0x0000FF00) >> 8;
    clr_op.sARGB8.u8Red = (fill_color & 0x00FF0000) >> 16;
    clr_op.sARGB8.u8Alpha = (fill_color & 0xFF000000) >> 24;
    
    clr_op.u32FBAddr = ADDR_DISP;
    clr_op.i32Stride = DISP_STRIDE;
    clr_op.eDisplayFmt = FMT_DST;
    clr_op.i32Blend = 0;    // No alpha blending.
    
    bltFIFill(clr_op);
}

/**
  * @brief              Scale source pattern with top-left point as center.
  * @param[in]  ox,oy   coordinates of center in FB CS.
  * @param[in]  sx      scale factor along x-axis
  * @param[in]  sy      scale factor along y-axis
  * 
  * Ma * Mt * Mm * Src = Dst, where
  * Ma: matrix for amendment to mapping point error
  * Ms: scale matrix
  * Mm: reflect matrix
  *
  *      / 1 0 -0.5 \       / 1 0 tx \       / sx 0  0 \
  * Ma = | 0 1 -0.5 |  Mt = | 0 1 ty |  Ms = | 0  sy 0 |
  *      \ 0 0 1    /       \ 0 0 1  /       \ 0  0  1 /
  *
  * Src = Inv(Ms) * Inv(Mt) * Inv(Ma) * DST
  *
  *           / 1/sx 0    0 \            / 1 0 -tx \            / 1 0 0.5 \
  * Inv(Ms) = | 0    1/sy 0 |  Inv(Mt) = | 0 1 -ty |  Inv(Ma) = | 0 1 0.5 |
  *           \ 0    0    1 /            \ 0 0 1   /            \ 0 0 1   /
  *
  *                               / 1/sx 0    -(1/sx)*tx+0.5*(a+c) \   / a c src.xoffset \
  * Inv(Ms) * Inv(Mt) * Inv(Ma) = | 0    1/sy -(1/sy)*ty+0.5*(b+d) | = | b d src.yoffset |
  *                               \ 0    0    1                    /   \ 0 0 0           /
  */
void demo_scale(float ox, float oy, float sx, float sy, int is_tiling)
{
    bltSetFillOP((E_DRVBLT_FILLOP) FALSE);  // Blit operation.
    bltSetDisplayFormat(FMT_DST);           // Set destination format.
    bltSetSrcFormat(eDRVBLT_SRC_ARGB8888);  // Set source image format to RGB888/ARGB8888.
    bltSetRevealAlpha(eDRVBLT_NO_EFFECTIVE);    // Source image format is non-premultiplied alpha.
    
    {   // Set transform matrix a/b/c/d
        S_DRVBLT_MATRIX xform_mx;
        
        xform_mx.a  =   (INT32) ((1 / sx) * 0x10000);
        xform_mx.b  =   0;
        xform_mx.c  =   0;
        xform_mx.d  =   (INT32) ((1 / sy) * 0x10000);
        
        bltSetTransformMatrix(xform_mx);
    }
    
    {   // Set color multiplier for color transform.
        S_DRVBLT_ARGB16 color_multiplier;
        
        color_multiplier.i16Blue    =   0x100;
        color_multiplier.i16Green   =   0x100;
        color_multiplier.i16Red     =   0x100;
        color_multiplier.i16Alpha   =   0x100;
        
        bltSetColorMultiplier(color_multiplier);
    }   
        
    {   // Set color offset for color transform
        S_DRVBLT_ARGB16 color_offset;
        
        color_offset.i16Blue    =   0;
        color_offset.i16Green   =   0;
        color_offset.i16Red     =   0;
        color_offset.i16Alpha   =   0;
        
        bltSetColorOffset(color_offset);
    }   
    
    // Apply color transformation on all 4 channels.
    if (SRCIMG_HASALPHA) {
        bltSetTransformFlag(eDRVBLT_HASTRANSPARENCY | eDRVBLT_HASCOLORTRANSFORM);
    } else {
        bltSetTransformFlag(eDRVBLT_HASCOLORTRANSFORM);
    }
    bltSetFillStyle((E_DRVBLT_FILL_STYLE) (eDRVBLT_NOTSMOOTH | (is_tiling ? 0 : eDRVBLT_NONE_FILL))); // No smoothing.

    {   // Set source image.
        S_DRVBLT_SRC_IMAGE src_img;
        
        src_img.u32SrcImageAddr = ADDR_SRCIMG;
        {
            S_DRVBLT_MATRIX xform_mx;
        
            src_img.i32XOffset = (INT32) (0 - ((1 / sx) * ox * 0x10000));       // 16.16
            src_img.i32YOffset = (INT32) (0 - ((1 / sy) * oy * 0x10000));       // 16.16
            
            // Apply amendment to mapping point error.
            bltGetTransformMatrix(&xform_mx);
            src_img.i32XOffset += (xform_mx.a + xform_mx.c) / 2;
            src_img.i32YOffset += (xform_mx.b + xform_mx.d) / 2;
        }
        src_img.i16Width = SRCIMG_WIDTH;
        src_img.i32Stride = SRCIMG_STRIDE;
        src_img.i16Height = SRCIMG_HEIGHT;
    
        bltSetSrcImage(src_img);
    }
    
    {   // Set destination buffer.
        S_DRVBLT_DEST_FB dst_img;
        
        dst_img.u32FrameBufAddr = ADDR_DISP;
        dst_img.i16Width = DISP_WIDTH;
        dst_img.i32Stride = DISP_STRIDE;     
        dst_img.i16Height = DISP_HEIGHT;
        
        bltSetDestFrameBuf(dst_img);
    }

    /* We assume txmem for CPU and BLT is non-cacheable, so we needn't do any cache-related
     * synchronization. */
    bltTrigger();   // Trigger Blit operation.  
    bltFlush();     // Wait for complete.
}

#define PI_OVER_180 0.01745329252f
/**
  * @brief              Rotate source pattern around pivot
  * @param[in]  ox,oy   coordinates of center in FB CS
  * @param[in]  px,py   coordinates of pivot relative to top-left point of source pattern  
  * @param[in]  deg     Angle in degree
  * 
  * Ma * Mt * Mr * Mp * Src = Dst, where
  * Ma: matrix for amendment to mapping point error
  * Mt: translation matrix
  * Mr: rotation matrix
  * Mp: translation matrix to adjust pivot
  *
  *      / 1 0 -0.5 \       / 1 0 tx \       / cos�c -sin�c 0 \       / 1 0 -px \
  * Ma = | 0 1 -0.5 |  Mt = | 0 1 ty |  Mr = | sin�c con�c  0 |  Mp = | 0 1 -py |
  *      \ 0 0 1    /       \ 0 0 1  /       \ 0    0     1 /       \ 0 0 1   /
  *
  * Src = Inv(Mp) * Inv(Mr) * Inv(Mt) * Inv(Ma) * DST
  *
  *           / 1 0 px \            / cos�c  sin�c 0 \            / 1 0 -tx \            / 1 0 0.5 \
  * Inv(Mp) = | 0 1 py |  Inv(Mr) = | -sin�c con�c 0 |  Inv(Mt) = | 0 1 -ty |  Inv(Ma) = | 0 1 0.5 |
  *           \ 0 0 1  /            \ 0     0    1 /            \ 0 0 1   /            \ 0 0 1   /
  *
  *                                         / con�c  sin�c px - con�c*tx-sin�c*ty+0.5*(a+c) \   / a c src.xoffset \
  * Inv(Mp) * Inv(Mr) * Inv(Mt) * Inv(Ma) = | -sin�c con�c py + sin�c*tx-con�c*ty+0.5*(b+d) | = | b d src.yoffset |
  *                                         \ 0     0    1                              /   \ 0 0 1           /
  */
void demo_rotate(float ox, float oy, float px, float py, float deg)
{
    bltSetFillOP((E_DRVBLT_FILLOP) FALSE);  // Blit operation.
    bltSetDisplayFormat(FMT_DST);           // Set destination format.
    bltSetSrcFormat(eDRVBLT_SRC_ARGB8888);  // Set source image format to RGB888/ARGB8888.
    bltSetRevealAlpha(eDRVBLT_NO_EFFECTIVE);    // Source image format is non-premultiplied alpha.
    
    {   // Set transform matrix a/b/c/d
        S_DRVBLT_MATRIX xform_mx;
        
        xform_mx.a  =   cos(PI_OVER_180 * deg) * 0x10000;
        xform_mx.b  =   -sin(PI_OVER_180 * deg) * 0x10000;
        xform_mx.c  =   sin(PI_OVER_180 * deg) * 0x10000;
        xform_mx.d  =   cos(PI_OVER_180 * deg) * 0x10000;
    
        bltSetTransformMatrix(xform_mx);
    }
    
    {   // Set color multiplier for color transform.
        S_DRVBLT_ARGB16 color_multiplier;
        
        color_multiplier.i16Blue    =   0x100;
        color_multiplier.i16Green   =   0x100;
        color_multiplier.i16Red     =   0x100;
        color_multiplier.i16Alpha   =   0x100;
        
        bltSetColorMultiplier(color_multiplier);
    }   
        
    {   // Set color offset for color transform
        S_DRVBLT_ARGB16 color_offset;
        
        color_offset.i16Blue    =   0;
        color_offset.i16Green   =   0;
        color_offset.i16Red     =   0;
        color_offset.i16Alpha   =   0;
        
        bltSetColorOffset(color_offset);
    }   

    // Apply color transformation on all 4 channels.
    if (SRCIMG_HASALPHA) {
        bltSetTransformFlag(eDRVBLT_HASTRANSPARENCY | eDRVBLT_HASCOLORTRANSFORM);
    } else {
        bltSetTransformFlag(eDRVBLT_HASCOLORTRANSFORM);
    }
    bltSetFillStyle((E_DRVBLT_FILL_STYLE) (eDRVBLT_NONE_FILL | eDRVBLT_NOTSMOOTH)); // No smoothing.

    {   // Set source image.
        S_DRVBLT_SRC_IMAGE src_img;
        
        
        src_img.u32SrcImageAddr = ADDR_SRCIMG;
        {
            S_DRVBLT_MATRIX xform_mx;

            // Pivot
            src_img.i32XOffset = px * 0x10000;      // 16.16
            src_img.i32YOffset = py * 0x10000;      // 16.16

            // Translate after rotate
            src_img.i32XOffset += -(cos(PI_OVER_180 * deg) * ox + sin(PI_OVER_180 * deg) * oy) * 0x10000;    // 16.16
            src_img.i32YOffset += (sin(PI_OVER_180 * deg) * ox - cos(PI_OVER_180 * deg) * oy) * 0x10000;     // 16.16
            
            // Apply amendment to mapping point error.
            bltGetTransformMatrix(&xform_mx);
            src_img.i32XOffset += (xform_mx.a + xform_mx.c) / 2;
            src_img.i32YOffset += (xform_mx.b + xform_mx.d) / 2;
        }
        src_img.i16Width = SRCIMG_WIDTH;
        src_img.i32Stride = SRCIMG_STRIDE;
        src_img.i16Height = SRCIMG_HEIGHT;
    
        bltSetSrcImage(src_img);
    }
    
    {   // Set destination buffer.
        S_DRVBLT_DEST_FB dst_img;
        
        dst_img.u32FrameBufAddr = ADDR_DISP;
        dst_img.i16Width = DISP_WIDTH;
        dst_img.i32Stride = DISP_STRIDE;     
        dst_img.i16Height = DISP_HEIGHT;
        
        bltSetDestFrameBuf(dst_img);
    }

    /* We assume txmem for CPU and BLT is non-cacheable, so we needn't do any cache-related
     * synchronization. */
    bltTrigger();   // Trigger Blit operation.  
    bltFlush();     // Wait for complete.
}

/**
  * @brief              Reflect source pattern about x-axis/y-axis/origin with top-left point as center.
  * @param[in]  ox,oy   coordinates of center in FB CS.
  * @param[in]  mx      reflect about x-axis
  * @param[in]  my      reflect about y-axis
  * 
  * Ma * Mt * Mm * Src = Dst, where
  * Ma: matrix for amendment to mapping point error
  * Mt: translation matrix
  * Mm: reflect matrix
  *
  *      / 1 0 -0.5 \       / 1 0 tx \       / my?-1:1 0       0 \
  * Ma = | 0 1 -0.5 |  Mt = | 0 1 ty |  Mm = | 0       mx?-1:1 0 |
  *      \ 0 0 1    /       \ 0 0 1  /       \ 0       0       1 /
  *
  * Src = Inv(Mm) * Inv(Mt) * Inv(Ma) * DST
  *
  *           / my?-1:1 0       0 \            / 1 0 -tx \            / 1 0 0.5 \
  * Inv(Mm) = | 0       mx?-1:1 0 |  Inv(Mt) = | 0 1 -ty |  Inv(Ma) = | 0 1 0.5 |
  *           \ 0       0       1 /            \ 0 0 1   /            \ 0 0 1   /
  *
  *                               / my?-1:1 0       -(my?-1:1)*tx+0.5*(a+c) \   / a c src.xoffset \
  * Inv(Mm) * Inv(Mt) * Inv(Ma) = | 0       mx?-1:1 -(mx?-1:1)*ty+0.5*(b+d) | = | b d src.yoffset |
  *                               \ 0       0       1                       /   \ 0 0 0           /
  */
void demo_reflect(float ox, float oy, int mx, int my)
{
    bltSetFillOP((E_DRVBLT_FILLOP) FALSE);  // Blit operation.
    bltSetDisplayFormat(FMT_DST);           // Set destination format.
    bltSetSrcFormat(eDRVBLT_SRC_ARGB8888);  // Set source image format to RGB888/ARGB8888.
    bltSetRevealAlpha(eDRVBLT_NO_EFFECTIVE);    // Source image format is non-premultiplied alpha.
    
    {   // Set transform matrix a/b/c/d
        S_DRVBLT_MATRIX xform_mx;
        
        xform_mx.a  =   (my ? -1 : 1) * 0x10000;
        xform_mx.b  =   0;
        xform_mx.c  =   0;
        xform_mx.d  =   (mx ? -1 : 1) * 0x10000;
    
        bltSetTransformMatrix(xform_mx);
    }
    
    {   // Set color multiplier for color transform.
        S_DRVBLT_ARGB16 color_multiplier;
        
        color_multiplier.i16Blue    =   0x100;
        color_multiplier.i16Green   =   0x100;
        color_multiplier.i16Red     =   0x100;
        color_multiplier.i16Alpha   =   0x100;
        
        bltSetColorMultiplier(color_multiplier);
    }   
        
    {   // Set color offset for color transform
        S_DRVBLT_ARGB16 color_offset;
        
        color_offset.i16Blue    =   0;
        color_offset.i16Green   =   0;
        color_offset.i16Red     =   0;
        color_offset.i16Alpha   =   0;
        
        bltSetColorOffset(color_offset);
    }   

    // Apply color transformation on all 4 channels.
    if (SRCIMG_HASALPHA) {
        bltSetTransformFlag(eDRVBLT_HASTRANSPARENCY | eDRVBLT_HASCOLORTRANSFORM);
    } else {
        bltSetTransformFlag(eDRVBLT_HASCOLORTRANSFORM);
    }
    bltSetFillStyle((E_DRVBLT_FILL_STYLE) (eDRVBLT_NONE_FILL | eDRVBLT_NOTSMOOTH)); // No smoothing.

    {   // Set source image.
        S_DRVBLT_SRC_IMAGE src_img;
        
        
        src_img.u32SrcImageAddr = ADDR_SRCIMG;
        {
            S_DRVBLT_MATRIX xform_mx;
        
            src_img.i32XOffset = -((my ? -1 : 1) * ox) * 0x10000;   // 16.16
            src_img.i32YOffset = -((mx ? -1 : 1) * oy) * 0x10000;   // 16.16
            
            // Apply amendment to mapping point error.
            bltGetTransformMatrix(&xform_mx);
            src_img.i32XOffset += (xform_mx.a + xform_mx.c) / 2;
            src_img.i32YOffset += (xform_mx.b + xform_mx.d) / 2;
        }
        src_img.i16Width = SRCIMG_WIDTH;
        src_img.i32Stride = SRCIMG_STRIDE;
        src_img.i16Height = SRCIMG_HEIGHT;
    
        bltSetSrcImage(src_img);
    }
    
    {   // Set destination buffer.
        S_DRVBLT_DEST_FB dst_img;
        
        dst_img.u32FrameBufAddr = ADDR_DISP;
        dst_img.i16Width = DISP_WIDTH;
        dst_img.i32Stride = DISP_STRIDE;     
        dst_img.i16Height = DISP_HEIGHT;
        
        bltSetDestFrameBuf(dst_img);
    }

    /* We assume txmem for CPU and BLT is non-cacheable, so we needn't do any cache-related
     * synchronization. */
    bltTrigger();   // Trigger Blit operation.  
    bltFlush();     // Wait for complete.
}

void demo_alpha(float ox, float oy, float alpha)
{
    bltSetFillOP((E_DRVBLT_FILLOP) FALSE);  // Blit operation.
    bltSetDisplayFormat(FMT_DST);           // Set destination format.
    bltSetSrcFormat(eDRVBLT_SRC_ARGB8888);  // Set source image format to RGB888/ARGB8888.
    bltSetRevealAlpha(eDRVBLT_NO_EFFECTIVE);    // Source image format is non-premultiplied alpha.
    
    {   // Set transform matrix to identify matrix. So no scaling, no rotation, no shearing, etc.
        S_DRVBLT_MATRIX xform_mx;
        
        xform_mx.a  =   0x10000;
        xform_mx.b  =   0;
        xform_mx.c  =   0;
        xform_mx.d  =   0x10000;
    
        bltSetTransformMatrix(xform_mx);
    }
    
    {   // Set color multiplier for color transform.
        S_DRVBLT_ARGB16 color_multiplier;
        
        color_multiplier.i16Blue    =   0x100;
        color_multiplier.i16Green   =   0x100;
        color_multiplier.i16Red     =   0x100;
        color_multiplier.i16Alpha   =   (int16_t) (alpha * 256);
        
        bltSetColorMultiplier(color_multiplier);
    }   
        
    {   // Set color offset for color transform
        S_DRVBLT_ARGB16 color_offset;
        
        color_offset.i16Blue    =   0;
        color_offset.i16Green   =   0;
        color_offset.i16Red     =   0;
        color_offset.i16Alpha   =   0;
        
        bltSetColorOffset(color_offset);
    }   

    // Apply color transformation on all 4 channels.
    if (SRCIMG_HASALPHA) {
        bltSetTransformFlag(eDRVBLT_HASTRANSPARENCY | eDRVBLT_HASCOLORTRANSFORM);
    } else {
        bltSetTransformFlag(eDRVBLT_HASCOLORTRANSFORM);
    }
    bltSetFillStyle((E_DRVBLT_FILL_STYLE) (eDRVBLT_NONE_FILL | eDRVBLT_NOTSMOOTH)); // No smoothing.

    {   // Set source image.
        S_DRVBLT_SRC_IMAGE src_img;
        
        src_img.u32SrcImageAddr = ADDR_SRCIMG;
        src_img.i32XOffset = -ox * 0x10000;     // 16.16
        src_img.i32YOffset = -oy * 0x10000;     // 16.16
        src_img.i16Width = SRCIMG_WIDTH;
        src_img.i32Stride = SRCIMG_STRIDE;
        src_img.i16Height = SRCIMG_HEIGHT;
    
        bltSetSrcImage(src_img);
    }
    
    {   // Set destination buffer.
        S_DRVBLT_DEST_FB dst_img;
        
        dst_img.u32FrameBufAddr = ADDR_DISP;
        dst_img.i16Width = DISP_WIDTH;
        dst_img.i32Stride = DISP_STRIDE;     
        dst_img.i16Height = DISP_HEIGHT;
        
        bltSetDestFrameBuf(dst_img);
    }

    /* We assume txmem for CPU and BLT is non-cacheable, so we needn't do any cache-related
     * synchronization. */
    bltTrigger();   // Trigger Blit operation.  
    bltFlush();     // Wait for complete.
}

int main()
{
    // Cache on.
    if (! sysGetCacheState()) {
        sysInvalidCache();
        sysEnableCache(CACHE_WRITE_THROUGH);
        sysFlushCache(I_D_CACHE);
    }

    {   // Initialize UART.
        UINT32 u32ExtFreq;
        WB_UART_T uart;
    
        u32ExtFreq = sysGetExternalClock();
        //sysUartPort(1);
        uart.uiFreq = u32ExtFreq;   //use APB clock
        uart.uiBaudrate = 115200;
        uart.uiDataBits = WB_DATA_BITS_8;
        uart.uiStopBits = WB_STOP_BITS_1;
        uart.uiParity = WB_PARITY_NONE;
        uart.uiRxTriggerLevel = LEVEL_1_BYTE;
        uart.uart_no = WB_UART_1;
        sysInitializeUART(&uart);
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

    {   // Initialize timer.
        UINT32 u32ExtFreq = sysGetExternalClock();
        
        sysSetTimerReferenceClock (TIMER0, u32ExtFreq);
    }

    {   // Initialize VPOST.
        LCDFORMATEX lcdFormat;
        
        lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;    // Initialize VPOST.
        vpostLCMInit(&lcdFormat, (UINT32 *) ADDR_DISP);
    }

    sysSetLocalInterrupt (ENABLE_IRQ);  // Enable CPSR I bit

    do {
        
        memcpy((void *) ADDR_SRCIMG, src_pat, SIZE_SRCIMG);
        bltOpen();
            
        // Scale. No tiling.
        clr_disp_buf(COLOR_GRAY);
        demo_scale(0.0f, 0.0f, 0.5f, 0.5f, 0);
        sysDelay(DELAY_INTER_FRAME);
        clr_disp_buf(COLOR_GRAY);
        demo_scale(0.0f, 0.0f, 1.0f, 1.0f, 0);
        sysDelay(DELAY_INTER_FRAME);
        clr_disp_buf(COLOR_GRAY);
        demo_scale(0.0f, 0.0f, 1.5f, 1.5f, 0);
        sysDelay(DELAY_INTER_FRAME);
        clr_disp_buf(COLOR_GRAY);
        demo_scale(0.0f, 0.0f, 2.0f, 2.0f, 0);
        sysDelay(DELAY_INTER_FRAME);
        
        // Scale without aspect ratio kept. No tiling.
        clr_disp_buf(COLOR_GRAY);
        demo_scale(0.0f, 0.0f, 2.0f, 0.5f, 0);
        sysDelay(DELAY_INTER_FRAME);
        clr_disp_buf(COLOR_GRAY);
        demo_scale(0.0f, 0.0f, 0.5f, 2.0f, 0);
        sysDelay(DELAY_INTER_FRAME);
        
        // Scale. Tiling.
        clr_disp_buf(COLOR_GRAY);
        demo_scale(0.0f, 0.0f, 0.5f, 0.5f, 1);
        sysDelay(DELAY_INTER_FRAME);
        clr_disp_buf(COLOR_GRAY);
        demo_scale(0.0f, 0.0f, 1.0f, 1.0f, 1);
        sysDelay(DELAY_INTER_FRAME);

        // Rotate with pivot unchanged
        {
            float deg_arr[] = {0.0f, 30.0f, 60.0f, 90.0f, 120.0f, 150.0f, 180.0f, 210.0f, 240.0f, 270.0f, 300.0f, 330.0f, 360.0f};
            float *deg_ind = deg_arr;
            float *deg_end = deg_arr + sizeof (deg_arr) / sizeof (deg_arr[0]);
            
            while (deg_ind != deg_end) {
                clr_disp_buf(COLOR_GRAY);
                demo_rotate(160.0f, 120.0f, 0.0f, 0.0f, *deg_ind);
                sysDelay(DELAY_INTER_FRAME);
                
                deg_ind ++;
            }
        }

        // Rotate with pivot changed
        {
            float deg_arr[] = {0.0f, 30.0f, 60.0f, 90.0f, 120.0f, 150.0f, 180.0f, 210.0f, 240.0f, 270.0f, 300.0f, 330.0f, 360.0f};
            float *deg_ind = deg_arr;
            float *deg_end = deg_arr + sizeof (deg_arr) / sizeof (deg_arr[0]);
            
            while (deg_ind != deg_end) {
                clr_disp_buf(COLOR_GRAY);
                demo_rotate(160.0f, 120.0f, 40.0f, 30.0f, *deg_ind);
                sysDelay(DELAY_INTER_FRAME);
                
                deg_ind ++;
            }
        }

        // Reflect
        // Reflect about x-axis
        clr_disp_buf(COLOR_GRAY);
        demo_reflect(0.0f, 120.0f, 1, 0);
        sysDelay(DELAY_INTER_FRAME);
        // Reflect about y-axis
        clr_disp_buf(COLOR_GRAY);
        demo_reflect(160.0f, 0.0f, 0, 1);
        sysDelay(DELAY_INTER_FRAME);
        // Reflect about both x-axis/y-axis (origin)
        clr_disp_buf(COLOR_GRAY);
        demo_reflect(160.0f, 120.0f, 1, 1);
        sysDelay(DELAY_INTER_FRAME);
        
        // Fade-in/Fase-out.
        {
            float alpha_arr[] = {
                0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 
                0.8f, 0.9f, 1.0f, 0.9f, 0.8f, 0.7f, 0.6f, 0.5f,
                0.4f, 0.3f, 0.2f, 0.1f, 0.0f
            };
            float *alpha_ind = alpha_arr;
            float *alpha_end = alpha_arr + sizeof (alpha_arr) / sizeof (alpha_arr[0]);
            
            while (alpha_ind != alpha_end) {
                clr_disp_buf(COLOR_GRAY);
                demo_alpha(80.0f, 60.0f, *alpha_ind);
                sysDelay(DELAY_INTER_FRAME);
                
                alpha_ind ++;
            }
        }
    }
    while (0);
    
    bltClose();
    
    return 0;
}
