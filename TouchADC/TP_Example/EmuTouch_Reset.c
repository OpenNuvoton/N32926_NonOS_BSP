#include <stdio.h>
#include "wblib.h"
#include "demo.h"
#include "w55fa92_ts_adc.h"

UINT32 u32RegArray[]={
  0x0000E000, 0x00000404, 0x00000000, 0x00000000, 
  0x00000000, 0x00000000
  }; 

INT32 EmuTouch_Reset(void)
{	
	UINT32 i;
	outp32(REG_APBCLK, inp32(REG_APBCLK) | TOUCH_CKE);
	outp32(REG_CLKDIV5, (inp32(REG_CLKDIV5) & ~(TOUCH_N1 | TOUCH_S| TOUCH_N0)) );		/* Fed to ADC clock need 12MHz=External clock */
	/* IP Reset */
	outp32(REG_APBIPRST, inp32(REG_APBIPRST) | TOUCHRST);
	outp32(REG_APBIPRST, inp32(REG_APBIPRST)  & ~TOUCHRST);
	for(i=0;i<=sizeof(u32RegArray)/sizeof(u32RegArray[0]); i=i+1){
		if(inp32(TP_BA+i*4) != u32RegArray[i]){
			sysprintf("Wrong register after reset\n");
			while(1);
		}	
	}
	sysprintf("Register reset pass\n");
	return Successful;
}	
