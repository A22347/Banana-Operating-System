
#ifndef _ASDX87_HPP_
#define _ASDX87_HPP_

#include <stdint.h>
#include <stddef.h>
#include "hal/device.hpp"

class DMAChannel
{
private:
	friend class DMA;
	int num;

	DMAChannel(int num);
	~DMAChannel();

protected:
	uint32_t addr;
	uint32_t count;

	bool hasMode;
	bool startedOnce;
	bool dma16;
	uint8_t mode;

	size_t dmaPointerToFree = 0;
	size_t dmaPointerSize = 0;

public:
	void setLength(uint32_t l);
	void setAddress(uint32_t addr);
	void allocateAddressAndSet(uint16_t l);
	uint32_t getAddress();
	int getChannelNum();

	void start();
	void stop();

	void mask(bool m = true);
	void unmask();

	void setMode(uint8_t mode);
};

class DMA: public Device
{
private:
	friend class DMAChannel;

protected:
	uint8_t available = 0;
	int getAvailable();

	DMAChannel* tryLockChannelWithBase(int base);
	DMAChannel* lockChannelWithBase(int base);

public:
	DMA();

	int open(int, int, void*);
	int close(int, int, void*);

	DMAChannel* tryLockChannel8();
	DMAChannel* tryLockChannel16();
	DMAChannel* tryLockChannel(int num);
	DMAChannel* lockChannel8();
	DMAChannel* lockChannel16();
	DMAChannel* lockChannel(int num);

	void unlockChannel(DMAChannel* channel);
};

#endif
