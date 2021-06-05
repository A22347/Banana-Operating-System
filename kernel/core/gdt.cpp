#include "core/gdt.hpp"
#include "core/main.hpp"
#include "core/common.hpp"
#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

void GDTEntry::setBase(uint32_t base)
{
	baseLow = base & 0xFFFFFF;
	baseHigh = (base >> 24) & 0xFF;
}

void GDTEntry::setLimit(uint32_t limit)
{
	limitLow = limit & 0xFFFF;
	limitHigh = (limit >> 16) & 0xF;
}


GDT::GDT()
{
	entryCount = 0;
}

int GDT::addEntry(GDTEntry entry)
{
	entries[entryCount] = entry.val;
	return (entryCount++) * 8;
}

int GDT::getNumberOfEntries()
{
	return entryCount;
}

GDTDescriptor gdtDescr;

extern "C" void loadGDT();

void GDT::flush()
{
	gdtDescr.size = entryCount * 8 - 1;
	gdtDescr.offset = (size_t) (void*) entries;

	loadGDT();
}

void GDT::setup()
{
	GDTEntry null;
	null.setBase(0);
	null.setLimit(0);
	null.readWrite = 0;
	null.directionAndConforming = 0;
	null.access = 0;
	null.flags = 0;

	GDTEntry code;
	code.setBase(0);
	code.setLimit(PLATFORM_ID == 64 ? 0 : 0xFFFFFF);
	code.bit64 = PLATFORM_ID == 64;
	code.size = PLATFORM_ID != 64;
	code.gran = 1;
	code.readWrite = 1;
	code.priv = 0;
	code.present = 1;
	code.executable = 1;
	code.type = 1;
	code.directionAndConforming = 0;

	GDTEntry data = code;
	data.executable = 0;

	GDTEntry userCode = code;
	userCode.priv = 3;

	GDTEntry userData = data;
	userData.priv = 3;

	GDTEntry code16 = code;
	code16.setBase(0);
	code16.size = 0;
	code16.gran = 0;

	GDTEntry data16 = data;
	data16.setBase(0);
	data16.size = 0;
	data16.gran = 0;

	addEntry(null);
	addEntry(code);			//0x08
	addEntry(data);			//0x10		
	addEntry(userCode);		//0x18
	addEntry(userData);		//0x20
	addEntry(code16);		//0x28
	addEntry(data16);		//0x30
	flush();
}