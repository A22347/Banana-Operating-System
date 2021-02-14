#include <inttypes.h>
#include <stdlib.h>
#include "desktop.h"
#include "rect.h"


//================| Desktop Class Implementation |================//

//Mouse image data
#define CA 0xFF000000 //Black
#define CB 0xFFFFFFFF //White
#define CD 0x00000000 //Clear

uint8_t mouse_data[MOUSE_WIDTH * MOUSE_HEIGHT / 8 * 2] = {
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
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

Desktop* Desktop_new(Context* context)
{

	//Malloc or fail 
	Desktop* desktop;
	if (!(desktop = (Desktop*) malloc(sizeof(Desktop))))
		return desktop;

	//Initialize the Window bits of our desktop
	if (!Window_init((Window*) desktop, 0, 0, context->width, context->height, WIN_NODECORATION, context)) {

		free(desktop);
		return (Desktop*) 0;
	}

	//Override our paint function
	desktop->window.paint_function = Desktop_paint_handler;

	//Now continue by filling out the desktop-unique properties 
	desktop->window.last_button_state = 0;

	//Init mouse to the center of the screen
	desktop->mouse_x = desktop->window.context->width / 2;
	desktop->mouse_y = desktop->window.context->height / 2;

	return desktop;
}

//Paint the desktop 
void Desktop_paint_handler(Window* desktop_window)
{
	Context_fill_rect(desktop_window->context, 0, 0, desktop_window->context->width, desktop_window->context->height, 0x2A2AD4 /*0x337FFF*/);
}

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
	static int oldMouse;
	int changedWin = 0;
	if (mouse_buttons && !oldMouse) {
		active_window = 0;
		changedWin = 1;
	}
	oldMouse = mouse_buttons;

	//Do the old generic mouse handling
	Window_process_mouse((Window*) desktop, mouse_x, mouse_y, mouse_buttons);

	//Window painting now happens inside of the window raise and move operations

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

	//No more hacky mouse, instead we're going to rather inefficiently 
	//copy the pixels from our mouse image into the framebuffer
	for (y = 0; y < 24; y++) {

		//Make sure we don't draw off the bottom of the screen
		if ((y + mouse_y) >= desktop->window.context->height) {
			break;
		}

		uint32_t wte = *(((uint32_t*) mouse_data) + y + 0);
		uint32_t blk = *(((uint32_t*) mouse_data) + y + 32);
		for (x = 0; x < 24; x++) {

			//Make sure we don't draw off the right side of the screen
			if ((x + mouse_x) >= desktop->window.context->width) {
				break;
			}

			if (blk & 1) {
				screenputpixel(x + mouse_x, y + mouse_y, 0);
				//desktop->window.context->buffer[(y + mouse_y) * desktop->window.context->width + (x + mouse_x)] = 0;
			} else if (wte & 1) {
				screenputpixel(x + mouse_x, y + mouse_y, 0xFFFFFF);
				//desktop->window.context->buffer[(y + mouse_y) * desktop->window.context->width + (x + mouse_x)] = 0xFFFFFF;
			}

			blk >>= 1;
			wte >>= 1;
		}
	}
}
