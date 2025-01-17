#include "krnl/common.hpp"
#include "krnl/powctrl.hpp"
#include "hal/keybrd.hpp"
#include "hal/device.hpp"
#include "krnl/terminal.hpp"
#include "libk/string.h"
#include "thr/prcssthr.hpp"
#include <krnl/signal.hpp>

#pragma GCC optimize ("O0")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

bool keystates[0x400];

void (*guiKeyboardHandler) (KeyboardToken kt, bool* keystates) = nullptr;

bool keyboardSetupYet = false;

ThreadControlBlock* keyboardWaitingTaskList = nullptr;

void sendKeyToTerminal(uint8_t code)
{
	activeTerminal->receiveKey(code);

	if (code == (uint8_t) '\n' || code == (uint8_t) '\3' || code == (uint8_t) '\x1C') {
		ThreadControlBlock* next_task;
		ThreadControlBlock* this_task;

		lockStuff();

		next_task = keyboardWaitingTaskList;
		keyboardWaitingTaskList = nullptr;

		while (next_task != nullptr) {
			this_task = next_task;
			next_task = (ThreadControlBlock*) this_task->next;

			unblockTask(this_task);
		}

		unlockStuff();
	}
}

#include "hal/video.hpp"
#include "thr/prcssthr.hpp"
#include "thr/elf.hpp"
#include "thr/elf2.hpp"

void startGUI(void* a)
{
	unlockScheduler();

	KeLoadAndExecuteDriver("C:/Banana/Drivers/vga.sys", computer);
	KeLoadAndExecuteDriver("C:/Banana/System/clipdraw.dll", computer);
	
	while (1);
	KePanic("startGUI returns");
}

void startGUIVESA(void* a)
{
	unlockScheduler();

	KeLoadAndExecuteDriver("C:/Banana/Drivers/vesa.sys", computer);
	KeLoadAndExecuteDriver("C:/Banana/System/clipdraw.dll", computer);
	
	while (1);
	KePanic("startGUIVESA returns");
}

