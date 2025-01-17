#include "thr/prcssthr.hpp"
#include "sys/syscalls.hpp"
#include "hal/intctrl.hpp"
#include "hal/timer.hpp"

#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

/// <summary>
/// Yields the currently running thread's timeslice.
/// </summary>
/// <returns>Returns zero.</returns>
/// 
uint64_t SysYield(regs* r)
{
	lockScheduler();
	schedule();
	unlockScheduler();
	return 0;
}
