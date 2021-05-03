﻿#include "core/main.hpp"
#include "core/physmgr.hpp"
#include "core/virtmgr.hpp"
#include "core/kheap.hpp"
#include "dbg/kconsole.hpp"
#include "core/computer.hpp"
#include "hw/ports.hpp"

#pragma GCC optimize ("O0")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

/*

Minimum System Requirements:

	CPU:	Intel 386 or better (hopefully, 486 is the oldest tested)
	RAM:	6 MB (12 MB to install it)
	HDD:	64 MB

	VGA (or EGA) compatible video card;
	VGA (or EGA) compatible, or MDA monitor;
	USB or PS/2 keyboard

	If the computer was made in the 90s, you'll be fine
	If the computer was made in the 80s, it'll be more intersting
*/

/*

Kernel Subsystems

Krnl	Core Kernel
Phys	Physical Memory Manager
Virt	Virtual Memory Manager
Sys		System Calls
Thr		Processes, Threads, Program / Driver Loading
Hal		Hardware Abstraction Library
Dev		Device Subsystem
Dbg		Debugging
Fs		Filesystem
Reg		Registry
Vm		x86 Virtualisation
User	User settings, etc.

*/

extern "C" {
	#include "libk/string.h"
}

uint32_t sysBootSettings = 0;
extern "C" void callGlobalConstructors();
extern VAS* firstVAS;
extern void installVgaTextImplementation();

#pragma GCC optimize ("O2")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

uint8_t titleScreen[] = { 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xBB, 0x20, 0x20, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xBB, 0x20, 0xDB, 0xDB, 0xDB, 0xBB, 0x20, 0x20, 0x20, 0xDB, 0xDB, 0xBB, 0x20, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xBB, 0x20, 0xDB, 0xDB, 0xDB, 0xBB, 0x20, 0x20, 0x20, 0xDB, 0xDB, 0xBB, 0x20, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xBB, 0x20, 0x0D, 0x0A, 0xDB, 0xDB, 0xC9, 0xCD, 0xCD, 0xDB, 0xDB, 0xBB, 0xDB, 0xDB, 0xC9, 0xCD, 0xCD, 0xDB, 0xDB, 0xBB, 0xDB, 0xDB, 0xDB, 0xDB, 0xBB, 0x20, 0x20, 0xDB, 0xDB, 0xBA, 0xDB, 0xDB, 0xC9, 0xCD, 0xCD, 0xDB, 0xDB, 0xBB, 0xDB, 0xDB, 0xDB, 0xDB, 0xBB, 0x20, 0x20, 0xDB, 0xDB, 0xBA, 0xDB, 0xDB, 0xC9, 0xCD, 0xCD, 0xDB, 0xDB, 0xBB, 0x0D, 0x0A, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xC9, 0xBC, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xBA, 0xDB, 0xDB, 0xC9, 0xDB, 0xDB, 0xBB, 0x20, 0xDB, 0xDB, 0xBA, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xBA, 0xDB, 0xDB, 0xC9, 0xDB, 0xDB, 0xBB, 0x20, 0xDB, 0xDB, 0xBA, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xBA, 0x0D, 0x0A, 0xDB, 0xDB, 0xC9, 0xCD, 0xCD, 0xDB, 0xDB, 0xBB, 0xDB, 0xDB, 0xC9, 0xCD, 0xCD, 0xDB, 0xDB, 0xBA, 0xDB, 0xDB, 0xBA, 0xC8, 0xDB, 0xDB, 0xBB, 0xDB, 0xDB, 0xBA, 0xDB, 0xDB, 0xC9, 0xCD, 0xCD, 0xDB, 0xDB, 0xBA, 0xDB, 0xDB, 0xBA, 0xC8, 0xDB, 0xDB, 0xBB, 0xDB, 0xDB, 0xBA, 0xDB, 0xDB, 0xC9, 0xCD, 0xCD, 0xDB, 0xDB, 0xBA, 0x0D, 0x0A, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xC9, 0xBC, 0xDB, 0xDB, 0xBA, 0x20, 0x20, 0xDB, 0xDB, 0xBA, 0xDB, 0xDB, 0xBA, 0x20, 0xC8, 0xDB, 0xDB, 0xDB, 0xDB, 0xBA, 0xDB, 0xDB, 0xBA, 0x20, 0x20, 0xDB, 0xDB, 0xBA, 0xDB, 0xDB, 0xBA, 0x20, 0xC8, 0xDB, 0xDB, 0xDB, 0xDB, 0xBA, 0xDB, 0xDB, 0xBA, 0x20, 0x20, 0xDB, 0xDB, 0xBA, 0x0D, 0x0A, 0xC8, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xBC, 0x20, 0xC8, 0xCD, 0xBC, 0x20, 0x20, 0xC8, 0xCD, 0xBC, 0xC8, 0xCD, 0xBC, 0x20, 0x20, 0xC8, 0xCD, 0xCD, 0xCD, 0xBC, 0xC8, 0xCD, 0xBC, 0x20, 0x20, 0xC8, 0xCD, 0xBC, 0xC8, 0xCD, 0xBC, 0x20, 0x20, 0xC8, 0xCD, 0xCD, 0xCD, 0xBC, 0xC8, 0xCD, 0xBC, 0x20, 0x20, 0xC8, 0xCD, 0xBC };

extern "C" void kernel_main()
{
	outb(0x3f8 + 1, 0x00);    // Disable all interrupts
	outb(0x3f8 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outb(0x3f8 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	outb(0x3f8 + 1, 0x00);    //                  (hi byte)
	outb(0x3f8 + 3, 0x03);    // 8 bits, no parity, one stop bit
	outb(0x3f8 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outb(0x3f8 + 4, 0x0B);    // IRQs enabled, RTS/DSR set

	kprintf("\n\nKERNEL HAS STARTED.\n");

	installVgaTextImplementation();

	uint16_t* b = (uint16_t*) 0xC20B8000;
	int x = 14;
	int y = 5;
	for (int i = 0; titleScreen[i]; ++i) {
		if (titleScreen[i] == '\r') continue;
		if (titleScreen[i] == '\n') {
			x = 13;
			++y;
		} else {
			*(b + y * 80 + x) = ((uint16_t) titleScreen[i]) | 0x0E00;
			++x;
		}
	}

	sysBootSettings = *((uint32_t*) 0x500);
	Phys::physicalMemorySetup(((*((uint32_t*) 0x524)) + 4095) & ~0xFFF);		//cryptic one-liner
	Virt::virtualMemorySetup();

	{
		VAS v;
		firstVAS = &v;

		callGlobalConstructors();

		computer = new Computer();
		computer->open(0, 0, nullptr);
	}
}
