#include "hw/intctrl/pic.hpp"
#include "krnl/hal.hpp"
#include "core/main.hpp"
#include "hal/intctrl.hpp"
#include "krnl/hal.hpp"

PIC::PIC() : InterruptController ("PIC (BAD!)")
{
}

void PIC::ioWait()
{
}

uint16_t PIC::getIRQReg(int ocw3)
{
	return 0;
}

void PIC::remap()
{
}

int PIC::open(int, int, void*)
{
	return 0;
}

int PIC::close(int, int, void*)
{
	return 0;
}