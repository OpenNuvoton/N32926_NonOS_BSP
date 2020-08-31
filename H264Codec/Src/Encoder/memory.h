/**************************************************************************//**
 * @file     memory.h
 * @version  V3.00
 * @brief    N3292x series H.264 driver header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#ifndef _MEMORY_H_
#define _MEMORY_H_
#include "port.h"

void *h264_malloc(unsigned int size,int alignment);
void h264_free(void *mem_ptr);

#endif
