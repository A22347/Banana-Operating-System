
#include <thr/prcssthr.hpp>
#include "syshandler.hpp"
#include "main.hpp"
#include "window.hpp"
#include <hal/keybrd.hpp>
#include <krnl/atexit.hpp>

extern "C" {
#include "userlink.h"
}

#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")

extern void monikaBsod(char*);
extern void (*guiPanicHandler)(char* message);
extern uint64_t(*systemCallHandlers[])(regs* r);
extern void (*guiKeyboardHandler) (KeyboardToken kt, bool* keystates);
extern void NiKeyhandler(KeyboardToken kt, bool* keystates);
extern void NiShutdownHandler(void*);

void NiInstallSysHooks()
{
	KeRegisterAtexit(NiShutdownHandler, nullptr);

	guiPanicHandler = monikaBsod;
	guiKeyboardHandler = NiKeyhandler;
	systemCallHandlers[(int) SystemCallNumber::WSBE] = NiSystemCallHandler;
}

uint64_t NiLinkCommandDestroyWindow(size_t val, NiLinkWindowStruct* win)
{
	kprintf("NiLinkCommandDestroyWindow\n");

	NIWindow* realwin = (NIWindow*) win->krnlWindow;

	desktop->deleteWindow(realwin);
	desktop->completeRefresh();

	return 0;
}

uint64_t NiLinkCommandCreateWindow(size_t val, NiLinkWindowStruct* win)
{
	kprintf("NiLinkCommandCreateWindow\n");
	NIWindow* realwin = new NIWindow(ctxt, win->x, win->y, win->w, win->h);

	for (int i = 0; i < WIN_MAX_FLAG_DWORDS; ++i) {
		realwin->flags[i] = win->flags[i];
	}

	desktop->addWindow(realwin);
	desktop->completeRefresh();

	win->krnlWindow = realwin;

	return 0;
}

uint64_t NiLinkCommandRedraw(size_t val, NiLinkWindowStruct* win)
{
	NIWindow* realwin = (NIWindow*) win->krnlWindow;

	realwin->rerender();

	realwin->flags[0] &= ~WIN_FLAGS_0_INTERNAL_HAS_BEEN_INVALIDATED;
	if (realwin->flags[0] & WIN_FLAGS_0_DRAW_RESIZE_MARKER) {
		realwin->drawResizeMarker();
	}

	desktop->refreshWindowBounds(realwin);

	return 0;
}

extern "C" {
#include "libk/string.h"
}

uint64_t NiLinkCommandResupplyDesktop(size_t val, uint16_t* data)
{
	if (val == 0 || val >= 0x1000) {
		if (val == 0) {
			memcpy(desktop->desktopBuffer, data, desktop->ctxt->width * desktop->ctxt->height * 4);
		} else {
			int offset = *((int*) val);
			int size = *(((int*) val) + 1);
			int l = *(((int*) val) + 2);
			int r = *(((int*) val) + 3);
			memcpy(desktop->desktopBuffer + offset, data, size * 4);
			int startSc = offset / desktop->ctxt->width;
			int endSc = (offset + size + desktop->ctxt->width - 1) / desktop->ctxt->width;
			desktop->rangeRefresh(startSc, endSc, l, r, nullptr);
			return 4;
		}
		return 0;

	} else if (val == 1) {
		memcpy(((uint8_t*) desktop->desktopDecodeLow) + 128 * 4, data, 128 * 4);
		return (desktop->ctxt->width << 16) | desktop->ctxt->height;
	
	} else if (val == 2) {
		desktop->completeRefresh();
		return 2;
	}

	return -1;
}

extern uint64_t milliTenthsSinceBoot;

uint64_t NiLinkCommandResupplyScanline(size_t val, NiLinkWindowStruct* win, bool paint)
{
	NIWindow* realwin = (NIWindow*) win->krnlWindow;

	realwin->request();

	int start = val & 0xFFFF;
	int end = (val >> 16) & 0xFFFF;

	if (start == 0xFFFF) {
		start = 0;
	}
	if (end == 0xFFFF) {
		end = realwin->height;
	}

	bool boolBuff[2048];

	int k = realwin->width * start;
	for (int j = start; j < end; ++j) {
		boolBuff[j + realwin->ypos] = false;
		for (int i = 0; i < realwin->width; ++i) {
			uint32_t px = win->buffer[k];
			if (px != 0xFFFFFFFF) {
				if (realwin->data32[k] != px) {
					boolBuff[j + realwin->ypos] = true;
				}
				realwin->data32[k] = px;
			}
			++k;
		}
	}

	realwin->flags[0] &= ~WIN_FLAGS_0_INTERNAL_HAS_BEEN_INVALIDATED;
	if (realwin->flags[0] & WIN_FLAGS_0_DRAW_RESIZE_MARKER) {
		realwin->drawResizeMarker();
	}

	if (paint) desktop->refreshWindowBounds(realwin, start, end, boolBuff);
	return 0;
}

uint64_t NiLinkCommandResupplyFramebuffer(size_t val, NiLinkWindowStruct* win)
{
	NiLinkCommandResupplyScanline(0xFFFFFFFF, win, false);
	desktop->completeRefresh();
	return 0;
}

uint64_t NiLinkCommandClearFramebuffer(size_t val, NiLinkWindowStruct* win)
{
	NIWindow* realwin = (NIWindow*) win->krnlWindow;
	realwin->request();
	realwin->drawBasicWindow();
	return 0;
}

