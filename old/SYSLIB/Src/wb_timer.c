/***************************************************************************
 *                                                                         *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              *
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
* FILENAME
*   wb_timer.c
*
* VERSION
*   1.0
*
* DESCRIPTION
*   The timer related function of Nuvoton ARM9 MCU
*
* HISTORY
*   2008-06-25  Ver 1.0 draft by Min-Nan Cheng
*
* REMARK
*   None
 **************************************************************************/
#include <stdio.h>
#include "wblib.h"

/* Global variables */
PVOID  _sys_pvOldTimer0Vect;
UINT32 _sys_uTimer0ClockRate = EXTERNAL_CRYSTAL_CLOCK;
UINT32 volatile _sys_uTimer0Count = 0;
UINT32 volatile _sys_uTime0EventCount = 0;
UINT32 volatile _sys_uTimer0TickPerSecond;
BOOL   _sys_bIsSetTime0Event = FALSE;
BOOL volatile _sys_bIsTimer0Initial = FALSE;

PVOID  _sys_pvOldTimer1Vect;
UINT32 _sys_uTimer1ClockRate = EXTERNAL_CRYSTAL_CLOCK;
UINT32 volatile _sys_uTimer1Count = 0;
UINT32 volatile _sys_uTime1EventCount = 0;
UINT32 volatile _sys_uTimer1TickPerSecond;
BOOL _sys_bIsSetTime1Event = FALSE;
BOOL volatile _sys_bIsTimer1Initial = FALSE;

PVOID  _sys_pvOldTimer2Vect;
UINT32 _sys_uTimer2ClockRate = EXTERNAL_CRYSTAL_CLOCK;
UINT32 volatile _sys_uTimer2Count = 0;
UINT32 volatile _sys_uTime2EventCount = 0;
UINT32 volatile _sys_uTimer2TickPerSecond;
BOOL _sys_bIsSetTime2Event = FALSE;
BOOL volatile _sys_bIsTimer2Initial = FALSE;

PVOID  _sys_pvOldTimer3Vect;
UINT32 _sys_uTimer3ClockRate = EXTERNAL_CRYSTAL_CLOCK;
UINT32 volatile _sys_uTimer3Count = 0;
UINT32 volatile _sys_uTime3EventCount = 0;
UINT32 volatile _sys_uTimer3TickPerSecond;
BOOL _sys_bIsSetTime3Event = FALSE;
BOOL volatile _sys_bIsTimer3Initial = FALSE;

#define SECONDS_PER_HOUR		3600
#define SECONDS_PER_DAY		86400
#define SECONDS_365_DAY		31536000
#define SECONDS_366_DAY		31622400

#define TimerEventCount		10

typedef void (*sys_pvTimeFunPtr)();   /* function pointer */
typedef struct timeEvent_t
{
	UINT32				active;
	UINT32				initTick;
	UINT32				curTick;
	sys_pvTimeFunPtr	funPtr;
} TimeEvent_T;
TimeEvent_T tTime0Event[TimerEventCount];
TimeEvent_T tTime1Event[TimerEventCount];
TimeEvent_T tTime2Event[TimerEventCount];
TimeEvent_T tTime3Event[TimerEventCount];

UINT32	volatile _sys_ReferenceTime_Clock = 0;
UINT32	volatile _sys_ReferenceTime_UTC;

static UINT32 month_seconds[] = 
{
	31 * SECONDS_PER_DAY, 
	28 * SECONDS_PER_DAY, 
	31 * SECONDS_PER_DAY, 
	30 * SECONDS_PER_DAY, 
	31 * SECONDS_PER_DAY, 
	30 * SECONDS_PER_DAY, 
	31 * SECONDS_PER_DAY, 
	31 * SECONDS_PER_DAY, 
	30 * SECONDS_PER_DAY, 
	31 * SECONDS_PER_DAY, 
	30 * SECONDS_PER_DAY, 
	31 * SECONDS_PER_DAY
};


