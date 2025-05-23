/**************************************************************************//**
 * @file     sensor_tvp5150_reg.h
 * @brief    TVP5150 header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
/*
 * tvp5150 - Texas Instruments TVP5150A/AM1 video decoder registers
 *
 * Copyright (c) 2005,2006 Mauro Carvalho Chehab (mchehab@infradead.org)
 * This code is placed under the terms of the GNU General Public License v2
 */

//typedef __u32 tvp_std_id;
typedef UINT32 tvp_std_id;

/* one bit for each */
#define TVP_STD_PAL_B          ((tvp_std_id)0x00000001)
#define TVP_STD_PAL_B1         ((tvp_std_id)0x00000002)
#define TVP_STD_PAL_G          ((tvp_std_id)0x00000004)
#define TVP_STD_PAL_H          ((tvp_std_id)0x00000008)
#define TVP_STD_PAL_I          ((tvp_std_id)0x00000010)
#define TVP_STD_PAL_D          ((tvp_std_id)0x00000020)
#define TVP_STD_PAL_D1         ((tvp_std_id)0x00000040)
#define TVP_STD_PAL_K          ((tvp_std_id)0x00000080)

#define TVP_STD_PAL_M          ((tvp_std_id)0x00000100)
#define TVP_STD_PAL_N          ((tvp_std_id)0x00000200)
#define TVP_STD_PAL_Nc         ((tvp_std_id)0x00000400)
#define TVP_STD_PAL_60         ((tvp_std_id)0x00000800)

#define TVP_STD_NTSC_M         ((tvp_std_id)0x00001000)
#define TVP_STD_NTSC_M_JP      ((tvp_std_id)0x00002000)
#define TVP_STD_NTSC_443       ((tvp_std_id)0x00004000)
#define TVP_STD_NTSC_M_KR      ((tvp_std_id)0x00008000)

/* some merged standards */
#define TVP_STD_MN  (TVP_STD_PAL_M|TVP_STD_PAL_N|TVP_STD_PAL_Nc|TVP_STD_NTSC)
#define TVP_STD_B   (TVP_STD_PAL_B|TVP_STD_PAL_B1)
#define TVP_STD_GH  (TVP_STD_PAL_G|TVP_STD_PAL_H)
#define TVP_STD_DK  (TVP_STD_PAL_DK)

/* some common needed stuff */
#define TVP_STD_PAL_BG      (TVP_STD_PAL_B      |\
                 TVP_STD_PAL_B1 |\
                 TVP_STD_PAL_G)
#define TVP_STD_PAL_DK      (TVP_STD_PAL_D      |\
                 TVP_STD_PAL_D1 |\
                 TVP_STD_PAL_K)
#define TVP_STD_PAL     (TVP_STD_PAL_BG |\
                 TVP_STD_PAL_DK |\
                 TVP_STD_PAL_H      |\
                 TVP_STD_PAL_I)
#define TVP_STD_NTSC           (TVP_STD_NTSC_M  |\
                 TVP_STD_NTSC_M_JP     |\
                 TVP_STD_NTSC_M_KR)

#define TVP_STD_525_60      (TVP_STD_PAL_M      |\
                 TVP_STD_PAL_60 |\
                 TVP_STD_NTSC       |\
                 TVP_STD_NTSC_443)
#define TVP_STD_625_50      (TVP_STD_PAL        |\
                 TVP_STD_PAL_N      |\
                 TVP_STD_PAL_Nc)

#define TVP_STD_UNKNOWN        0
#define TVP_STD_ALL            (TVP_STD_525_60  |\
                 TVP_STD_625_50)


#define TVP5150_H_MAX               720
#define TVP5150_V_MAX_525_60        480
#define TVP5150_V_MAX_OTHERS        576
#define TVP5150_MAX_CROP_LEFT       511
#define TVP5150_MAX_CROP_TOP        127
#define TVP5150_CROP_SHIFT          2

