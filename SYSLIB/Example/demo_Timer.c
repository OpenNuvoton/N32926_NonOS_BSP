/***************************************************************************
 *                                                                         									     *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              					     *
 *                                                                         									     *
 ***************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "wblib.h"
#include "demo.h"


volatile unsigned int count=0, test1_flag=0, test3_flag=0;
volatile unsigned int IsWdtTimeOut=FALSE, IsClearWdt=TRUE, IsWdtResetFlagSet=FALSE;


void WatchDog_ISR()
{
    	sysClearWatchDogTimerInterruptStatus();
	IsWdtTimeOut = TRUE;
	if (IsClearWdt == TRUE){	
		sysClearWatchDogTimerCount();
		IsClearWdt = FALSE; //2nd interrupt doesn't clear WDT, the WDT will reset system if not clear WTR bit.
		sysprintf("The first WDT int clear WTR\n");
	}else
		sysprintf("The second WDT int does not clear WTR\n");			
	if (inpw(REG_WTCR) & 0x04)
	{
		IsWdtResetFlagSet = TRUE; /* check WDT reset flag bit */
		sysprintf("WDT reset flag set\n");
	} 
}

void Test()
{
	DBG_PRINTF("test -> %d\n", ++count);
}

void Test1()
{
	DBG_PRINTF("Hello World!\n");
}

void Test2()
{
	DBG_PRINTF("timer 1 test\n");
}


void Test3()
{
	DBG_PRINTF("Hello Timer1!\n");
	if (test3_flag == 0)
	{
		sysClearTimerEvent(TIMER0, 2);
		test3_flag = 1;
	}
}


void DemoAPI_Timer0(void)
{
	//	unsigned int volatile i;
	volatile unsigned int btime, etime, tmp, tsec;
	volatile UINT32 u32TimeOut = 0;
	UINT32 u32ExtFreq;
	
	u32ExtFreq = sysGetExternalClock();
	DBG_PRINTF("Timer 0 Test...\n");	
	sysSetTimerReferenceClock(TIMER0, u32ExtFreq); //External Crystal
	
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);			/* 100 ticks/per sec ==> 1tick/10ms */

	tmp = sysSetTimerEvent(TIMER0, 100, (PVOID)Test);	/* 100 ticks = 1s call back */
	DBG_PRINTF("No. of Event [%d]\n", tmp);

	tmp = sysSetTimerEvent(TIMER0, 300, (PVOID)Test1);	/* 300 ticks =3s call back */
	DBG_PRINTF("No. of Event [%d]\n", tmp);	
	sysSetLocalInterrupt(ENABLE_IRQ);

	btime = sysGetTicks(TIMER0);
	tsec = 0;
	tmp = btime;
	while (1)
	{			
		etime = sysGetTicks(TIMER0);
		if ((etime - btime) >= 100)
		{
			DBG_PRINTF("tick = %d\n", ++tsec);			
			btime = etime;
			u32TimeOut = u32TimeOut +1;
			if(u32TimeOut==10)			/* 10s Time out  */
				break;			
		}
	}
	DBG_PRINTF("Finish timer 0 testing...\n");
	sysStopTimer(TIMER0);
}
/*
typedef struct datetime_t
{
	UINT32	year;
	UINT32	mon;
	UINT32	day;
	UINT32	hour;
	UINT32	min;
	UINT32	sec;
} DateTime_T;
*/
void DemoAPI_Timer0_SetLocalTime(void)
{
	//	unsigned int volatile i;
	volatile unsigned int btime, etime, tmp, tsec;
	volatile UINT32 u32TimeOut = 0;
	UINT32 u32ExtFreq;
	DateTime_T t_daytime;
		
	u32ExtFreq = sysGetExternalClock();
	DBG_PRINTF("Timer 0 Test...\n");	
	sysSetTimerReferenceClock(TIMER0, u32ExtFreq); //External Crystal
	
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);			/* 100 ticks/per sec ==> 1tick/10ms */

	tmp = sysSetTimerEvent(TIMER0, 100, (PVOID)Test);	/* 100 ticks = 1s call back */
	DBG_PRINTF("No. of Event [%d]\n", tmp);

	tmp = sysSetTimerEvent(TIMER0, 300, (PVOID)Test1);	/* 300 ticks =3s call back */
	DBG_PRINTF("No. of Event [%d]\n", tmp);	
	sysSetLocalInterrupt(ENABLE_IRQ);
	
	t_daytime.year = 2012;
	t_daytime.mon = 12;
	t_daytime.day = 31;
	t_daytime.hour = 23;
	t_daytime.min = 59;
	t_daytime.sec = 40;
	sysSetLocalTime(t_daytime);
	

	btime = sysGetTicks(TIMER0);
	tsec = 0;
	tmp = btime;
	while (1)
	{			
		etime = sysGetTicks(TIMER0);
		if ((etime - btime) >= 100)
		{
			DBG_PRINTF("tick = %d\n", ++tsec);			
			btime = etime;
			u32TimeOut = u32TimeOut +1;
			if(u32TimeOut==10)			/* 10s Time out  */
				break;			
		}
	}
	DBG_PRINTF("Finish timer 0 testing...\n");
	
	btime = sysGetTicks(TIMER0);
	tsec = 0;
	tmp = btime;
	while(1)
	{
		etime = sysGetTicks(TIMER0);
		if ((etime - btime) >= 100)
		{
			sysGetCurrentTime(&t_daytime);
			btime = etime;
			DBG_PRINTF("Year = %d \n", t_daytime.year);
			DBG_PRINTF("mon = %d \n", t_daytime.mon);
			DBG_PRINTF("day = %d \n", t_daytime.day);
			DBG_PRINTF("hour = %d \n", t_daytime.hour);
			DBG_PRINTF("min = %d \n", t_daytime.min);
			DBG_PRINTF("sec = %d \n", t_daytime.sec);
			DBG_PRINTF("\n");
			if( t_daytime.sec == 00)
				break;
		}	
	}
	sysStopTimer(TIMER0);
}

