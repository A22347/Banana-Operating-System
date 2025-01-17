#include "hw/diskphys/ata.hpp"
#include "hw/diskctrl/ide.hpp"
#include "krnl/hal.hpp"
#include "hw/acpi.hpp"
#include "krnl/common.hpp"
#include "krnl/physmgr.hpp"
#include "krnl/virtmgr.hpp"
#include "thr/prcssthr.hpp"
#include "hal/vcache.hpp"
#include "hal/logidisk.hpp"
#include "hw/cpu.hpp"

int ataSectorsRead = 0;
int ataSectorsWritten = 0;

#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

ATA::ATA() : PhysicalDisk("ATA Hard Drive", 512)
{
	
}

void ATA::flush(bool lba48)
{
	//send a flush command to the drive
	ide->write(channel, ATA_REG_COMMAND, lba48 ? ATA_CMD_CACHE_FLUSH_EXT : ATA_CMD_CACHE_FLUSH);
	ide->polling(channel, 0);
}

bool ATA::readyForCommand()
{
	//wait for the device to be ready
	int timeout = 0;
	while (ide->read(channel, ATA_REG_ALTSTATUS) & (ATA_SR_BSY | ATA_SR_DRQ)) {
		++timeout;
		if (timeout >= 100) {
			milliTenthSleep(10);
		}
		if (timeout == 2000) {
			return false;
		}
	}

	return true;
}

#define MODE_CHS	0
#define MODE_LBA28	1
#define MODE_LBA48	2
#define MODE_DMA28	3
#define MODE_DMA48	4

