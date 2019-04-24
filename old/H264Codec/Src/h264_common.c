#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wblib.h"

extern volatile int trans_done_flag, bus_error_flag, slice_end_flag, frame_end_flag, MV_overRang_flag, BSM_empty_flag, Enc_FrameDone_flag;

#define TIME_OUT_CHECK_TICK		100

void clearAllINTflag(void)
{
	trans_done_flag = 0;
	bus_error_flag = 0;
	slice_end_flag = 0;
	frame_end_flag = 0;
	MV_overRang_flag = 0;
	BSM_empty_flag =0;
}

// return 1 if timeout for tick
unsigned int TimeOutCheck(unsigned int tick)
{
#if 1
	if ((sysGetTicks(TIMER0) - tick) > TIME_OUT_CHECK_TICK)
		return 1;
	else
		return 0;	
#else	
	return 0;	
#endif		
}





