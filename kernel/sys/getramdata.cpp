#include "thr/prcssthr.hpp"
#include "sys/syscalls.hpp"
#include "hal/intctrl.hpp"
#include "hal/timer.hpp"
#include "krnl/physmgr.hpp"

#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")



/// <summary>
/// Returns the total amount of RAM and the current amount of RAM available.
/// </summary>
/// <returns>Returns the amount of 4KiB pages the system has in the low 3 bytes, and the twice the percentage used in the high byte.</returns>
/// 
uint64_t SysGetRAMData(regs* r)
{
	extern int swapBalance;

	int percent = (Phys::usedPages + swapBalance) * 200 / Phys::usablePages;
	return Phys::usablePages | (percent << 24);
}
