/****************************************************************************
*                                                                           *
* Copyright (c) 2013 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "w55fa92_reg.h"
#include "w55fa92_sic.h"
#include "w55fa92_gnand.h"
#include "wblib.h"

#include "nvtloader.h"

static void backlight_init(void)
{

}
static void backlight_enable(void)
{

}
static void backlight_disable(void)
{

}
static void lcmpower_init(void)
{

}
static void lcmpower_enable(void)
{

}
static void lcmpower_disable(void)
{

}
static void spkpower_init(void)
{

}
static void spkpower_enable(void)
{

}
static void spkpower_disable(void)
{

}
	
static void earphone_init(void)
{

}
static BOOL earphone_detect(void)
{
	
	return 0;
}
static void mute_init(void)
{

}
static void mute_enable(void)
{

}
static void mute_disable(void)
{

}

BOARD_S board_info =
{
	backlight_init,			// void (*backlight_init)(void);
	backlight_enable, 		// void (*backlight_enable)(void);
	backlight_disable,		// void (*backlight_disable)(void);
	
	lcmpower_init,			// void (*lcmpower_init)(void);
	lcmpower_enable,		// void (*lcmpower_enable)(void);
	lcmpower_disable,		// void (*lcmpower_disable)(void);
						
	spkpower_init,			// void (*spkpower_init)(void);
	spkpower_enable,		// void (*spkpower_enable)(void);
	spkpower_disable,		// void (*spkpower_disable)(void);
						
	earphone_init,			// void (*earphone_init)(void);
	earphone_detect,		// BOOL (*earphone_detect)(void);
						
	mute_init,				// void (*mute_init)(void);
	mute_enable,			// void (*mute_enable)(void);
	mute_disable,			// void (*mute_disable)(void);	
};


INT32 register_board(BOARD_S* ps_board)
{
	*ps_board = board_info;
	return Successful;	
}