#ifndef _WSBE_ENTRY_HPP
#define _WSBE_ENTRY_HPP

#include "frame.hpp"

void _entryKeyDownHandler(Frame* f, Message msg);

class Entry : public Frame
{
private:

protected:
	friend void _entryKeyDownHandler(Frame* f, Message msg);
	virtual void _impl() override;
	
	char* text;
	int cur;
	int x;
	int y;
	int width;

	void entryKeyDownHandler(Frame* f, Message msg);

	int textLength;

	WsbeScript currScript;

	void redoPaintScript();

public:
	Entry(int x, int y, int width, const char* text = "");
	void setText(const char* text);	
};

#endif