/* Interrupt service routine */
VOID sysTimerISR()
{
	int volatile i;

	/*----- check Timer 0 -----*/
	if (inpw(REG_TISR) & 0x00000001)
	{
		_sys_uTimer0Count++;
		outpw(REG_TISR, 0x01); /* clear TIF0 */
		if (_sys_uTimer0Count >= 0xfffffff)
		  	_sys_uTimer0Count = 0;

		if (_sys_bIsSetTime0Event)
		{
			for (i=0; i<TimerEventCount; i++)
			{
				if (tTime0Event[i].active)
				{
					tTime0Event[i].curTick--;
					if (tTime0Event[i].curTick == 0)
					{
						(*tTime0Event[i].funPtr)();
						tTime0Event[i].curTick = tTime0Event[i].initTick;
					}
				}
			}
		}
	}
	/*----- check Timer 1 -----*/
	if (inpw(REG_TISR) & 0x00000002)
	{
		_sys_uTimer1Count++;
		outpw(REG_TISR, 0x02); /* clear TIF0 */
		if (_sys_uTimer1Count >= 0xfffffff)
		  	_sys_uTimer1Count = 0;

		if (_sys_bIsSetTime1Event)
		{
			for (i=0; i<TimerEventCount; i++)
			{
				if (tTime1Event[i].active)
				{
					tTime1Event[i].curTick--;
					if (tTime1Event[i].curTick == 0)
					{
						(*tTime1Event[i].funPtr)();
						tTime1Event[i].curTick = tTime1Event[i].initTick;
					}
				}
			}
		}
	}
	/*----- check Timer 2 -----*/
	if (inpw(REG_TISR2) & 0x00000001)
	{
		_sys_uTimer2Count++;
		outpw(REG_TISR2, 0x01); /* clear TIF0 */
		if (_sys_uTimer2Count >= 0xfffffff)
		  	_sys_uTimer2Count = 0;

		if (_sys_bIsSetTime2Event)
		{
			for (i=0; i<TimerEventCount; i++)
			{
				if (tTime2Event[i].active)
				{
					tTime2Event[i].curTick--;
					if (tTime2Event[i].curTick == 0)
					{
						(*tTime2Event[i].funPtr)();
						tTime2Event[i].curTick = tTime2Event[i].initTick;
					}
				}
			}
		}
	}
	/*----- check Timer 3 -----*/
	if (inpw(REG_TISR2) & 0x00000002)
	{
		_sys_uTimer3Count++;
		outpw(REG_TISR2, 0x02); /* clear TIF0 */
		if (_sys_uTimer3Count >= 0xfffffff)
		  	_sys_uTimer3Count = 0;

		if (_sys_bIsSetTime3Event)
		{
			for (i=0; i<TimerEventCount; i++)
			{
				if (tTime3Event[i].active)
				{
					tTime3Event[i].curTick--;
					if (tTime3Event[i].curTick == 0)
					{
						(*tTime3Event[i].funPtr)();
						tTime3Event[i].curTick = tTime3Event[i].initTick;
					}
				}
			}
		}
	}
}



/* Timer library functions */
UINT32 sysGetTicks(INT32 nTimeNo)
{
	switch (nTimeNo)
	{
		case TIMER0:
			return _sys_uTimer0Count;
		case TIMER1:
			return _sys_uTimer1Count;
		case TIMER2:
			return _sys_uTimer2Count;
		case TIMER3:
			return _sys_uTimer3Count;	
		default:
		   	;
	}
  	return Successful;
}

INT32 sysResetTicks(INT32 nTimeNo)
{
	switch (nTimeNo)
	{
		case TIMER0:
			_sys_uTimer0Count = 0;
			break;
		case TIMER1:
			_sys_uTimer1Count = 0;
			break;
		case TIMER2:
			_sys_uTimer2Count = 0;
			break;
		case TIMER3:
			_sys_uTimer3Count = 0;
			break;	

		default:
		   	;
	}
	return Successful;
}

