#include <stdlib.h>
//#include <stdio.h>
#include "memory.h"
#include "sequence.h"

#include "userdef.h"
#include "wblib.h"

//#define CACHE_BIT31 	0x80000000

#define ET_SIZE 300      //!< size of error text buffer
char errortext[ET_SIZE]; //!< buffer for error message for exit with error()

#define	MALLOC_DBG	0

#define	EXTRA_ALLOC	2 //66//36

void *h264_malloc(unsigned int size,int alignment)
{
	uint8_t *mem_ptr;
	uint8_t *tmp;

	if (!alignment) {
		if ((mem_ptr = (uint8_t *) malloc(size + EXTRA_ALLOC)) != NULL) {		
			*mem_ptr = 0;
			*(mem_ptr+1) = 0;			
			return (void *) (mem_ptr+2);
		}
	} else {
		size += EXTRA_ALLOC;	// KC
		if ((tmp = (uint8_t *) malloc(size + alignment)) != NULL) {
			mem_ptr = (uint8_t *) ((ptr_t) (tmp + alignment - 2) & (~(ptr_t) (alignment - 1)));			
			if (mem_ptr == tmp)
				mem_ptr += alignment;
					
			*(mem_ptr - 1) = (uint8_t) ((mem_ptr - tmp) & 0xFF);
			if (mem_ptr - tmp > 255)
				*(mem_ptr - 2) = (uint8_t) ((mem_ptr - tmp) >> 8);	
			else	
				*(mem_ptr - 2) = 0;
#if MALLOC_DBG		
		    Console_Printf("Mem Allocate 0x%x, size=0x%x, alignment to 0x%x, diff=0x%x\n", tmp,size,mem_ptr ,mem_ptr - tmp );
#endif				

			return (void *) (mem_ptr);
					
		}
	}
	return NULL;

}

void h264_free(void *mem_ptr)
{
		
  if (mem_ptr)
  {
    int offset;
    

    offset = (UINT16)(*((UINT8 *)mem_ptr - 2)) <<8 | *((UINT8 *)mem_ptr-1);

#if MALLOC_DBG
	sysprintf("free 0x%x\n", (UINT8 *)mem_ptr - offset);
#endif	
	free((uint8_t *) mem_ptr - offset);	
	
  }    
}

/*!
 ************************************************************************
 * \brief
 *    Exit program if memory allocation failed (using error())
 * \param where
 *    string indicating which memory allocation failed
 ************************************************************************
 */
/*
void h264_no_mem_exit(char *where)
{
   snprintf(errortext, ET_SIZE, "Could not allocate memory: %s",where);
   error (errortext, 100);
}
*/
