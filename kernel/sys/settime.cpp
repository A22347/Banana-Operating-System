#include "krnl/computer.hpp"
#include "thr/prcssthr.hpp"
#include "sys/syscalls.hpp"
#include "hal/intctrl.hpp"
#include "hal/clock.hpp"
#include "hal/timer.hpp"

#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")


/*
extern uint64_t SystemCall(size_t, size_t, size_t, size_t);
		uint32_t b = seconds + minute * 60 + hour * 3600;
		uint32_t c = (day - 1) + (month - 1) * 32;
		uint32_t d = year;
		int res = SystemCall(SetTime, b, c, d);
		*/

/// <summary>
/// Sets the system time.
/// </summary>
/// <param name="ebx">The number of seconds since midnight of the current day.</param>
/// <param name="ecx">This is equal to DAY_OF_MONTH + MONTH * 32, both are zero based.</param>
/// <param name="edx">The year is stored in the low word. The high word is reserved.</param>
/// <returns>Returns zero on success, non-zero otherwise.</returns>
/// 
uint64_t SysSetTime(regs* r)
{
	//zero based
	int seconds = r->ebx % 60;
	int minutes = (r->ebx / 60) % 60;
	int hours = (r->ebx / 3600) % 24;

	//one based
	int day = ((r->ecx) % 32) + 1;

	//zero based
	int month = ((r->ecx / 32) % 12);

	int year = r->edx & 0xFFFF;

	datetime_t dt;
	dt.day = day;
	dt.month = month;
	dt.year = year;
	dt.second = seconds;
	dt.minute = minutes;
	dt.hour = hours;

	kprintf("Setting to (A) %d/%d/%d %d:%d:%d\n", day, month, year, hours, minutes, seconds);
	kprintf("Setting to (B) %d/%d/%d %d:%d:%d\n", dt.day, dt.month, dt.year, dt.hour, dt.minute, dt.second);

	return !computer->clock->setTimeInDatetimeLocal(dt);
}
