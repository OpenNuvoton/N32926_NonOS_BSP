/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef _DRVSPU_H_
#define _DRVSPU_H_

#include "wbio.h"
#include "wblib.h"
#include "wbtypes.h"

//#define OPT_FPGA_DEBUG
#define OPT_DIRECT_SET_DFA
/*---------------------------------------------------------------------------------------------------------*/
/* Includes of system headers                                                                              */
/*---------------------------------------------------------------------------------------------------------*/

//Error Code Definition
//E_DRVSPU_WRONG_CHANNEL			Wrong channel selection
//E_DRVSPU_WRONG_INTERRUPT			Wrong channel source
//#define E_DRVSPU_WRONG_CHANNEL     _SYSINFRA_ERRCODE(TRUE, MODULE_ID_DRVSPU, 0) /* Wrong channel */
//#define E_DRVSPU_WRONG_INTERRUPT   _SYSINFRA_ERRCODE(TRUE, MODULE_ID_DRVSPU, 1) /* Wrong interrupt setting */

#define E_DRVSPU_WRONG_CHANNEL     -1	/* Wrong channel */
#define E_DRVSPU_WRONG_INTERRUPT   -2 	/* Wrong interrupt setting */

#define DRVSPU_USER_INT			EV_USR_FG		// User event interrupt enable bit
#define DRVSPU_SILENT_INT		EV_SLN_FG		// Silent event interrupt enable bit
#define DRVSPU_LOOPSTART_INT	EV_LP_FG		// Loop start event interrupt enable bit
#define DRVSPU_END_INT			EV_END_FG		// End event interrupt enable bit
#define DRVSPU_ENDADDRESS_INT	END_FG			// End Address event interrupt enable bit
#define DRVSPU_THADDRESS_INT	TH_FG			// Threshold Address event interrupt enable bit
#define DRVSPU_ALL_INT			EV_USR_FG+EV_SLN_FG+EV_LP_FG+EV_END_FG+END_FG+TH_FG	


#define DRVSPU_LOAD_SELECTED_CHANNEL	0x01	// load selected channel
#define DRVSPU_UPDATE_ALL_SETTINGS		0x02	// update all registers settings in selected channel
#define DRVSPU_UPDATE_PARTIAL_SETTINGS	0x03	// update partial registers settings in selected channel
#define DRVSPU_UPDATE_IRQ_PARTIAL		0x80	// update Interrupt partial
#define DRVSPU_UPDATE_DFA_PARTIAL		0x40	// update DFA partial
#define DRVSPU_UPDATE_PAN_PARTIAL		0x20	// update PAN partial
#define DRVSPU_UPDATE_VOL_PARTIAL		0x10	// update Volume partial
#define DRVSPU_UPDATE_ALL_PARTIALS		0xF0	// update ALL partials

#define DRVSPU_MDPCM_FORMAT				0x00	// source format is MDPCM
#define DRVSPU_LP8_FORMAT				0x01	// source format is LP8
#define DRVSPU_PCM16_FORMAT				0x03	// source format is PCM16
#define DRVSPU_TONE_FORMAT				0x04	// source format is Tone
#define DRVSPU_STEREO_PCM16_LEFT		0x06	// stereo PCM16 left channel [15:0]
#define DRVSPU_STEREO_PCM16_RIGHT		0x07	// stereo PCM16 left channel [31:16]


typedef int (PFN_DRVSPU_CB_FUNC)(UINT8 *);
typedef int (*PFN_DRVSPU_INT_CALLBACK)(UINT8*, UINT32);


typedef enum 
{
	eDRVSPU_MDPCM = 0,
	eDRVSPU_LP8,
	eDRVSPU_PCM16 = 3,
	eDRVSPU_TONE_FORMAT = 4,
	eDRVSPU_MONO_PCM16 = 5,
	eDRVSPU_STEREO_PCM16_LEFT = 6,
	eDRVSPU_STEREO_PCM16_RIGHT = 7	
	
} E_DRVSPU_FORMAT;