#define TVP5150_A1P1A           0
#define TVP5150_A1P1B           1
#define TVP5150_SVIDEO          2


//#define   TVP5150_SENSOR

#define TVP5150_VD_IN_SRC_SEL_1      0x00 /* Video input source selection #1 */
#define TVP5150_ANAL_CHL_CTL         0x01 /* Analog channel controls */
#define TVP5150_OP_MODE_CTL          0x02 /* Operation mode controls */
#define TVP5150_MISC_CTL             0x03 /* Miscellaneous controls */
#define TVP5150_AUTOSW_MSK           0x04 /* Autoswitch mask: TVP5150A / TVP5150AM */

/* Reserved 05h */

#define TVP5150_COLOR_KIL_THSH_CTL   0x06 /* Color killer threshold control */
#define TVP5150_LUMA_PROC_CTL_1      0x07 /* Luminance processing control #1 */
#define TVP5150_LUMA_PROC_CTL_2      0x08 /* Luminance processing control #2 */
#define TVP5150_BRIGHT_CTL           0x09 /* Brightness control */
#define TVP5150_SATURATION_CTL       0x0a /* Color saturation control */
#define TVP5150_HUE_CTL              0x0b /* Hue control */
#define TVP5150_CONTRAST_CTL         0x0c /* Contrast control */
#define TVP5150_DATA_RATE_SEL        0x0d /* Outputs and data rates select */
#define TVP5150_LUMA_PROC_CTL_3      0x0e /* Luminance processing control #3 */
#define TVP5150_CONF_SHARED_PIN      0x0f /* Configuration shared pins */

/* Reserved 10h */

#define TVP5150_ACT_VD_CROP_ST_MSB   0x11 /* Active video cropping start MSB */
#define TVP5150_ACT_VD_CROP_ST_LSB   0x12 /* Active video cropping start LSB */
#define TVP5150_ACT_VD_CROP_STP_MSB  0x13 /* Active video cropping stop MSB */
#define TVP5150_ACT_VD_CROP_STP_LSB  0x14 /* Active video cropping stop LSB */
#define TVP5150_GENLOCK              0x15 /* Genlock/RTC */
#define TVP5150_HORIZ_SYNC_START     0x16 /* Horizontal sync start */

/* Reserved 17h */

#define TVP5150_VERT_BLANKING_START 0x18 /* Vertical blanking start */
#define TVP5150_VERT_BLANKING_STOP  0x19 /* Vertical blanking stop */
#define TVP5150_CHROMA_PROC_CTL_1   0x1a /* Chrominance processing control #1 */
#define TVP5150_CHROMA_PROC_CTL_2   0x1b /* Chrominance processing control #2 */
#define TVP5150_INT_RESET_REG_B     0x1c /* Interrupt reset register B */
#define TVP5150_INT_ENABLE_REG_B    0x1d /* Interrupt enable register B */
#define TVP5150_INTT_CONFIG_REG_B   0x1e /* Interrupt configuration register B */

/* Reserved 1Fh-27h */

#define TVP5150_VIDEO_STD           0x28 /* Video standard */

/* Reserved 29h-2bh */

#define TVP5150_CB_GAIN_FACT        0x2c /* Cb gain factor */
#define TVP5150_CR_GAIN_FACTOR      0x2d /* Cr gain factor */
#define TVP5150_MACROVISION_ON_CTR  0x2e /* Macrovision on counter */
#define TVP5150_MACROVISION_OFF_CTR 0x2f /* Macrovision off counter */
#define TVP5150_REV_SELECT          0x30 /* revision select (TVP5150AM1 only) */

/* Reserved 31h-7Fh */

