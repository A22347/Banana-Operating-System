
#include "hw/video/vga.hpp"
#include "vm86/vm8086.hpp"
#include "fs/vfs.hpp"
#include "hw/ports.hpp"
#include "core/prcssthr.hpp"
#include "hal/intctrl.hpp"
#include "hw/cpu.hpp"

#pragma GCC optimize ("O0")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")


VGAVideo::VGAVideo() : Video("VGA Display")
{

}

void VGAVideo::setPlane(int pl)
{
	outb(0x3CE, 4);
	outb(0x3CF, pl & 3);

	outb(0x3C4, 2);
	outb(0x3C5, 1 << (pl & 3));
}

int VGAVideo::close(int a, int b, void* c)
{
	return 0;
}

int VGAVideo::open(int a, int b, void* c)
{
	loadVM8086FileAsThread(kernelProcess, "C:/Banana/System/VGASET.COM", 0x0000, 0x90, 0x12, 0x12);
	sleep(1);
	kprintf("loaded vm86 file.\n");

	width = 640;
	height = 480;

	mono = false;

	return 0;
}

uint8_t colLookup[4][4][4] = {
	{
		{0, 0, 1, 1},
		{2, 3, 3, 1},
		{2, 2, 3, 9},
		{10, 10, 11, 11},
	},
	{
		{4, 5, 5, 1},
		{6, 8, 8, 9},
		{2, 2, 3, 9},
		{10, 10, 11, 11},
	},
	{
		{4, 4, 5, 5},
		{6, 6, 13, 13},
		{6, 7, 7, 13},
		{10, 10, 14, 15},
	},
	{
		{12, 12, 13, 13},
		{12, 12, 13, 13},
		{12, 12, 13, 13},
		{14, 14, 15, 15},
	},
};