int ATA::access(uint64_t lba, int count, void* buffer, bool write)
{
	uint8_t lbaIO[6];
	uint8_t lbaMode;
	uint8_t head;
	uint8_t command;

	bool hasLBA = ide->devices[ideDeviceNum].hasLBA;
	bool hasDMA = 0 && ide->devices[ideDeviceNum].hasDMA && (count >= 8 && count < 64);

	//sort out the what LBA values will be sent depending on
	//the features supported by this drive

	if (!hasDMA) {
		if (hasLBA && (lba >> 28)) {
			//LBA48
			lbaMode = MODE_LBA48;

			for (int i = 0; i < 6; ++i) {
				lbaIO[i] = (lba >> (i << 3)) & 0xFF;
			}

			head = 0;

		} else if (hasLBA) {
			//LBA28
			lbaMode = MODE_LBA28;

			lbaIO[0] = (lba >> 0) & 0xFF;
			lbaIO[1] = (lba >> 8) & 0xFF;
			lbaIO[2] = (lba >> 16) & 0xFF;
			lbaIO[3] = 0;
			lbaIO[4] = 0;
			lbaIO[5] = 0;

			head = (lba >> 24) & 0xF;

		} else {
			//CHS
			lbaMode = MODE_CHS;

			uint8_t sect = (lba % 63) + 1;
			uint16_t cyl = lba / (16 * 63);

			lbaIO[0] = sect;
			lbaIO[1] = (cyl >> 0) & 0xFF;
			lbaIO[2] = (cyl >> 8) & 0xFF;
			lbaIO[3] = 0;
			lbaIO[4] = 0;
			lbaIO[5] = 0;

			head = (lba / 63) % 16;
		}

	} else {		
		//LBA28 (for now...)

		if (lba >> 28) {

		} else {
			lbaMode = MODE_DMA28;

			lbaIO[0] = (lba >> 0) & 0xFF;
			lbaIO[1] = (lba >> 8) & 0xFF;
			lbaIO[2] = (lba >> 16) & 0xFF;
			lbaIO[3] = 0;
			lbaIO[4] = 0;
			lbaIO[5] = 0;

			head = (lba >> 24) & 0xF;
		}
	}

	//disable IRQs
	ide->enableIRQs(channel, false);

	//wait for the drive to be not busy
	if (!readyForCommand()) {
		return 1;
	}

	lockScheduler();

	//select the drive
	if (lbaMode == MODE_CHS) {
		ide->write(channel, ATA_REG_HDDEVSEL, 0xA0 | (drive << 4) | head);

	} else {
		ide->write(channel, ATA_REG_HDDEVSEL, 0xE0 | (drive << 4) | head);
	}

	if (hasDMA) {
		uint64_t* prdt = (uint64_t*) prdtVirtAddr;
		*prdt = (1ULL << 63ULL);
		*prdt |= ataPhysAddr;
		*prdt |= (count * 512) << 32;

		if (write) {
			memcpy((void*)ataVirtAddr, buffer, 512 * count);
		}

		ide->write(channel, ATA_REG_BUSMSTR_COMMAND, write ? 0 : 8);

		uint8_t status = ide->read(channel, ATA_REG_BUSMSTR_STATUS);
		status &= ~(1 << 1);		//clear error flag
		status &= ~(1 << 2);		//clear IRQ received flag
		ide->write(channel, ATA_REG_BUSMSTR_STATUS, status);

		ide->write(channel, ATA_REG_FEATURES, 0);

	} else {
		//set PIO mode
		ide->write(channel, ATA_REG_FEATURES, 0);
	}

	//write LBA48 data
	if (lbaMode == MODE_LBA48) {
		ide->write(channel, ATA_REG_SECCOUNT1, 0);
		ide->write(channel, ATA_REG_LBA3, lbaIO[3]);
		ide->write(channel, ATA_REG_LBA4, lbaIO[4]);
		ide->write(channel, ATA_REG_LBA5, lbaIO[5]);
	}

	//write sector count and LBA (or CHS)
	ide->write(channel, ATA_REG_SECCOUNT0, count);
	ide->write(channel, ATA_REG_LBA0, lbaIO[0]);
	ide->write(channel, ATA_REG_LBA1, lbaIO[1]);
	ide->write(channel, ATA_REG_LBA2, lbaIO[2]);

	//determine which command to send
	if (write && lbaMode == MODE_LBA48) command = ATA_CMD_WRITE_PIO_EXT;
	else if (write && lbaMode == MODE_LBA28) command = ATA_CMD_WRITE_PIO;
	else if (write && lbaMode == MODE_DMA28) command = ATA_CMD_WRITE_DMA;
	else if (!write && lbaMode == MODE_LBA48) command = ATA_CMD_READ_PIO_EXT;
	else if (!write && lbaMode == MODE_LBA28) command = ATA_CMD_READ_PIO;
	else if (!write && lbaMode == MODE_DMA28) command = ATA_CMD_READ_DMA;
	else {
		return 1;
	}
		
	//send the command
	ide->write(channel, ATA_REG_COMMAND, command);
	unlockScheduler();

	if (hasDMA) {
		ide->write(channel, ATA_REG_BUSMSTR_COMMAND, 1 | (write ? 0 : 8));
	}

	if (!hasDMA) {
		int ogcount = count;
		while (count--) {
			uint16_t* buffer16 = (uint16_t*)buffer;

			//wait for the device to be ready
			uint8_t err = ide->polling(channel, 1);

			if (err) {
				ide->printError(channel, drive, err);
				return err;
			}

			//read/write data
			if (write) {
				lockScheduler();
				for (int i = 0; i < 256; ++i) {
					outw(ide->getBase(channel), *buffer16++);
				}
				unlockScheduler();

			} else {
				lockScheduler();
				asm("cld; rep insw" : : "c"(256), "d"(ide->getBase(channel)), "D"(buffer16) : "flags", "memory");
				unlockScheduler();
			}

			buffer = (void*)(((uint8_t*)buffer) + 512);
		}

	} else {
		ide->waitInterrupt(channel);

		uint8_t status = ide->read(channel, ATA_REG_BUSMSTR_STATUS);
		ide->write(channel, ATA_REG_BUSMSTR_COMMAND, 0);

		if (!write) {
			memcpy(buffer, (const void*)ataVirtAddr, 512 * count);
		}
	}

	if (write) {
		flush(lbaMode == MODE_LBA48);
	}

	return 0;
}

void ATA::detectCHS()
{
	ide->devices[ideDeviceNum].size = 0;
	KePanic("ENTER CHS VALUES!");
}

