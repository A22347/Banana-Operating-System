#include "core/common.hpp"
#include "thr/elf.hpp"
#include "krnl/hal.hpp"
#include "hw/acpi.hpp"
#include "reg/registry.hpp"

#include "ps2.hpp"
#include "ps2port.hpp"
#include "ps2mouse.hpp"
#include "ps2key.hpp"

#pragma GCC optimize ("O0")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

char ps2portname[] = "PS/2 Port";
PS2Port::PS2Port(): Bus(ps2portname)
{
	
}

int PS2Port::open(int prtNum, int b, void* ctrl)
{
	portNum = prtNum;
	controller = (PS2*) ctrl;

	detect();
	return 0;
}

int PS2Port::close(int, int, void*)
{
	return 0;
}

void PS2Port::detect()
{
	if (portNum == PS2_PORT1) {
		PS2Keyboard* kbd = new PS2Keyboard();
		addChild(kbd);
		kbd->open(0, 0, controller);

	} else if (portNum == PS2_PORT2) {
		PS2Mouse* mse = new PS2Mouse();
		addChild(mse);
		mse->open(1, 0, controller);
	}
}

bool PS2Port::deviceWrite(uint8_t command)
{
	//send it to the second port if needed
	if (portNum == PS2_PORT2) {
		controller->controllerWrite(PS2_CMD_WRITE_NEXT_TO_PORT_2_INPUT);
	}

	int timeout = 0;
	while (1) {
		//read the status
		uint8_t status = inb(PS2_STATUS);

		//check if it is ready
		if (!(status & PS2_STATUS_BIT_IN_FULL)) {
			break;
		}

		//check for any errors
		if ((status & PS2_STATUS_BIT_TIMEOUT) || (status & PS2_STATUS_BIT_PARITY)) {
			break;
		}
		
		//if we timeout, just send it anyway
		if (timeout++ == 1600) {
			break;
		}
	}
	
	//send the command
	outb(PS2_DATA, command);

	return true;
}

uint8_t PS2Port::deviceRead()
{
	return controller->controllerRead();
}
