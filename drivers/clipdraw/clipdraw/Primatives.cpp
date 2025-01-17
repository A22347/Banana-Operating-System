
#include "Primatives.hpp"
#include "WSBEFont.hpp"
#include "Font.hpp"

#include <dbg/kconsole.hpp>

#pragma GCC optimize ("Os")

uint32_t drawCharacter(Screen scr, Region rgn, int inX, int inY, uint32_t colour, char c)
{
	int characterHeight = 12;
	int characterWidth = 8;

	int scanline = 0;
	int i = 0;
	while (scanline < rgn.height && scanline < inY + characterHeight) {
		uint32_t dword = rgn.data[i++];
		int numInversions = dword & 0xFFFF;
		int times = dword >> 16;

		uint32_t* data = (uint32_t*) (rgn.data + i);

		i += numInversions;

		bool in = false;
		int x = 0;

		if (times + scanline >= inY) {
			while (x < rgn.width) {
				if (*data == x) {
					++data;
					in ^= true;
					--numInversions;
				}

				int modX = x - inX;
				if (in && modX >= 0 && modX < characterWidth) {
					for (int n = 0; n < times; ++n) {
						int row = scanline + n - inY;
						if (row >= 0 && row < characterHeight) {
							if (((font_array[((int) c) + row * 128] << modX) & 0x80)) {
								videoPutpixel(scr, rgn.relX + x, rgn.relY + scanline + n, colour);
							}
						}
					}
				}

				if (numInversions && *data < (unsigned) inX) {
					x = *data;
				} else {
					x++;
				}

				if (x > inX + characterWidth) {
					break;
				}
			}
		}

		scanline += times;
	}

	return characterWidth | (characterHeight << 16);
}

uint32_t drawFontCharacter(Screen scr, Region clipRgn, int fontHandle, int chr, int x, int y, uint32_t colour)
{
	bool needsFree;
	int realW, realH;
	Region fontRgn = getFontRegion(fontHandle, chr, &needsFree, &realW, &realH);
	fontRgn.relX = x;
	fontRgn.relY = y;
	Region rgn = getRegionIntersection(clipRgn, fontRgn);
	fillRegion(scr, rgn, colour);

	if (needsFree) {
		free(fontRgn.data);
	}
	free(rgn.data);

	return realW | (realH << 16);
}

void blitRegion(Screen scr, Region rgn, int inX, int inY, uint32_t* blitData, int blitWidth, int blitHeight)
{
	int scanline = 0;
	int i = 0;
	while (scanline < rgn.height && scanline < inY + blitHeight) {
		uint32_t dword = rgn.data[i++];
		int numInversions = dword & 0xFFFF;
		int times = dword >> 16;

		uint32_t* data = (uint32_t*) (rgn.data + i);

		i += numInversions;

		bool in = false;
		int x = 0;

		if (times + scanline >= inY) {
			while (x < rgn.width) {
				if (*data == x) {
					++data;
					in ^= true;
					--numInversions;
				}

				int modX = x - inX;
				if (in && modX >= 0 && modX < blitWidth) {
					for (int n = 0; n < times; ++n) {
						int row = scanline + n - inY;
						if (row >= 0 && row < blitHeight) {
							videoPutpixel(scr, rgn.relX + x, rgn.relY + scanline + n, 0xFF0000);		// blitData[row * blitWidth + modX]
						}
					}
				}

				if (numInversions && *data < (unsigned) inX) {
					x = *data;
				} else {
					x++;
				}

				if (x > inX + blitWidth) {
					break;
				}
			}
		}

		scanline += times;
	}
}

void shitBlit(Screen scr, Region rgn, int inX, int inY, uint32_t* blitData, int blitWidth, int blitHeight)
{
	for (int y = rgn.relY; y < rgn.relY + rgn.height; ++y) {
		for (int x = rgn.relX; x < rgn.relX + rgn.width; ++x) {

			int offsX = x - inX;
			int offsY = y - inY;

			if (offsX >= 0 && offsY >= 0 && offsX < 32 && offsY < 32) {
				if (isPointInRegion(rgn, x, y)) {
					videoPutpixel(scr, x, y, blitData[offsY * 32 + offsX]);
				}
			}
		}
	}
}

void fillRegion(Screen scr, Region rgn, uint32_t colour)
{
	int scanline = 0;
	int i = 0;

	while (scanline < rgn.height) {
		uint32_t dword = rgn.data[i++];
		if (dword == 0xFFFFFFFF) {
			return;
		}
		int numInversions = dword & 0xFFFF;
		int times = dword >> 16;

		uint32_t* data = (uint32_t*) (rgn.data + i);
		i += numInversions;

		bool in = false;
		int x = 0;
		while (x < rgn.width) {
			if (*data == x) {
				++data;
				in ^= true;
				--numInversions;
			}

			if (in) {
				//if (rgn.dotted) {
				//	videoDrawRectDotted(scr, rgn.relX + x, rgn.relY + scanline, (*data) - x, times, colour);
				//} else {
					videoDrawRect(scr, rgn.relX + x, rgn.relY + scanline, (*data) - x, times, colour);
				//}
			}

			if (numInversions) {
				x = *data;

			} else if (!in) {		// no more inversions, and we're not in
				break;

			} else {
				++x;
			}
		}

		scanline += times;
	}
}
