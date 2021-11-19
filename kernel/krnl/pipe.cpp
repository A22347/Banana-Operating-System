#include "krnl/pipe.hpp"
#include "krnl/common.hpp"
#include "thr/prcssthr.hpp"

#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

Pipe::Pipe(int bufferSize) : UnixFile()
{
	maxSize = bufferSize;
	bufferBase = 0;
	bufferLength = 0;
	buffer = (uint8_t*) malloc(bufferSize);

	blockOnRead = false;
}

Pipe::~Pipe()
{

}

bool Pipe::isAtty()
{
	return false;
}

FileStatus Pipe::read(uint64_t bytes, void* where, int* bytesRead)
{
	//wait until we have enough data if we are set to block
	while (blockOnRead && bufferLength < bytes) {
		lockScheduler();
		schedule();
		unlockScheduler();
	}

	if (bufferLength < bytes) {
		bytes = bufferLength;
	}

	int preWrap = (maxSize - bufferBase) > bytes ? bytes : maxSize - bufferBase;
	int postWrap = bytes - preWrap;
	
	uint8_t* destBuffer = (uint8_t*) where;

	if (preWrap) memcpy(destBuffer, buffer + bufferBase, preWrap);
	if (postWrap) memcpy(destBuffer + preWrap, buffer, postWrap);

	bufferLength -= bytes;
	bufferBase = (bufferBase + bytes) % maxSize;

	if (!bytesRead) {
		KePanic("PIPE READ NULL BR PASSED IN");
	}

	*bytesRead = bytes;

	return FileStatus::Success;
}

FileStatus Pipe::write(uint64_t bytes, void* where, int* bytesWritten)
{
	if (bytes + bufferLength > maxSize) {
		return FileStatus::Failure;
	}

	int roomLeft = maxSize - (bufferBase + bufferLength) % maxSize;
	int postWrap = bytes < roomLeft ? 0 : bytes - roomLeft;
	int preWrap = bytes - postWrap;

	uint8_t* sourceBuffer = (uint8_t*) where;
	if (preWrap) memcpy(buffer + (bufferBase + bufferLength) % maxSize, sourceBuffer, preWrap);
	if (postWrap) memcpy(buffer, sourceBuffer + preWrap, postWrap);

	bufferLength += bytes;

	if (!bytesWritten) {
		KePanic("PIPE WRITE NULL BR PASSED IN");
	}

	*bytesWritten = bytes;
	return FileStatus::Success;
}