#include "core/main.hpp"
#include "thr/prcssthr.hpp"
#include "fs/vfs.hpp" 
#include "hal/logidisk.hpp"
#include "libk/string.h"
#include "fs/symlink.hpp"

extern "C" {
#include "libk/string.h"
}

#include <krnl/random.hpp>
#include <krnl/atexit.hpp>

#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

uint32_t KiBaseSymlinkID = 0;

#define MAX_WAITING_ROOM_LINKS 4

char KiNewlyCreatedSymlinks[MAX_WAITING_ROOM_LINKS][256];
uint64_t KiNewlyCreatedIDs[MAX_WAITING_ROOM_LINKS];
int KiNumWaitingRoomSymlinks = 0;

uint64_t KiCreateSymlinkID()
{
	int insanityCounter = 0;
	uint64_t id;
	do {
		id = KeRand();
		id <<= 27;

		id ^= KiBaseSymlinkID++;

		if (++insanityCounter == 666) {
			KePanic("INSANITY");
		}

	} while (!id);
	return id;
}

void KiFlushSymlinkChanges()
{
	//TODO: write KiBaseSymlinkID back to disk

	File* f = new File("C:/Banana/System/symlinks.sys", kernelProcess);
	if (!f) {
		KePanic("CANNOT WRITE SYMLINKS (A)");
	}
	
	FileStatus status = f->open(FILE_OPEN_APPEND);
	if (status != FileStatus::Success) {
		KePanic("CANNOT WRITE SYMLINKS (B)");
	}

	int br;
	for (int i = 0; i < KiNumWaitingRoomSymlinks; ++i) {
		f->write(256, KiNewlyCreatedSymlinks[i], &br);
		if (br != 256) {
			KePanic("CANNOT WRITE SYMLINKS (C)");
		}
	}

	KiNumWaitingRoomSymlinks = 0;

	f->close();
	delete f;
}

void KiDeinitialiseSymlinks(void* context)
{
	KiFlushSymlinkChanges();
}

void KeRegisterSymlink(const char* linkName, uint64_t linkID)
{
	KiNewlyCreatedIDs[KiNumWaitingRoomSymlinks] = linkID;
	memset(KiNewlyCreatedSymlinks[KiNumWaitingRoomSymlinks], 0, 256);
	strcpy(KiNewlyCreatedSymlinks[KiNumWaitingRoomSymlinks], linkName);

	++KiNumWaitingRoomSymlinks;

	if (KiNumWaitingRoomSymlinks >= MAX_WAITING_ROOM_LINKS) {
		KiFlushSymlinkChanges();
	}
}

void KeInitialiseSymlinks()
{
	KiNumWaitingRoomSymlinks = 0;

	//TODO: load KiBaseSymlinkID

	KeRegisterAtexit(KiDeinitialiseSymlinks, nullptr);

	File* f = new File("C:/Banana/System/symlinks.sys", kernelProcess);
	if (!f) {
		KePanic("CANNOT CREATE SYMLINK FILE A");
	}
	if (!f->exists()) {
		FileStatus status = f->open(FILE_OPEN_WRITE_NORMAL);
		if (status != FileStatus::Success) {
			KePanic("CANNOT CREATE SYMLINK FILE B");
		}
		f->close();
	}
	delete f;

	KeCreateSymlink("C:/Banana/System/KERNEL32.EXE", "C:/kernel.txt");
}


uint64_t KiIsSymlinkRegistered(const char* linkName)
{
	for (int i = 0; i < KiNumWaitingRoomSymlinks; ++i) {
		if (!strcmp(linkName, KiNewlyCreatedSymlinks[i])) {
			return KiNewlyCreatedIDs[i];
		}
	}

	//TODO: check the symlink file
	return 0;
}

int KeDereferenceSymlink(const char* linkName, char* dereferencedBuffer)
{
	kprintf("dereferencing symlink %s\n", linkName);
	uint64_t registrationID = KiIsSymlinkRegistered(linkName);
	if (!registrationID) {
		return 0;
	}

	kprintf("regid l 0x%X\n", (uint32_t) registrationID);
	kprintf("regid h 0x%X\n", (uint32_t) (registrationID >> 32));

	File* fil = new File(linkName, kernelProcess);
	if (!fil) {
		return -1;
	}

	kprintf("A\n");

	FileStatus status = fil->open(FileOpenMode::Read);
	if (status != FileStatus::Success) {
		delete fil;
		return -1;
	}

	kprintf("B\n");

	char buffer[9];
	int br;
	status = fil->read(8, buffer, &br);
	if (status != FileStatus::Success || br != 8) {
		fil->close();
		delete fil;
		return -1;
	}

	kprintf("C\n");

	uint64_t id;
	status = fil->read(8, &id, &br);
	if (status != FileStatus::Success || br != 8) {
		fil->close();
		delete fil; 
		return -1;
	}
	if (id != registrationID) {
		return 0;
	}

	kprintf("D\n");

	uint64_t siz = 0;
	bool dir;
	fil->stat(&siz, &dir);

	if (dir || siz < 16) {
		fil->close();
		delete fil; 
		return -1;
	}

	kprintf("D\n");

	siz -= 16;
	if (siz > 255) {
		return -1;
	}
	status = fil->read(siz, dereferencedBuffer, &br);
	if (status != FileStatus::Success || br != siz) {
		fil->close();
		delete fil;
		return -1;
	}

	kprintf("E\n");

	fil->close();
	delete fil;
	kprintf("F\n");

	return 1;
}


int KeCreateSymlink(const char* existing, const char* linkName)
{
	File* fil = new File(linkName, kernelProcess, false);
	if (!fil) {
		return 1;
	}

	FileStatus status = fil->open(FILE_OPEN_WRITE_NORMAL);
	if (status != FileStatus::Success) {
		delete fil;
		return 1;
	}

	int br;
	char symsig[] = "SYMLINK!";
	status = fil->write(8, symsig, &br);
	if (br != 8 || status != FileStatus::Success) {
		fil->close();
		delete fil;
		return 1;
	}

	uint64_t id = KiCreateSymlinkID();
	status = fil->write(8, &id, &br);
	if (br != 8 || status != FileStatus::Success) {
		fil->close();
		delete fil;
		return 1;
	}

	status = fil->write(strlen(existing), (void*) existing, &br);
	if (br != strlen(existing) || status != FileStatus::Success) {
		fil->close();
		delete fil;
		return 1;
	}

	KeRegisterSymlink(linkName, id);

	fil->close();
	delete fil;

	return 0;
}