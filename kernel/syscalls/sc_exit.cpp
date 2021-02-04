#include "core/prcssthr.hpp"
#include "core/syscalls.hpp"
#include "hal/intctrl.hpp"

//#pragma GCC optimize ("Os")

/// <summary>
/// Terminates the currently running thread. If this is the last running thread, the entire process will be terminated.
/// </summary>
/// <param name="ebx">The exit code for the thread.</param>
/// <returns>This system call should not return. If it does, it will return -1.</returns>
/// 
uint64_t sysCallExit(regs* r)
{
	terminateTask(r->ebx);
	return -1;
}
