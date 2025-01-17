#include "krnl/common.hpp"
#include "hw/cpu.hpp"
#include "krnl/virtmgr.hpp"
#include "krnl/physmgr.hpp"
#include "fs/vfs.hpp"
#include "thr/prcssthr.hpp"
#include "thr/elf.hpp"

VAS* keFirstVAS = nullptr;
#pragma GCC optimize ("O2")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")
//#pragma GCC optimize ("-fpeephole2")

namespace Virt
{
	char swapfileDrive = 'C';
	size_t swapfileSector = 98304;					//the 48 MB mark of the disk
	size_t swapfileLength = 24576;					//12 MBs of swap file
	int swapfileSectorsPerPage = 4096 / 512;
	size_t* swapfileBitmap;

	enum class VirtPageState : uint8_t
	{
		//we only have 1 nibble here

		Free = 0,
		Used = 1,
		Start = 2,
		End = 3,
		StartAndEnd = 4,
		Unusable = 0xF,
	};

	uint8_t* bitmap = (uint8_t*) VIRT_VIRT_PAGE_BITMAP;

	void setPageState(size_t pageNum, VirtPageState state)
	{
		pageNum -= VIRT_HEAP_MIN / 4096;

		size_t byte = pageNum / 2;
		size_t nibble = pageNum & 1;

		if (!nibble) {
			uint8_t* ramAddr = bitmap + byte;
			*ramAddr &= 0xF0;
			*ramAddr |= (uint8_t) state;

		} else {
			uint8_t* ramAddr = bitmap + byte;
			*ramAddr &= 0x0F;
			*ramAddr |= ((uint8_t) state) << 4;
		}
	}

	VirtPageState getPageState(size_t pageNum)
	{
		pageNum -= VIRT_HEAP_MIN / 4096;

		size_t byte = pageNum / 2;
		size_t nibble = pageNum & 1;

		if (!nibble) {
			uint8_t* ramAddr = bitmap + byte;
			return (VirtPageState) (*ramAddr & 0xF);

		} else {
			uint8_t* ramAddr = bitmap + byte;
			return (VirtPageState) ((*ramAddr >> 4) & 0xF);
		}

		return VirtPageState::Unusable;
	}

	size_t pageAllocPtr = VIRT_HEAP_MIN / 4096;

	size_t allocateKernelVirtualPages(int pages)
	{
		bool failures = false;

		size_t inARow = 0;
		size_t firstInRow = 0;

		while (1) {
			VirtPageState state = getPageState(pageAllocPtr);

			if (state == VirtPageState::Free) {
				if (inARow == 0) {
					firstInRow = pageAllocPtr;
				}
				++inARow;

				if (inARow == pages) {
					pageAllocPtr = firstInRow;

					if (pages == 1) {
						setPageState(pageAllocPtr++, VirtPageState::StartAndEnd);
					} else {
						for (size_t i = 0; i < pages; ++i) {
							if (i == 0)						 setPageState(pageAllocPtr++, VirtPageState::Start);
							else if (i == pages - 1)		 setPageState(pageAllocPtr++, VirtPageState::End);
							else							 setPageState(pageAllocPtr++, VirtPageState::Used);
						}
					}


					size_t x = firstInRow * 4096;

					return x;
				}

			} else {
				inARow = 0;
			}
			pageAllocPtr++;

			if (pageAllocPtr > VIRT_HEAP_MAX / 4096) {
				pageAllocPtr = VIRT_HEAP_MIN / 4096;
				if (failures) {
					KePanic("KERNEL VIRTUAL MEMORY EXHAUSTED");
				}
				failures = true;
			}
		}

		return 0;
	}

