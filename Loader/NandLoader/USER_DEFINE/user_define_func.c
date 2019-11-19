/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2013 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
#ifdef __USER_DEFINE_FUNC

#include "user_define_func.h"

void initVPostShowLogo(BOARD_S* ps_board);
BOARD_S s_board;


/*-----------------------------------------------------------------------------------*/
/* The entry point of User Define Function that called by NandLoader.                */
/*-----------------------------------------------------------------------------------*/
void user_define_func()
{
    //--- This is a sample code for user define function.
    //--- This sample code will show Logo to panel on Nuvoton FA92 Demo Board.
    initVPostShowLogo(&s_board);
}


static BOOL bIsInitVpost=FALSE;
LCDFORMATEX lcdInfo;
void initVPostShowLogo(BOARD_S* ps_board)
{
    if(ps_board->lcmpower_init != NULL)
        ps_board->lcmpower_init();
    if(ps_board->lcmpower_enable != NULL)
        ps_board->lcmpower_enable();
    if(bIsInitVpost==FALSE)
    {
        bIsInitVpost = TRUE;
        lcdInfo.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
        lcdInfo.nScreenWidth = PANEL_WIDTH;
        lcdInfo.nScreenHeight = PANEL_HEIGHT;
        vpostLCMInit(&lcdInfo, (UINT32*)FB_ADDR);
    }
}

#else

/*-----------------------------------------------------------------------------------*/
/* The entry point of User Define Function that called by NandLoader.                */
/*-----------------------------------------------------------------------------------*/
void user_define_func()
{
    //--- Keep empty if user define nothing for User Define Function.
}

#endif  // end of #ifdef __USER_DEFINE_FUNC
