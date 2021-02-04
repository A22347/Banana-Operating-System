#include "hal/sound/sndchannel.hpp"
#include "core/common.hpp"
#include "core/kheap.hpp"
#include "hal/device.hpp"
#include "core/prcssthr.hpp"
//#pragma GCC optimize ("Os")

extern "C" {
	#include "libk/string.h"
	#include "libk/math.h"
}

SoundChannel::SoundChannel(int _sampleRate, int _bits, int _direction, int _buffSize)
{
	pause();
	setVolume(80);
	setSpeed(1.0);

	sampleRate = _sampleRate;
	bits = _bits;
	direction = _direction;

	if (_buffSize == -1) {
		_buffSize = sampleRate * bits / 4 + 4000;
	}

	buffSize = _buffSize;

	buffStart = 0;
	buffUsed = 0;
	buff = (float*) malloc(buffSize * sizeof(float));

	int error;
	conv = src_new(SRC_LINEAR, 1, &error);
	if (error) {
		panic("TODO: SOUND CHANNEL ERROR");
	}
}

SoundChannel::~SoundChannel()
{
	free(buff);
}

void SoundChannel::setSpeed(float multi)
{
	speed = multi;
}

float SoundChannel::getSpeed()
{
	return speed;
}

void SoundChannel::setVolume(int v)
{
	if (v < 0) v = 0;
	if (v > 100) v = 100;
	volume = v;
}

int SoundChannel::getVolume()
{
	return volume;
}

void SoundChannel::pause()
{
	isPaused = true;
}

void SoundChannel::play()
{
	isPaused = false;
}

bool SoundChannel::paused()
{
	return isPaused;
}

int SoundChannel::getSampleRate()
{
	return sampleRate;
}

int SoundChannel::getBits()
{
	return bits;
}

int SoundChannel::getBufferUsed()
{
	return buffUsed;
}

int SoundChannel::getBufferSize()
{
	return buffSize;
}

int SoundChannel::unbuffer(float* output, int outSampleRate, int maxOut)
{
	SRC_DATA data;

	data.data_in = buff;
	data.input_frames = buffUsed;
	data.output_frames = maxOut;
	data.data_out = output;
	data.src_ratio = ((float) outSampleRate) / ((float) sampleRate) / speed;
	data.end_of_input = 0;

	src_process(conv, &data);

	buffUsed -= data.input_frames_used;

	memmove(buff, buff + data.input_frames_used, (buffSize - data.input_frames_used) * sizeof(float));
	return data.output_frames_gen;
}

int SoundChannel::buffer8(uint8_t* data, int len)
{
	int done = 0;

	if (buffUsed == buffSize) {
		return 0;
	}

	for (int i = 0; i < len; ++i) {
		buff[buffUsed++] = ((float) data[i]) / (1.0 * 0x80);
		
		++done;

		if (buffUsed == buffSize) {
			//buffer full
			return done;
		}
	}
	
	return done;
}

