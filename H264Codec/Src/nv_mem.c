#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "memory.h"

#include "wblib.h"

#define	BUF_LEAKAGE_CHECK 0

void* nv_malloc(int size, int alignment)
{
#if BUF_LEAKAGE_CHECK
	void *ptr;
	ptr = (void *)h264_malloc(size, alignment);
	sysprintf("malloc at 0x%x, size = 0x%x(%d), alignment at %d\n",ptr, size,size,alignment);	
	return ptr;
#else	
	//if (alignment > 256)
	//	alignment=256;
    return (void *)h264_malloc(size, alignment);
#endif    
}

int nv_free(void* ptr)
{
#if BUF_LEAKAGE_CHECK
	sysprintf("free at 0x%x\n",ptr);	
	h264_free(ptr);	
#else
	h264_free(ptr);
#endif	
	
	return 0;      
}



