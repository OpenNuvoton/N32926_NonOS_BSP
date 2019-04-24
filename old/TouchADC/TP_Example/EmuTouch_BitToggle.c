
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wbtypes.h"
#include "wbio.h"
#include "wblib.h"
#include "W55fa92_reg.h"
#include "stdio.h"


typedef struct tgaRegInfo
{
	UINT RegIndexOffset;
	UINT RegMask;
}REG_INFO;

static REG_INFO capRegInfo[]=
{       //addr ,  mask             //mask = 0xff means the reg is read only
        //VPE control register
        
        {0x0000,  0xFF00017F}, //1 ignore. 0==>test
        {0x0004,	~0x000000FF},
        {0x0008, 0xFFFFFF8F},
        {0x000C, 0xFFFFFFFF},
         
        {0x0010,  0xFFFFFFFF}, 
        {0x0014,	0xFFFFFFFF},
       
        
        {0xFFFF,    0xFFFFFFFF},
};

INT32 Emu_RegisterBitToggle(void)
{
    	UINT32 cur_test_idx=0;
  	UINT32 reg_addroffset=0;
  	UINT32 reg_mask=0;
  	UINT32 reg_read_data=0;	
  	UINT32 i=0;
	UINT32 Fail_flag=0x0;
	
	sysSetLocalInterrupt(DISABLE_FIQ_IRQ);		
	reg_addroffset = capRegInfo[cur_test_idx].RegIndexOffset;
  	while (reg_addroffset!= 0xffff)
  	{
	    	i=0x01;			
		reg_mask = capRegInfo[cur_test_idx].RegMask;    		
		if( reg_mask != 0xFFFFFFFF)
		{		
			reg_read_data= inpw(TP_BA+reg_addroffset);		//W687_VPE_BA[reg_addroffset];			  				    		    		    						  		  						    		    			
			while( i != 0x0 )
			{
                
				if(((~reg_mask)&i) !=0x00)
				{//The bit need test
					UINT rwdata, cmpdata;
					UINT post_data=reg_read_data & (~i);
					//W687_VPE_BA[reg_addroffset]=post_data;	//Write 0
					outpw(TP_BA+reg_addroffset, post_data);
				    	//if(W687_VPE_BA[reg_addroffset]!= post_data)
				    	
				    	
				    	rwdata = (inpw(TP_BA+reg_addroffset)&(~reg_mask));
				    	cmpdata = post_data&(~reg_mask);
				    	if(rwdata  != cmpdata)
				    	{
				    		sysprintf("Fail Addr 0x%x\n", (TP_BA+reg_addroffset) );
				    		sysprintf("%3x\n", rwdata );
				    		sysprintf("%3x\n", cmpdata);
				    		sysprintf("Bit %d\n", i);
				        	Fail_flag=1;	
					    	return -1;
				    	}
				    	else
				    	{					//Write 1
					    	post_data=reg_read_data | i;
                       				//W687_VPE_BA[reg_addroffset]=post_data;	
                       				outpw(TP_BA+reg_addroffset, post_data);
                       				rwdata = (inpw(TP_BA+reg_addroffset)&(~reg_mask));
					    	cmpdata = post_data&(~reg_mask);
				                if( rwdata!=cmpdata  )
                        			{
                        				sysprintf("Fail Addr 0x%x\n", (TP_BA+reg_addroffset) );
                        				sysprintf("%3x\n", rwdata );
					    		sysprintf("%3x\n", cmpdata);
					    		sysprintf("Bit %d\n", i);
                        				Fail_flag=1;
                            				return -1; 
                        			}
					   	outpw(TP_BA+reg_addroffset, reg_read_data);//Restore	
				    }								
			    }			
			    i=i<<1;				    				    				    		    												
			}    			    			
		}
		sysprintf("Checked Register Index = %3x Pass\n", reg_addroffset);    		
		cur_test_idx=cur_test_idx+1;		    		    				
		reg_addroffset = capRegInfo[cur_test_idx].RegIndexOffset;			
  	}			 		  				  	
  	sysSetLocalInterrupt(ENABLE_FIQ_IRQ);	
    	return Successful;
}