INT32 sysUpdateTickCount(INT32 nTimeNo, UINT32 uCount)
{
	switch (nTimeNo)
	{
		case TIMER0:
			_sys_uTimer0Count = uCount;
			break;
		case TIMER1:
			_sys_uTimer1Count = uCount;
			break;
		case TIMER2:
			_sys_uTimer2Count = uCount;
			break;
		case TIMER3:
			_sys_uTimer3Count = uCount;
			break;	
		default:
		   	;
	}
	return Successful;
}

INT32 sysSetTimerReferenceClock(INT32 nTimeNo, UINT32 uClockRate)
{
	switch (nTimeNo)
	{
		case TIMER0:
			_sys_uTimer0ClockRate = uClockRate;
			break;
		case TIMER1:
			_sys_uTimer1ClockRate = uClockRate;
			break;
		case TIMER2:
			_sys_uTimer2ClockRate = uClockRate;
			break;
		case TIMER3:
			_sys_uTimer3ClockRate = uClockRate;
			break;	
		default:		
		   	;
	}
	return Successful;
}

INT32 sysStartTimer(INT32 nTimeNo, UINT32 uTicksPerSecond, INT32 nOpMode)
{
	int volatile i;
	UINT32 _mTicr, _mTcr;

	_mTcr = 0x60000000 | (nOpMode << 27);
	switch (nTimeNo)
	{
		case TIMER0:
			_sys_bIsTimer0Initial = TRUE;
			_sys_uTimer0TickPerSecond = uTicksPerSecond;
			outp32(REG_APBCLK, inp32(REG_APBCLK) | TMR0_CKE);
			outp32(REG_APBIPRST, inp32(REG_APBIPRST) | TMR0RST);
			outp32(REG_APBIPRST, inp32(REG_APBIPRST) & ~TMR0RST);

			for (i=0; i<TimerEventCount; i++)
				tTime0Event[i].active = FALSE;

			_sys_pvOldTimer0Vect = sysInstallISR(IRQ_LEVEL_1, 
											IRQ_TMR0, 
											(PVOID)sysTimerISR);
			sysEnableInterrupt(IRQ_TMR0);
			sysSetInterruptType(IRQ_TMR0, 1);	//eDRVAIC_HIGH_LEVEL
			
			_sys_uTimer0Count = 0;
			_mTicr = _sys_uTimer0ClockRate / uTicksPerSecond;
			outpw(REG_TICR0, _mTicr);
			outpw(REG_TCSR0, (inpw(REG_TCSR0) & 0x87FFFF00) | ((nOpMode | 0xC)<<27));  //0xC means CEN and IE were enable			
			break;
		case TIMER1:
			_sys_bIsTimer1Initial = TRUE;
			_sys_uTimer1TickPerSecond = uTicksPerSecond;
			outp32(REG_APBCLK, inp32(REG_APBCLK) | TMR1_CKE);
			outp32(REG_APBIPRST, inp32(REG_APBIPRST) | TMR1RST);
			outp32(REG_APBIPRST, inp32(REG_APBIPRST) & ~TMR1RST);

			for (i=0; i<TimerEventCount; i++)
				tTime1Event[i].active = FALSE;

			_sys_pvOldTimer1Vect = sysInstallISR(IRQ_LEVEL_1, 
											IRQ_TMR1, 
											(PVOID)sysTimerISR);
			sysEnableInterrupt(IRQ_TMR1);
			sysSetInterruptType(IRQ_TMR1, 1);	//eDRVAIC_HIGH_LEVEL
			
			_sys_uTimer1Count = 0;
			_mTicr = _sys_uTimer1ClockRate / uTicksPerSecond;
			outpw(REG_TICR1, _mTicr);
			outpw(REG_TCSR1, (inpw(REG_TCSR1) & 0x87FFFF00) | ((nOpMode | 0xC)<<27));  //0xC means CEN and IE were enable			
			break;
		case TIMER2:
			_sys_bIsTimer2Initial = TRUE;
			_sys_uTimer2TickPerSecond = uTicksPerSecond;
			outp32(REG_APBCLK, inp32(REG_APBCLK) | TMR2_CKE);
			outp32(REG_APBIPRST, inp32(REG_APBIPRST) | TMR2RST);
			outp32(REG_APBIPRST, inp32(REG_APBIPRST) & ~TMR2RST);

			for (i=0; i<TimerEventCount; i++)
				tTime2Event[i].active = FALSE;

			_sys_pvOldTimer1Vect = sysInstallISR(IRQ_LEVEL_1, 
											IRQ_TMR2, 
											(PVOID)sysTimerISR);
			sysEnableInterrupt(IRQ_TMR2);
			sysSetInterruptType(IRQ_TMR2, 1);	//eDRVAIC_HIGH_LEVEL
			
			_sys_uTimer2Count = 0;
			_mTicr = _sys_uTimer2ClockRate / uTicksPerSecond;
			outpw(REG_TICR2, _mTicr);
			outpw(REG_TCSR2, (inpw(REG_TCSR2) & 0x87FFFF00) | ((nOpMode | 0xC)<<27)); //0xC means CEN and IE were enable			
			break;	
		case TIMER3:
			_sys_bIsTimer3Initial = TRUE;
			_sys_uTimer3TickPerSecond = uTicksPerSecond;
			outp32(REG_APBCLK, inp32(REG_APBCLK) | TMR3_CKE);
			outp32(REG_APBIPRST, inp32(REG_APBIPRST) | TMR3RST);
			outp32(REG_APBIPRST, inp32(REG_APBIPRST) & ~TMR3RST);

			for (i=0; i<TimerEventCount; i++)
				tTime3Event[i].active = FALSE;

			_sys_pvOldTimer3Vect = sysInstallISR(IRQ_LEVEL_1, 
											IRQ_TMR3, 
											(PVOID)sysTimerISR);
			sysEnableInterrupt(IRQ_TMR3);
			sysSetInterruptType(IRQ_TMR3, 1);	//eDRVAIC_HIGH_LEVEL
			
			_sys_uTimer1Count = 0;
			_mTicr = _sys_uTimer3ClockRate / uTicksPerSecond;
			outpw(REG_TICR3, _mTicr);
			outpw(REG_TCSR3, (inpw(REG_TCSR3) & 0x87FFFF00) | ((nOpMode | 0xC)<<27));  //0xC means CEN and IE were enable			
			break;
		case WDTIMER:
			outp32(REG_APBCLK, inp32(REG_APBCLK) | WDCLK_CKE);	
			if( (inpw(REG_WTCR) & WTRF) == WTRF)
				sysprintf("It is warm reset\n");
			else
				sysprintf("It is cold reset\n");	
			break;		
		default:
		    ;
	}
	sysSetLocalInterrupt(ENABLE_IRQ);
	return Successful;
}