	void freeKernelVirtualPages(size_t address)
	{
		size_t page = address / 4096;
		bool first = true;

		while (1) {
			VirtPageState state = getPageState(page);

			if (state == VirtPageState::Free) {
				KePanic("FREEING PAGES NOT ALLOCATED");
			}

			size_t* entry = getAKernelVAS()->getPageTableEntry(page * 4096);
			if (*entry & PAGE_ALLOCATED) {
				Phys::freePage(*entry & ~0xFFF);
			}
			*entry = 0;

			if (state == VirtPageState::StartAndEnd) {
				if (first) {
					setPageState(page, VirtPageState::Free);
				} else {
					KePanic("FREEING PAGES START AND END PROBLEM");
				}
				return;

			} else if (state == VirtPageState::Start) {
				if (first) {
					setPageState(page, VirtPageState::Free);
				} else {
					KePanic("FREEING PAGES START PROBLEM");
				}

			} else if (state == VirtPageState::End) {
				if (!first) {
					setPageState(page, VirtPageState::Free);
				} else {
					KePanic("FREEING PAGES END PROBLEM");
				}
				return;

			} else if (state == VirtPageState::Used) {
				setPageState(page, VirtPageState::Free);
			}

			++page;
			first = false;
		}
	}

	void freeSwapfilePage(size_t id)
	{
		swapfileBitmap[id >> 5] &= ~(1 << (id & 0x1F));
	}

	size_t swapIDToSector(size_t id)
	{
		return id * swapfileSectorsPerPage + swapfileSector;
	}

	size_t allocateSwapfilePage()
	{
		size_t pagesLen = swapfileLength / swapfileSectorsPerPage;

		for (size_t i = 0; i < pagesLen; ++i) {
			size_t bitmapIndex = i >> 5;
			size_t bitmask = 1 << (i & 0x1F);

			if (!(swapfileBitmap[bitmapIndex] & bitmask)) {
				swapfileBitmap[bitmapIndex] |= bitmask;
				return i;
			}
		}

		KePanic("NO SWAPFILE SPACE LEFT");
		return 0;
	}

	void virtualMemorySetup()
	{
		int start = VIRT_HEAP_MIN / 4096;
		int end = VIRT_HEAP_MAX / 4096;

		for (int i = start; i < end; ++i) {
			setPageState(i, VirtPageState::Unusable);
		}

		for (int i = start; i < end; ++i) {
			setPageState(i, VirtPageState::Free);
		}

		if (1) {
			for (int i = 0xD0000; i < 0xE0000; ++i) {
				setPageState(i, VirtPageState::Unusable);
			}
		}
	}

	void setupPageSwapping(int megs)
	{
		uint64_t sec = disks['C' - 'A']->fs->allocateSwapfile("C:/Banana/SWAPFILE.SYS", megs);

		swapfileDrive = 'C';
		swapfileLength = megs * 1024 / 512 * 1024;
		swapfileSector = sec;
		swapfileSectorsPerPage = 4096 / 512;
		swapfileBitmap = (size_t*) malloc(swapfileLength / swapfileSectorsPerPage / (sizeof(size_t) * 8));
		memset(swapfileBitmap, 0, swapfileLength / swapfileSectorsPerPage / (sizeof(size_t) * 8));
	}
}

size_t VAS::allocatePages(int count, int flags)
{
	if (supervisorVAS) {
		size_t virt = Virt::allocateKernelVirtualPages(count);
		if (virt >= VIRT_KERNEL_BASE) {
			flags |= PAGE_GLOBAL;
		}
		for (int i = 0; i < count; ++i) {
			size_t phys = Phys::allocatePage();
			mapPage(phys, virt + i * 4096, flags | PAGE_ALLOCATED);
		}

		HalFlushTLB();
		return virt;

	} else {
		if (!sbrk) {
			KePanic("NEED TO LOAD TASK BEFORE ALLOCATING PAGES");
		}

		size_t virt = sbrk;
		sbrk += count * 4096;

		for (int i = 0; i < count; ++i) {
			size_t phys = Phys::allocatePage();
			mapPage(phys, virt + i * 4096, flags | PAGE_ALLOCATED);
		}

		HalFlushTLB();
		return virt;
	}

	return 0;
}

size_t VAS::virtualToPhysical(size_t virt)
{
	return (*getPageTableEntry(virt)) & ~0xFFF;
}

