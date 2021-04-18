
#include <stdio.h>
#include <stdlib.h>

#include <uwsbe/krnllink.h>

int CHAR_WIDTH = 8;
int CHAR_HEIGHT = 16;

uint16_t* vga;
Window* con;
WsbeScript script;

uint32_t vgaColours[16] = {
	0x000000,
	0x0000AA,
	0x00AA00,
	0x00AAAA,
	0xAA0000,
	0xAA00AA,
	0xAA5500,
	0xAAAAAA,
	0x555555,
	0x5555FF,
	0x55FF55,
	0x55FFFF,
	0xFF5555,
	0xFF55FF,
	0xFFFF55,
	0xFFFFFF
};

void redoPaintScript()
{
	script = wsbeNewScript();
	wsbeBufferFillRect(&script, 0, 0, WSBE_MATH_WIDTH_DEREF, WSBE_MATH_HEIGHT_DEREF, 0);

	uint16_t* v = vga;
	for (int y = 0; y < 25; ++y) {
		for (int x = 0; x < 80; ++x) {
			uint16_t ch = *v++;
			char txt[2];
			txt[0] = ch & 0xFF;
			txt[1] = 0;
			wsbeBufferDrawText(&script, x * CHAR_WIDTH, y * CHAR_HEIGHT, txt, vgaColours[(ch >> 8) & 0xF]);
		}
	}

	wsbeSetRepaintScript(con, script);
	wsbeDeleteScript(script);
}

int main (int argc, char *argv[])
{
	CHAR_WIDTH = 7;
	CHAR_HEIGHT = 12;

	vga = (uint16_t*) calloc(80 * 25 * 2, 1);
	vga[0] = 'T' | (0x0F << 8);
	vga[1] = 'e' | (0x0F << 8);
	vga[2] = 's' | (0x0F << 8);
	vga[3] = 't' | (0x0F << 8);
	vga[80] = 'A' | (0x01 << 8);
	vga[81] = 'B' | (0x02 << 8);
	vga[82] = 'C' | (0x03 << 8);
	vga[83] = 'D' | (0x04 << 8);
	vga[84] = 'E' | (0x05 << 8);
	vga[85] = 'F' | (0x06 << 8);
	vga[160] = 'G' | (0x09 << 8);
	vga[161] = 'H' | (0x0A << 8);
	vga[162] = 'I' | (0x0B << 8);
	vga[163] = 'J' | (0x0C << 8);
	vga[164] = 'K' | (0x0D << 8);
	vga[165] = 'L' | (0x0E << 8);

	vga[320] = '1' | (0x07 << 8);
	vga[321] = '2' | (0x07 << 8);
	vga[322] = '3' | (0x07 << 8);

	Window* win = wsbeCreateWindow(40, 40, 80 * CHAR_WIDTH + 10, 25 + 25 * CHAR_HEIGHT + 10, WIN_TOPLEVELWIN | WIN_NORESIZING);
	wsbeSetWindowTitle(win, "Console");
	wsbeAddWindow(wsbeDesktopWindow(), win);
	wsbePaintWindow(win);

	con = wsbeCreateWindow(5, 30, 80 * CHAR_WIDTH, 25 * CHAR_HEIGHT, WIN_NODECORATION);
	redoPaintScript();
	wsbeAddWindow(win, con);
	wsbePaintWindow(con);

	while (1) {
		;
	}

	return 0;
}