INT32 sysStopTimer(INT32 nTimeNo)
{
	switch (nTimeNo)
	{
		case TIMER0:
			_sys_bIsTimer0Initial = FALSE;
			sysInstallISR(IRQ_LEVEL_1, IRQ_TMR0, _sys_pvOldTimer0Vect);
			sysDisableInterrupt(IRQ_TMR0);

			outpw(REG_TICR0, 0);           /* disable timer */
			outpw(REG_TISR, TIF0);           /* clear for safty */

			_sys_uTime0EventCount = 0;
			_sys_bIsSetTime0Event = FALSE;
			outp32(REG_APBCLK, inp32(REG_APBCLK) & ~TMR0_CKE);
			break;
		case TIMER1:
			_sys_bIsTimer1Initial = FALSE;
			sysInstallISR(IRQ_LEVEL_1, IRQ_TMR1, _sys_pvOldTimer1Vect);
			sysDisableInterrupt(IRQ_TMR1);

			outpw(REG_TICR1, 0);           /* disable timer */
			outpw(REG_TISR, TIF1);           /* clear for safty */

			_sys_uTime1EventCount = 0;
			_sys_bIsSetTime1Event = FALSE;
			outp32(REG_APBCLK, inp32(REG_APBCLK) & ~TMR1_CKE);
			break;
		case TIMER2:
			_sys_bIsTimer2Initial = FALSE;
			sysInstallISR(IRQ_LEVEL_1, IRQ_TMR2, _sys_pvOldTimer2Vect);
			sysDisableInterrupt(IRQ_TMR2);

			outpw(REG_TICR2, 0);           /* disable timer */
			outpw(REG_TISR2, TIF0);       /* clear for safty */

			_sys_uTime2EventCount = 0;
			_sys_bIsSetTime2Event = FALSE;
			outp32(REG_APBCLK, inp32(REG_APBCLK) & ~TMR2_CKE);
			break;	
		case TIMER3:
			_sys_bIsTimer3Initial = FALSE;
			sysInstallISR(IRQ_LEVEL_1, IRQ_TMR3, _sys_pvOldTimer3Vect);
			sysDisableInterrupt(IRQ_TMR3);

			outpw(REG_TICR3, 0);           /* disable timer */
			outpw(REG_TISR2, TIF1);       /* clear for safty */

			_sys_uTime3EventCount = 0;
			_sys_bIsSetTime3Event = FALSE;
			outp32(REG_APBCLK, inp32(REG_APBCLK) & ~TMR3_CKE);
			break;
		case WDTIMER:
			outp32(REG_APBCLK, inp32(REG_APBCLK) & ~WDCLK_CKE);	
			
			break;			
		default:
			;
	}
	//   WB_SetLocalInterrupt(DISABLE_IRQ);
	return Successful;
}

