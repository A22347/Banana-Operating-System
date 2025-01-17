#include "hal/sound/sndport.hpp"
#include "hal/sound/sndcard.hpp"
#include "krnl/common.hpp"
#include "krnl/kheap.hpp"
#include "hal/device.hpp"
#include "thr/prcssthr.hpp"

#pragma GCC optimize ("Ofast")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

extern "C" {
	#include "libk/string.h"
}


SoundPort::SoundPort(int _sampleRate, int _bits, int _channels, int _buffSize)
{
	sampleRate = _sampleRate;
	bits = _bits;
	channels = _channels;

	if (_buffSize == -1) {
		_buffSize = sampleRate * 2 + 8000;
	}

	buffSize = _buffSize;

	buffUsed = 0;
	buff = (int32_t*) malloc(buffSize * sizeof(int32_t));
}

SoundPort::~SoundPort()
{
	if (buff) {
		free(buff);
		buff = nullptr;
	}
}


void SoundPort::unpause()
{
	paused = false;
}

void SoundPort::pause()
{
	paused = true;
}

int SoundPort::getBufferUsed()
{
	return buffUsed;
}

int SoundPort::getBufferSize()
{
	return buffSize;
}

int SoundPort::buffer8(int8_t* data, int samples)
{
	if (buffUsed == buffSize) {
		return 0;
	}

	int i = 0;
	for (; i < samples; ++i) {
		buff[buffUsed++] = data[i] * 0xFFFFFF;

		if (buffUsed == buffSize) {
			//buffer full
			return i + 1;
		}
	}

	return i;
}

int SoundPort::buffer16(int16_t* data, int samples)
{
	if (buffUsed == buffSize) {
		return 0;
	}

	int i = 0;
	for (; i < samples; ++i) {
		buff[buffUsed++] = data[i] * 0xFFFF;

		if (buffUsed == buffSize) {
			//buffer full
			return i + 1;
		}
	}

	return i;
}

int SoundPort::buffer32(int32_t* data, int samples)
{
	if (buffUsed == buffSize) {
		return 0;
	}

	int i = 0;
	for (; i < samples; ++i) {
		buff[buffUsed++] = data[i];

		if (buffUsed == buffSize) {
			//buffer full
			return i + 1;
		}
	}

	return i;
}

int SoundPort::unbufferAndAdd16(int max, int16_t* buffer, SoundCard* card)
{
	if (sampleRate != card->currentSampleRate) {
		KePanic("TODO 16! Convert sample rates!");
	}
	if (bits != card->currentBits) {
		KePanic("TODO 16! Convert bit rates!");
	}
	if (channels != card->currentChannels) {
		KePanic("TODO 16! Convert bit rates!");
	}

	int amount = buffUsed > max ? max : buffUsed;
	for (int i = 0; i < amount; ++i) {
		buffer[i] += buff[i] / 0x8000;
	}

	memmove(buff, buff + amount, (buffSize - amount) * sizeof(int32_t));
	buffUsed -= amount;
	return amount;
}

int SoundPort::unbufferAndAdd32(int max, int32_t* buffer, SoundCard* card)
{
	if (sampleRate != card->currentSampleRate) {
		KePanic("TODO 32! Convert sample rates!");
	}
	if (bits != card->currentBits) {
		KePanic("TODO 32! Convert bit rates!");
	}
	if (channels != card->currentChannels) {
		KePanic("TODO 32! Convert bit rates!");
	}

	int amount = buffUsed > max ? max : buffUsed;
	for (int i = 0; i < amount; ++i) {
		buffer[i] += buff[i];
	}

	memmove(buff, buff + amount, (buffSize - amount) * sizeof(int32_t));
	buffUsed -= amount;
	return amount;
}