void VAS::freeAllocatedPages(size_t virt)
{
	if (supervisorVAS) {
		Virt::freeKernelVirtualPages(virt);

	} else {
		kprintf("@@@ VAS::freeAllocatedPages TODO!");
	}
}

VAS::VAS()
{
	extern int __start_userkernel;
	extern int __stop_userkernel;

	keFirstVAS = this;

	supervisorVAS = true;
	specialkeFirstVAS = true;
	pageDirectoryBase = (size_t*) VIRT_KRNL_PAGE_DIRECTORY;

	reflagRange(((size_t) &__start_userkernel), \
		(((size_t) &__stop_userkernel) - ((size_t) &__start_userkernel) + 4095) / 4096, \
		- 1, \
		PAGE_USER);
}

VAS::~VAS()
{
	int fp = 0;

	lockScheduler();

	currentTaskTCB->processRelatedTo->vas->mapOtherVASIn(true, this);

	for (int i = 0; i < 256 * 3; ++i) {
		size_t oldEntry = pageDirectoryBase[i];

		if (oldEntry & PAGE_PRESENT) {
			for (int j = 0; j < 1024; ++j) {
				size_t vaddr = ((size_t) i) * 0x400000 + ((size_t) j) * 0x1000;
				size_t* oldPageEntryPtr = currentTaskTCB->processRelatedTo->vas->getForeignPageTableEntry(true, vaddr);
				size_t oldPageEntry = *oldPageEntryPtr;

				if ((oldPageEntry & PAGE_ALLOCATED) && (oldPageEntry & PAGE_PRESENT)) {
					Phys::freePage(oldPageEntry & ~0xFFF);
					++fp;
				}
			}

			if (oldEntry & PAGE_ALLOCATED) {
				Phys::freePage(oldEntry & ~0xFFF);
				++fp;
			}
		}
	}

	Virt::freeKernelVirtualPages((size_t) pageDirectoryBase);
	Phys::freePage(pageDirectoryBasePhysical);
	++fp;

	unlockScheduler();
}

void VAS::setCPUSpecific(size_t physAddr)
{
	//map the CPU specific data in (kernel mode can write to readable pages)
	//mapPage(physAddr, VIRT_CPU_SPECIFIC, PAGE_PRESENT | PAGE_READONLY | PAGE_USER);
}

VAS::VAS(VAS* old)
{
	
}

VAS::VAS(bool kernel)
{
	supervisorVAS = kernel;

	pageDirectoryBasePhysical = Phys::allocatePage();

	//DO NOT mark as allocated, as we shouldn't be able to swap out a page table
	pageDirectoryBase = (size_t*) Virt::getAKernelVAS()->mapRange(pageDirectoryBasePhysical, Virt::allocateKernelVirtualPages(1), 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_SUPERVISOR);

	//make everything non-present to start with
	for (int i = 0; i < 1024; ++i) {
		pageDirectoryBase[i] = PAGE_NOT_PRESENT | PAGE_WRITABLE | PAGE_SUPERVISOR;
	}

	//map in the kernel
	for (int i = 768; i < 1024; ++i) {
		pageDirectoryBase[i] = PAGE_PRESENT | PAGE_USER | (0x100000 + (i - 768) * 4096) | PAGE_GLOBAL;

		if (1 && (i - 768) >= 64 && (i - 768) < 64 * 3) {
			pageDirectoryBase[i] = PAGE_NOT_PRESENT | PAGE_WRITABLE | PAGE_SUPERVISOR;
		}
	}

	pageDirectoryBase[0xC20 / 4] = 0x4003 | PAGE_GLOBAL;

	//the first VAS on each CPU gets called with a different constructor
	setCPUSpecific((size_t) CPU::current()->cpuSpecificPhysAddr);

	//set up recursive mapping (wizardry!)
	pageDirectoryBase[1023] = (size_t) pageDirectoryBasePhysical | PAGE_PRESENT | PAGE_WRITABLE | PAGE_SUPERVISOR | PAGE_GLOBAL;

	if (!strcmp(CPU::current()->getName(), "Intel Pentium")) {
		HalDisableInterrupts();
		mapPage((*getPageTableEntry(CPU::current()->idt.getPointerToInvalidOpcodeEntryForF00F())) & ~0xFFF, CPU::current()->idt.getPointerToInvalidOpcodeEntryForF00F() & ~0xFFF, PAGE_PRESENT | PAGE_SUPERVISOR | PAGE_CACHE_DISABLE);
		HalEnableInterrupts();
	}
}

