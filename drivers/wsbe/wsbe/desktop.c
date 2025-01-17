#include <inttypes.h>
#include <stdlib.h>
#include "desktop.h"
#include "rect.h"


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

bool invertMouse = true;
uint32_t desktopColour = 0x2A2AD4;

void Desktop_set_mouse(Desktop* d, int offset)
{
	d->cursor_data = ___mouse_data + offset;
}

Desktop* Desktop_new(Context* context)
{
	Desktop* desktop = (Desktop*) malloc(sizeof(Desktop));

	//Initialize the Window bits of our desktop
	Window_init((Window*) desktop, 0, 0, context->width, context->height, WIN_NODECORATION, context);

	//Override our paint function
	desktop->window.paint_function = Desktop_paint_handler;

	//Now continue by filling out the desktop-unique properties 
	desktop->window.last_button_state = 0;

	//Init mouse to the center of the screen
	desktop->mouse_x = desktop->window.context->width / 2;
	desktop->mouse_y = desktop->window.context->height / 2;

	Desktop_set_mouse(desktop, MOUSE_OFFSET_NORMAL);

	return desktop;
}


//Paint the desktop 
void Desktop_paint_handler(Window* desktop_window)
{
	extern uint32_t desktopBgCol;
	extern bool desktopUseBgCol;

	if (desktopUseBgCol) {
		Context_fill_rect(desktop_window->context, 0, 0, desktop_window->context->width, desktop_window->context->height, desktopBgCol);
		return;
	}
	
	extern int desktopImageWidth;
	extern int desktopImageHeight;
	extern uint32_t* parsedTGA;

	int numX = desktop_window->width / desktopImageWidth;
	if (desktop_window->width % desktopImageWidth) {
		numX++;
	}

	int numY = desktop_window->height / desktopImageHeight;

	int yRemainder = desktop_window->height % desktopImageHeight;
	

	for (int y = 0; y < numY; ++y) {
		for (int x = 0; x < numX; ++x) {
			Context_draw_bitmap(desktop_window->context, parsedTGA, desktopImageWidth * x, desktopImageHeight * y, desktopImageWidth, desktopImageHeight);
		}
	}

	if (yRemainder) {
		for (int x = 0; x < numX; ++x) {
			Context_draw_bitmap(desktop_window->context, parsedTGA, desktopImageWidth * x, desktop_window->height - yRemainder, desktopImageWidth, yRemainder);
		}
	}
}

int oldMouse;
//Our overload of the Window_process_mouse function used to capture the screen mouse position 
void Desktop_process_mouse(Desktop* desktop, uint16_t mouse_x,
						   uint16_t mouse_y, uint8_t mouse_buttons)
{
	int i, x, y;
	Window* child;
	List* dirty_list;
	Rect* mouse_rect;

	extern Window* active_window;
	Window* old_active = active_window;
	int changedWin = 0;
	if (mouse_buttons && !oldMouse) {
		if (active_window) {
			Message m;
			m.window = active_window;
			m.type = MESSAGE_FOCUS_LEAVE;
			dispatchMessage((Window*) m.window, m);
		}
		active_window = 0;
		changedWin = 1;
	}
	oldMouse = mouse_buttons;

	//Do the old generic mouse handling
	Window_process_mouse((Window*) desktop, mouse_x, mouse_y, mouse_buttons);

	//Build a dirty rect list for the mouse area
	if (!(dirty_list = List_new()))
		return;

	if (!(mouse_rect = Rect_new(desktop->mouse_y, desktop->mouse_x,
		desktop->mouse_y + MOUSE_HEIGHT - 1,
		desktop->mouse_x + MOUSE_WIDTH - 1))) {

		free(dirty_list);
		return;
	}

	List_add(dirty_list, mouse_rect);

	//Do a dirty update for the desktop, which will, in turn, do a 
	//dirty update for all affected child windows
	Window_paint((Window*) desktop, dirty_list, 1);

	//Clean up mouse dirty list
	List_remove_at(dirty_list, 0);
	free(dirty_list);
	free(mouse_rect);

	if (active_window && changedWin && old_active != active_window) {
		Window_update_title(active_window);
	}
	if (old_active && changedWin && old_active != active_window) {
		Window_update_title(old_active);
	}

	//Update mouse position
	desktop->mouse_x = mouse_x;
	desktop->mouse_y = mouse_y;
	
	screendrawcursor(mouse_x, mouse_y, desktop->cursor_data);
}