int ATA::open(int __a, int _ideDeviceNum, void* _ide)
{
	//save parameters
	ide = (IDE*) _ide;
	ideDeviceNum = _ideDeviceNum;
	channel = ide->devices[ideDeviceNum].channel;
	drive = ide->devices[ideDeviceNum].drive;
	sectorSize = 512;
	removable = false;

	//set disk size
	if (!ide->devices[ideDeviceNum].hasLBA) {
		detectCHS();
	}
	sizeInKBs = ide->devices[ideDeviceNum].size / 2;

	//reset the drive
	ide->write(channel, ATA_REG_CONTROL, 4);
	asm("nop; nop");
	ide->write(channel, ATA_REG_CONTROL, 0);

	if (ide->devices[ideDeviceNum].hasDMA) {
		ataPhysAddr = Phys::allocateContiguousPages(8);
		kprintf("DEBUG! FATAL ERROR ensure doesn't cross 64K boundary!!!\n");
		ataVirtAddr = Virt::allocateKernelVirtualPages(2);
		Virt::getAKernelVAS()->mapPage(ataPhysAddr, ataVirtAddr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_SUPERVISOR);
	
		prdtPhysAddr = Phys::allocatePage();
		prdtVirtAddr = Virt::allocateKernelVirtualPages(1);
		Virt::getAKernelVAS()->mapPage(prdtPhysAddr, prdtVirtAddr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_SUPERVISOR);

		kprintf("DEBUG! FATAL ERROR if two devices on same channel, as they need to share the PRDT, not create seperate ones!!!\n");
		ide->write(channel, ATA_REG_BUSMSTR_PRDT0, (prdtPhysAddr >> 0) & 0xFF);
		ide->write(channel, ATA_REG_BUSMSTR_PRDT1, (prdtPhysAddr >> 8) & 0xFF);
		ide->write(channel, ATA_REG_BUSMSTR_PRDT2, (prdtPhysAddr >> 16) & 0xFF);
		ide->write(channel, ATA_REG_BUSMSTR_PRDT3, (prdtPhysAddr >> 24) & 0xFF);
	}
	
	//set up logical disks
	startCache();
	createPartitionsForDisk(this);

	return 0;
}

uint16_t combineCharAndColour(char c, uint8_t d)
{
	uint16_t a = d;
	a <<= 8;
	return a | c;
}

uint8_t combineColours(uint8_t a, uint8_t b)
{
	return a | (b << 4);
}

int ATA::read(uint64_t lba, int count, void* buffer)
{
	//check for sane values
	if (count > 255 || count <= 0) {
		return (int) DiskError::BadSectorCount;
	}
	if (buffer == nullptr) {
		return (int) DiskError::BadBuffer;
	}

	//perform the read operation
	int err = access(lba, count, buffer, false);

	//error checking
	if (err) {
		return (int) DiskError::Failure;
	}

#if 0 //#ifdef KERNEL_DEBUG
	ataSectorsRead += count;

	uint16_t* p = (uint16_t*) 0xC20B8000;
	p += 63;

	*p++ = combineCharAndColour((ataSectorsRead / 10000) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsRead / 1000) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsRead / 100) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsRead / 10) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsRead / 1) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	p++;
	*p++ = combineCharAndColour((ataSectorsWritten / 10000) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsWritten / 1000) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsWritten / 100) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsWritten / 10) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsWritten / 1) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
#endif

	return (int) DiskError::Success;
}

int ATA::write(uint64_t lba, int count, void* buffer)
{
	//check for sane values
	if (count > 255 || count <= 0) {
		return (int) DiskError::BadSectorCount;
	}
	if (buffer == nullptr) {
		return (int) DiskError::BadBuffer;
	}

	//perform the read operation
	int err = access(lba, count, buffer, true);

	//error checking
	if (err) {
		return (int) DiskError::Failure;
	}

#if 0 //#ifdef KERNEL_DEBUG
	ataSectorsWritten += count;

	uint16_t* p = (uint16_t*) 0xC20B8000;
	p += 63;

	*p++ = combineCharAndColour((ataSectorsRead / 10000) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsRead / 1000) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsRead / 100) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsRead / 10) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsRead / 1) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	p++;
	*p++ = combineCharAndColour((ataSectorsWritten / 10000) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsWritten / 1000) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsWritten / 100) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsWritten / 10) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
	*p++ = combineCharAndColour((ataSectorsWritten / 1) % 10 + '0', combineColours((uint8_t) 0, (uint8_t) 15));
#endif

	return (int) DiskError::Success;
}

int ATA::close(int a, int b, void* c)
{
	delete cache;
	return 0;
}

void ATA::powerSaving(PowerSavingLevel level)
{

}