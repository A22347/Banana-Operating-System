#include "desktop.hpp"
#include "window.hpp"
#include <krnl/panic.hpp>
#include <krnl/kheap.hpp>
#include <krnl/hal.hpp>
#include <fs/vfs.hpp>
#include <hal/keybrd.hpp>

extern "C" {
#include "userlink.h"
#include <libk/string.h>
}

#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")

uint8_t ___mouse_data[CURSOR_DATA_SIZE * MAX_CURSOR_TYPES] = {
	0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00,
	0x1F, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
	0xFF, 0x01, 0x00, 0x00, 0xFF, 0x03, 0x00, 0x00, 0xFF, 0x07, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00,
	0xF7, 0x00, 0x00, 0x00, 0xF3, 0x00, 0x00, 0x00, 0xE1, 0x01, 0x00, 0x00, 0xE0, 0x01, 0x00, 0x00,
	0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
	0x11, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x81, 0x00, 0x00, 0x00,
	0x01, 0x01, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0xC1, 0x07, 0x00, 0x00, 0x49, 0x00, 0x00, 0x00,
	0x95, 0x00, 0x00, 0x00, 0x93, 0x00, 0x00, 0x00, 0x21, 0x01, 0x00, 0x00, 0x20, 0x01, 0x00, 0x00,
	0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

extern void (*guiMouseHandler) (int xdelta, int ydelta, int buttons, int z);
NIDesktop* mouseDesktop;

bool desktopHasFocus = false;

NiEvent NiCreateEvent(NIWindow* win, int type, bool redraw)
{
	NiEvent evnt;
	evnt.type = type;
	evnt.needsRedraw = redraw;
	evnt.krnlWindow = (void*) win;
	evnt.mouseX = mouseDesktop->mouseX;
	evnt.mouseY = mouseDesktop->mouseY;
	evnt.mouseButtons = mouseDesktop->mouseButtons;
	return evnt;
}

void NiKeyhandler(KeyboardToken kt, bool* keystates)
{
	auto h = mouseDesktop->head->getHead();
	if (!h) return;

	NIWindow* win = h->getValue();
	if (!win) return;

	NiEvent evnt = NiCreateEvent(win, kt.release ? EVENT_TYPE_KEYUP : EVENT_TYPE_KEYDOWN, false);
	evnt.key = (uint16_t)kt.halScancode;

	evnt.ctrl = (keystates[(int)KeyboardSpecialKeys::Ctrl]);
	evnt.shift = (keystates[(int)KeyboardSpecialKeys::Shift]);
	evnt.alt = (keystates[(int)KeyboardSpecialKeys::Alt]);

	if (!desktopHasFocus) {
		win->postEvent(evnt);
	}

	extern uint8_t* desktopWindowDummy;

	if (desktopWindowDummy && desktopHasFocus) {
		NIWindow* a = (NIWindow*) desktopWindowDummy;
		a->postEvent(evnt);
	}
}

void NiHandleMouse(int xdelta, int ydelta, int buttons, int z)
{
	mouseDesktop->handleMouse(xdelta, ydelta, buttons, z);
}

void NiLoadCursors()
{
	File* f = new File("C:/Banana/Cursors/STANDARD.CUR", kernelProcess);
	FileStatus status = f->open(FileOpenMode::Read);
	if (status != FileStatus::Success) {
		kprintf("CURSOR LOAD: BAD 1\n");
		return;
	}

	uint64_t size;
	bool dir;
	f->stat(&size, &dir);
	int read;
	uint8_t* curdata = (uint8_t*) malloc(size);
	f->read(size, curdata, &read);
	if (read != (int) size) {
		kprintf("CURSOR LOAD: BAD 2\n");
		return;
	}

	int numCursors = size / 260;
	for (int i = 0; i < numCursors; ++i) {
		int offset;
		if (!memcmp(curdata + i * 4, "NRML", 4)) {
			offset = MOUSE_OFFSET_NORMAL;
		} else if (!memcmp((char*) curdata + i * 4, "WAIT", 4)) {
			offset = MOUSE_OFFSET_WAIT;
		} else if (!memcmp((char*) curdata + i * 4, "TLDR", 4)) {
			offset = MOUSE_OFFSET_TLDR;
		} else if (!memcmp((char*) curdata + i * 4, "TEXT", 4)) {
			offset = MOUSE_OFFSET_TEXT;
		} else if (!memcmp((char*) curdata + i * 4, "VERT", 4)) {
			offset = MOUSE_OFFSET_VERT;
		} else if (!memcmp((char*) curdata + i * 4, "HORZ", 4)) {
			offset = MOUSE_OFFSET_HORZ;
		} else if (!memcmp((char*) curdata + i * 4, "HAND", 4)) {
			offset = MOUSE_OFFSET_HAND;
		} else {
			kprintf("CURSOR LOAD: BAD 3\n");
			break;
		}

		memcpy(___mouse_data + offset, curdata + numCursors * 4 + i * CURSOR_DATA_SIZE, CURSOR_DATA_SIZE);
	}

	free(curdata);
}

uint32_t NIDesktop::desktopDecode(int val)
{
	if (val >= 0x1000) {
		return val & 0xFFFFFF;

	} else {
		return desktopDecodeLow[val & 0xFF];
	}
}

NIDesktop::NIDesktop(NIContext* context)
{
	ctxt = context;

	mouseX = 30;
	mouseY = 30;
	cursorOffset = MOUSE_OFFSET_NORMAL;

	for (int i = 0; i < 128; ++i) {
		uint32_t b = (i >> 0) & 3;
		uint32_t g = (i >> 2) & 7;
		uint32_t r = (i >> 5) & 3;

		r *= 255; r /= 3;				//R, B are from 0-3
		b *= 255; b /= 3;
		g *= 255; g /= 7;				//G is from 0-7

		desktopDecodeLow[i] = (r << 16) | (g << 8) | b;
	}

	desktopBuffer = (uint32_t*) malloc(4 * ctxt->height * ctxt->width);

	head = new List<NIWindow*>();

	mouseDesktop = this;
	guiMouseHandler = NiHandleMouse;
}


void NIDesktop::addWindow(NIWindow* window)
{
	head->insertAtHead(window);
	refreshWindowBounds(window);
}

void NIDesktop::raiseWindow(NIWindow* window)
{
	head->deleteElement(window);
	head->insertAtHead(window);
	refreshWindowBounds(window);
}

void NIDesktop::deleteWindow(NIWindow* window)
{
	head->deleteElement(window);
	refreshWindowBounds(window);
}

void NIDesktop::rangeRefresh(int top, int bottom, int left, int right, bool* boolBuf)
{
	for (int y = top; y < bottom; ++y) {
		if (boolBuf && !boolBuf[y]) continue;
		if (y >= ctxt->height) break;
		renderScanline(y, left, right);
	}
}

void NIDesktop::completeRefresh()
{
	rangeRefresh(mouseY, mouseY + MOUSE_HEIGHT, mouseX, mouseX + MOUSE_WIDTH);
	
	for (int y = 0; y < ctxt->height; ++y) {
		renderScanline(y, 0, ctxt->width);
	}

	ctxt->screen->drawCursor(mouseX, mouseY, (uint32_t*) (___mouse_data + cursorOffset), 0);
}

void NiShutdownHandler(void* v)
{
	for (int y = 0; y < mouseDesktop->ctxt->screen->getHeight(); ++y) {
		for (int x = 0; x < mouseDesktop->ctxt->screen->getWidth(); ++x) {
			if ((x + y) & 1) mouseDesktop->ctxt->screen->putpixel(x, y, 0);
		}
	}
}

void NIDesktop::refreshWindowBounds(NIWindow* window, int start, int end, bool* boolBuf)
{
	rangeRefresh(mouseY, mouseY + MOUSE_HEIGHT, mouseX, mouseX + MOUSE_WIDTH);
	
	int a = window->ypos < 5 ? 0 : window->ypos - 5;
	int b = window->ypos + window->height > ctxt->height - 6 ? ctxt->height - 1 : window->ypos + window->height + 5;
	int c = window->xpos < 5 ? 0 : window->xpos - 5;
	int d = window->xpos + window->width > ctxt->width - 6 ? ctxt->width - 1 : window->xpos + window->width + 5;
	
	if (start != -1) {
		a = window->ypos + start < 5 ? 0 : window->ypos + start - 5;
	}

	if (end != -1) {
		b = window->ypos + end > ctxt->height - 6 ? ctxt->height - 1 : window->ypos + end + 5;
	}

	rangeRefresh(a, b, c, d, boolBuf);
	ctxt->screen->drawCursor(mouseX, mouseY, (uint32_t*) (___mouse_data + cursorOffset), 0);
}

extern uint64_t milliTenthsSinceBoot;

void NIDesktop::invalidateAllDueToFullscreen(NIWindow* ignoredWindow)
{
	auto curr = head->getHead();
	while (curr->next) {
		if (!curr) break;
		NIWindow* window = curr->getValue();

		if (window && window != ignoredWindow) {
			window->invalidate();
			invalidateAllJustOccured = milliTenthsSinceBoot;
			window->postEvent(NiCreateEvent(window, EVENT_TYPE_REPAINT, true));
		}

		curr = curr->next;
	}
}

NIWindow* NIDesktop::getTopmostWindowAtPixel(int x, int line)
{
	auto curr = head->getHead();
	while (curr->next) {
		if (!curr) break;
		NIWindow* window = curr->getValue();
		if (!window) break;

		if (window->ypos <= line && line < window->ypos + window->height) {
			int ls = window->renderTable[line - window->ypos].leftSkip;
			int rs = window->renderTable[line - window->ypos].rightSkip;
			if (window->xpos + ls <= x && x < window->xpos + window->width - rs) {
				return window;
			}
		}

		curr = curr->next;
	}

	return nullptr;
}

#pragma GCC optimize ("Os")

uint8_t render[2048];
uint8_t shadow[2048];
uint32_t renderData[2048];
NIWindow* movingWin = nullptr;

#define MOVE_TYPE_MOVE			1
#define MOVE_TYPE_RESIZE_BR		2
#define MOVE_TYPE_RESIZE_B		3
#define MOVE_TYPE_RESIZE_R		4

//these should probably be within NIDesktop, but who cares for now
int movingType = 0;
NIWindow* clickonWhenMouseFirstClicked;
NIWindow* prevClickon;

void NIDesktop::handleMouse(int xdelta, int ydelta, int buttons, int z)
{
	static int previousButtons = 0;
	static int moveBaseX = 0;
	static int moveBaseY = 0;
	static uint64_t lastClick = 0;

	//clear old mouse
	rangeRefresh(mouseY, mouseY + MOUSE_HEIGHT, mouseX, mouseX + MOUSE_WIDTH);

	int oldX = mouseX;
	int oldY = mouseY;

	//move mouse
	mouseX += xdelta;
	mouseY += ydelta;

	int oldButtons = mouseButtons;

	mouseButtons = buttons;
	
	//check boundaries
	if (mouseX < 0) mouseX = 0;
	if (mouseY < 0) mouseY = 0;
	if (mouseX > ctxt->width - 1) mouseX = ctxt->width - 1;
	if (mouseY > ctxt->height - 1) mouseY = ctxt->height - 1;

	NIWindow* clickon = getTopmostWindowAtPixel(mouseX, mouseY);

	extern uint8_t* desktopWindowDummy;
	if (clickon || prevClickon) {
		NIWindow* a = clickon;
		if (!clickon) a = prevClickon;

		if (xdelta || ydelta) {
			if (a && (a == clickonWhenMouseFirstClicked || a == prevClickon)) {
				a->postEvent(NiCreateEvent(a, buttons & 1 ? EVENT_TYPE_MOUSE_DRAG : EVENT_TYPE_MOUSE_MOVE, false));
			} else if (clickon) {
				clickonWhenMouseFirstClicked = a;
				clickon->postEvent(NiCreateEvent(clickon, EVENT_TYPE_ENTER, false));
			}

		} else if ((buttons & 1) && !(oldButtons & 1)) {
			if (clickon) clickonWhenMouseFirstClicked = a;
			desktopHasFocus = false;
			a->postEvent(NiCreateEvent(a, EVENT_TYPE_MOUSE_DOWN, false));

			if (!(a->flags[0] & WIN_FLAGS_0_NO_CLOSE) && mouseY > a->ypos && mouseY < a->ypos + 25 && mouseX > a->xpos + a->width - 22 && mouseX < a->xpos + a->width - 3) {
				a->postEvent(NiCreateEvent(a, EVENT_TYPE_DESTROY, false));

			}

		} else if (!(buttons & 1) && (oldButtons & 1)) {
			clickonWhenMouseFirstClicked = nullptr;
			a->postEvent(NiCreateEvent(a, EVENT_TYPE_MOUSE_UP, false));
		}

		if ((buttons & 2) && !(oldButtons & 2)) {
			a->postEvent(NiCreateEvent(a, EVENT_TYPE_RMOUSE_DOWN, false));

		} else if (!(buttons & 2) && (oldButtons & 2)) {
			a->postEvent(NiCreateEvent(a, EVENT_TYPE_RMOUSE_UP, false));
		}

	} else if (desktopWindowDummy) {
		NIWindow* a = (NIWindow*) desktopWindowDummy;
		if ((xdelta || ydelta) && (buttons & 1)) {
			a->postEvent(NiCreateEvent(a, EVENT_TYPE_MOUSE_DRAG, false));

		} else if ((buttons & 1) && !(oldButtons & 1)) {
			desktopHasFocus = true;
			a->postEvent(NiCreateEvent(a, EVENT_TYPE_MOUSE_DOWN, false));

		} else if (!(buttons & 1) && (oldButtons & 1)) {
			a->postEvent(NiCreateEvent(a, EVENT_TYPE_MOUSE_UP, false));
		}

		if ((buttons & 2) && !(oldButtons & 2)) {
			a->postEvent(NiCreateEvent(a, EVENT_TYPE_RMOUSE_DOWN, false));

		} else if (!(buttons & 2) && (oldButtons & 2)) {
			a->postEvent(NiCreateEvent(a, EVENT_TYPE_RMOUSE_UP, false));
		}
	}

	if (movingWin && movingType == MOVE_TYPE_MOVE) {
		bool release = !(buttons & 1) && (previousButtons & 1);

		for (int y = 1; y < movingWin->height - 1; ++y) {
			for (int x = 1; x < movingWin->width - 1; ++x) {
				if (!((x + y) & 63) && !(y & 7)) {
					if (oldX - moveBaseX + x >= 0 && oldX - moveBaseX + x < ctxt->width) rangeRefresh(oldY - moveBaseY + y, oldY - moveBaseY + y + 1, oldX - moveBaseX + x, oldX - moveBaseX + x + 1);
					if (!release && mouseX - moveBaseX + x >= 0 && mouseX - moveBaseX + x < ctxt->width && mouseY - moveBaseY + y < ctxt->height) ctxt->screen->putpixel(mouseX - moveBaseX + x, mouseY - moveBaseY + y, 0);
				}
			}
		}

		if (!release) {
			int x1 = mouseX - moveBaseX;
			if (x1 < 0) x1 = 0;
			int x2 = mouseX - moveBaseX + movingWin->width - x1;
			if (x1 + x2 >= ctxt->width) {
				x2 = ctxt->width - x1;
				if (x2 < 0) x2 = 0;
			}
			if (x2) {
				int ox1 = oldX - moveBaseX;
				if (ox1 < 0) ox1 = 0;
				int ox2 = oldX - moveBaseX + movingWin->width - ox1;
				if (ox1 + ox2 >= ctxt->width) {
					ox2 = ctxt->width - ox1;
					if (ox2 < 0) ox2 = 0;
				}
				rangeRefresh(oldY - moveBaseY, oldY - moveBaseY + 2, ox1, ox1 + ox2);
				rangeRefresh(oldY - moveBaseY + movingWin->height - 2, oldY - moveBaseY + movingWin->height, ox1, ox1 + ox2);

				if (mouseY - moveBaseY < ctxt->height) {
					ctxt->screen->putrect(x1, mouseY - moveBaseY, x2, 1, 0);
					ctxt->screen->putrect(x1, mouseY - moveBaseY + 1, x2, 1, 0xFFFFFF);
				}
				if (mouseY - moveBaseY + movingWin->height - 1 < ctxt->height) {
					ctxt->screen->putrect(x1, mouseY - moveBaseY + movingWin->height - 1, x2, 1, 0);
				}
				if (mouseY - moveBaseY + movingWin->height - 2 < ctxt->height) {
					ctxt->screen->putrect(x1, mouseY - moveBaseY + movingWin->height - 2, x2, 1, 0xFFFFFF);
				}
			}
		}

		if (release) {
			auto win = movingWin;
			movingWin->xpos = mouseX - moveBaseX;
			movingWin->ypos = mouseY - moveBaseY;
			movingWin = nullptr;
			addWindow(win);

			NiEvent evnt = NiCreateEvent(win, EVENT_TYPE_MOVED, true);
			win->postEvent(evnt);
		}
	}

	if (movingWin && (movingType == MOVE_TYPE_RESIZE_BR || movingType == MOVE_TYPE_RESIZE_B || movingType == MOVE_TYPE_RESIZE_R)) {
		bool release = !(buttons & 1) && (previousButtons & 1);

		int oldW = movingWin->width + oldX - moveBaseX;
		int oldH = movingWin->height + oldY - moveBaseY;
		int newW = movingWin->width + mouseX - moveBaseX;
		int newH = movingWin->height + mouseY - moveBaseY;

		if (movingType == MOVE_TYPE_RESIZE_B) newW = oldW = movingWin->width;
		if (movingType == MOVE_TYPE_RESIZE_R) newH = oldH = movingWin->height;

		if (newW < 50) newW = 50;
		if (newH < 50) newH = 50;
		if (oldW < 50) oldW = 50;
		if (oldH < 50) oldH = 50;

		int mxW = oldW > newW ? oldW : newW;
		int mxH = oldH > newH ? oldH : newH;
		for (int y = 1; y < mxH; ++y) {
			for (int x = 1; x < mxW; ++x) {
				if (!((x + y) & 63) && !(y & 7)) {
					if (movingWin->xpos + x >= 0 && movingWin->xpos + x < ctxt->width) rangeRefresh(movingWin->ypos + y, movingWin->ypos + 1 + y, movingWin->xpos + x, movingWin->xpos + 1 + x);
					if (!release && x < newW && y < newH && movingWin->xpos + x >= 0 && movingWin->xpos + x < ctxt->width && movingWin->ypos + y < ctxt->height) ctxt->screen->putpixel(movingWin->xpos + x, movingWin->ypos + y, 0);
				}
			}
		}

		if (!release) {
			if (movingWin->xpos < 0) {
				rangeRefresh(movingWin->ypos, movingWin->ypos + 1, 0, movingWin->xpos + oldW);
				if (movingWin->ypos < ctxt->height) ctxt->screen->putrect(0, movingWin->ypos, newW + movingWin->xpos, 1, 0);
				rangeRefresh(movingWin->ypos + oldH, movingWin->ypos + 1 + oldH, 0, movingWin->xpos + oldW);
				if (movingWin->ypos + newH < ctxt->height) ctxt->screen->putrect(0, movingWin->ypos + newH, newW + movingWin->xpos, 1, 0);
			} else {
				rangeRefresh(movingWin->ypos, movingWin->ypos + 2, movingWin->xpos, movingWin->xpos + oldW);

				if (movingWin->ypos < ctxt->height) {
					ctxt->screen->putrect(movingWin->xpos, movingWin->ypos, newW, 1, 0);
					ctxt->screen->putrect(movingWin->xpos, movingWin->ypos + 1, newW, 1, 0xFFFFFF);
				}

				rangeRefresh(movingWin->ypos + oldH - 1, movingWin->ypos + 1 + oldH, movingWin->xpos, movingWin->xpos + oldW);
				if (movingWin->ypos + newH < ctxt->height) {
					ctxt->screen->putrect(movingWin->xpos, movingWin->ypos + newH, newW, 1, 0);
					ctxt->screen->putrect(movingWin->xpos, movingWin->ypos + newH - 1, newW, 1, 0xFFFFFF);
				}
			}
		}

		if (release) {
			auto win = movingWin;
			movingWin = nullptr;

			NiEvent evnt = NiCreateEvent(win, EVENT_TYPE_RESIZED, true);
			win->width = newW;
			win->height = newH;

			win->rerender();
			addWindow(win);
			refreshWindowBounds(win);
			win->postEvent(evnt);
			cursorOffset = MOUSE_OFFSET_NORMAL;
		}
	}

	if ((buttons & 1) && clickon && clickon == clickonWhenMouseFirstClicked) {
		if (!(previousButtons & 1)) {
			uint64_t sincePrev = milliTenthsSinceBoot - lastClick;

			if (sincePrev < 3000 && mouseY - clickon->ypos < WINDOW_TITLEBAR_HEIGHT && !(clickon->flags[0] & WIN_FLAGS_0_NO_RESIZE)) {
				if (clickon->fullscreen) {
					clickon->xpos = clickon->rstrx;
					clickon->ypos = clickon->rstry;
					clickon->width = clickon->rstrw;
					clickon->height = clickon->rstrh;
					invalidateAllDueToFullscreen(clickon);

				} else {
					clickon->rstrx = clickon->xpos;
					clickon->rstry = clickon->ypos;
					clickon->rstrw = clickon->width;
					clickon->rstrh = clickon->height;

					clickon->xpos = 0;
					clickon->ypos = 0;
					clickon->width = ctxt->width;
					clickon->height = ctxt->height;
				}
				sincePrev = 0;
				clickon->fullscreen ^= 1;
				NiEvent evnt = NiCreateEvent(clickon, EVENT_TYPE_RESIZE_DOWN, true);
				clickon->postEvent(evnt);
				clickon->rerender();
				completeRefresh();

			} else {
				if (clickon != head->getHead()->getValue()) {
					raiseWindow(clickon);
				}
			}

			lastClick = milliTenthsSinceBoot;

		} 
		if (!movingWin && !(previousButtons & 1)) {
			if (mouseY - clickon->ypos > clickon->height - 15 && !clickon->fullscreen && !(clickon->flags[0] & WIN_FLAGS_0_NO_RESIZE)) {
				movingWin = clickon;
				movingType = MOVE_TYPE_RESIZE_B;
				cursorOffset = MOUSE_OFFSET_VERT;
				moveBaseX = mouseX;
				moveBaseY = mouseY;
				deleteWindow(clickon);
			} 
			if (mouseX - clickon->xpos > clickon->width - 15 && !clickon->fullscreen && !(clickon->flags[0] & WIN_FLAGS_0_NO_RESIZE)) {
				if (!movingWin) {
					movingWin = clickon;
					movingType = MOVE_TYPE_RESIZE_R;
					cursorOffset = MOUSE_OFFSET_HORZ;
					moveBaseX = mouseX;
					moveBaseY = mouseY;
					deleteWindow(clickon);
				} else {
					movingType = MOVE_TYPE_RESIZE_BR;
					cursorOffset = MOUSE_OFFSET_TLDR;
				}
			}

			if (!movingWin && mouseY - clickon->ypos < WINDOW_TITLEBAR_HEIGHT && !clickon->fullscreen) {
				movingWin = clickon;
				movingType = MOVE_TYPE_MOVE;
				moveBaseX = mouseX - clickon->xpos;
				moveBaseY = mouseY - clickon->ypos;
				deleteWindow(clickon);
			}
		}
	}

	//paint new mouse
	ctxt->screen->drawCursor(mouseX, mouseY, (uint32_t*) (___mouse_data + cursorOffset), 0);

	previousButtons = buttons;
	prevClickon = clickon;
}

void NIDesktop::renderScanline(int line, int left, int right)
{
	if (line < 0 || line >= ctxt->height) return;

	if (left < 0) left = 0;
	if (left >= ctxt->width) left = ctxt->width - 1;
	if (right < 0) right = 0;
	if (right >= ctxt->width) right = ctxt->width - 1;
	if (left > right) {
		int temp = left;
		left = right;
		right = temp;
	}

	int expectedBytes = right - left;
	int lineOffset = line * ctxt->width;

	memset(render + left, 0, expectedBytes);
	memset(shadow + left, 128, expectedBytes);

	bool wasAnyShadows = false;
	auto curr = head->getHead();
	while (curr->next) {
		if (!curr) break;
		NIWindow* window = curr->getValue();
		if (!window) break;

		if (movingWin == window) {
			curr = curr->next;
			continue;
		}

		if ((window->flags[0] & WIN_FLAGS_0_HIDE_ON_INVALIDATE) &&
			(window->flags[0] & WIN_FLAGS_0_INTERNAL_HAS_BEEN_INVALIDATED)) {
			curr = curr->next;
			continue;
		}

		if (window->flags[0] & WIN_FLAGS_0_HIDDEN) {
			curr = curr->next;
			continue;
		}

		window->request();

		if (line < window->ypos + window->height && !window->fullscreen && !(window->flags[0] & WIN_FLAGS_0_NO_SHADOWS)) {
			for (int i = window->xpos; i < window->xpos + window->width; ++i) {
				if (i < left) continue;
				if (i > right) break;
				int j = line;				
				int z = (window->flags[0] & WIN_FLAGS_0_DRAW_OUTLINE_INSTEAD_OF_SHADOW) ? 2 : 5;

				while (j < window->ypos || \
					   (j - window->ypos >= 0 && window->renderTable[j - window->ypos].leftSkip > i - window->xpos) || \
					   (j - window->ypos >= 0 && i > window->xpos + window->width - window->renderTable[j - window->ypos].rightSkip) \
					   ) {
					++j;
					if (j - line > z) break;
				}
				int diff = j - line;
				if (diff < z && diff > 0) {
					if (!render[i]) {
						diff--;
						wasAnyShadows = true;
						if (window->flags[0] & WIN_FLAGS_0_DRAW_OUTLINE_INSTEAD_OF_SHADOW) {
							shadow[i] = 0;
						} else {
							shadow[i] = ((101 + diff * 8) * shadow[i] / 256) + (101 + diff * 8) / 2;
						}
					}
				}
			}

		} else if (line > window->ypos && !window->fullscreen && !(window->flags[0] & WIN_FLAGS_0_NO_SHADOWS)) {
			for (int i = window->xpos; i < window->xpos + window->width; ++i) {
				if (i < left) continue;
				if (i > right) break;
				int j = line;
				
				int z = (window->flags[0] & WIN_FLAGS_0_DRAW_OUTLINE_INSTEAD_OF_SHADOW) ? 2 : 5;
				do {
					--j;
					if (line - j > z) break;
				} while (j >= window->ypos + window->height || \
						 (j - window->ypos < window->height && window->renderTable[j - window->ypos].leftSkip > i - window->xpos) || \
						 (j - window->ypos < window->height && i > window->xpos + window->width - window->renderTable[j - window->ypos].rightSkip) \
						 );

				int diff = line - j;
				if (diff < z && diff >= 0) {
					if (!render[i]) {
						wasAnyShadows = true;
						if (window->flags[0] & WIN_FLAGS_0_DRAW_OUTLINE_INSTEAD_OF_SHADOW) {
							shadow[i] = 0;
						} else {
							shadow[i] = ((93 + diff * 8) * shadow[i] / 256) + (93 + diff * 8) / 2;
						}
					}
				}
			}
		}
		

		if (window->ypos <= line && line < window->ypos + window->height) {
			int ls = window->renderTable[line - window->ypos].leftSkip;
			int rs = window->renderTable[line - window->ypos].rightSkip;

			if (!window->fullscreen && !(window->flags[0] & WIN_FLAGS_0_NO_SHADOWS)) {
				for (int i = (window->flags[0] & WIN_FLAGS_0_DRAW_OUTLINE_INSTEAD_OF_SHADOW) ? 3 : 1; i < 4; ++i) {
					int j = window->xpos + ls - 4 + i;
					int k = window->xpos + window->width - rs + i - ((window->flags[0] & WIN_FLAGS_0_DRAW_OUTLINE_INSTEAD_OF_SHADOW) ? 3 : 0);
					if (!render[j]) {
						wasAnyShadows = true;
						if (window->flags[0] & WIN_FLAGS_0_DRAW_OUTLINE_INSTEAD_OF_SHADOW) {
							shadow[j] = 0;
						} else {
							shadow[j] = ((125 - i * 8) * shadow[j] / 256) + (125 - i * 8) / 2;
						}
					}
					if (!render[k]) {
						wasAnyShadows = true;
						if (window->flags[0] & WIN_FLAGS_0_DRAW_OUTLINE_INSTEAD_OF_SHADOW) {
							shadow[k] = 0;
						} else {
							shadow[k] = ((101 + i * 8) * shadow[k] / 256) + (101 + i * 8) / 2;
						}
					}
				}
			}

			for (int i = window->xpos + ls; i < window->xpos + window->width - rs; ++i) {
				if (i < left) continue;
				if (i > right) break;
				if (!render[i]) {
					render[i] = 1;
					renderData[i] = window->data32[(line - window->ypos) * window->width + (i - window->xpos)];
					--expectedBytes;
					if (expectedBytes == 0) {
						goto done;
					}
				}
			}
		}

		if (window->fullscreen) break;
		curr = curr->next;
	}

	for (int i = left; i < right; ++i) {
		if (!render[i]) {
			render[i]++;		//if it was zero, now it is one
			renderData[i] = desktopDecode(desktopBuffer[i + lineOffset]);		// 0x55afff;
			--expectedBytes;
			if (expectedBytes == 0) {
				goto done;
			}
		}
	}

done:
	curr = head->getHead();
	if (curr && curr->getValue() && !curr->getValue()->fullscreen && wasAnyShadows) {
		for (int i = left; i < right; ++i) {
			int amount = shadow[i];
			if (amount != 128) {
				uint32_t og = renderData[i];

				uint32_t r = (og >> 16) & 0xFF;
				uint32_t g = (og >> 8) & 0xFF;
				uint32_t b = (og >> 0) & 0xFF;

				r = (r * amount) >> 7;
				g = (g * amount) >> 7;
				b = (b * amount) >> 7;

				renderData[i] = (r << 16) | (g << 8) | b;
			}
		}
	}

	ctxt->screen->bitblit(left, line, left, 0, right - left, 1, 0, renderData);
}
#pragma GCC optimize ("Os")