VOID sysClearWatchDogTimerCount()
{
	UINT32 volatile _mWtcr;

	_mWtcr = inpw(REG_WTCR) | WTR;  /* write WTR */
	outpw(REG_WTCR, _mWtcr);
}

VOID sysClearWatchDogTimerInterruptStatus()
{
	UINT32 volatile _mWtcr;

	_mWtcr = inpw(REG_WTCR) | WTIF;
	outpw(REG_WTCR, _mWtcr);
}

VOID sysDisableWatchDogTimer()
{
	UINT32 volatile _mWtcr;

	_mWtcr = inpw(REG_WTCR) & ~WTE;
	outpw(REG_WTCR, _mWtcr);
}

VOID sysDisableWatchDogTimerReset()
{
	UINT32 volatile _mWtcr;

	_mWtcr = inpw(REG_WTCR) & ~WTRE;
	outpw(REG_WTCR, _mWtcr);
}
VOID sysEnableWatchDogTimer()
{
	UINT32 volatile _mWtcr;
	_mWtcr = inpw(REG_WTCR) | WTE;
	outpw(REG_WTCR, _mWtcr);
}

VOID sysEnableWatchDogTimerReset()
{
	UINT32 volatile _mWtcr;
	_mWtcr = inpw(REG_WTCR) | WTRE;
	outpw(REG_WTCR, _mWtcr);
}


PVOID sysInstallWatchDogTimerISR(INT32 nIntTypeLevel, PVOID pvNewISR)
{
	PVOID _mOldVect;
	UINT32 volatile _mWtcr;
	
	_mWtcr = inpw(REG_WTCR);
	_mWtcr |= 0x40;
	outpw(REG_WTCR, _mWtcr);
	_mOldVect = sysInstallISR(nIntTypeLevel, IRQ_WDT, pvNewISR);
	sysEnableInterrupt(IRQ_WDT);
	sysSetLocalInterrupt(ENABLE_IRQ);

	return _mOldVect;
}

INT32 sysSetWatchDogTimerInterval(INT32 nWdtInterval)
{
	UINT32 volatile _mWtcr;
	_mWtcr = inpw(REG_WTCR) & (~0x30); /* SW Fixed */
	_mWtcr = _mWtcr|(0x400)|(nWdtInterval << 4);
	outpw(REG_WTCR, _mWtcr);

	return Successful;
}


