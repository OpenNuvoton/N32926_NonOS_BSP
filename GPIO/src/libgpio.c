/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.                  *
*                                                                           *
*****************************************************************************/

/****************************************************************************
* FILENAME
*   libgpio.c
*
* VERSION
*   1.0
*
* DESCRIPTION
*   GPIO library source code
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*
* HISTORY
*
* REMARK
*   None
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wblib.h"
#include "w55fa92_gpio.h"

// accetiable debounce clock
static const unsigned int _clk[16] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 2*256, 4*256, 8*256, 16*256, 32*256, 64*256, 128*256};

int gpio_open(unsigned char port)
{
	sysprintf("GPIO library replaced gpio_open() with gpio_configure()...\n");
	return(0);
}

void gpio_set_portg2digital(unsigned short num)
{
	switch (num) {
	
		case 15:
		case 14:
		case 13:
		case 12:
			outpw(REG_SHRPIN_TOUCH , inpw(REG_SHRPIN_TOUCH) &~ TP_AEN);
			break;		
		case 9:
			outpw(REG_SHRPIN_TOUCH , inpw(REG_SHRPIN_TOUCH) &~ SAR_AHS_AEN);
			break;
		case 8:
			outpw(REG_SHRPIN_AUDIO , inpw(REG_SHRPIN_AUDIO) &~ AIN2_AEN);
			break;
		case 7:
			outpw(REG_SHRPIN_AUDIO , inpw(REG_SHRPIN_AUDIO) &~ AIN3_AEN);
			break;		
		case 5:
		case 4:
		case 3:
		case 2:
			outpw(REG_SHRPIN_TVDAC , inpw(REG_SHRPIN_TVDAC) &~ SMTVDACAEN);
			break;
					
		
		default:
			;

	}
}

int gpio_configure(unsigned char port, unsigned short num)
{

	switch (port) {			
		case GPIO_PORTA:
			if(num <= 11)
			{
				if(num <= 7)
					outpw(REG_GPAFUN0 , inpw(REG_GPAFUN0) &~ (0xF << (num<<2)));
				else
					outpw(REG_GPAFUN1 , inpw(REG_GPAFUN1) &~ (0xF << ((num%8)<<2)));				
			}
			else
				outpw(REG_GPAFUN1 , (inpw(REG_GPAFUN1) &~ (0xF << ((num%8)<<2))) | (0x2 << ((num%8)<<2)));
			break;	
		case GPIO_PORTB:
			if(num <= 7)
				outpw(REG_GPBFUN0, inpw(REG_GPBFUN0) &~ (0xF << (num<<2)));
			else
				outpw(REG_GPBFUN1, inpw(REG_GPBFUN1) &~ (0xF << ((num%8)<<2)));
			break;
		case GPIO_PORTC:
			if(num <= 7)
				outpw(REG_GPCFUN0, inpw(REG_GPCFUN0) &~ (0xF << (num<<2)));
			else
				outpw(REG_GPCFUN1, inpw(REG_GPCFUN1) &~ (0xF << ((num%8)<<2)));			
			break;
		case GPIO_PORTD:
			if(num <= 7)
				outpw(REG_GPDFUN0 , inpw(REG_GPDFUN0) &~ (0xF << (num<<2)));
			else
				outpw(REG_GPDFUN1 , inpw(REG_GPDFUN1) &~ (0xF << ((num%8)<<2)));			
			break;
		case GPIO_PORTE:
			if(num>11)
				return(-1);
			if(num <= 7)
				outpw(REG_GPEFUN0 , inpw(REG_GPEFUN0) &~ (0xF << (num<<2)));
			else
				outpw(REG_GPEFUN1 , inpw(REG_GPEFUN1) &~ (0xF << ((num%8)<<2)));			
			break;	
		case GPIO_PORTG:
			if(num>=2 && num<=5)
			{
				gpio_set_portg2digital(num);
				outpw(REG_GPGFUN0 , inpw(REG_GPGFUN0) &~ (0xF << (num<<2)));			
			}
			else if(num>=7 && num<=9)
			{
				gpio_set_portg2digital(num);
				if(num == 7)
					outpw(REG_GPGFUN0 , inpw(REG_GPGFUN0) &~ (0xF << (num<<2)));
				else
					outpw(REG_GPGFUN1 , inpw(REG_GPGFUN1) &~ (0xF << ((num%8)<<2)));							
			}
			else if(num>=12 && num<=15)
			{
				gpio_set_portg2digital(num);
				outpw(REG_GPGFUN1 , inpw(REG_GPGFUN1) &~ (0xF << ((num%8)<<2)));			
			}
			else
				return(-1);
			break;
		case GPIO_PORTH:
			if(num<=7)				
				outpw(REG_GPHFUN , inpw(REG_GPHFUN) &~ (0x3 << (num<<1)));			
			else
				return(-1);
			break;
			
		default:
			return(-1);

	}
	return(0);

}

int gpio_readport(unsigned char port, unsigned short *val)
{
	switch (port) {
	
		case GPIO_PORTA:
			*val = (inpw(REG_GPIOA_PIN) & 0xffff);
			break;
		case GPIO_PORTB:
			*val = (inpw(REG_GPIOB_PIN) & 0xffff);
			break;			
		case GPIO_PORTC:
			*val = (inpw(REG_GPIOC_PIN) & 0xffff);
			break;		
		case GPIO_PORTD:
			*val = (inpw(REG_GPIOD_PIN) & 0xffff);
			break;	
		case GPIO_PORTE:
			*val = (inpw(REG_GPIOE_PIN) & 0x0fff);
			break;	
		case GPIO_PORTG:
			*val = (inpw(REG_GPIOG_PIN) & 0xffff);
			break;
		case GPIO_PORTH:
			*val = (inpw(REG_GPIOH_PIN) & 0x21);
			break;
		default:
			return(-1);

	}
	return(0);
}

int gpio_setportdir(unsigned char port, unsigned short mask, unsigned short dir)
{
	switch (port) {

		case GPIO_PORTA:
			outpw(REG_GPIOA_OMD , inpw(REG_GPIOA_OMD) & ~(mask & (mask ^ dir)));
			outpw(REG_GPIOA_OMD , inpw(REG_GPIOA_OMD) | (mask & dir));			
			break;
		case GPIO_PORTB:
			outpw(REG_GPIOB_OMD , inpw(REG_GPIOB_OMD) & ~(mask & (mask ^ dir)));
			outpw(REG_GPIOB_OMD , inpw(REG_GPIOB_OMD) | (mask & dir));	
			break;			
		case GPIO_PORTC:
			outpw(REG_GPIOC_OMD , inpw(REG_GPIOC_OMD) & ~(mask & (mask ^ dir)));
			outpw(REG_GPIOC_OMD , inpw(REG_GPIOC_OMD) | (mask & dir));	
			break;		
		case GPIO_PORTD:
			outpw(REG_GPIOD_OMD , inpw(REG_GPIOD_OMD) & ~(mask & (mask ^ dir)));
			outpw(REG_GPIOD_OMD , inpw(REG_GPIOD_OMD) | (mask & dir));	
			break;	
		case GPIO_PORTE:
			outpw(REG_GPIOE_OMD , inpw(REG_GPIOE_OMD) & ~(mask & (mask ^ dir)));
			outpw(REG_GPIOE_OMD , inpw(REG_GPIOE_OMD) | (mask & dir));	
			break;	
		case GPIO_PORTG:				
			outpw(REG_GPIOG_OMD , inpw(REG_GPIOG_OMD) & ~(mask & (mask ^ dir)));
			outpw(REG_GPIOG_OMD , inpw(REG_GPIOG_OMD) | (mask & dir));	
			break;
		case GPIO_PORTH:
			outpw(REG_GPIOH_OMD , inpw(REG_GPIOH_OMD) & ~(mask & (mask ^ dir)));
			outpw(REG_GPIOH_OMD , inpw(REG_GPIOH_OMD) | (mask & dir));	
			break;
		default:
			return(-1);

	}
	return(0);
	

}

int gpio_setportval(unsigned char port, unsigned short mask, unsigned short val)
{
	switch (port) {
	
		case GPIO_PORTA:
			outpw(REG_GPIOA_DOUT , inpw(REG_GPIOA_DOUT) & ~(mask & (mask ^ val)));
			outpw(REG_GPIOA_DOUT , inpw(REG_GPIOA_DOUT) | (mask & val));			
			break;
		case GPIO_PORTB:
			outpw(REG_GPIOB_DOUT , inpw(REG_GPIOB_DOUT) & ~(mask & (mask ^ val)));
			outpw(REG_GPIOB_DOUT , inpw(REG_GPIOB_DOUT) | (mask & val));
			break;			
		case GPIO_PORTC:
			outpw(REG_GPIOC_DOUT , inpw(REG_GPIOC_DOUT) & ~(mask & (mask ^ val)));
			outpw(REG_GPIOC_DOUT , inpw(REG_GPIOC_DOUT) | (mask & val));
			break;		
		case GPIO_PORTD:
			outpw(REG_GPIOD_DOUT , inpw(REG_GPIOD_DOUT) & ~(mask & (mask ^ val)));
			outpw(REG_GPIOD_DOUT , inpw(REG_GPIOD_DOUT) | (mask & val));
			break;	
		case GPIO_PORTE:
			outpw(REG_GPIOE_DOUT , inpw(REG_GPIOE_DOUT) & ~(mask & (mask ^ val)));
			outpw(REG_GPIOE_DOUT , inpw(REG_GPIOE_DOUT) | (mask & val));
			break;	
		case GPIO_PORTG:
			outpw(REG_GPIOG_DOUT , inpw(REG_GPIOG_DOUT) & ~(mask & (mask ^ val)));
			outpw(REG_GPIOG_DOUT , inpw(REG_GPIOG_DOUT) | (mask & val));
			break;	
		case GPIO_PORTH:
			outpw(REG_GPIOH_DOUT , inpw(REG_GPIOH_DOUT) & ~(mask & (mask ^ val)));
			outpw(REG_GPIOH_DOUT , inpw(REG_GPIOH_DOUT) | (mask & val));
			break;	
		default:
			return(-1);

	}
	return(0);

}

int gpio_setportpull(unsigned char port, unsigned short mask, unsigned short pull)
{
	switch (port) {
	
		case GPIO_PORTA:
			outpw(REG_GPIOA_PUEN , inpw(REG_GPIOA_PUEN) & ~(mask & (mask ^ pull)));
			outpw(REG_GPIOA_PUEN , inpw(REG_GPIOA_PUEN) | (mask & pull));			
			break;
		case GPIO_PORTB:
			outpw(REG_GPIOB_PUEN , inpw(REG_GPIOB_PUEN) & ~(mask & (mask ^ pull)));
			outpw(REG_GPIOB_PUEN , inpw(REG_GPIOB_PUEN) | (mask & pull));	
			break;			
		case GPIO_PORTC:
			outpw(REG_GPIOC_PUEN , inpw(REG_GPIOC_PUEN) & ~(mask & (mask ^ pull)));
			outpw(REG_GPIOC_PUEN , inpw(REG_GPIOC_PUEN) | (mask & pull));	
			break;	
		case GPIO_PORTD:
			outpw(REG_GPIOD_PUEN , inpw(REG_GPIOD_PUEN) & ~(mask & (mask ^ pull)));
			outpw(REG_GPIOD_PUEN , inpw(REG_GPIOD_PUEN) | (mask & pull));	
			break;
		case GPIO_PORTE:
			outpw(REG_GPIOE_PUEN , inpw(REG_GPIOE_PUEN) & ~(mask & (mask ^ pull)));
			outpw(REG_GPIOE_PUEN , inpw(REG_GPIOE_PUEN) | (mask & pull));	
			break;
		case GPIO_PORTG:
			outpw(REG_GPIOG_PUEN , inpw(REG_GPIOG_PUEN) & ~(mask & (mask ^ pull)));
			outpw(REG_GPIOG_PUEN , inpw(REG_GPIOG_PUEN) | (mask & pull));	
			break;
		case GPIO_PORTH:
			outpw(REG_GPIOH_PUEN , inpw(REG_GPIOH_PUEN) & ~(mask & (mask ^ pull)));
			outpw(REG_GPIOH_PUEN , inpw(REG_GPIOH_PUEN) | (mask & pull));	
			break;
		default:
			return(-1);

	}
	return(0);

}

int gpio_setdebounce(unsigned int clk, unsigned char src)
{
	int i;
	
	if(clk > 128*256 || src > 0xf)
		return(-1);
	
	// clk could only be 1, 2, 4, 8, ... 128*256
	for(i = 0 ; i < 16; i++) {
		if(_clk[i] == clk)
			break;
	}
	if(i == 16)
		return(-1);

	outpw(REG_DBNCECON , (i << 4 | src));
		
	return(0);				
}

void gpio_getdebounce(unsigned int *clk, unsigned char *src)
{
	*clk = _clk[(inpw(REG_DBNCECON) >> 4) & 0xf];
	*src = inpw(REG_DBNCECON) & 0xf;
	
	return;
}

int gpio_setsrcgrp(unsigned char port, unsigned short mask, unsigned char irq)
{

	const unsigned int _irq[4] = {0, 0x55555555, 0xaaaaaaaa, 0xffffffff};
	unsigned int _mask = 0;
	int i;
	
	
	if(irq > 3)
		return(-1);	
	
	if(mask > 0xffff)
		return(-1);
		
	for(i = 0; i < 16; i++) {
		if(mask & (1 << i))
			_mask += (3 << (i << 1));	
	}
		
	switch (port) {	
		case GPIO_PORTA:
			outpw(REG_IRQSRCGPA , inpw(REG_IRQSRCGPA) & ~_mask);
			outpw(REG_IRQSRCGPA , inpw(REG_IRQSRCGPA) | (_mask & _irq[irq]));			
			break;
		case GPIO_PORTB:
			outpw(REG_IRQSRCGPB , inpw(REG_IRQSRCGPB) & ~_mask);
			outpw(REG_IRQSRCGPB , inpw(REG_IRQSRCGPB) | (_mask & _irq[irq]));
			break;			
		case GPIO_PORTC:
			outpw(REG_IRQSRCGPC , inpw(REG_IRQSRCGPC) & ~_mask);
			outpw(REG_IRQSRCGPC , inpw(REG_IRQSRCGPC) | (_mask & _irq[irq]));
			break;	
		case GPIO_PORTD:
			outpw(REG_IRQSRCGPD , inpw(REG_IRQSRCGPD) & ~_mask);
			outpw(REG_IRQSRCGPD , inpw(REG_IRQSRCGPD) | (_mask & _irq[irq]));
			break;
		case GPIO_PORTE:
			outpw(REG_IRQSRCGPE , inpw(REG_IRQSRCGPE) & ~_mask);
			outpw(REG_IRQSRCGPE , inpw(REG_IRQSRCGPE) | (_mask & _irq[irq]));
			break;	
		case GPIO_PORTG:
			outpw(REG_IRQSRCGPG , inpw(REG_IRQSRCGPG) & ~_mask);
			outpw(REG_IRQSRCGPG , inpw(REG_IRQSRCGPG) | (_mask & _irq[irq]));
			break;
		case GPIO_PORTH:
			outpw(REG_IRQSRCGPH , inpw(REG_IRQSRCGPH) & ~_mask);
			outpw(REG_IRQSRCGPH , inpw(REG_IRQSRCGPH) | (_mask & _irq[irq]));
			break;
		default:
			return(-1);

	}
	return(0);
}

int gpio_getsrcgrp(unsigned char port, unsigned int *val)
{
	switch (port) {	
		case GPIO_PORTA:		
			*val = inpw(REG_IRQSRCGPA);
			break;
		case GPIO_PORTB:
			*val = inpw(REG_IRQSRCGPB);
			break;			
		case GPIO_PORTC:
			*val = inpw(REG_IRQSRCGPC);
			break;	
		case GPIO_PORTD:
			*val = inpw(REG_IRQSRCGPD);
			break;
		case GPIO_PORTE:			
			*val = inpw(REG_IRQSRCGPE);
			break;	
		case GPIO_PORTG:			
			*val = inpw(REG_IRQSRCGPG);
			break;
		case GPIO_PORTH:			
			*val = inpw(REG_IRQSRCGPH);
			break;
		default:
			return(-1);

	}
	return(0);

}

int gpio_setintmode(unsigned char port, unsigned short mask, unsigned short falling, unsigned short rising)
{
	
	if(mask > 0xffff)
		return(-1);
		
	switch (port) {	
		case GPIO_PORTA:	
			outpw(REG_IRQENGPA , inpw(REG_IRQENGPA) & ~((mask << 16) | mask));
			outpw(REG_IRQENGPA , inpw(REG_IRQENGPA) | (((mask & rising) << 16) | (mask & falling)));			
			break;
		case GPIO_PORTB:
			outpw(REG_IRQENGPB , inpw(REG_IRQENGPB) & ~((mask << 16) | mask));
			outpw(REG_IRQENGPB , inpw(REG_IRQENGPB) | (((mask & rising) << 16) | (mask & falling)));
			break;			
		case GPIO_PORTC:
			outpw(REG_IRQENGPC , inpw(REG_IRQENGPC) & ~((mask << 16) | mask));
			outpw(REG_IRQENGPC , inpw(REG_IRQENGPC) | (((mask & rising) << 16) | (mask & falling)));
			break;	
		case GPIO_PORTD:
			outpw(REG_IRQENGPD , inpw(REG_IRQENGPD) & ~((mask << 16) | mask));
			outpw(REG_IRQENGPD , inpw(REG_IRQENGPD) | (((mask & rising) << 16) | (mask & falling)));
			break;	
		case GPIO_PORTE:
			outpw(REG_IRQENGPE , inpw(REG_IRQENGPE) & ~((mask << 16) | mask));
			outpw(REG_IRQENGPE , inpw(REG_IRQENGPE) | (((mask & rising) << 16) | (mask & falling)));		
			break;	
		case GPIO_PORTG:
			outpw(REG_IRQENGPG , inpw(REG_IRQENGPG) & ~((mask << 16) | mask));
			outpw(REG_IRQENGPG , inpw(REG_IRQENGPG) | (((mask & rising) << 16) | (mask & falling)));		
			break;
		case GPIO_PORTH:
			outpw(REG_IRQENGPH , inpw(REG_IRQENGPH) & ~((mask << 16) | mask));
			outpw(REG_IRQENGPH , inpw(REG_IRQENGPH) | (((mask & rising) << 16) | (mask & falling)));		
			break;
		default:
			return(-1);

	}
	return(0);		
		
}


int gpio_getintmode(unsigned char port, unsigned short *falling, unsigned short *rising)
{

	switch (port) {	
		case GPIO_PORTA:		
			*rising = inpw(REG_IRQENGPA) >> 16;
			*falling = inpw(REG_IRQENGPA) & 0xffff;
			break;
		case GPIO_PORTB:
			*rising = inpw(REG_IRQENGPB) >> 16;
			*falling = inpw(REG_IRQENGPB) & 0xffff;
			break;			
		case GPIO_PORTC:
			*rising = inpw(REG_IRQENGPC) >> 16;
			*falling = inpw(REG_IRQENGPC) & 0xffff;
			break;	
		case GPIO_PORTD:
			*rising = inpw(REG_IRQENGPD) >> 16;
			*falling = inpw(REG_IRQENGPD) & 0xffff;
			break;
		case GPIO_PORTE:
			*rising = inpw(REG_IRQENGPE)  >> 16;
			*falling = inpw(REG_IRQENGPE) & 0xffff;		
			break;	
		case GPIO_PORTG:
			*rising = inpw(REG_IRQENGPG)  >> 16;
			*falling = inpw(REG_IRQENGPG) & 0xffff;		
			break;
		case GPIO_PORTH:
			*rising = inpw(REG_IRQENGPH)  >> 16;
			*falling = inpw(REG_IRQENGPH) & 0xffff;		
			break;
		default:
			return(-1);

	}
	return(0);
}


int gpio_setlatchtrigger(unsigned char src)
{
	if(src > 0xf)
		return(-1);

	outpw(REG_IRQLHSEL , src);		
	return(0);

}

void gpio_getlatchtrigger(unsigned char *src)
{

	*src = inpw(REG_IRQLHSEL) & 0xf;
	return;
}

int gpio_getlatchval(unsigned char port, unsigned short *val)
{

	switch (port) {	
		case GPIO_PORTA:		
			*val = inpw(REG_IRQLHGPA) & 0xffff;
			break;
		case GPIO_PORTB:
			*val = inpw(REG_IRQLHGPB) & 0xffff;
			break;			
		case GPIO_PORTC:
			*val = inpw(REG_IRQLHGPC) & 0xffff;
			break;	
		case GPIO_PORTD:
			*val = inpw(REG_IRQLHGPD) & 0xffff;
			break;
		case GPIO_PORTE:
			*val = inpw(REG_IRQLHGPE) & 0xffff;
			break;	
		case GPIO_PORTG:
			*val = inpw(REG_IRQLHGPG) & 0xffff;
			break;
		case GPIO_PORTH:
			*val = inpw(REG_IRQLHGPH) & 0xffff;
			break;
		default:
			return(-1);

	}

	return(0);
}


int gpio_gettriggersrc(unsigned char port, unsigned short *src)
{

	switch (port) {	
		case GPIO_PORTA:		
			*src = inpw(REG_IRQTGSRC0) & 0xffff;						
			break;
		case GPIO_PORTB:
			*src = (inpw(REG_IRQTGSRC0) & 0xffff0000) >> 16;						
			break;			
		case GPIO_PORTC:
			*src = inpw(REG_IRQTGSRC1) & 0xffff;						
			break;	
		case GPIO_PORTD:
			*src = (inpw(REG_IRQTGSRC1) & 0xffff0000) >> 16;						
			break;	
		case GPIO_PORTE:
			*src = inpw(REG_IRQTGSRC2) & 0xffff;						
			break;	
		case GPIO_PORTG:
			*src = (inpw(REG_IRQTGSRC2) & 0xffff0000) >> 16;						
			break;	
		case GPIO_PORTH:
			*src = inpw(REG_IRQTGSRC3) & 0xffff;						
			break;
		default:
			return(-1);

	}

	return(0);
}

int gpio_cleartriggersrc(unsigned char port)
{

	switch (port) {	
		case GPIO_PORTA:					
			outpw(REG_IRQTGSRC0 , inpw(REG_IRQTGSRC0) & 0xffff);			
			break;
		case GPIO_PORTB:			
			outpw(REG_IRQTGSRC0 , inpw(REG_IRQTGSRC0) & 0xffff0000);			
			break;			
		case GPIO_PORTC:			
			outpw(REG_IRQTGSRC1 , inpw(REG_IRQTGSRC1) & 0xffff);			
			break;	
		case GPIO_PORTD:			
			outpw(REG_IRQTGSRC1 , inpw(REG_IRQTGSRC1) & 0xffff0000);			
			break;	
		case GPIO_PORTE:			
			outpw(REG_IRQTGSRC2 , inpw(REG_IRQTGSRC2) & 0xffff);			
			break;	
		case GPIO_PORTG:			
			outpw(REG_IRQTGSRC2 , inpw(REG_IRQTGSRC2) & 0xffff0000);			
			break;
		case GPIO_PORTH:			
			outpw(REG_IRQTGSRC3 , inpw(REG_IRQTGSRC3) & 0xffff);			
			break;
		default:
			return(-1);

	}

	return(0);
}