void sendKeyboardToken(KeyboardToken kt)
{
	KeUserIOReceived();

	keystates[kt.halScancode] = !kt.release;

	if (guiKeyboardHandler) {
		guiKeyboardHandler(kt, keystates);
	}

	bool keypadNum = false;

	static int buffering = 0;
	
	if (kt.numlock) {
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad0) kt.halScancode = (uint16_t) KeyboardSpecialKeys::Insert;
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad1) kt.halScancode = (uint16_t) KeyboardSpecialKeys::End;
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad2) kt.halScancode = (uint16_t) KeyboardSpecialKeys::Down;
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad3) kt.halScancode = (uint16_t) KeyboardSpecialKeys::PageDown;
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad4) kt.halScancode = (uint16_t) KeyboardSpecialKeys::Left;
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad5) kt.halScancode = (uint16_t) '5';
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad6) kt.halScancode = (uint16_t) KeyboardSpecialKeys::Right;
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad7) kt.halScancode = (uint16_t) KeyboardSpecialKeys::Home;
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad8) kt.halScancode = (uint16_t) KeyboardSpecialKeys::Up;
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad9) kt.halScancode = (uint16_t) KeyboardSpecialKeys::PageUp;
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::KeypadPeriod) kt.halScancode = (uint16_t) KeyboardSpecialKeys::Delete;
	} else {
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad0) { kt.halScancode = (uint16_t) '0'; keypadNum = true; }
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad1) { kt.halScancode = (uint16_t) '1'; keypadNum = true; }
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad2) { kt.halScancode = (uint16_t) '2'; keypadNum = true; }
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad3) { kt.halScancode = (uint16_t) '3'; keypadNum = true; }
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad4) { kt.halScancode = (uint16_t) '4'; keypadNum = true; }
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad5) { kt.halScancode = (uint16_t) '5'; keypadNum = true; }
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad6) { kt.halScancode = (uint16_t) '6'; keypadNum = true; }
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad7) { kt.halScancode = (uint16_t) '7'; keypadNum = true; }
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad8) { kt.halScancode = (uint16_t) '8'; keypadNum = true; }
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Keypad9) { kt.halScancode = (uint16_t) '9'; keypadNum = true; }
	}

	static bool guiStated = false;
	if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Home && !guiStated) {
		kernelProcess->createThread(startGUI, nullptr, 1);
		guiStated = true;
	} else if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::End && !guiStated) {
		kernelProcess->createThread(startGUIVESA, nullptr, 1);
		guiStated = true;
	}
	

	if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::KeypadEnter) kt.halScancode = (uint16_t) KeyboardSpecialKeys::Enter;
	if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::KeypadMinus) kt.halScancode = (uint16_t) '-';
	if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::KeypadPlus) kt.halScancode = (uint16_t) '+';
	if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::KeypadMultiply) kt.halScancode = (uint16_t) '*';
	if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::KeypadDivide) kt.halScancode = (uint16_t) '/';
	if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::KeypadPeriod) kt.halScancode = (uint16_t) '.';

	bool noMoreSend = false;
	if (!kt.release) {
		if (keystates[(uint16_t) KeyboardSpecialKeys::Alt]) {
			noMoreSend = true;
			if (keypadNum) {
				buffering *= 10;
				buffering += kt.halScancode - '0';
			} else {
				buffering = 0;
			}

		} else {
			if (buffering) {
				sendKeyToTerminal(buffering);
				noMoreSend = true;

			}
			buffering = 0;
		}

	} else {
		if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::Alt) {
			if (buffering) {
				sendKeyToTerminal(buffering);
				noMoreSend = true;

			}
			buffering = 0;
		}
	}

	//add to buffer if it is a normal character on press (not release) (e.g. A-Z, a-z, 0-9, symbols, space, tab, newline, etc.)
	//OR if it is a printable character and the control key (doesn't *nix map things like Ctrl-C or Ctrl-Z to an ASCII character?)

	if (!noMoreSend && !kt.release && !keystates[(int) KeyboardSpecialKeys::Ctrl] && ((kt.halScancode >= 32 && kt.halScancode < 127) \
		|| kt.halScancode == (uint16_t) KeyboardSpecialKeys::Enter \
		|| kt.halScancode == (uint16_t) KeyboardSpecialKeys::Backspace)) {

		sendKeyToTerminal(kt.halScancode);
	}

	if (!noMoreSend && !kt.release && keystates[(int) KeyboardSpecialKeys::Ctrl] && (kt.halScancode >= 64 && kt.halScancode < 128)) {
		//also deals with lowercase instead of uppercase
		sendKeyToTerminal(kt.halScancode - '@' - (kt.halScancode >= 96 ? 32 : 0));
	}

	if (kt.halScancode == (uint16_t) KeyboardSpecialKeys::F1 && !kt.release) {
		doTerminalCycle();
	}

	//also set event states, such as 'released L' or 'pressed W' for GUI apps
}

//should be called by read(), if it is stdin
//e.g.
//
// if (file == stdinFileDescForThisTask) {
//		return readKeyboard(currentTCB, currentTCB->terminal, buffer, count)
// }
//

void clearInternalKeybuffer(VgaText* terminal)
{
	memset(terminal->keybufferSent, 0, strlen(terminal->keybufferSent));
}

int readKeyboard(VgaText* terminal, char* buf, size_t count)
{
	if (guiKeyboardHandler) {
		KeyboardToken kt;
		kt.halScancode = 0;
		guiKeyboardHandler(kt, keystates);
	}

	asm("sti");
	int charsRead = 0;
	while (count) {
		while (terminal->keybufferSent[0] == 0) {
			lockScheduler();
			schedule();
			unlockScheduler();
		}

		//we shouldn't block twice on keyboard, we should just return if we don't get enough characters
		//blocked = true;

		//put in the buffer
		*buf++ = terminal->keybufferSent[0];

		char key = terminal->keybufferSent[0];

		if ((uint8_t) key == (uint8_t) '\3') {
			KeRaiseSignal(currentTaskTCB->processRelatedTo->signals, SIGINT);
		}
		if ((uint8_t) key == (uint8_t) '\x1C') {
			KeRaiseSignal(currentTaskTCB->processRelatedTo->signals, SIGKILL);
		}

		//remove first char from that buffer
		memmove(terminal->keybufferSent, terminal->keybufferSent + 1, strlen(terminal->keybufferSent));

		--count;
		++charsRead;

		if (key == '\n' || key == '\3' || key == '\x1C') {
			return charsRead;
		}
	}

	return charsRead;
}

Keyboard::Keyboard(const char* name) : Device(name)
{
	deviceType = DeviceType::Keyboard;

	keyboardSetupYet = true;
}

Keyboard::~Keyboard()
{

}
