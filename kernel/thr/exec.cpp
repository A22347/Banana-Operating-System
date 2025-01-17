#include <thr/prcssthr.hpp>
#include <krnl/computer.hpp>
#include <krnl/common.hpp>
#include <krnl/kheap.hpp>
#include <krnl/physmgr.hpp>
#include <krnl/virtmgr.hpp>
#include <thr/elf.hpp>

#include "libk/string.h"

#pragma GCC optimize ("O2")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

int KeProcessExec(Process* prcss, const char* filename)
{
	delete prcss->vas;
	prcss->vas = new VAS(false);
	Thr::loadProgramIntoMemory(prcss, filename);
	return 0;
}