uint64_t NiLinkCommandGetEvents(size_t val, NiLinkWindowStruct* win)
{
	NIWindow* realwin = (NIWindow*) win->krnlWindow;
	if (!realwin->getEventCount()) return -1;
	win->evnt = realwin->grabEvent();
	win->x = realwin->xpos;
	win->y = realwin->ypos;
	win->w = realwin->width;
	win->h = realwin->height;
	return realwin->getEventCount();
}

uint64_t NiLinkCommandUpsync(size_t val, NiLinkWindowStruct* win)
{
	NIWindow* realwin = (NIWindow*) win->krnlWindow;
	realwin->xpos = win->x;
	realwin->ypos = win->y;
	realwin->width = win->w;
	realwin->height = win->h;
	realwin->rerender();
	return 0;
}

uint64_t NiLinkCommandReadFlags(size_t val, NiLinkWindowStruct* win)
{
	NIWindow* realwin = (NIWindow*) win->krnlWindow;

	for (int i = 0; i < WIN_MAX_FLAG_DWORDS; ++i) {
		win->flags[i] = realwin->flags[i];
	}
	win->x = realwin->xpos;
	win->y = realwin->ypos;
	win->w = realwin->width;
	win->h = realwin->height;
	return 0;
}

uint64_t NiLinkCommandUpdateFlags(size_t val, NiLinkWindowStruct* win)
{
	NIWindow* realwin = (NIWindow*) win->krnlWindow;

	for (int i = 0; i < WIN_MAX_FLAG_DWORDS; ++i) {
		realwin->flags[i] = win->flags[i];
	}
	return 0;
}

uint8_t* desktopWindowDummy = nullptr;
uint64_t NiLinkCommandBeTheDesktop(size_t val, uint8_t* win)
{
	if (desktopWindowDummy == nullptr) {
		desktopWindowDummy = win;
		return 0;
	}
	return 1;
}

extern uint8_t ___mouse_data[CURSOR_DATA_SIZE * MAX_CURSOR_TYPES];
uint64_t NiLinkCommandSetCursor(size_t val, NiLinkWindowStruct* win)
{
	if (val == LINKCMD_CURSOR_NORMAL)		desktop->cursorOffset = MOUSE_OFFSET_NORMAL;
	if (val == LINKCMD_CURSOR_HOURGLASS)	desktop->cursorOffset = MOUSE_OFFSET_WAIT;
	if (val == LINKCMD_CURSOR_TEXT)			desktop->cursorOffset = MOUSE_OFFSET_TEXT;

	desktop->rangeRefresh(desktop->mouseY, desktop->mouseY + MOUSE_HEIGHT, desktop->mouseX, desktop->mouseX + MOUSE_WIDTH);
	desktop->ctxt->screen->drawCursor(desktop->mouseX, desktop->mouseY, (uint32_t*) (___mouse_data + desktop->cursorOffset), 0);

	return 0;
}

uint64_t NiSystemCallHandler(regs* r)
{
	lockScheduler();
	uint64_t retv = -1;

	currentTaskTCB->processRelatedTo->threads[0].guiTask = true;

	switch (r->ebx) {
	case WSBE_FORCE_INIT_EBX:
		retv = 4;
		break;
	case LINKCMD_CREATE_WINDOW:
		retv = NiLinkCommandCreateWindow((size_t) r->ecx, (NiLinkWindowStruct*) r->edx);
		break;
	case LINKCMD_DESTROY_WINDOW:
		retv = NiLinkCommandDestroyWindow((size_t) r->ecx, (NiLinkWindowStruct*) r->edx);
		break;
	case LINKCMD_RESUPPLY_FRAMEBUFFER:
		retv = NiLinkCommandResupplyFramebuffer((size_t) r->ecx, (NiLinkWindowStruct*) r->edx);
		break;
	case LINKCMD_CLEAR_FRAMEBUFFER:
		retv = NiLinkCommandClearFramebuffer((size_t) r->ecx, (NiLinkWindowStruct*) r->edx);
		break;
	case LINKCMD_GET_EVENTS:
		retv = NiLinkCommandGetEvents((size_t) r->ecx, (NiLinkWindowStruct*) r->edx);
		break;
	case LINKCMD_UPDATE_FLAGS:
		retv = NiLinkCommandUpdateFlags((size_t) r->ecx, (NiLinkWindowStruct*) r->edx);
		break;
	case LINKCMD_REDRAW:
		retv = NiLinkCommandRedraw((size_t) r->ecx, (NiLinkWindowStruct*) r->edx);
		break;
	case LINKCMD_READ_FLAGS:
		retv = NiLinkCommandReadFlags((size_t) r->ecx, (NiLinkWindowStruct*) r->edx);
		break;
	case LINKCMD_RESUPPLY_SCANLINE:
		retv = NiLinkCommandResupplyScanline((size_t) r->ecx, (NiLinkWindowStruct*) r->edx, true);
		break;
	case LINKCMD_RESUPPLY_DESKTOP:
		retv = NiLinkCommandResupplyDesktop((size_t) r->ecx, (uint16_t*) r->edx);
		break;
	case LINKCMD_BE_THE_DESKTOP:
		retv = NiLinkCommandBeTheDesktop((size_t) r->ecx, (uint8_t*) r->edx);
		break;
	case LINKCMD_SET_CURSOR:
		retv = NiLinkCommandSetCursor((size_t) r->ecx, (NiLinkWindowStruct*) r->edx);
		break;
	case LINKCMD_UPSYNC:
		retv = NiLinkCommandUpsync((size_t) r->ecx, (NiLinkWindowStruct*) r->edx);
		break;
	}
	unlockScheduler();
	return retv;
}