size_t VAS::mapRange(size_t physicalAddr, size_t virtualAddr, int pages, int flags)
{
	for (int i = 0; i < pages; ++i) {
		mapPage(physicalAddr + i * 4096, virtualAddr + i * 4096, flags);
	}

	HalFlushTLB();
	return virtualAddr;
}

size_t* VAS::getForeignPageTableEntry(bool secondSlot, size_t virt)
{
	size_t pageTableNumber = virt / 0x400000;

	size_t pageNumber = (virt % 0x400000) / 0x1000;
	size_t* pageTable = (size_t*) ((secondSlot ? VIRT_RECURSIVE_SPOT_2 : VIRT_RECURSIVE_SPOT_1) + pageTableNumber * 0x1000);

	return pageTable + pageNumber;
}

size_t* VAS::getPageTableEntry(size_t virt)
{
	size_t pageTableNumber = virt / 0x400000;

	size_t pageNumber = (virt % 0x400000) / 0x1000;
	size_t* pageTable = (size_t*) (0xFFC00000 + pageTableNumber * 0x1000);

	return pageTable + pageNumber;
}

void VAS::reflagRange(size_t virtualAddr, int pages, size_t andFlags, size_t orFlags)
{
	for (int i = 0; i < pages; ++i) {
		size_t* entry = getPageTableEntry(virtualAddr + i * 4096);
		*entry &= andFlags;
		*entry |= orFlags;
	}
}

void VAS::setToWriteCombining(size_t virtualAddr, int pages)
{
	reflagRange(virtualAddr, pages, -1, PAGE_PAT);
}

void VAS::mapOtherVASIn(bool secondSlot, VAS* other)
{
	//recursively map the thingy
	pageDirectoryBase[(secondSlot ? VIRT_RECURSIVE_SPOT_2 : VIRT_RECURSIVE_SPOT_1) / 0x400000] = other->pageDirectoryBasePhysical | PAGE_PRESENT | PAGE_WRITABLE | PAGE_SUPERVISOR;

	//flush TLB
	HalFlushTLB();
}

void VAS::mapForeignPage(bool secondSlot, VAS* other, size_t physicalAddr, size_t virtualAddr, int flags)
{
	if ((virtualAddr | physicalAddr) & 0xFFF) {
		KePanic("UNALIGNED PAGE MAPPING REQUESTED 2");
	}

	size_t pageTableNumber = virtualAddr / 0x400000;

	if (!(other->pageDirectoryBase[pageTableNumber] & PAGE_PRESENT)) {
		//create the page table first
		size_t addr = Phys::allocatePage();

		//clear it
		void* virtaddr = (void*) Virt::getAKernelVAS()->mapRange(addr, Virt::allocateKernelVirtualPages(1), 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_SUPERVISOR);
		memset(virtaddr, 0, 4096);
		Virt::freeKernelVirtualPages((size_t) virtaddr);

		other->pageDirectoryBase[pageTableNumber] = addr | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER | PAGE_ALLOCATED;
	}

	//map it in
	size_t pageNumber = (virtualAddr % 0x400000) / 0x1000;
	size_t* pageTable = (size_t*) ((secondSlot ? VIRT_RECURSIVE_SPOT_2 : VIRT_RECURSIVE_SPOT_1) + pageTableNumber * 0x1000);
	pageTable[pageNumber] = physicalAddr | flags;
}