typedef enum 
{
	eDRVSPU_USER_INT = 0, 
	eDRVSPU_SILENT_INT,
	eDRVSPU_LOOPSTART_INT,
	eDRVSPU_END_INT, 
	eDRVSPU_ENDADDRESS_INT,
	eDRVSPU_THADDRESS_INT	
				
} E_DRVSPU_INT;

typedef enum 
{
	eDRVSPU_EQBAND_DC = 0, 
	eDRVSPU_EQBAND_1, 
	eDRVSPU_EQBAND_2, 
	eDRVSPU_EQBAND_3, 
	eDRVSPU_EQBAND_4, 
	eDRVSPU_EQBAND_5, 
	eDRVSPU_EQBAND_6, 
	eDRVSPU_EQBAND_7, 
	eDRVSPU_EQBAND_8, 
	eDRVSPU_EQBAND_9,
	eDRVSPU_EQBAND_10	
	
} E_DRVSPU_EQ_BAND;
				

typedef enum 
{
	eDRVSPU_CHANNEL_0 = 0, 
	eDRVSPU_CHANNEL_1, 
	eDRVSPU_CHANNEL_2, 
	eDRVSPU_CHANNEL_3, 
	eDRVSPU_CHANNEL_4, 
	eDRVSPU_CHANNEL_5, 
	eDRVSPU_CHANNEL_6, 
	eDRVSPU_CHANNEL_7, 
	eDRVSPU_CHANNEL_8, 
	eDRVSPU_CHANNEL_9,
	eDRVSPU_CHANNEL_10, 
	eDRVSPU_CHANNEL_11, 
	eDRVSPU_CHANNEL_12, 
	eDRVSPU_CHANNEL_13, 
	eDRVSPU_CHANNEL_14, 
	eDRVSPU_CHANNEL_15, 
	eDRVSPU_CHANNEL_16, 
	eDRVSPU_CHANNEL_17, 
	eDRVSPU_CHANNEL_18, 
	eDRVSPU_CHANNEL_19, 
	eDRVSPU_CHANNEL_20, 
	eDRVSPU_CHANNEL_21, 
	eDRVSPU_CHANNEL_22, 
	eDRVSPU_CHANNEL_23, 
	eDRVSPU_CHANNEL_24, 
	eDRVSPU_CHANNEL_25, 
	eDRVSPU_CHANNEL_26, 
	eDRVSPU_CHANNEL_27, 
	eDRVSPU_CHANNEL_28, 
	eDRVSPU_CHANNEL_29, 
	eDRVSPU_CHANNEL_30, 
	eDRVSPU_CHANNEL_31	
	
} E_DRVSPU_CHANNEL;
			
typedef enum 
{
	eDRVSPU_EQGAIN_M7DB = 0, 
	eDRVSPU_EQGAIN_M6DB,
	eDRVSPU_EQGAIN_M5DB,
	eDRVSPU_EQGAIN_M4DB,
	eDRVSPU_EQGAIN_M3DB,
	eDRVSPU_EQGAIN_M2DB,
	eDRVSPU_EQGAIN_M1DB,
	eDRVSPU_EQGAIN_M0DB,
	eDRVSPU_EQGAIN_P1DB,
	eDRVSPU_EQGAIN_P2DB,
	eDRVSPU_EQGAIN_P3DB,
	eDRVSPU_EQGAIN_P4DB,
	eDRVSPU_EQGAIN_P5DB,
	eDRVSPU_EQGAIN_P6DB,
	eDRVSPU_EQGAIN_P7DB,
	eDRVSPU_EQGAIN_P8DB	
	
} E_DRVSPU_EQ_GAIN;

