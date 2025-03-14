/**************************************************************************//**
 * @file     file_name.h
 * @brief    N3292x series sample header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#undef DBG_PRINTF
#define DBG_PRINTF      sysprintf
//#define DBG_PRINTF(...)

INT32 KeyPad(UINT32 u32Channel);
INT32 VoltageDetection(UINT32 u32Channel);
INT32 Raw_TouchPanel(void);                                 /* Report RAW data but have big error if charge/discharge */
INT32 Polling_Processed_TouchPanel(void);       /* To skip charge/discharge issue */
INT32 Integration(void);
INT32 Emu_RegisterBitToggle(void);
INT32 EmuTouch_Reset(void);
