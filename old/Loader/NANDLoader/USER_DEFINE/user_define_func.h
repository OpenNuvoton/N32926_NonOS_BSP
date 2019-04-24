/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2013 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/

#ifndef _W55FA92_NANDLOADER_USER_DEFINE_FUNC_H
#define _W55FA92_NANDLOADER_USER_DEFINE_FUNC_H

/*-----------------------------------------------------------------------------------*/
/* This include file should keep empty if user define nothing for user_define_func() */
/*-----------------------------------------------------------------------------------*/

#ifdef __USER_DEFINE_FUNC

#include <string.h>
#include "wblib.h"
#include "turbowriter.h"
#include "w55fa92_vpost.h"

typedef struct {
    void (*backlight_init)(void);
    void (*backlight_enable)(void);
    void (*backlight_disable)(void);
    void (*lcmpower_init)(void);
    void (*lcmpower_enable)(void);
    void (*lcmpower_disable)(void);
    void (*spkpower_init)(void);
    void (*spkpower_enable)(void);
    void (*spkpower_disable)(void);
    void (*earphone_init)(void);
    BOOL (*earphone_detect)(void);
    void (*mute_init)(void);
    void (*mute_enable)(void);
    void (*mute_disable)(void);
} BOARD_S;

// for Nuvoton FA92 Demo Board panel 320x240
#define PANEL_WIDTH     320
#define PANEL_HEIGHT    240

// got Logo image from memory address FB_ADDR
#define FB_ADDR         0x00500000

#endif  // end of #ifdef __USER_DEFINE_FUNC

#endif  // end of _W55FA92_NANDLOADER_USER_DEFINE_FUNC_H
