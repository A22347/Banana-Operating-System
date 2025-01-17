#include "krnl/main.hpp"
#include "hal/sound/sndcard.hpp"
#include "krnl/common.hpp"
#include "krnl/kheap.hpp"
extern "C" {
#include "libk/string.h"
#include "libk/math.h"
}
#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")


SoundCard::SoundCard(const char* name) : Device(name)
{
	deviceType = DeviceType::Audio;

	for (int i = 0; i < SOUND_DEVICE_MAX_VIRTUAL_CHANNELS; ++i) {
		channels[i] = nullptr;
	}

	playing = false;
}

SoundCard::~SoundCard()
{
	
}

bool SoundCard::configureRates(int sampleRate, int bits, int channels)
{
	if (!playing) {
		currentSampleRate = sampleRate;
		currentBits = bits;
		currentChannels = channels;
		return true;

	} else {
		return false;
	}
}

int SoundCard::getSamples16(int max, int16_t* buffer)
{
	int maxGot = 0;
	memset(buffer, 0, max * sizeof(int16_t));

	for (int i = 0; i < SOUND_DEVICE_MAX_VIRTUAL_CHANNELS; ++i) {
		if (channels[i] != nullptr && !channels[i]->paused) {
			int got = channels[i]->unbufferAndAdd16(max, buffer, this);
			if (got > maxGot) {
				maxGot = got;
			}
		}
	}

	return maxGot;
}

int SoundCard::getSamples32(int max, int32_t* buffer)
{
	int maxGot = 0;
	memset(buffer, 0, max * sizeof(int32_t));

	for (int i = 0; i < SOUND_DEVICE_MAX_VIRTUAL_CHANNELS; ++i) {
		if (channels[i] != nullptr && !channels[i]->paused) {
			int got = channels[i]->unbufferAndAdd32(max, buffer, this);
			if (got > maxGot) {
				maxGot = got;
			}
		}
	}

	if (maxGot == 0 && playing) {
		stopPlayback();

	} else if (maxGot && !playing) {
		beginPlayback();
	}

	return maxGot;
}

int SoundCard::addChannel(SoundPort* ch)
{
	int id = -1;
	for (int i = 0; i < SOUND_DEVICE_MAX_VIRTUAL_CHANNELS; ++i) {
		if (channels[i] == nullptr) {
			id = i;
			break;
		}
	}

	if (id == -1) {
		KePanic("DEBUG: Could not add channel!\n");
		return -1;
	}

	channels[id] = ch;
	return id;
}

void SoundCard::removeChannel(int id)
{
	channels[id] = nullptr;
}

void SoundCard::beginPlayback()
{
	KePanic("PSEDUO-PURE VIRTUAL SoundCard CALLED");
}

void SoundCard::stopPlayback()
{
	KePanic("PSEDUO-PURE VIRTUAL SoundCard CALLED");
}
