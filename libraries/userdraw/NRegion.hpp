//
//  NRegion.hpp
//  NGUI
//
//  Created by Alex Boxall on 16/9/21.
//  Copyright © 2021 Alex Boxall. All rights reserved.
//

#ifndef NRegion_hpp
#define NRegion_hpp

#define _USER_WSBE_WANT_KEYBRD_
#include "C:/Users/Alex/Desktop/Banana/kernel/hal/keybrd.hpp"
#undef _USER_WSBE_WANT_KEYBRD_

extern "C" {
    #include "context.h"
    #include "window.h"
}

class NRegion {
protected:
    Context* ctxt;
    
    friend class NTopLevel;
    void* parent;

    void internalInvalidate();
    bool autoInvalidate;
    bool invalidating;

public:
    Window* win;
    int width;
    int height;
    int x;
    int y;
    
    int (*paintHandler)(NRegion* self);

    void setRightMouseDownHandler(WindowMousedownHandler handler);
    void setRightMouseUpHandler(WindowMousedownHandler handler);
    
    Context* getContext();

    NRegion(int x, int y, int w, int h, Context* context);
    ~NRegion();

    void add(NRegion* rgn);
    void remove(NRegion* rgn);
  
    void invalidate();
    void disableAutomaticInvalidation();
    void enableAutomaticInvalidation(bool on = true);

    void resize(int w, int h);

    void fillRect(int x, int y, int w, int h, uint32_t col);
    void drawRect(int x, int y, int w, int h, uint32_t col);
    void horizontalLine(int x, int y, int w, uint32_t col);
    void verticalLine(int x, int y, int h, uint32_t col);
    void drawBasicText(int x, int y, uint32_t col, const char* text);
};

#endif /* NRegion_hpp */