uint8_t dither16Data[512][2] = {
{0, 0},
{0, 0},
{0, 4},
{0, 4},
{4, 4},
{4, 4},
{4, 4},
{4, 4},
{0, 0},
{0, 6},
{0, 6},
{0, 6},
{4, 6},
{4, 6},
{4, 6},
{4, 6},
{0, 2},
{0, 2},
{2, 4},
{2, 4},
{6, 6},
{6, 6},
{6, 6},
{6, 6},
{0, 2},
{0, 2},
{2, 6},
{2, 6},
{6, 6},
{6, 6},
{6, 6},
{6, 6},
{2, 2},
{2, 6},
{2, 6},
{2, 6},
{2, 6},
{6, 6},
{2, 6},
{12, 14},
{2, 2},
{2, 2},
{2, 6},
{2, 2},
{2, 2},
{6, 6},
{2, 2},
{12, 14},
{2, 2},
{2, 10},
{2, 6},
{2, 6},
{6, 6},
{6, 6},
{7, 14},
{14, 14},
{2, 2},
{2, 2},
{10, 10},
{10, 10},
{10, 14},
{10, 14},
{14, 14},
{14, 14},
{0, 0},
{0, 0},
{0, 4},
{0, 4},
{4, 4},
{4, 4},
{4, 12},
{4, 4},
{0, 0},
{0, 8},
{0, 8},
{0, 12},
{0, 12},
{4, 12},
{4, 12},
{4, 12},
{0, 2},
{0, 8},
{6, 8},
{6, 8},
{6, 8},
{6, 12},
{6, 12},
{12, 12},
{0, 2},
{0, 10},
{0, 10},
{0, 14},
{0, 14},
{4, 14},
{4, 14},
{12, 12},
{2, 2},
{0, 10},
{0, 10},
{0, 14},
{0, 14},
{4, 14},
{4, 14},
{12, 14},
{2, 2},
{2, 10},
{6, 10},
{6, 10},
{6, 10},
{6, 14},
{6, 14},
{12, 14},
{2, 10},
{2, 10},
{2, 10},
{2, 14},
{2, 14},
{2, 14},
{6, 14},
{14, 14},
{2, 2},
{2, 10},
{10, 10},
{10, 10},
{10, 14},
{10, 14},
{14, 14},
{14, 14},
{0, 1},
{0, 1},
{0, 5},
{0, 5},
{4, 5},
{4, 5},
{4, 5},
{4, 5},
{0, 1},
{0, 8},
{1, 6},
{1, 6},
{5, 6},
{5, 6},
{4, 12},
{12, 12},
{0, 3},
{0, 8},
{0, 7},
{0, 7},
{4, 7},
{4, 7},
{6, 12},
{12, 12},
{0, 3},
{0, 10},
{3, 6},
{3, 6},
{6, 7},
{6, 7},
{4, 14},
{12, 12},
{2, 3},
{0, 10},
{3, 6},
{3, 6},
{6, 7},
{6, 7},
{4, 14},
{12, 14},
{2, 3},
{2, 3},
{2, 7},
{2, 7},
{8, 14},
{8, 14},
{6, 14},
{12, 14},
{2, 3},
{2, 10},
{2, 10},
{2, 14},
{2, 14},
{10, 14},
{7, 14},
{14, 14},
{2, 3},
{10, 10},
{10, 10},
{10, 10},
{10, 14},
{10, 14},
{14, 14},
{14, 14},
{0, 1},
{0, 1},
{0, 5},
{0, 5},
{4, 5},
{4, 5},
{4, 5},
{4, 5},
{0, 1},
{0, 9},
{1, 6},
{0, 13},
{0, 13},
{5, 6},
{4, 13},
{12, 12},
{0, 3},
{0, 9},
{0, 7},
{6, 9},
{6, 9},
{4, 7},
{6, 13},
{12, 12},
{0, 3},
{0, 11},
{3, 6},
{0, 15},
{0, 15},
{6, 7},
{4, 15},
{12, 12},
{2, 3},
{0, 11},
{3, 6},
{0, 15},
{0, 15},
{6, 7},
{4, 15},
{12, 14},
{2, 3},
{2, 11},
{2, 7},
{6, 11},
{6, 11},
{8, 14},
{6, 15},
{12, 14},
{2, 3},
{2, 11},
{2, 11},
{2, 15},
{2, 15},
{7, 14},
{7, 14},
{14, 14},
{2, 3},
{10, 10},
{10, 10},
{10, 10},
{10, 14},
{10, 14},
{14, 14},
{14, 14},
{1, 1},
{1, 1},
{1, 5},
{1, 5},
{5, 5},
{5, 5},
{5, 5},
{5, 5},
{1, 1},
{0, 9},
{0, 9},
{0, 13},
{0, 13},
{4, 13},
{4, 13},
{12, 13},
{1, 3},
{0, 9},
{1, 7},
{6, 9},
{6, 9},
{5, 7},
{6, 13},
{12, 13},
{1, 3},
{0, 11},
{1, 7},
{0, 15},
{0, 15},
{5, 7},
{4, 15},
{12, 13},
{3, 3},
{0, 11},
{3, 7},
{0, 15},
{0, 15},
{7, 7},
{4, 15},
{12, 15},
{3, 3},
{2, 11},
{3, 7},
{6, 11},
{6, 11},
{7, 7},
{6, 15},
{12, 15},
{3, 3},
{2, 11},
{2, 11},
{2, 15},
{2, 15},
{7, 14},
{7, 14},
{14, 15},
{3, 3},
{10, 11},
{10, 11},
{10, 11},
{10, 15},
{10, 15},
{14, 15},
{14, 15},
{1, 1},
{1, 1},
{1, 5},
{1, 5},
{5, 5},
{5, 5},
{5, 5},
{5, 5},
{1, 1},
{1, 9},
{1, 5},
{1, 13},
{1, 13},
{5, 5},
{5, 13},
{12, 13},
{1, 3},
{1, 3},
{1, 7},
{1, 7},
{5, 7},
{5, 7},
{6, 13},
{12, 13},
{1, 3},
{1, 11},
{1, 7},
{1, 7},
{5, 7},
{5, 7},
{5, 15},
{12, 13},
{3, 3},
{1, 11},
{3, 7},
{3, 7},
{7, 7},
{7, 7},
{5, 15},
{12, 15},
{3, 3},
{3, 3},
{3, 7},
{3, 7},
{7, 7},
{7, 7},
{6, 15},
{12, 15},
{3, 3},
{3, 11},
{3, 11},
{3, 15},
{3, 15},
{10, 15},
{7, 15},
{14, 15},
{3, 3},
{10, 11},
{10, 11},
{10, 11},
{10, 15},
{10, 15},
{14, 15},
{14, 15},
{1, 1},
{1, 9},
{1, 5},
{1, 5},
{5, 5},
{5, 5},
{5, 13},
{13, 13},
{1, 9},
{1, 9},
{1, 9},
{1, 13},
{1, 13},
{5, 13},
{5, 13},
{5, 13},
{1, 3},
{1, 9},
{1, 9},
{1, 13},
{1, 13},
{5, 13},
{5, 13},
{13, 13},
{1, 3},
{1, 11},
{1, 11},
{1, 15},
{1, 15},
{5, 15},
{5, 15},
{13, 13},
{3, 3},
{1, 11},
{1, 11},
{1, 15},
{1, 15},
{5, 15},
{5, 15},
{13, 15},
{3, 3},
{3, 11},
{3, 11},
{3, 15},
{3, 15},
{9, 15},
{7, 15},
{13, 15},
{3, 11},
{3, 11},
{3, 11},
{3, 15},
{3, 15},
{7, 15},
{7, 15},
{7, 15},
{11, 11},
{3, 11},
{11, 11},
{11, 11},
{11, 15},
{11, 15},
{7, 15},
{15, 15},
{1, 1},
{1, 1},
{1, 5},
{1, 5},
{5, 5},
{5, 5},
{13, 13},
{13, 13},
{1, 1},
{1, 9},
{9, 9},
{9, 9},
{9, 13},
{9, 13},
{5, 13},
{13, 13},
{1, 3},
{9, 9},
{9, 9},
{9, 9},
{9, 13},
{9, 13},
{13, 13},
{13, 13},
{1, 3},
{9, 9},
{9, 9},
{9, 9},
{9, 13},
{9, 13},
{13, 13},
{13, 13},
{3, 3},
{9, 11},
{9, 11},
{9, 11},
{9, 15},
{9, 15},
{13, 15},
{13, 15},
{3, 3},
{9, 11},
{9, 11},
{9, 11},
{9, 15},
{9, 15},
{13, 15},
{13, 15},
{11, 11},
{3, 11},
{11, 11},
{11, 11},
{11, 15},
{11, 15},
{7, 15},
{15, 15},
{11, 11},
{11, 11},
{11, 11},
{11, 11},
{11, 15},
{11, 15},
{15, 15},
{15, 15},
};

