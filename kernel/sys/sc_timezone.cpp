#include "core/computer.hpp"
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

namespace Sys
{
	/// <summary>
	/// Gets or sets the timezone.
	/// </summary>
	/// <param name="ebx">When setting the timezone, this is the timezone ID. This is the zero-based line number in the timezones file.</param>
	/// <param name="ecx">Set to zero if getting the time zone, or any other value to set.</param>
	/// <returns>If setting the timezone, returns zero on success, or non-zero on failure. If getting the timezone, it returns the timezone ID, or -1 if this cannot be found.</returns>
	/// 
	uint64_t timezone(regs* r)
	{
		if (r->ecx == 0) {
			//get timezone
			kprintf("TODO: unimplemented, sc_timezone.cpp: get the timezone\n");
			return -1;

		} else {
			//set the timezone
			kprintf("TODO: unimplemented, sc_timezone.cpp: TODO! WRITE TO REGISTRY\n");
			User::loadClockSettings(r->ebx);
			return 1;
		}
	}
}