void VAS::mapPage(size_t physicalAddr, size_t virtualAddr, int flags)
{
	if (virtualAddr < VIRT_KERNEL_BASE) {
		if (CPU::current()->readCR3() != (size_t) pageDirectoryBasePhysical) {
			kprintf("\n\nFATAL 'WARNING':\n    CANNOT MAP NON-KERNEL IN NON-CURRENT VAS.\n    THIS COULD BE A *FATAL ERROR*.\n");
		}
	}

	if ((virtualAddr | physicalAddr) & 0xFFF) {
		KePanic("UNALIGNED PAGE MAPPING REQUESTED");
	}

	size_t pageTableNumber = virtualAddr / 0x400000;

	if (!(pageDirectoryBase[pageTableNumber] & PAGE_PRESENT)) {
		//create the page table first
		size_t addr = Phys::allocatePage();

		//clear the page
		void* virtaddr = (void*) Virt::getAKernelVAS()->mapRange(addr, Virt::allocateKernelVirtualPages(1), 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_SUPERVISOR);
		memset(virtaddr, 0, 4096);
		Virt::freeKernelVirtualPages((size_t) virtaddr);

		pageDirectoryBase[pageTableNumber] = addr | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER | PAGE_ALLOCATED;
	}

	//map it in
	size_t pageNumber = (virtualAddr % 0x400000) / 0x1000;
	size_t* pageTable = (size_t*) (0xFFC00000 + pageTableNumber * 0x1000);

	pageTable[pageNumber] = physicalAddr | flags;
}

int swapBalance = 0;
int twswaps = 0;

void VAS::evict(size_t virt)
{
	extern int irqDisableCounter; kprintf("    --> evict A: %d\n", irqDisableCounter);

	size_t id = Virt::allocateSwapfilePage();

	for (int i = 0; i < Virt::swapfileSectorsPerPage; ++i) {
		disks[Virt::swapfileDrive - 'A']->write(Virt::swapIDToSector(id) + i, 1, ((uint8_t*) virt) + 512 * i);
	}

	size_t* entry = getPageTableEntry(virt);
	size_t physAddr = *entry & ~0xFFF;
	*entry &= ~PAGE_PRESENT;					//not present
	*entry &= ~PAGE_SWAPPABLE;					//clear bit 11
	*entry &= 0xFFFU;							//clear the address
	*entry |= id << 12;							//put the swap ID in

	++swapBalance;

	Phys::freePage(physAddr);

	HalFlushTLB();

	kprintf("\nevicting phys 0x%X, virt 0x%X, swap balance %d\n", physAddr, virt, swapBalance);
	kprintf("Total swaps: %d\n\n", twswaps++);
	extern int irqDisableCounter; kprintf("    --> evict B: %d\n", irqDisableCounter);

}

bool VAS::canLockPage(size_t virtualAddr)
{
	size_t* entryPtr = getPageTableEntry(virtualAddr);
	return ((*entryPtr) & PAGE_SWAPPABLE);
}

void VAS::lockPage(size_t virtualAddr)
{
	size_t* entryPtr = getPageTableEntry(virtualAddr);

	if (!((*entryPtr) & PAGE_PRESENT)) {
		bool wasReloaded = VAS::tryLoadBackOffDisk(*entryPtr & ~0xFFF);
		if (!wasReloaded) {
			KePanic("PAGE CANNOT BE LOCKED, NOT PRESENT OR ON DISK");
		}
	}

	if ((*entryPtr) & PAGE_SWAPPABLE) {
		*entryPtr &= ~PAGE_SWAPPABLE;
	} else {
		KePanic("PAGE CANNOT BE LOCKED, USE canLockPage FIRST");
	}
}

void VAS::unlockPage(size_t virtualAddr)
{
	size_t* entryPtr = getPageTableEntry(virtualAddr);
	if ((*entryPtr) & PAGE_SWAPPABLE) {
		KePanic("PAGE CANNOT BE LOCKED, USE canLockPage FIRST");
	} else {
		*entryPtr |= PAGE_SWAPPABLE;
	}
}

