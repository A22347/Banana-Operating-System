#include <inttypes.h>
#include "context.h"
#include "rect.h"
#include "font.h"

#include "libk/string.h"


//================| Context Class Implementation |================//

//Constructor for our context
Context* Context_new(uint16_t width, uint16_t height, uint32_t* buffer) {

    //Attempt to allocate
    Context* context;
    if(!(context = (Context*)malloc(sizeof(Context))))
        return context; 

    //Attempt to allocate new rect list 
    if(!(context->clip_rects = List_new())) {

        free(context);
        return (Context*)0;
    }

    //Finish assignments
    context->width = width; 
    context->height = height; 
    context->buffer = buffer;
    context->clipping_on = 0;

    return context;
}

void Context_clipped_rect(Context* context, int x, int y, unsigned int width,
                          unsigned int height, Rect* clip_area, uint32_t color) {

    int cur_x;
    int max_x = x + width;
    int max_y = y + height;

    //Translate the rectangle coordinates by the context translation values
    x += context->translate_x;
    y += context->translate_y;
    max_x += context->translate_x;
    max_y += context->translate_y;

    //Make sure we don't go outside of the clip region:
    if(x < clip_area->left)
        x = clip_area->left;
    
    if(y < clip_area->top)
        y = clip_area->top;

    if(max_x > clip_area->right + 1)
        max_x = clip_area->right + 1;

    if(max_y > clip_area->bottom + 1)
        max_y = clip_area->bottom + 1;

    screenputrect(x, y, max_x, max_y, color);
}

//Simple for-loop rectangle into a context
void Context_fill_rect(Context* context, int x, int y,  
                      unsigned int width, unsigned int height, uint32_t color) {

    int max_x = x + width;
    int max_y = y + height;
    int i;
    Rect* clip_area;
    Rect screen_area;

    //Fix from last time: Make sure we don't try to draw offscreen
    if(max_x > context->width)
        max_x = context->width;

    if(max_y > context->height)
        max_y = context->height;
   
    if(x < 0)
        x = 0;
    
    if(y < 0)
        y = 0;
    
    width = max_x - x;
    height = max_y - y;    

    //If there are clipping rects, draw the rect clipped to
    //each of them. Otherwise, draw unclipped (clipped to the screen)
    if(context->clip_rects->count) {
       
        for(i = 0; i < context->clip_rects->count; i++) {    

            clip_area = (Rect*)List_get_at(context->clip_rects, i);
            Context_clipped_rect(context, x, y, width, height, clip_area, color);
        }
    } else {

        if(!context->clipping_on) {

            screen_area.top = 0;
            screen_area.left = 0;
            screen_area.bottom = context->height - 1;
            screen_area.right = context->width - 1;
            Context_clipped_rect(context, x, y, width, height, &screen_area, color);
        }
    }
}

//A horizontal line as a filled rect of height 1
void Context_horizontal_line(Context* context, int x, int y,
                             unsigned int length, uint32_t color) {

    Context_fill_rect(context, x, y, length, 1, color);
}

//A vertical line as a filled rect of width 1
void Context_vertical_line(Context* context, int x, int y,
                           unsigned int length, uint32_t color) {

    Context_fill_rect(context, x, y, 1, length, color);
}

//Rectangle drawing using our horizontal and vertical lines
void Context_draw_rect(Context* context, int x, int y, 
                       unsigned int width, unsigned int height, uint32_t color) {

    Context_horizontal_line(context, x, y, width, color); //top
    Context_vertical_line(context, x, y + 1, height - 2, color); //left 
    Context_horizontal_line(context, x, y + height - 1, width, color); //bottom
    Context_vertical_line(context, x + width - 1, y + 1, height - 2, color); //right
}

