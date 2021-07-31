#include "thr/prcssthr.hpp"
#include "sys/syscalls.hpp"
#include "hal/intctrl.hpp"
#include "hal/timer.hpp"

#include "fs/symlink.hpp"

#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

/// <summary>
/// Creates a symbolic link for a file or folder.
/// </summary>
/// <param name="ecx">A pointer to the target filename string.</param>
/// <param name="edx">A pointer to the new symlink filename string.</param>
/// <returns>Returns zero on success, 2 on ELOOP, and non-zero otherwise.</returns>
/// 
uint64_t SysSymlink(regs* r)
{
	if (r->ecx == 0 || r->edx == 0) return -1;
	return KeCreateSymlink((const char*) r->ecx, (const char*) r->edx);
}