bool VAS::canLockPages(size_t virtualAddr, int pages)
{
	for (int i = 0; i < pages; ++i) {
		if (!canLockPage(virtualAddr)) return false;
		virtualAddr += 4096;
	}
	return true;
}

void VAS::lockPages(size_t virtualAddr, int pages)
{
	for (int i = 0; i < pages; ++i) {
		lockPage(virtualAddr);
		virtualAddr += 4096;
	}
}

void VAS::unlockPages(size_t virtualAddr, int pages)
{
	for (int i = 0; i < pages; ++i) {
		unlockPage(virtualAddr);
		virtualAddr += 4096;
	}
}

bool VAS::tryLoadBackOffDisk(size_t faultAddr)
{
	if (faultAddr >= 0xFFC00000U) {
		KePanic("NESTED PAGE FAULT");
	}

	bool onPageBoundary = (faultAddr & 0xFFF) > 0xFE0;

	faultAddr &= ~0xFFF;
	size_t* entry = getPageTableEntry(faultAddr);
	if (!faultAddr) {
		return false;
	}

	if (entry && ((*entry) & PAGE_COPY_ON_WRITE)) {
		//TODO: check that it was a write operation which caused the page fault

		kprintf("Copy on write handler called.\n");

		size_t oldphys = (*entry) & ~0xFFF;
		size_t newphys = Phys::allocatePage();

		*entry |= PAGE_PRESENT;
		HalFlushTLB();

		uint8_t buffer[4096];
		memcpy(buffer, (const void*) (oldphys), 4096);			// faultAddr & ~0xFFF instead of oldphys???

		*entry &= ~PAGE_COPY_ON_WRITE;
		*entry |= PAGE_WRITABLE;

		*entry &= 0xFFF;
		*entry |= newphys;

		HalFlushTLB();

		memcpy((void*) (faultAddr & ~0xFFF), buffer, 4096);

		return true;
	}

	if (entry && ((*entry) & PAGE_ALLOCATED) && !((*entry) & PAGE_PRESENT)) {
		size_t id = (*entry) >> 12;				//we need the ID
		size_t phys = Phys::allocatePage();		//get a new physical page

		*entry &= 0xFFF;						//clear address
		*entry |= PAGE_PRESENT;					//it is now present
		*entry |= PAGE_SWAPPABLE;				//if it was swapped it had to be swappable we don't need to
												//clear this as the low bit of the ID, as we want it set to 1
		*entry |= phys;

		for (int i = 0; i < Virt::swapfileSectorsPerPage; ++i) {
			kprintf("RELOADING TO HERE: 0x%X\n", ((uint8_t*) faultAddr) + 512 * i);
			disks[Virt::swapfileDrive - 'A']->read(Virt::swapIDToSector(id) + i, 1, ((uint8_t*) faultAddr) + 512 * i);
		}

		--swapBalance;
		kprintf("reloading: 0x%X, %d\n", faultAddr, swapBalance);

		Virt::freeSwapfilePage(id);
		//unlockScheduler();
		HalFlushTLB();

		return true;
	}

	return false;
}

size_t VAS::allocateSharedMemoryWithKernel(size_t pageCount, size_t* krnlVirt)
{
	size_t kvirt = Virt::allocateKernelVirtualPages(pageCount);
	size_t uvirt = this->allocatePages(pageCount, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);

	for (int i = 0; i < pageCount; ++i) {
		size_t phys = Phys::allocatePage();

		mapPage(phys, kvirt, PAGE_PRESENT | PAGE_SUPERVISOR | PAGE_WRITABLE);
		mapPage(phys, uvirt, PAGE_PRESENT | PAGE_USER | PAGE_WRITABLE);
	}

	*krnlVirt = kvirt;

	return uvirt;
}

void VAS::freeSharedMemoryWithKernel(size_t vaddr, size_t krnlVirt)
{
	kprintf("Note: VAS::freeSharedMemoryWithKernel should only be used by kernel code (never a system call)\n");
	kprintf("      (this doesn't mean this has actually happened, just here to remind you so you don't ever let untrusted apps call it)\n");

	Virt::freeKernelVirtualPages(krnlVirt);
}