//Update the clipping rectangles to only include those areas within both the
//existing clipping region AND the passed Rect
void Context_intersect_clip_rect(Context* context, Rect* rect) {

    int i;
    List* output_rects;
    Rect* current_rect;
    Rect* intersect_rect;
 
    context->clipping_on = 1;

    if(!(output_rects = List_new()))
        return;

    for(i = 0; i < context->clip_rects->count; i++) {

        current_rect = (Rect*)List_get_at(context->clip_rects, i);
        intersect_rect = Rect_intersect(current_rect, rect);

        if(intersect_rect)
            List_add(output_rects, intersect_rect);
    }

    //Delete the original rectangle list
    while(context->clip_rects->count)
        free(List_remove_at(context->clip_rects, 0));
    free(context->clip_rects);

    //And re-point it to the new one we built above
    context->clip_rects = output_rects;

    //Free the input rect
    free(rect);
}

//split all existing clip rectangles against the passed rect
void Context_subtract_clip_rect(Context* context, Rect* subtracted_rect) {

    //Check each item already in the list to see if it overlaps with
    //the new rectangle
    int i, j;
    Rect* cur_rect;
    List* split_rects;

    context->clipping_on = 1;

    for(i = 0; i < context->clip_rects->count; ) {

        cur_rect = List_get_at(context->clip_rects, i);

        //Standard rect intersect test (if no intersect, skip to next)
        //see here for an example of why this works:
        //http://stackoverflow.com/questions/306316/determine-if-two-rectangles-overlap-each-other#tab-top
        if(!(cur_rect->left <= subtracted_rect->right &&
		   cur_rect->right >= subtracted_rect->left &&
		   cur_rect->top <= subtracted_rect->bottom &&
		   cur_rect->bottom >= subtracted_rect->top)) {

            i++;
            continue;
        }

        //If this rectangle does intersect with the new rectangle, 
        //we need to split it
        List_remove_at(context->clip_rects, i); //Original will be replaced w/splits
        split_rects = Rect_split(cur_rect, subtracted_rect); //Do the split
        free(cur_rect); //We can throw this away now, we're done with it

        //Copy the split, non-overlapping result rectangles into the list 
        while(split_rects->count) {

            cur_rect = (Rect*)List_remove_at(split_rects, 0);
            List_add(context->clip_rects, cur_rect);
        }

        //Free the empty split_rect list 
        free(split_rects);

        //Since we removed an item from the list, we need to start counting over again 
        //In this way, we'll only exit this loop once nothing in the list overlaps 
        i = 0;    
    }
}

void Context_add_clip_rect(Context* context, Rect* added_rect) {
    
    Context_subtract_clip_rect(context, added_rect);

    //Now that we have made sure none of the existing rectangles overlap
    //with the new rectangle, we can finally insert it 
    List_add(context->clip_rects, added_rect);
}

//Remove all of the clipping rects from the passed context object
void Context_clear_clip_rects(Context* context) {

    Rect* cur_rect;

    context->clipping_on = 0;

    //Remove and free until the list is empty
    while(context->clip_rects->count) {

        cur_rect = (Rect*)List_remove_at(context->clip_rects, 0);
        free(cur_rect);
    }
}


#define CHARS 255
#define FONTS 1
#define CELLH 14

uint8_t reverse(uint8_t b);

extern int System;            //the system font
extern uint16_t Fonts[FONTS][CHARS][CELLH];
extern uint8_t FontWidths[FONTS][CHARS];

struct FONTDATA1
{
    char chr;
    uint8_t font[14];
    uint8_t size;
};

struct FONTDATA2
{
    char signature[16];
    struct FONTDATA1 FONTD[256];
};

struct FONTDATA2 FONT;

