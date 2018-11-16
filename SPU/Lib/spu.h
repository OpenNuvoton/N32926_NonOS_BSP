/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.  *
 *                                                              *
 ****************************************************************/
 
#ifndef	__SPU_H__
#define __SPU_H__

#include "wbio.h"
#include "wblib.h"
#include "wbtypes.h"
#include "w55fa92_spu.h"



//==================================================================================
//Constant Definition
//==================================================================================
#define	MIXED_PCM_BUFFER_NUMBER	4  				// m_u16MixedPcmBufferSize = m_u16SpuBufferSize*MIXED_PCM_BUFFER_NUMBER

#define MIXER_BUSY			0x0001
#define	MIXER_STEREO		0x0002
#define MIXER_SPU_RAMP_ZERO	0x0008

#define	MIXER_CH_BUSY	0x0001
#define	MIXER_CH_STEREO	0x0002
#define	MIXER_CH_DECAY	0x0004
#define MIXER_CH_PAUSE	0x0008

#define MIXER_CH_FILL_RAW_BUF_AT_CRITICAL	0x0010	// 1-indicate has filled raw buffer when raw buffer ptr is rawbufsize-1

#define S_SPU_PARCON_MODE			BIT16
#define S_SPU_PARCON_STEREO			0xFFFFFFFE
#define S_SPU_PARCON_MONO			0x00000001

//=================================================================================
//struct declare
//=================================================================================
typedef union tagFF16_16
{
	UINT32 m_u32Value;
	struct
	{
		UINT16 m_u16Fraction;
		UINT16 m_u16Integer;
	} m_sFF16;
} U_FF16_16;

// ioctls
#define SPU_IOCTL_SET_BASE_ADDRESS 	0
#define SPU_IOCTL_SET_TH1_ADDRESS	1
#define SPU_IOCTL_SET_TH2_ADDRESS	2
#define SPU_IOCTL_SET_VOLUME		3
#define SPU_IOCTL_SET_MONO			4
#define SPU_IOCTL_GET_FRAG_SIZE		5
#define SPU_IOCTL_SET_STEREO		6

#define FRAG_SIZE		(32 * 1024)
#define HALF_FRAG_SIZE	(FRAG_SIZE/2)
extern UINT8	*_pucPlayAudioBuff;

VOID spuDacOn(UINT8 level);
VOID spuSetDacSlaveMode(void);
VOID spuDacOff(void);
VOID spuStartPlay(PFN_DRVSPU_CB_FUNC *fnCallBack, UINT8 *data);
VOID spuStopPlay(void);
VOID spuIoctl(UINT32 cmd, UINT32 arg0, UINT32 arg1);
VOID spuOpen(UINT32 u32SampleRate);
VOID spuClose (void);
VOID spuEqOpen (E_DRVSPU_EQ_BAND eEqBand, E_DRVSPU_EQ_GAIN eEqGain);
VOID spuEqClose (void);

#endif