INT32 sysSetTimerEvent(INT32 nTimeNo, UINT32 uTimeTick, PVOID pvFun)
{
	int volatile i;
	int val=0;

	switch (nTimeNo)
	{
		case TIMER0:
			_sys_bIsSetTime0Event = TRUE;
			_sys_uTime0EventCount++;
			for (i=0; i<TimerEventCount; i++)
			{
				if (tTime0Event[i].active == FALSE)
				{
					tTime0Event[i].active = TRUE;
					tTime0Event[i].initTick = uTimeTick;
					tTime0Event[i].curTick = uTimeTick;
					tTime0Event[i].funPtr = (sys_pvTimeFunPtr)pvFun;
					val = i+1;
					break;
				}
			}
			/*SW ADD*/
			if(i==TimerEventCount)
				return -1;		/*-1 means invalid channel*/			
			/**/	
			break;
		case TIMER1:
			_sys_bIsSetTime1Event = TRUE;
			_sys_uTime1EventCount++;
			for (i=0; i<TimerEventCount; i++)
			{
				if (tTime1Event[i].active == FALSE)
				{
					tTime1Event[i].active = TRUE;
					tTime1Event[i].initTick = uTimeTick;
					tTime1Event[i].curTick = uTimeTick;
					tTime1Event[i].funPtr = (sys_pvTimeFunPtr)pvFun;
					val = i+1;
					break;
				}
			}
			/*SW ADD*/
			if(i==TimerEventCount)
				return -1;		/*-1 means invalid channel*/			
			/**/	
			break;	
		case TIMER2:
			_sys_bIsSetTime2Event = TRUE;
			_sys_uTime2EventCount++;
			for (i=0; i<TimerEventCount; i++)
			{
				if (tTime2Event[i].active == FALSE)
				{
					tTime2Event[i].active = TRUE;
					tTime2Event[i].initTick = uTimeTick;
					tTime2Event[i].curTick = uTimeTick;
					tTime2Event[i].funPtr = (sys_pvTimeFunPtr)pvFun;
					val = i+1;
					break;
				}
			}
			/*SW ADD*/
			if(i==TimerEventCount)
				return -1;		/*-1 means invalid channel*/			
			/**/	
			break;	
		case TIMER3:
			_sys_bIsSetTime3Event = TRUE;
			_sys_uTime3EventCount++;
			for (i=0; i<TimerEventCount; i++)
			{
				if (tTime3Event[i].active == FALSE)
				{
					tTime3Event[i].active = TRUE;
					tTime3Event[i].initTick = uTimeTick;
					tTime3Event[i].curTick = uTimeTick;
					tTime3Event[i].funPtr = (sys_pvTimeFunPtr)pvFun;
					val = i+1;
					break;
				}
			}
			/*SW ADD*/
			if(i==TimerEventCount)
				return -1;		/*-1 means invalid channel*/			
			/**/	
			break;			
		default:
			;
	}

	return val;
}

VOID sysClearTimerEvent(INT32 nTimeNo, UINT32 uTimeEventNo)
{
	switch (nTimeNo)
	{
		case TIMER0:
			tTime0Event[uTimeEventNo-1].active = FALSE;
			_sys_uTime0EventCount--;
			if (_sys_uTime0EventCount == 0)
				_sys_bIsSetTime0Event = FALSE;
			break;
		case TIMER1:
			tTime1Event[uTimeEventNo-1].active = FALSE;
			_sys_uTime1EventCount--;
			if (_sys_uTime1EventCount == 0)
				_sys_bIsSetTime1Event = FALSE;
			break;
		case TIMER2:
			tTime2Event[uTimeEventNo-1].active = FALSE;
			_sys_uTime2EventCount--;
			if (_sys_uTime2EventCount == 0)
				_sys_bIsSetTime2Event = FALSE;
			break;
		case TIMER3:
			tTime3Event[uTimeEventNo-1].active = FALSE;
			_sys_uTime3EventCount--;
			if (_sys_uTime3EventCount == 0)
				_sys_bIsSetTime3Event = FALSE;
			break;		
		default:
			;
	}
}

