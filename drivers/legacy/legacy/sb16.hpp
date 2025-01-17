
#ifndef _SB16HPP_
#define _SB16HPP_

#include <stdint.h>

#include "krnl/main.hpp"
#include "krnl/physmgr.hpp"
#include "krnl/common.hpp"
#include "krnl/kheap.hpp"
#include "krnl/terminal.hpp"
#include "hal/intctrl.hpp"
#include "krnl/hal.hpp"
#include "hw/acpi.hpp"
#include "hal/sound/sndcard.hpp"
#include "fs/vfs.hpp"

extern "C" {
#include "libk/string.h"
#include "libk/math.h"
}

class SoundBlaster16: public SoundCard
{
private:

protected:

public:
	DMAChannel* dmaChannel;
	DMAChannel* dmaChannel16;

	uint32_t dmaAddr;
	uint32_t dma16Addr;

	SoundBlaster16();

	void handleIRQ();

	uint8_t dspVersion = 0;

	int open(int a, int b, void* c);
	int _open(int a, int b, void* c);
	int close(int a, int b, void* c);

	virtual void beginPlayback() override;
	virtual void stopPlayback() override;

	void onInterrupt();

	void DSPOut(uint16_t port, uint8_t val);
	void resetDSP();
	void turnSpeakerOn(bool on = true);
};


#endif
