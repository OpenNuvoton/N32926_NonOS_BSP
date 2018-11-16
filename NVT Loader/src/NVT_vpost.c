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
#include "w55fa92_vpost.h"
#include "wblib.h"

#include "nvtloader.h"
static BOOL bIsInitVpost=FALSE;
LCDFORMATEX lcdInfo;
void initVPostShowLogo(BOARD_S* ps_board)
{
#ifdef __HAVE_VPOST__
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
#endif 	
}