int pixelLookup(int source, int addr, int pitch)
{
	return dither16Data[((source & 0xE00000) >> 21) | ((source & 0xE000) >> 10) | ((source & 0xE0) << 1)][(addr + (addr / pitch)) & 1];
}

void VGAVideo::putrect(int x, int y, int w, int h, uint32_t colour)
{
	uint8_t red = (colour >> 22) & 3;
	uint8_t green = (colour >> 14) & 3;
	uint8_t blue = (colour >> 6) & 3;

	int px = pixelLookup(colour, y * width + x, width); //colLookup[red][green][blue];

	int originalX = x;
	int maxY = y + h;
	int originalW = w;

	uint8_t* vram = (uint8_t*) (VIRT_LOW_MEGS + 0xA0000);

	for (; y < maxY; ++y) {
		for (; x < originalX + w; ++x) {
			if (0 && !(x & 7) && ((x + 8) < (originalX + w))) {
				//this works, but it messes up the dithering
				int addr = (y * width + x) >> 3;
				setPlane(0);
				vram[addr] = ((px >> 0) & 1) ? 0xFF : 0;
				setPlane(1);
				vram[addr] = ((px >> 1) & 1) ? 0xFF : 0;
				setPlane(2);
				vram[addr] = ((px >> 2) & 1) ? 0xFF : 0;
				setPlane(3);
				vram[addr] = ((px >> 3) & 1) ? 0xFF : 0;
				x += 7;

			} else {
				putpixel(x, y, colour);
			}
		}
	}
	
}

void VGAVideo::putpixel(int x, int y, uint32_t colour)
{
	uint8_t* vram = (uint8_t*) (VIRT_LOW_MEGS + 0xA0000);

	int addr = y * width + x;

	int bit = 7 - (addr & 7);
	addr >>= 3;

	uint8_t red = (colour >> 22) & 3;
	uint8_t green = (colour >> 14) & 3;
	uint8_t blue = (colour >> 6) & 3;

	int px = pixelLookup(colour, y * width + x, width); //colLookup[red][green][blue];

	int w = ~(1 << bit);
	for (int i = 0; i < 4; ++i) {
		if (mono && i != 0) {
			px >>= 1;
			continue;
		}
		setPlane(i);
		vram[addr] = (vram[addr] & w) | ((px & 1) << bit);
		px >>= 1;
	}
}