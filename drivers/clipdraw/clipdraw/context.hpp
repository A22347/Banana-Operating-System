#pragma once

#include <stdint.h>
#include <hal/video.hpp>

#include "brush.hpp"
#include "list.hpp"

struct CRect
{
	int left;
	int right;
	int top;
	int bottom;

	CRect();
	CRect(int t, int l, int b, int r);

	List<CRect*>* split(CRect* cutting_rect);
};


struct Context
{
	Video* screen;
	int width;
	int height;

	List<CRect*>* clip_rects;

	Context(Video* screen);

	void clearClipRects();
	void addClipRect(CRect* added_rect);
	void subClipRect(CRect* added_rect);

	void drawRect(int x, int y, int width, int height, uint32_t colour);
	void drawHorizontalLine(int x, int y, int length, uint32_t colour);
	void drawVerticalLine(int x, int y, int length, uint32_t colour);
	
	void clippedRect(int x, int y, int width, int height, CRect* clip_area, uint32_t colour);
	void clippedRect(int x, int y, int width, int height, CRect* clip_area, Brush* brush);

	void fillRect(int x, int y, int width, int height, uint32_t colour);
	void fillRect(int x, int y, int width, int height, Brush* brush);
};