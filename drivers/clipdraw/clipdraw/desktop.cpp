#include "desktop.hpp"
#include "window.hpp"
#include <krnl/panic.hpp>
#include <core/kheap.hpp>
#include <krnl/hal.hpp>

extern "C" {
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

void clipdrawHandleMouse(int xdelta, int ydelta, int buttons, int z)
{
	mouseDesktop->handleMouse(xdelta, ydelta, buttons, z);
}

NIDesktop::NIDesktop(NIContext* context)
{
	ctxt = context;

	mouseX = 30;
	mouseY = 30;
	cursorOffset = MOUSE_OFFSET_NORMAL;

	head = new List<NIWindow*>();

	mouseDesktop = this;
	guiMouseHandler = clipdrawHandleMouse;
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

void NIDesktop::rangeRefresh(int top, int bottom, int left, int right)
{
	for (int y = top; y < bottom; ++y) {
		renderScanline(y, left, right);
	}
}

void NIDesktop::completeRefresh()
{
	for (int y = 0; y < ctxt->height; ++y) {
		renderScanline(y, 0, ctxt->width);
	}

	ctxt->screen->drawCursor(mouseX, mouseY, (uint32_t*) (___mouse_data + cursorOffset), 0);
}


void NIDesktop::refreshWindowBounds(NIWindow* window)
{
	rangeRefresh(window->ypos, window->ypos + window->height, window->xpos, window->xpos + window->width);
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



#pragma GCC optimize ("Ofast")

uint8_t render[2048];
uint32_t renderData[2048];
NIWindow* movingWin = nullptr;

extern uint64_t milliTenthsSinceBoot;

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

	//check boundaries
	if (mouseX < 0) mouseX = 0;
	if (mouseY < 0) mouseY = 0;
	if (mouseX > ctxt->width - 1) mouseX = ctxt->width - 1;
	if (mouseY > ctxt->height - 1) mouseY = ctxt->height - 1;

	NIWindow* clickon = getTopmostWindowAtPixel(mouseX, mouseY);

	if (movingWin) {
		bool release = !(buttons & 1) && (previousButtons & 1);

		int thingy = 32;
		/*while (movingWin->height * movingWin->width / thingy > 4096) {
			thingy <<= 1;
		}*/
		thingy--;
		for (int y = 1; y < movingWin->height - 1; ++y) {
			for (int x = 1; x < movingWin->width - 1; ++x) {
				if (!((x + y) & thingy) && !(y & 3)) {
					rangeRefresh(oldY - moveBaseY + y, oldY - moveBaseY + y + 1, oldX - moveBaseX + x, oldX - moveBaseX + x + 1);
					if (!release) ctxt->screen->putpixel(mouseX - moveBaseX + x, mouseY - moveBaseY + y, 0);
				}
			}
		}

		rangeRefresh(oldY - moveBaseY, oldY - moveBaseY + 1, oldX - moveBaseX, oldX - moveBaseX + movingWin->width);
		rangeRefresh(oldY - moveBaseY + movingWin->height - 1, oldY - moveBaseY + movingWin->height, oldX - moveBaseX, oldX - moveBaseX + movingWin->width);

		if (!release) {
			ctxt->screen->putrect(mouseX - moveBaseX, mouseY - moveBaseY, movingWin->width, 1, 0);
			ctxt->screen->putrect(mouseX - moveBaseX, mouseY - moveBaseY + movingWin->height - 1, movingWin->width, 1, 0);
		}

		if (release) {
			auto win = movingWin;
			movingWin->xpos = mouseX - moveBaseX;
			movingWin->ypos = mouseY - moveBaseY;
			movingWin = nullptr;
			addWindow(win);
		}
	}

	if ((buttons & 1) && clickon) {
		if (!(previousButtons & 1)) {
			uint64_t sincePrev = milliTenthsSinceBoot - lastClick;

			if (sincePrev < 3000) {
				if (clickon->fullscreen) {
					clickon->xpos = clickon->rstrx;
					clickon->ypos = clickon->rstry;
					clickon->width = clickon->rstrw;
					clickon->height = clickon->rstrh;

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
				clickon->fullscreen ^= 1;

				clickon->rerender();
				completeRefresh();

			} else {
				if (clickon != head->getHead()->getValue()) {
					raiseWindow(clickon);
				}
			}
			
			lastClick = milliTenthsSinceBoot;

		} else if (!movingWin) {
			if (mouseY - clickon->ypos < WINDOW_TITLEBAR_HEIGHT) {
				movingWin = clickon;
				moveBaseX = mouseX - clickon->xpos;
				moveBaseY = mouseY - clickon->ypos;
				deleteWindow(clickon);
			}
		}
	}

	//paint new mouse
	ctxt->screen->drawCursor(mouseX, mouseY, (uint32_t*) (___mouse_data + cursorOffset), 0);

	previousButtons = buttons;
}

void NIDesktop::renderScanline(int line, int left, int right)
{
	memset(render, 0, sizeof(render));

	int expectedBytes = right - left;

	auto curr = head->getHead();
	while (curr->next) {
		if (!curr) break;
		NIWindow* window = curr->getValue();
		if (!window) break;
		if (movingWin == window) {
			curr = curr->next;
			continue;
		}

		if (window->ypos <= line && line < window->ypos + window->height) {
			int ls = window->renderTable[line - window->ypos].leftSkip;
			int rs = window->renderTable[line - window->ypos].rightSkip;
			for (int i = window->xpos + ls; i < window->xpos + window->width - rs; ++i) {
				if (!render[i] && i >= left && i < right) {
					render[i] = 1;
					renderData[i] = window->renderTable[line - window->ypos].data32[i - window->xpos];
					--expectedBytes;
					if (expectedBytes == 0) {
						goto done;
					}
				}
			}
		}

		curr = curr->next;
	}

	for (int i = left; i < right; ++i) {
		if (!render[i]) {
			render[i] = 1;
			renderData[i] = 0x5580FF;
			--expectedBytes;
			if (expectedBytes == 0) {
				goto done;
			}
		}
	}

done:
	for (int i = left; i < right; ++i) {
		ctxt->screen->putpixel(i, line, renderData[i]);
	}
}
#pragma GCC optimize ("Os")