uint8_t reverse(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

int System = 0;            //the system font
uint16_t Fonts[FONTS][CHARS][CELLH];
uint8_t FontWidths[FONTS][CHARS];

struct FONTDATA2 FONT;

char systemFontBuiltin[] = {
	"\x46\x4F\x4E\x54\x20\x46\x49\x4C\x45\x20\x46\x4F\x52\x4D\x41\x54"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x06\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x07\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x09\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x0A\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x0B\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x0C\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x0D\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x0E\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x0F\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x11\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x12\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x13\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x14\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x15\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x16\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x17\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x18\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x19\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x1A\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x1B\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x1C\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x1D\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x1E\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x1F\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x20\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04"
	"\x21\x00\x00\x80\x80\x80\x80\x80\x80\x80\x00\x80\x00\x00\x00\x03"
	"\x22\x00\x00\xA0\xA0\xA0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03"
	"\x23\x00\x14\x14\x14\x7E\x28\x28\xFC\x50\x50\x50\x00\x00\x00\x09"
	"\x24\x00\x20\x70\xA8\xA0\x60\x30\x28\xA8\x70\x20\x00\x00\x00\x06"
	"\x25\x00\x60\x91\x92\x64\x08\x10\x26\x49\x89\x06\x00\x00\x00\x09"
	"\x26\x00\x30\x48\x48\x48\x30\x50\x88\x8A\x84\x72\x00\x00\x00\x08"
	"\x27\x00\x00\x80\x80\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03"
	"\x28\x00\x20\x40\x40\x80\x80\x80\x80\x80\x40\x40\x20\x00\x00\x03"
	"\x29\x00\x80\x40\x40\x20\x20\x20\x20\x20\x40\x40\x80\x00\x00\x03"
	"\x2A\x00\x00\x00\xA0\x40\xA0\x00\x00\x00\x00\x00\x00\x00\x00\x03"
	"\x2B\x00\x00\x00\x00\x00\x20\x20\xF8\x20\x20\x00\x00\x00\x00\x06"
	"\x2C\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x40\x80\x00\x00\x03"
	"\x2D\x00\x00\x00\x00\x00\x00\x00\xC0\x00\x00\x00\x00\x00\x00\x03"
	"\x2E\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00\x00\x03"
	"\x2F\x00\x08\x08\x10\x10\x20\x20\x40\x40\x80\x80\x00\x00\x00\x06"
	"\x30\x00\x00\x70\x88\x88\x88\x88\x88\x88\x88\x70\x00\x00\x00\x06"
	"\x31\x00\x00\x20\xE0\x20\x20\x20\x20\x20\x20\x20\x00\x00\x00\x06"
	"\x32\x00\x00\x70\x88\x08\x08\x10\x20\x40\x80\xF8\x00\x00\x00\x06"
	"\x33\x00\x00\x70\x88\x08\x08\x30\x08\x08\x88\x70\x00\x00\x00\x06"
	"\x34\x00\x00\x30\x50\x50\x90\x90\xF8\x10\x10\x10\x00\x00\x00\x06"
	"\x35\x00\x00\xF8\x80\x80\xF0\x08\x08\x08\x88\x70\x00\x00\x00\x06"
	"\x36\x00\x00\x70\x88\x80\x80\xF0\x88\x88\x88\x70\x00\x00\x00\x06"
	"\x37\x00\x00\xF8\x08\x10\x10\x20\x20\x40\x40\x40\x00\x00\x00\x06"
	"\x38\x00\x00\x70\x88\x88\x88\x70\x88\x88\x88\x70\x00\x00\x00\x06"
	"\x39\x00\x00\x70\x88\x88\x88\x78\x08\x08\x88\x70\x00\x00\x00\x06"
	"\x3A\x00\x00\x00\x00\x00\x80\x00\x00\x00\x00\x80\x00\x00\x00\x03"
	"\x3B\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x40\x80\x00\x00\x03"
	"\x3C\x00\x00\x00\x00\x10\x20\x40\x80\x40\x20\x10\x00\x00\x00\x06"
	"\x3D\x00\x00\x00\x00\x00\x00\xF8\x00\xF8\x00\x00\x00\x00\x00\x06"
	"\x3E\x00\x00\x00\x00\x80\x40\x20\x10\x20\x40\x80\x00\x00\x00\x06"
	"\x3F\x00\x00\x70\x88\x08\x08\x10\x20\x20\x00\x20\x00\x00\x00\x06"
	"\x40\x00\x00\x18\x66\x41\x9D\xA5\xA5\x9B\x40\x60\x1C\x00\x00\x09"
	"\x41\x00\x00\x10\x10\x28\x28\x44\x44\x7C\x82\x82\x00\x00\x00\x09"
	"\x42\x00\x00\xF0\x88\x88\x88\xF0\x88\x88\x88\xF0\x00\x00\x00\x07"
	"\x43\x00\x00\x78\x84\x80\x80\x80\x80\x80\x84\x78\x00\x00\x00\x07"
	"\x44\x00\x00\xF0\x88\x84\x84\x84\x84\x84\x88\xF0\x00\x00\x00\x08"
	"\x45\x00\x00\xF8\x80\x80\x80\xF0\x80\x80\x80\xF8\x00\x00\x00\x07"
	"\x46\x00\x00\xF8\x80\x80\x80\xF0\x80\x80\x80\x80\x00\x00\x00\x06"
	"\x47\x00\x00\x78\x84\x80\x80\x9C\x84\x84\x8C\x74\x00\x00\x00\x08"
	"\x48\x00\x00\x84\x84\x84\x84\xFC\x84\x84\x84\x84\x00\x00\x00\x08"
	"\x49\x00\x00\x80\x80\x80\x80\x80\x80\x80\x80\x80\x00\x00\x00\x03"
	"\x4A\x00\x00\x10\x10\x10\x10\x10\x10\x90\x90\x60\x00\x00\x00\x06"
	"\x4B\x00\x00\x88\x90\xA0\xC0\xC0\xA0\x90\x88\x84\x00\x00\x00\x07"
	"\x4C\x00\x00\x80\x80\x80\x80\x80\x80\x80\x80\xF8\x00\x00\x00\x06"
	"\x4D\x00\x00\x82\x82\xC6\xC6\xAA\xAA\x92\x92\x82\x00\x00\x00\x09"
	"\x4E\x00\x00\x84\xC4\xC4\xA4\xA4\x94\x8C\x8C\x84\x00\x00\x00\x08"
	"\x4F\x00\x00\x78\x84\x84\x84\x84\x84\x84\x84\x78\x00\x00\x00\x08"
	"\x50\x00\x00\xF8\x84\x84\x84\xF8\x80\x80\x80\x80\x00\x00\x00\x07"
	"\x51\x00\x00\x78\x84\x84\x84\x84\x84\x94\x8C\x78\x04\x00\x00\x08"
	"\x52\x00\x00\xF8\x84\x84\x84\xF8\x84\x84\x84\x84\x00\x00\x00\x08"
	"\x53\x00\x00\x70\x88\x80\x80\x70\x08\x08\x88\x70\x00\x00\x00\x07"
	"\x54\x00\x00\xF8\x20\x20\x20\x20\x20\x20\x20\x20\x00\x00\x00\x07"
	"\x55\x00\x00\x84\x84\x84\x84\x84\x84\x84\x84\x78\x00\x00\x00\x07"
	"\x56\x00\x00\x82\x82\x44\x44\x44\x28\x28\x10\x10\x00\x00\x00\x08"
	"\x57\x00\x00\x82\x82\x82\x44\x54\x54\x54\x28\x28\x00\x00\x00\x09"
	"\x58\x00\x00\x82\x82\x44\x28\x10\x28\x44\x82\x82\x00\x00\x00\x08"
	"\x59\x00\x00\x82\x82\x44\x28\x10\x10\x10\x10\x10\x00\x00\x00\x08"
	"\x5A\x00\x00\xFE\x02\x04\x08\x10\x20\x40\x80\xFE\x00\x00\x00\x08"
	"\x5B\x00\x00\xC0\x80\x80\x80\x80\x80\x80\x80\xC0\x00\x00\x00\x03"
	"\x5C\x00\x80\x80\x40\x40\x20\x20\x10\x10\x08\x08\x00\x00\x00\x06"
	"\x5D\x00\x00\xC0\x40\x40\x40\x40\x40\x40\x40\xC0\x00\x00\x00\x03"
	"\x5E\x00\x20\x50\x88\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x06"
	"\x5F\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFC\x00\x00\x00\x06"
	"\x60\x00\x00\x80\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03"
	"\x61\x00\x00\x00\x00\x00\x70\x08\x78\x88\x88\x78\x00\x00\x00\x06"
	"\x62\x00\x00\x80\x80\x80\xF0\x88\x88\x88\x88\xF0\x00\x00\x00\x06"
	"\x63\x00\x00\x00\x00\x00\x70\x88\x80\x80\x88\x70\x00\x00\x00\x06"
	"\x64\x00\x00\x08\x08\x08\x78\x88\x88\x88\x88\x78\x00\x00\x00\x06"
	"\x65\x00\x00\x00\x00\x00\x70\x88\xF8\x80\x88\x70\x00\x00\x00\x06"
	"\x66\x00\x00\x40\x80\x80\xC0\x80\x80\x80\x80\x80\x00\x00\x00\x03"
	"\x67\x00\x00\x00\x00\x00\x78\x88\x88\x88\x88\x78\x08\xF0\x00\x06"
	"\x68\x00\x00\x80\x80\x80\xB0\xC8\x88\x88\x88\x88\x00\x00\x00\x06"
	"\x69\x00\x00\x80\x00\x00\x80\x80\x80\x80\x80\x80\x00\x00\x00\x03"
	"\x6A\x00\x00\x40\x00\x00\x40\x40\x40\x40\x40\x40\x40\x80\x00\x03"
	"\x6B\x00\x00\x80\x80\x80\x90\xA0\xC0\xA0\x90\x88\x00\x00\x00\x06"
	"\x6C\x00\x00\x80\x80\x80\x80\x80\x80\x80\x80\x80\x00\x00\x00\x02"
	"\x6D\x00\x00\x00\x00\x00\xEC\x92\x92\x92\x92\x92\x00\x00\x00\x08"
	"\x6E\x00\x00\x00\x00\x00\xB0\xC8\x88\x88\x88\x88\x00\x00\x00\x06"
	"\x6F\x00\x00\x00\x00\x00\x70\x88\x88\x88\x88\x70\x00\x00\x00\x06"
	"\x70\x00\x00\x00\x00\x00\xF0\x88\x88\x88\x88\xF0\x80\x80\x00\x06"
	"\x71\x00\x00\x00\x00\x00\x78\x88\x88\x88\x88\x78\x08\x08\x00\x06"
	"\x72\x00\x00\x00\x00\x00\xA0\xC0\x80\x80\x80\x80\x00\x00\x00\x04"
	"\x73\x00\x00\x00\x00\x00\x60\x90\x40\x20\x90\x60\x00\x00\x00\x05"
	"\x74\x00\x00\x00\x80\x80\xC0\x80\x80\x80\x80\x40\x00\x00\x00\x03"
	"\x75\x00\x00\x00\x00\x00\x88\x88\x88\x88\x98\x68\x00\x00\x00\x06"
	"\x76\x00\x00\x00\x00\x00\x88\x88\x50\x50\x20\x20\x00\x00\x00\x06"
	"\x77\x00\x00\x00\x00\x00\x92\x92\xAA\xAA\x44\x44\x00\x00\x00\x08"
	"\x78\x00\x00\x00\x00\x00\x84\x48\x30\x30\x48\x84\x00\x00\x00\x07"
	"\x79\x00\x00\x00\x00\x00\x48\x48\x48\x48\x30\x20\x20\xC0\x00\x06"
	"\x7A\x00\x00\x00\x00\x00\xF0\x10\x20\x40\x80\xF0\x00\x00\x00\x06"
	"\x7B\x00\x20\x40\x40\x40\x40\x80\x40\x40\x40\x40\x20\x00\x00\x03"
	"\x7C\x00\x00\x80\x80\x80\x80\x80\x80\x80\x80\x80\x00\x00\x00\x03"
	"\x7D\x00\x80\x40\x40\x40\x40\x20\x40\x40\x40\x40\x80\x00\x00\x03"
	"\x7E\x00\x00\x00\x64\x98\x00\x00\x00\x00\x00\x00\x00\x00\x00\x07"
	"\x7F\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x81\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x82\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x83\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x84\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x85\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x86\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x87\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x88\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x89\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x8A\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x8B\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x8C\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x8D\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x8E\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x8F\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x90\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x91\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x92\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x93\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x94\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x95\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x96\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x97\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x98\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x99\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x9A\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x9B\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x9C\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x9D\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x9E\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\x9F\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xA0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xA1\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xA2\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xA3\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xA4\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xA5\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xA6\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xA7\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xA8\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xA9\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xAA\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xAB\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xAC\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xAD\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xAE\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xAF\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xB0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xB1\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xB2\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xB3\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xB4\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xB5\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xB6\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xB7\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xB8\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xB9\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xBA\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xBB\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xBC\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xBD\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xBE\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xBF\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xC0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xC1\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xC2\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xC3\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xC4\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xC5\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xC6\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xC7\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xC8\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xC9\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xCA\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xCB\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xCC\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xCD\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xCE\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xCF\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xD0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xD1\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xD2\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xD3\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xD4\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xD5\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xD6\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xD7\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xD8\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xD9\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xDA\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xDB\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xDC\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xDD\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xDE\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xDF\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xE0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xE1\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xE2\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xE3\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xE4\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xE5\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xE6\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xE7\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xE8\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xE9\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xEA\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xEB\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xEC\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xED\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xEE\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xEF\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xF0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xF1\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xF2\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xF3\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xF4\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xF5\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xF6\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xF7\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xF8\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xF9\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xFA\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xFB\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xFC\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xFD\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xFE\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
	"\xFF\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
};


void loadbuiltinfonts()
{
    memcpy(&FONT, systemFontBuiltin, 4112);		//copy the system font in
    for (int i = 32; i < 128; ++i) {
        char current = FONT.FONTD[i].chr;
        FontWidths[0][(int) current] = FONT.FONTD[i].size;
        for (int c = 0; c < CELLH; ++c) {
            Fonts[0][(int) current][c] = FONT.FONTD[i].font[(int) c];
        }
    }

    System = 0;
}

void bitblit(int sx, int sy, int x, int y, int w, int h, int pitch, uint32_t* data);

void Context_draw_bitmap_clipped(Context* context, uint32_t* data, int x, int y, int w, int h, Rect* bound_rect)
{
	int font_x, font_y;
	int off_x = 0;
	int off_y = 0;
	int count_x = w;
	int count_y = h;
	uint8_t shift_line;

	//Make sure to take context translation into account
	x += context->translate_x;
	y += context->translate_y;

	//Check to see if the character is even inside of this rectangle
	if (x > bound_rect->right || (x + count_x) <= bound_rect->left ||
		y > bound_rect->bottom || (y + count_y) <= bound_rect->top)
		return;

	//Limit the drawn portion of the character to the interior of the rect
	if (x < bound_rect->left)
		off_x = bound_rect->left - x;

	if ((x + count_x) > bound_rect->right)
		count_x = bound_rect->right - x + 1;

	if (y < bound_rect->top)
		off_y = bound_rect->top - y;

	if ((y + count_y) > bound_rect->bottom)
		count_y = bound_rect->bottom - y + 1;

	bitblit(x + off_x, y + off_y, off_x, off_y, count_x - off_x, count_y - off_y, w, data);

	/*for (font_y = off_y; font_y < count_y; font_y++) {
		for (font_x = off_x; font_x < count_x; font_x++) {
			screenputpixel(font_x + x, font_y + y, data[font_y * w + font_x]);
		}
	}*/
}

//Draw a single character with the specified font color at the specified coordinates
void Context_draw_char_clipped(Context* context, char character, int x, int y,
                               uint32_t color, Rect* bound_rect) {

    int font_x, font_y;
    int off_x = 0;
    int off_y = 0;
	int count_x = FontWidths[System][(int) character];
	int count_y = CELLH;
    uint8_t shift_line;

    //Make sure to take context translation into account
    x += context->translate_x;
    y += context->translate_y;

    //Our font only handles the core set of 128 ASCII chars
    character &= 0x7F;

    //Check to see if the character is even inside of this rectangle
    if(x > bound_rect->right || (x + count_x) <= bound_rect->left ||
       y > bound_rect->bottom || (y + count_y) <= bound_rect->top)
        return;

    //Limit the drawn portion of the character to the interior of the rect
    if(x < bound_rect->left)
        off_x = bound_rect->left - x;        

    if((x + count_x) > bound_rect->right)
        count_x = bound_rect->right - x + 1;

    if(y < bound_rect->top)
        off_y = bound_rect->top - y;

    if((y + count_y) > bound_rect->bottom)
        count_y = bound_rect->bottom - y + 1;

	for (font_y = off_y; font_y < count_y; font_y++) {
		shift_line = Fonts[System][character][font_y];
		shift_line <<= off_x;

		for (font_x = off_x; font_x < count_x; font_x++) {
			if (shift_line & 0x80) {
				screenputpixel(font_x + x, font_y + y, color);
			}

			shift_line <<= 1;
		}
	}
}

void Context_draw_bitmap(Context* context, uint32_t* data, int x, int y, int w, int h)
{
	int i;
	Rect* clip_area;
	Rect screen_area;

	//If there are clipping rects, draw the character clipped to
	//each of them. Otherwise, draw unclipped (clipped to the screen)
	if (context->clip_rects->count) {
		for (i = 0; i < context->clip_rects->count; i++) {
			clip_area = (Rect*) List_get_at(context->clip_rects, i);
			Context_draw_bitmap_clipped(context, data, x, y, w, h, clip_area);
		}

	} else {

		if (!context->clipping_on) {

			screen_area.top = 0;
			screen_area.left = 0;
			screen_area.bottom = context->height - 1;
			screen_area.right = context->width - 1;
			Context_draw_bitmap_clipped(context, data, x, y, w, h, clip_area);
		}
	}
}

//This will be a lot like Context_fill_rect, but on a bitmap font character
int Context_draw_char(Context* context, char character, int x, int y, uint32_t color) {

    int i;
    Rect* clip_area;
    Rect screen_area;

    //If there are clipping rects, draw the character clipped to
    //each of them. Otherwise, draw unclipped (clipped to the screen)
    if(context->clip_rects->count) {
        for (i = 0; i < context->clip_rects->count; i++) {    
            clip_area = (Rect*)List_get_at(context->clip_rects, i);
			Context_draw_char_clipped(context, character, x, y, color, clip_area);
        }

	} else {

        if(!context->clipping_on) {

            screen_area.top = 0;
            screen_area.left = 0;
            screen_area.bottom = context->height - 1;
            screen_area.right = context->width - 1;
            Context_draw_char_clipped(context, character, x, y, color, clip_area);
        }
    }

	return FontWidths[System][(int) character];
}

//Draw a line of text with the specified font color at the specified coordinates
void Context_draw_text(Context* context, char* string, int x, int y, uint32_t color, int flags) {

    for (; *string; ) {
		if (flags & TEXT_FLAG_BOLD) {
			Context_draw_char(context, *string, x++, y, color);
		}
        x += Context_draw_char(context, *(string++), x, y, color);
    }
}
