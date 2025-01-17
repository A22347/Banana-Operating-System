#include "krnl/physmgr.hpp"
#include "krnl/common.hpp"
#include "krnl/terminal.hpp"
#include "hal/vcache.hpp"		//cache purging
#include <krnl/unaligned.hpp>

#pragma GCC optimize ("O2")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

#define STATE_FREE false
#define STATE_ALLOCATED true
#define DMA_BLOCK_SIZE 4096

/*
PUBLIC:
void			PmInitialise();
PhysicalAddr	PmAllocatePage();
PhysicalAddr	PmAllocateContiguousPages(int count);
PhysicalAddr	PmAllocatePagesForDMA(int count);
size_t			PmGetTotalUsedBytes();
size_t			PmGetTotalFreeBytes();
void			PmFreePage(PhysicalAddr addr);

PRIVATE:
PhysicalAddr	PmAllocateContiguousPagesWithConstraints(PhysicalAddr minAddr, PhysicalAddr maxAddr, int num, bool mayCross64KBoundary);
*/

namespace Phys
{
	uint8_t dmaUsage[SIZE_DMA_MEMORY_1 / DMA_BLOCK_SIZE];

	size_t allocateDMA(size_t size)
	{
		int blocks = (size + DMA_BLOCK_SIZE - 1) / DMA_BLOCK_SIZE;

		int start = 0;
		int count = 0;
		int backtrack = 0;

		for (int i = 0; i < sizeof(dmaUsage); ++i) {
			if (dmaUsage[i] != 0) {
				count = 0;
			} else {
				if (!count) {
					start = i;
					backtrack = i;
				}
				++count;
				if (count == blocks) {
					//check we didn't cross a 64KB boundary

					int startSeg = start / (65536 / DMA_BLOCK_SIZE);
					int endSeg = (start + count - 1) / (65536 / DMA_BLOCK_SIZE);

					if (startSeg != endSeg) {
						i = backtrack;
						count = 0;
						continue;
					}

					for (int j = 0; j < blocks; ++j) {
						dmaUsage[start + j] = 1;
					}

					usedPages += (size + 4095) / 4096;
					if (startSeg < SIZE_DMA_MEMORY_1 / 65536) {
						return VIRT_DMA_MEMORY_1 + start * DMA_BLOCK_SIZE;
					} else {
						KePanic("DMA RAM EXHAUSTED");
					}
				}
			}
		}

		KePanic("DMA RAM EXHAUSTED");

		return 0;
	}

	void freeDMA(size_t addr, size_t size)
	{
		int blocks = (size + DMA_BLOCK_SIZE - 1) / DMA_BLOCK_SIZE;
		usedPages -= (size + 4095) / 4096;

		addr -= VIRT_DMA_MEMORY_1;
		addr /= DMA_BLOCK_SIZE;

		for (int i = 0; i < blocks; ++i) {
			dmaUsage[addr + i] = 0;
		}
	}

	int usablePages = 0;
	int usedPages = 0;
	int oldPercent = 0;
	uint8_t* bitmap = (uint8_t*) VIRT_PHYS_PAGE_BITMAP;
	size_t highestMem = 0;

	void setPageState(size_t pageNum, bool state)
	{
		size_t byteNum = pageNum / 8;
		size_t bitNum = pageNum % 8;

		bitmap[byteNum] &= ~(1 << bitNum);
		bitmap[byteNum] |= (((int) state) << bitNum);

		if (state == STATE_FREE) {
			//kprintf("allowing you to use page %d\n", pageNum);
		}
	}

	bool getPageState(size_t pageNum)
	{
		size_t byteNum = pageNum / 8;
		size_t bitNum = pageNum % 8;

		if (bitmap[byteNum] & (1 << bitNum)) return STATE_ALLOCATED;
		else return STATE_FREE;
	}

	void freePage(size_t address)
	{
		if ((address >= PHYS_DMA_MEMORY_1 && address < PHYS_DMA_MEMORY_1 + SIZE_DMA_MEMORY_1)/* || (address >= PHYS_DMA_MEMORY_2 && address < PHYS_DMA_MEMORY_2 + SIZE_DMA_MEMORY_2)*/) {
			freeDMA(address, 4096);
			return;
		}

		size_t page = address / 4096;
		if (getPageState(page) == STATE_FREE) {
			KePanic("FREEING NOT ALLOCATED");
			return;
		}

		--usedPages;
		setPageState(page, STATE_FREE);
	}

	size_t currentPagePointer = 0;
	bool forbidEvictions = false;

	size_t allocatePage()
	{
		size_t first = currentPagePointer;

		while (1) {
			if (getPageState(currentPagePointer) == STATE_FREE) {
				setPageState(currentPagePointer, STATE_ALLOCATED);
				++usedPages;
				//kprintf("allocated page %d / %d. 0x%X\n", usedPages, usablePages, 4096 * currentPagePointer);
				return 4096 * currentPagePointer;
			}

			++currentPagePointer;
			if (currentPagePointer < 0 || currentPagePointer > 1024 * 1024) {
				KePanic("SHIT");
			}
			if (currentPagePointer == 1024 * 1024) {
				currentPagePointer = 0;
			}
			if (currentPagePointer == first) {
				size_t evict = currentTaskTCB->processRelatedTo->vas->scanForEviction();
				if (evict) {
					++usedPages;
					setPageState(evict / 4096, STATE_ALLOCATED);
					return evict;
				}

				KePanic("NO MORE SWAPPABLE PAGES OR DMA! OUT OF MEMORY!");
			}
		}
	}