/* 
 *  The default time: 2005.05.01 Sun 00:00 00:00
 *
 *  one day = 3600 * 24 = 86400
 *  one year = 86400 * 365 = 31536000
 *  year 2005 = 31536000 * 35 + 8 * 86400 = 1104451200
 */

UINT32 sysDOS_Time_To_UTC(DateTime_T ltime)
{
	int		i, leap_year_cnt;
	UINT32	utc;
	
	if ((ltime.mon < 1) || (ltime.mon > 12) || (ltime.day < 1) || (ltime.day > 31) ||
		(ltime.hour > 23) || (ltime.min > 59) || (ltime.sec > 59))
	{
		//_debug_msg("DOS_Time_To_UTC - illegal time!! %d-%d-%d %d:%d:%d\n", year, month, day, hour, minute, second);
		return WB_INVALID_TIME;
	}

	ltime.year = ltime.year - 1970;		/* DOS is 1980 started, UTC is 1970 started */
	
	leap_year_cnt = (ltime.year + 1) / 4;
	
	utc = ltime.year * SECONDS_365_DAY + leap_year_cnt * SECONDS_PER_DAY;
	
	if ((ltime.year + 2) % 4 == 0)
		month_seconds[1] = 29 * SECONDS_PER_DAY;	/* leap year */
	else
		month_seconds[1] = 28 * SECONDS_PER_DAY;	/* non-leap year */
	
	for (i = 0; i < ltime.mon - 1; i++) 
		utc += month_seconds[i];
		
	utc += (ltime.day - 1) * SECONDS_PER_DAY;
	
	utc += ltime.hour * SECONDS_PER_HOUR + ltime.min * 60 + ltime.sec;
	
	return utc;
}


VOID  sysUTC_To_DOS_Time(UINT32 utc, DateTime_T *dos_tm)
{
	int		the_year = 1970;
	int		i, seconds;
		
	while (1)
	{
		if (the_year % 4 == 0)
			seconds = SECONDS_366_DAY;
		else
			seconds = SECONDS_365_DAY;
		if (utc >= seconds)
		{
			utc -= seconds;
			the_year++;
		}
		else
			break;
	}
	
	dos_tm->year = the_year;
	
	if (the_year % 4 == 0)
		month_seconds[1] = 29 * SECONDS_PER_DAY;
	else
		month_seconds[1] = 28 * SECONDS_PER_DAY;
	
	dos_tm->mon = 1;
	for (i = 0; i < 11; i++)
	{
		if (utc >= month_seconds[i])
		{
			utc -= month_seconds[i];
			dos_tm->mon++;
		}
		else
			break;
	}
	
	dos_tm->day = 1 + (utc / SECONDS_PER_DAY);
	utc %= SECONDS_PER_DAY;
	
	dos_tm->hour = utc / SECONDS_PER_HOUR;
	utc %= SECONDS_PER_HOUR;
	
	dos_tm->min = utc / 60;
	dos_tm->sec = utc % 60;
}


VOID sysSetLocalTime(DateTime_T ltime)
{
	_sys_ReferenceTime_Clock = _sys_uTimer0Count;
	_sys_ReferenceTime_UTC = sysDOS_Time_To_UTC(ltime);
}

VOID sysGetCurrentTime(DateTime_T *curTime)
{
	UINT32 clock, utc_time;

	clock = _sys_uTimer0Count;
#if 1
	// To skip Timer 0 not work	
	utc_time = _sys_ReferenceTime_UTC + (clock - _sys_ReferenceTime_Clock) / _sys_uTimer0TickPerSecond;
#else
	utc_time = 20;
#endif	

	sysUTC_To_DOS_Time(utc_time, curTime);
}


VOID sysDelay(UINT32 uTicks)
{
	UINT32 volatile btime;

	if(!_sys_bIsTimer0Initial)
	{
	    	sysStartTimer(TIMER0, 100, PERIODIC_MODE);
	}

	btime = sysGetTicks(TIMER0);
	while(1)
	{
		if((sysGetTicks(TIMER0) - btime) > uTicks)
		{
		    	break;
		}
	}
}