typedef enum 
{
	eDRVSPU_FREQ_48000 = 48000, 
	eDRVSPU_FREQ_44100 = 44100, 
	eDRVSPU_FREQ_32000 = 32000, 
	eDRVSPU_FREQ_24000 = 24000, 
	eDRVSPU_FREQ_22050 = 22050, 
	eDRVSPU_FREQ_16000 = 16000, 
	eDRVSPU_FREQ_12000 = 12000, 
	eDRVSPU_FREQ_11025 = 11025, 
	eDRVSPU_FREQ_8000  = 8000	

} E_DRVSPU_SAMPLING;

typedef struct 
{
	UINT32 u32ChannelIndex;
	UINT8  u8ChannelVolume;
	UINT16  u16PAN;
	UINT8  u8DataFormat;				
	UINT16 u16DFA;				
//	UINT8  u8SubIndex;				
//	UINT8  u8EventIndex;								
	UINT32 u32SrcBaseAddr;
	UINT32 u32SrcThresholdAddr;				
	UINT32 u32SrcEndAddr;								
//	UINT32 u32CurrentAddr;				
//	UINT32 u32LoopStartAddr;								
//	UINT32 u32LoopPlayByteCnt;												
	UINT16 u16SrcSampleRate;												
	UINT16 u16OutputSampleRate;																
} S_CHANNEL_CTRL;

typedef struct 
{
	UINT16 u16PositivePulse;
	UINT16 u16NegativePulse;	
	UINT16 u16PositiveAmp;
	UINT16 u16NegativeAmp;	
	
} S_TONE_CTRL;

void DrvSPU_IntHandler(void);

ERRCODE
DrvSPU_InstallCallBack(
	E_DRVSPU_CHANNEL eChannel,
	UINT32 eIntSource,
	PFN_DRVSPU_INT_CALLBACK	pfnCallback,
	PFN_DRVSPU_INT_CALLBACK *pfnOldCallback
);

ERRCODE
DrvSPU_Open(void);
void DrvSPU_Close(void);

void DrvSPU_ISR(void);
ERRCODE
DrvSPU_ChannelOpen(
	E_DRVSPU_CHANNEL eChannel
);

ERRCODE	
DrvSPU_ChannelClose(
	E_DRVSPU_CHANNEL eChannel
);

BOOL
DrvSPU_IsChannelEnabled(
	E_DRVSPU_CHANNEL eChannel
);	
	
ERRCODE	
DrvSPU_EnableInt(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 eInt,
	PFN_DRVSPU_CB_FUNC* pfnCallBack		
);

BOOL
DrvSPU_IsIntEnabled(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 eInt 
);

ERRCODE	
DrvSPU_DisableInt(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 eInt
);

ERRCODE	
DrvSPU_ClearInt(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 eInt 
);

ERRCODE
DrvSPU_PollInt(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 eInt 
);

ERRCODE 
DrvSPU_SetBaseAddress(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32Address
);

ERRCODE 
DrvSPU_SetThresholdAddress(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32Address
);

ERRCODE 
DrvSPU_SetEndAddress(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32Address
);

ERRCODE 
DrvSPU_SetPauseAddress(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32Address
);

ERRCODE 
DrvSPU_GetBaseAddress(
	E_DRVSPU_CHANNEL eChannel,
	UINT32* pu32Address	
);

ERRCODE 
DrvSPU_GetThresholdAddress(
	E_DRVSPU_CHANNEL eChannel,
	UINT32* pu32Address		
);

ERRCODE
DrvSPU_GetCurrentAddress(
	E_DRVSPU_CHANNEL eChannel,
	UINT32* pu32Address	
);

ERRCODE
DrvSPU_GetLoopStartAddress(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32* pu32Address	
);

ERRCODE 
DrvSPU_GetEndAddress(
	E_DRVSPU_CHANNEL eChannel,
	UINT32* pu32Address		
);	

ERRCODE
DrvSPU_GetPauseAddress(
	E_DRVSPU_CHANNEL eChannel,
	UINT32* pu32Address
);