#define TVP5150_MSB_DEV_ID          0x80 /* MSB of device ID */
#define TVP5150_LSB_DEV_ID          0x81 /* LSB of device ID */
#define TVP5150_ROM_MAJOR_VER       0x82 /* ROM major version */
#define TVP5150_ROM_MINOR_VER       0x83 /* ROM minor version */
#define TVP5150_VERT_LN_COUNT_MSB   0x84 /* Vertical line count MSB */
#define TVP5150_VERT_LN_COUNT_LSB   0x85 /* Vertical line count LSB */
#define TVP5150_INT_STATUS_REG_B    0x86 /* Interrupt status register B */
#define TVP5150_INT_ACTIVE_REG_B    0x87 /* Interrupt active register B */
#define TVP5150_STATUS_REG_1        0x88 /* Status register #1 */
#define TVP5150_STATUS_REG_2        0x89 /* Status register #2 */
#define TVP5150_STATUS_REG_3        0x8a /* Status register #3 */
#define TVP5150_STATUS_REG_4        0x8b /* Status register #4 */
#define TVP5150_STATUS_REG_5        0x8c /* Status register #5 */
/* Reserved 8Dh-8Fh */
/* Closed caption data registers */
#define TVP5150_CC_DATA_INI         0x90
#define TVP5150_CC_DATA_END         0x93

/* WSS data registers */
#define TVP5150_WSS_DATA_INI        0x94
#define TVP5150_WSS_DATA_END        0x99

/* VPS data registers */
#define TVP5150_VPS_DATA_INI        0x9a
#define TVP5150_VPS_DATA_END        0xa6

/* VITC data registers */
#define TVP5150_VITC_DATA_INI       0xa7
#define TVP5150_VITC_DATA_END       0xaf

#define TVP5150_VBI_FIFO_READ_DATA  0xb0 /* VBI FIFO read data */

/* Teletext filter 1 */
#define TVP5150_TELETEXT_FIL1_INI  0xb1
#define TVP5150_TELETEXT_FIL1_END  0xb5

/* Teletext filter 2 */
#define TVP5150_TELETEXT_FIL2_INI  0xb6
#define TVP5150_TELETEXT_FIL2_END  0xba

#define TVP5150_TELETEXT_FIL_ENA    0xbb /* Teletext filter enable */
/* Reserved BCh-BFh */
#define TVP5150_INT_STATUS_REG_A    0xc0 /* Interrupt status register A */
#define TVP5150_INT_ENABLE_REG_A    0xc1 /* Interrupt enable register A */
#define TVP5150_INT_CONF            0xc2 /* Interrupt configuration */
#define TVP5150_VDP_CONF_RAM_DATA   0xc3 /* VDP configuration RAM data */
#define TVP5150_CONF_RAM_ADDR_LOW   0xc4 /* Configuration RAM address low byte */
#define TVP5150_CONF_RAM_ADDR_HIGH  0xc5 /* Configuration RAM address high byte */
#define TVP5150_VDP_STATUS_REG      0xc6 /* VDP status register */
#define TVP5150_FIFO_WORD_COUNT     0xc7 /* FIFO word count */
#define TVP5150_FIFO_INT_THRESHOLD  0xc8 /* FIFO interrupt threshold */
#define TVP5150_FIFO_RESET          0xc9 /* FIFO reset */
#define TVP5150_LINE_NUMBER_INT     0xca /* Line number interrupt */
#define TVP5150_PIX_ALIGN_REG_LOW   0xcb /* Pixel alignment register low byte */
#define TVP5150_PIX_ALIGN_REG_HIGH  0xcc /* Pixel alignment register high byte */
#define TVP5150_FIFO_OUT_CTRL       0xcd /* FIFO output control */
/* Reserved CEh */
#define TVP5150_FULL_FIELD_ENA      0xcf /* Full field enable 1 */

/* Line mode registers */
#define TVP5150_LINE_MODE_INI       0xd0
#define TVP5150_LINE_MODE_END       0xfb

#define TVP5150_FULL_FIELD_MODE_REG 0xfc /* Full field mode register */
/* Reserved FDh-FFh */
