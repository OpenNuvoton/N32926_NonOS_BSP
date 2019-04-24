/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.  *
 *                                                              *
 ****************************************************************/

#include <stdio.h>
#include <string.h>
#include "w55fa92_reg.h"
#include "wblib.h"
#include "w55fa92_ts_adc.h"

/*
#define AIN_KEY_SW1	0x559
#define AIN_KEY_SW2	0x6DE
#define AIN_KEY_SW3	0x99E
#define AIN_KEY_SW4	0xB15
#define AIN_KEY_SW5	0xC01
#define AIN_KEY_SW6	0xCA3
#define AIN_KEY_SW7	0xD18
#define AIN_KEY_SW8	0xD72
#define (AIN_KEY_SW1+AIN_KEY_SW2) 	0x3B3
#define (AIN_KEY_SW1+AIN_KEY_SW3) 	0x460
#define (AIN_KEY_SW1+AIN_KEY_SW4) 	0x4A6
#define (AIN_KEY_SW1+AIN_KEY_SW5) 	0x4D0
#define (AIN_KEY_SW1+AIN_KEY_SW6) 	0x4E9
#define (AIN_KEY_SW1+AIN_KEY_SW7) 	0x4FA
#define (AIN_KEY_SW1+AIN_KEY_SW8) 	0x507
*/
typedef struct tagKeyMap{
	UINT32 u32AinKey;
	UINT32 u32MapKey;
}S_KEYMAP;


S_KEYMAP sKeymap[] = {0x602, (1<<0), 
					0x6DE, (1<<1), 
					0x99E, (1<<2), 
					0xB15, (1<<3), 
					0xC01, (1<<4), 
					0xCA3, (1<<5), 
					0xD18, (1<<6), 
					0xD72, (1<<7), 
					
					0x400, ((1<<0) | (1<<1)), 
					0x4CE, ((1<<0) | (1<<2)), 
					0x525, ((1<<0) | (1<<3)), 
					0x556, ((1<<0) | (1<<4)), 
					0x576, ((1<<0) | (1<<5)), 
					0x58D, ((1<<0) | (1<<6)), 
					0x59C, ((1<<0) | (1<<7))
					};
UINT32 keymap(UINT16 u16AinCode)
{
	UINT16 u16AinMax, u16AinMin;
	UINT32 u32idx;
	for(u32idx=0; u32idx<sizeof(sKeymap)/sizeof(sKeymap[0]) ; u32idx=u32idx+1){
		u16AinMax = sKeymap[u32idx].u32AinKey+6;
		u16AinMin = sKeymap[u32idx].u32AinKey-6;
		if( (u16AinCode>u16AinMin) && (u16AinCode<u16AinMax) )
			return sKeymap[u32idx].u32MapKey;
	}	
	return 0;
}					