void DemoAPI_Timer1(void)
{
	volatile unsigned int btime, etime, tmp, tsec;
	volatile UINT32 u32TimeOut = 0;
	UINT32 u32ExtFreq;
	
	u32ExtFreq = sysGetExternalClock();
	DBG_PRINTF("Timer 1 Test...\n");
	
	sysSetTimerReferenceClock(TIMER1, u32ExtFreq); 		//External Crystal
	sysStartTimer(TIMER1, 100, PERIODIC_MODE);			/* 100 ticks/per sec ==> 1tick/10ms */
	tmp = sysSetTimerEvent(TIMER1, 100, (PVOID)Test);	/* 100 ticks = 1s call back */
	DBG_PRINTF("No. of Event [%d]\n", tmp);
	tmp = sysSetTimerEvent(TIMER1, 300, (PVOID)Test1);	/* 300 ticks/per sec */
	DBG_PRINTF("No. of Event [%d]\n", tmp);
	sysSetLocalInterrupt(ENABLE_IRQ);
	btime = sysGetTicks(TIMER1);
	tsec = 0;
	tmp = btime;
	while (1)
	{
		etime = sysGetTicks(TIMER1);	
		if ((etime - btime) >= 100)
		{
			DBG_PRINTF("tick = %d\n", ++tsec);
			btime = etime;
			u32TimeOut = u32TimeOut +1;
			if(u32TimeOut==10)			/* 10s Time out  */
				break;	
		}
		
	}
	DBG_PRINTF("Finish timer 1 testing...\n");

	//	for (i=0; i<0x5000; i++);

	sysStopTimer(TIMER1);
}

void DemoAPI_Timer2(void)
{
	volatile unsigned int btime, etime, tmp, tsec;
	volatile UINT32 u32TimeOut = 0;
	UINT32 u32ExtFreq;
	
	u32ExtFreq = sysGetExternalClock();
	DBG_PRINTF("Timer 2 Test...\n");
	
	sysSetTimerReferenceClock(TIMER2, u32ExtFreq); 		//External Crystal
	//sysStartTimer(TIMER2, 100, TOGGLE_MODE);			/* 100 ticks/per sec ==> 1tick/10ms */
	sysStartTimer(TIMER2, 100, PERIODIC_MODE);
	tmp = sysSetTimerEvent(TIMER2, 100, (PVOID)Test);	/* 100 ticks = 1s call back */
	DBG_PRINTF("No. of Event [%d]\n", tmp);
	tmp = sysSetTimerEvent(TIMER2, 300, (PVOID)Test1);	/* 300 ticks/per sec */
	DBG_PRINTF("No. of Event [%d]\n", tmp);
	sysSetLocalInterrupt(ENABLE_IRQ);
	btime = sysGetTicks(TIMER2);
	tsec = 0;
	tmp = btime;
	while (1)
	{
		etime = sysGetTicks(TIMER2);	
		if ((etime - btime) >= 100)
		{
			DBG_PRINTF("tick = %d\n", ++tsec);
			btime = etime;
			u32TimeOut = u32TimeOut +1;
			if(u32TimeOut==10)			/* 10s Time out  */
				break;	
		}
		
	}
	DBG_PRINTF("Finish timer 2 testing...\n");

	//	for (i=0; i<0x5000; i++);

	sysStopTimer(TIMER2);
}


