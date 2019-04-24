#ifndef _PORT_H_
#define _PORT_H_

#if 0
#define boolean int
#define int8_t   char
#define uint8_t  unsigned char
#define int16_t  short
#define uint16_t unsigned short
#define int32_t  int
#define uint32_t unsigned int
#define int64_t  __int64
#define uint64_t unsigned __int64
#else
#include <stdint.h>
#endif
#define bool int

typedef unsigned char	Uint8;
typedef unsigned int	Uint32;
typedef unsigned short	Uint16;

#define CACHE_LINE  32
#define ptr_t uint32_t
#define		CACHE_BIT31 0x80000000

#define	NULL	0
#define mdelay	sysDelay


#define	M_DEBUG	sysprintf
#define printk sysprintf

#endif

