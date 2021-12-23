#include "thr/prcssthr.hpp"
#include "sys/syscalls.hpp"
#include "hal/intctrl.hpp"

#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")


/// <summary>
/// Reads data from a file.
/// </summary>
/// <param name="ebx">The file number.</param>
/// <param name="ecx">Maximum number of bytes to read.</param>
/// <param name="edx">Pointer to store read data.</param>
/// <returns>The number of bytes read, or -1 on failure.</returns>
/// 
uint64_t SysRead(regs* r)
{
	UnixFile* file = nullptr;

	if (r->ebx <= 2) {
		file = currentTaskTCB->processRelatedTo->terminal;

	} else {
		file = KeGetFileFromDescriptor(r->ebx);
	}

	if (file == nullptr) {
		return -1;
	}

	int br = 0;
	FileStatus status = file->read(r->ecx, (void*) r->edx, &br);

	return br;
}