void DemoAPI_Timer3(void)
{
	volatile unsigned int btime, etime, tmp, tsec;
	volatile UINT32 u32TimeOut = 0;
	UINT32 u32ExtFreq;
	
	u32ExtFreq = sysGetExternalClock();
	DBG_PRINTF("Timer 3 Test...\n");
	
	sysSetTimerReferenceClock(TIMER3, u32ExtFreq); 		//External Crystal
	//sysStartTimer(TIMER3, 100, UNINTERRUPT_MODE);			/* 100 ticks/per sec ==> 1tick/10ms */
	sysStartTimer(TIMER3, 100, PERIODIC_MODE);
	tmp = sysSetTimerEvent(TIMER3, 100, (PVOID)Test);	/* 100 ticks = 1s call back */
	DBG_PRINTF("No. of Event [%d]\n", tmp);
	tmp = sysSetTimerEvent(TIMER3, 300, (PVOID)Test1);	/* 300 ticks/per sec */
	DBG_PRINTF("No. of Event [%d]\n", tmp);
	sysSetLocalInterrupt(ENABLE_IRQ);
	btime = sysGetTicks(TIMER3);
	tsec = 0;
	tmp = btime;
	while (1)
	{
		etime = sysGetTicks(TIMER3);	
		if ((etime - btime) >= 100)
		{
			DBG_PRINTF("tick = %d\n", ++tsec);
			btime = etime;
			u32TimeOut = u32TimeOut +1;
			if(u32TimeOut==10)			/* 10s Time out  */
				break;	
		}
		
	}
	DBG_PRINTF("Finish timer 3 testing...\n");

	//	for (i=0; i<0x5000; i++);
	sysStopTimer(TIMER3);
}

void DemoAPI_WDT(void)
{
	unsigned int btime, etime;
	UINT32 u32ExtFreq;
	
	u32ExtFreq = sysGetExternalClock();
	DBG_PRINTF("Watchdog Timer Test...\n");
#if 1 
	/* start timer 1 */
	sysSetTimerReferenceClock(TIMER0, u32ExtFreq);
	sysStartTimer(TIMER0, 100, PERIODIC_MODE); /* SW Modify sysStartTimer(TIMER0, 0, PERIODIC_MODE);*/
#endif
	/* set up watch dog timer */	
	sysprintf("The demo will generate twice WDT interrupt\n");
	sysprintf("The first WDT interrupt should not reset system by clear WTR bit\n");
	//sysSetWatchDogTimerInterval(WDT_INTERVAL_1);  /* The reset time is about 0.371 base on 12MHz*/ 
	//sysSetWatchDogTimerInterval(WDT_INTERVAL_2);  /* The reset time is about 5.614s base on 12MHz*/
	sysStartTimer(WDTIMER, NULL, NULL); 			/* Enable WDT timer */
	sysSetWatchDogTimerInterval(WDT_INTERVAL_3);  /* The reset time is about 22.391s base on 12MHz*/ 
	sysInstallWatchDogTimerISR(IRQ_LEVEL_1, (PVOID)WatchDog_ISR);
	sysEnableWatchDogTimer();
	sysEnableWatchDogTimerReset();
	
	IsWdtTimeOut = FALSE;
	btime = sysGetTicks(TIMER0);
	sysprintf("Pass time\n");	
	while (IsWdtResetFlagSet == FALSE)
	{
		//if (IsWdtTimeOut == TRUE)
		{
			IsWdtTimeOut = FALSE;
			etime =sysGetTicks(TIMER0);
			//if (((etime - btime)%10) == 0)
				//sysClearWatchDogTimerInterruptStatus	
		}			
	}
	sysStopTimer(WDTIMER);
	DBG_PRINTF("Finish watch dog timer testing...\n");
}