ERRCODE
DrvSPU_GetUserEventIndex(
	E_DRVSPU_CHANNEL eChannel,
	UINT8* pu8EventIndex,
	UINT8* pu8SubIndex
);

//	ERRCODE DrvSPU_SetDFA(UINT32 u32Channel, UINT16 u16DFA);

#ifdef OPT_DIRECT_SET_DFA
	ERRCODE 
	DrvSPU_SetDFA(
		E_DRVSPU_CHANNEL eChannel, 
		UINT16 u16DFA
	);
#else
	ERRCODE 
	DrvSPU_SetDFA(
		E_DRVSPU_CHANNEL eChannel, 
		UINT16 u16SrcSampleRate, 
		UINT16 u16OutputSampleRate
	);	
#endif	

ERRCODE 
DrvSPU_GetDFA(
	E_DRVSPU_CHANNEL eChannel,
	UINT16* pu16DFA			
);

ERRCODE 
DrvSPU_SetPAN(
	E_DRVSPU_CHANNEL eChannel, 
	UINT16 u16PAN				// MSB 8-bit = right channel; LSB 8-bit = left channel
);	

ERRCODE 
DrvSPU_GetPAN(
	E_DRVSPU_CHANNEL eChannel,
	UINT16* pu16PAN		
);

ERRCODE 
DrvSPU_SetSrcType(
	E_DRVSPU_CHANNEL eChannel, 
	E_DRVSPU_FORMAT eDataFormat
);

ERRCODE	
DrvSPU_GetSrcType(
	E_DRVSPU_CHANNEL eChannel,
	UINT16* pu16SrcType	
);

ERRCODE	
DrvSPU_SetChannelVolume(
	E_DRVSPU_CHANNEL eChannel, 
	UINT8 u8Volume
);

ERRCODE
DrvSPU_GetChannelVolume(
	E_DRVSPU_CHANNEL eChannel,
	UINT8* pu8Volume
);

ERRCODE 
DrvSPU_SetChannelTone(
	E_DRVSPU_CHANNEL eChannel, 
	S_TONE_CTRL* psToneCtrl
);

ERRCODE
DrvSPU_GetChannelTone(
	E_DRVSPU_CHANNEL eChannel,
	S_TONE_CTRL* psToneCtrl
);

void DrvSPU_EqOpen(
	E_DRVSPU_EQ_BAND eEQBAND, 
	E_DRVSPU_EQ_GAIN eEQGAIN
);

void DrvSPU_EqClose(void);

void DrvSPU_SetVolume(
	UINT16 u16Volume
);

void DrvSPU_GetVolume(UINT16* pu16Volume);

void DrvSPU_StartPlay(void);
void DrvSPU_StopPlay(void);

BOOL
DrvSPU_IsSPUPlaying(void);

UINT32 	
DrvSPU_SetSampleRate(
	E_DRVSPU_SAMPLING eSampleRate
);	

ERRCODE	
DrvSPU_UploadChannelContents(
	E_DRVSPU_CHANNEL eChannel
);

ERRCODE 
DrvSPU_ChannelCtrl(
	S_CHANNEL_CTRL *psChannelCtrl
);	

ERRCODE	
DrvSPU_ChannelPause(
	E_DRVSPU_CHANNEL eChannel
);

ERRCODE	
DrvSPU_ChannelResume(
	E_DRVSPU_CHANNEL eChannel
);	

ERRCODE 
DrvSPU_SetToneAmp(
	E_DRVSPU_CHANNEL eChannel,
	UINT32 u32Amp
);

ERRCODE 
DrvSPU_SetTonePulse(
	E_DRVSPU_CHANNEL eChannel, 
	UINT32 u32Pulse
);

UINT8
DrvSPU_ReadDACReg(
	UINT8 DACRegIndex
);

VOID DrvSPU_WriteDACReg(
	UINT8 DACRegIndex, 
	UINT8 DACRegData
);

//	UINT32 DrvSPU_GetSampleRate (void);

#endif	/*_DRVNAND_H_*/














