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
	A pull up resister 20 K cascade with 
	S6 36K	
	S5 25K
	S4 22K
	S3 15K
	S2 12K
	S1 5.1K
*/
typedef struct tagKeyMap{
	UINT32 u32AinKey;
	UINT32 u32MapKey;
}S_KEYMAP;
 

#if 1/*B version DEV */
S_KEYMAP sKeymap[] = {0x616, (1<<4), 		//S6     	[U](Board no3/no4 ==> 0x612/0x61B)
					0x6F5, (1<<5), 		//S5		[D]
					0x9C0, (1<<3), 		//S4		[L]
					0xB41, (1<<2), 		//S3		[R]
					0xC33, (1<<1), 		//S2		[Home]
					0xCD8, (1<<0), 		//S1		[Enter]
					
					0x410, ((1<<4) | (1<<5)), 	//[U]+[D]
					0x4E1, ((1<<4) | (1<<3)), 	//[U]+[L]
					0x53A, ((1<<4) | (1<<2)), 	//[U]+[R]			
					0x56C, ((1<<4) | (1<<1)),	//[U]+[Home]				 
					0x58C, ((1<<4) | (1<<0)), 	//[U]+[Enter]
				
					};
UINT32 keymap(UINT16 u16AinCode)
{
	UINT16 u16AinMax, u16AinMin;
	UINT32 u32idx;
	UINT32 u32Item = sizeof(sKeymap)/sizeof(sKeymap[0]);
	for(u32idx=0; u32idx<u32Item ; u32idx=u32idx+1){
		if(u16AinCode < (sKeymap[0].u32AinKey + sKeymap[u32Item-1].u32AinKey)/2){//compound key
			u16AinMax = sKeymap[u32idx].u32AinKey+0xF;
			u16AinMin = sKeymap[u32idx].u32AinKey-0xF;
		}else{
			u16AinMax = sKeymap[u32idx].u32AinKey+0x50;
			u16AinMin = sKeymap[u32idx].u32AinKey-0x50;		
		}			
		if( (u16AinCode>u16AinMin) && (u16AinCode<u16AinMax) )
			return sKeymap[u32idx].u32MapKey;
	}	
	return 0;
}					
	

#else 
/* A version DEV */
S_KEYMAP sKeymap[] = {0x616, (1<<0), 		//S6     	[B] (Board no3/no4 ==> 0x612/0x61B)
					0x6F5, (1<<1), 		//S5		[A]
					0x9C0, (1<<2), 		//S4		[R]
					0xB41, (1<<3), 		//S3		[L]
					0xC33, (1<<4), 		//S2		[U]
					0xCD8, (1<<5), 		//S1		[D]
					
					0x410, ((1<<0) | (1<<1)), 	//S6+S5
					0x4E1, ((1<<0) | (1<<2)), 	//S6+S4
					0x53A, ((1<<0) | (1<<3)), 	//S6+S3				
					0x56C, ((1<<0) | (1<<4)),	//S6+S2				 
					0x58C, ((1<<0) | (1<<5)), 	//S6+S1
				
					};
UINT32 keymap(UINT16 u16AinCode)
{
	UINT16 u16AinMax, u16AinMin;
	UINT32 u32idx;
	UINT32 u32Item = sizeof(sKeymap)/sizeof(sKeymap[0]);
	for(u32idx=0; u32idx<u32Item ; u32idx=u32idx+1){
		if(u16AinCode < (sKeymap[0].u32AinKey + sKeymap[u32Item-1].u32AinKey)/2){//compound key
			u16AinMax = sKeymap[u32idx].u32AinKey+0xF;
			u16AinMin = sKeymap[u32idx].u32AinKey-0xF;
		}else{
			u16AinMax = sKeymap[u32idx].u32AinKey+0x50;
			u16AinMin = sKeymap[u32idx].u32AinKey-0x50;		
		}			
		if( (u16AinCode>u16AinMin) && (u16AinCode<u16AinMax) )
			return sKeymap[u32idx].u32MapKey;
	}	
	return 0;
}						
	
#endif	
