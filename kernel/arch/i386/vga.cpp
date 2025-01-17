
#include <arch/i386/hal.hpp>
#include <krnl/main.hpp>

#define VGA_TEXT_MODE_ADDRESS (VIRT_LOW_MEGS + 0xB8000)

void HalConsoleScroll(int fg, int bg)
{
	uint8_t* ptr = (uint8_t*) VGA_TEXT_MODE_ADDRESS;
	for (int y = 1; y < 25; ++y) {
		for (int x = 0; x < 160; ++x) {
			ptr[y * 160 + x - 160] = ptr[y * 160 + x];
			if (y == 24) {
				if (x & 1) {
					ptr[y * 160 + x] = (fg & 0xF) | ((bg & 0xF) << 4);
				} else {
					ptr[y * 160 + x] = ' ';
				}
			}
		}
	}
}

void HalConsoleWriteCharacter(char c, int fg, int bg, int x, int y)
{
	uint16_t word = ((uint8_t) c) | ((fg & 0xF) | ((bg & 0xF) << 4)) << 8;
	uint16_t* ptr = (uint16_t*) VGA_TEXT_MODE_ADDRESS;
	ptr += (y * 80 + x);
	*ptr = word;
}

void HalConsoleCursorUpdate(int x, int y)
{
	uint16_t pos = x + y * 80;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}