size_t VAS::scanForEviction()
{
	extern int irqDisableCounter; kprintf("    --> scanForEviction 1: %d\n", irqDisableCounter);

	kprintf("*** scanning for eviction.\n");
	int runs = 0;
	while (1) {
		//first check that this page directory is present
		if ((evictionScanner & 0x3FFFFF) == 0) {
			size_t oldEntry = pageDirectoryBase[evictionScanner / 0x400000];

			if (!(oldEntry & PAGE_PRESENT)) {
				evictionScanner += 0x400000;
				continue;
			}
		}

		//kprintf("scanning for eviction... 0x%X\n", evictionScanner);

		//now we have an actual page directory, check the pages within
		size_t* oldEntry = getPageTableEntry(evictionScanner);
		if ((*oldEntry >> 12) && (*oldEntry & PAGE_SWAPPABLE) && (*oldEntry & PAGE_ALLOCATED)) {
			if (*oldEntry & PAGE_PRESENT) {
				if (*oldEntry & PAGE_ACCESSED) {
					*oldEntry &= ~PAGE_ACCESSED;
				} else {
					size_t ret = *oldEntry & ~0xFFF;
					kprintf("will evict... 0x%X\n", evictionScanner);
					evict(evictionScanner);
					kprintf("did evict... 0x%X\n", evictionScanner);
					kprintf("ret... 0x%X\n", ret);
					evictionScanner += 4096;		//saves a check the next time this gets called

					extern int irqDisableCounter; kprintf("    --> scanForEviction 2: %d\n", irqDisableCounter);

					return ret;
				}
			}
		}

		evictionScanner += 4096;
		if (evictionScanner >= 0xFF000000U) {
			evictionScanner = 0;
			++runs;
			if (runs == 3) {
				return 0;
			}
		}
	}
}

extern uint8_t inb(uint16_t);

extern "C" void mapVASFirstTime()
{
	extern int __start_userkernel;
	extern int __stop_userkernel;

	if (currentTaskTCB->firstTimeEIP == 1) {
		kprintf("STARTING A FORKED TASK.\n");
		return;
	}

	int threadNo = currentTaskTCB->rtid;
	VAS* vas = currentTaskTCB->processRelatedTo->vas;

	//24KB kernel (interrupt handler) stack
	for (int i = 0; i < 6; ++i) {
		vas->mapRange(Phys::allocatePage(), VIRT_APP_STACK_KRNL_TOP - 4096 * (1 + i) - threadNo * SIZE_APP_STACK_TOTAL, 1, PAGE_PRESENT | PAGE_ALLOCATED | PAGE_WRITABLE | PAGE_SUPERVISOR);
	}

	//OLD: 8KB user (or kernel mode task) stack
	//NEW: 128KB user stack
	for (int i = 0; i < 32; ++i) {
		size_t physp = Phys::allocatePage();
		vas->mapRange(physp, VIRT_APP_STACK_USER_TOP - 4096 * (1 + i) - threadNo * SIZE_APP_STACK_TOTAL, 1, PAGE_PRESENT | PAGE_ALLOCATED | PAGE_WRITABLE | (vas->supervisorVAS ? PAGE_SUPERVISOR : PAGE_USER));

		size_t* e = vas->getPageTableEntry(VIRT_APP_STACK_USER_TOP - 4096 * (1 + i) - threadNo * SIZE_APP_STACK_TOTAL);
	}

	vas->reflagRange(((size_t)&__start_userkernel), \
		(((size_t)&__stop_userkernel) - ((size_t)&__start_userkernel) + 4095) / 4096, \
		- 1, \
		PAGE_USER);

	Virt::getAKernelVAS()->reflagRange(((size_t)&__start_userkernel), \
		(((size_t)&__stop_userkernel) - ((size_t)&__start_userkernel) + 4095) / 4096, \
		- 1, \
		PAGE_USER);

	HalFlushTLB();
}