	size_t allocateContiguousPages(int needed)
	{
		//this one is going to be slow, and only really useful
		//for very early allocations (like the AHCI driver)

		//you can reimplement this later if you need to

		lockScheduler();

		kprintf("Phys::allocateContiguousPages CRITICAL: TODO: pages that we check should be freed if not used!\n");

		int got = 0;
		size_t base;
		while (1) {
			if (got == 0) {
				base = allocatePage();
				got = 1;
			} else {
				size_t pg = allocatePage();
				if (pg == base + got * 4096) {
					++got;
				} else {
					base = pg;
					got = 1;
				}
			}

			if (got == needed) {
				break;
			}
		}

		unlockScheduler();
		return base;
	}

	void allowSegmentToBeUsed(size_t bottom, size_t top)
	{
		if (top > highestMem) {
			highestMem = top;
		}

		//4KB per page
		size_t btmByte = bottom / 4096;
		size_t topByte = top / 4096;

		if (topByte <= btmByte) {
			//very small amount of memory (less than 3 or 4 pages), so skip it
			return;
		}

		while (btmByte < topByte) {
			//clear the byte
			setPageState(btmByte++, STATE_FREE);
		}
	}

	void physicalMemorySetup(uint32_t highestUsedAddr)
	{
		memset(dmaUsage, 0, sizeof(dmaUsage));
		highestUsedAddr = ((uint32_t) (highestUsedAddr / 4096)) * 4096 + 4096;
		kprintf("highestUsedAddr = 0x%X\n", highestUsedAddr);

		if (SIZE_PHYS_PAGE_BITMAP * 8 != 1024 * 1024) {
			KePanic("ASSERT FAILED physicalMemorySetup");
		}
		for (int i = 0; i < SIZE_PHYS_PAGE_BITMAP * 8; ++i) {
			setPageState(i, STATE_ALLOCATED);
		}

		uint64_t* ramTable = (uint64_t*) VIRT_RAM_TABLE;
		uint16_t* ramTableLengthAddr = (uint16_t*) VIRT_RAM_TABLE_SIZE;
		uint16_t ramTableLength = KeReadUnaligned16(ramTableLengthAddr);
		if (ramTableLength == 0) {
			KePanic("No RAM table!");
		}

		for (int i = 0; i < ramTableLength; ++i) {

			uint64_t bottom = *(ramTable + 0);
			uint64_t length = *(ramTable + 1);
			uint64_t top = bottom + length;
			uint64_t type = *(((uint32_t*) ramTable) + 4);

			kprintf("Memory range: 0x%X -> 0x%X (%d)\n", (uint32_t) bottom, (uint32_t) top, (uint32_t) type);

			//check that the high bits are clear
			if ((bottom >> 32) || (top >> 32)) {
				//stop the search if we're accessing memory above 4GB (in 32 bit mode)
				//(I'm not sure the memory bitmap handles any more than 4GB, I'll have to check,
				// otherwise we'll get Win9x style errors with 'too much RAM'
				break;
			}

			bool belowEBDA = false;
			if (type == 1 && bottom <= 0x80000 && 0x80000 < top) {
				bottom = 0x80000;
				if (top >= 0xA0000) {
					top = 0xA0000;
				}
				length = top - bottom;
				belowEBDA = true;
			}

			//check that it is usable, and that at least some of it is in the range
			//we want it to be in
			if (type == 1 && (top >= highestUsedAddr || belowEBDA) && length >= 0x2000) {
				//if it starts too low, move it up, recalculate length
				if (bottom < highestUsedAddr && !belowEBDA) {
					bottom = highestUsedAddr;
					length = top - bottom;
				}

				kprintf("Allowing range to be used: 0x%X -> 0x%X\n", (uint32_t) bottom, (uint32_t) top);

				//allow it to be used
				allowSegmentToBeUsed(bottom, top);

				//keep track to show the user, etc.
				usablePages += length / 4096;

				kprintf("%d pages are usable, %d KB\n", usablePages, usablePages * 4);
			}

			ramTable += 3;	//24 bytes / uint64_t = 3
		}

		/*
		// these all live below 'highestUsedAddr' so they were ignored in the above loop
		//
		// TODO: check that these locations really do exist
		//
		setPageState(6, STATE_FREE);
		setPageState(7, STATE_FREE);
		usablePages += 2;

		//used to be DMA RAM
		for (int i = 0; i < 32; ++i) {
			setPageState(0x60 + i, STATE_FREE);
		}
		usablePages += 32;

		for (int i = 0x140000 / 0x1000; i < 0x1C0000 / 0x1000 && usablePages < 1024; ++i) {
			setPageState(i, STATE_FREE);
			usablePages++;
		}

		for (int i = 0x1D0000 / 0x1000; i < 0x1E0000 / 0x1000 && usablePages < 2048; ++i) {
			setPageState(i, STATE_FREE);
			usablePages++;
		}*/
	}
}