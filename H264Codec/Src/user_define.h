#ifndef _USER_DEFINE_H_
#define _USER_DEFINE_H_

// Constant value, don't change
/*
#define OUTPUT_FMT_CbYCrY	0
#define OUTPUT_FMT_RGB555	1
#define OUTPUT_FMT_RGB888	2
#define OUTPUT_FMT_RGB565	3
#define OUTPUT_FMT_YUV420	4
#define OUTPUT_FMT_YUV422	5
*/


#define MAX_REF_FRAME_NUM 32
#define MAX_DISP_FRAME_NUM 16
#define MAX_SPS_NUM 32
#define MAX_PPS_NUM 256	

#ifndef _DISABLE_REORDER_
#define DISPLAY_REORDER_CTRL		// Normal Release to define
#endif

#define	MAX_DEFAULT_WIDTH	1280
#define MAX_DEFAULT_HEIGHT	720

#endif
