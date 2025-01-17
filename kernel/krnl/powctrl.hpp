#ifndef _POWCTRL_HPP_
#define _POWCTRL_HPP_

#include <stdint.h>
#include <stddef.h>
#include "hal/device.hpp"

enum class PowerButtonOption : int
{
	Nothing,
	Sleep,
	Shutdown,
	DisplayOff,
};

struct PowerSettings
{
	PowerButtonOption powerButton;
	PowerButtonOption sleepButton;
	PowerButtonOption lidClosed;

	int poweredSecsBeforeDisplayOff;
	int poweredSecsBeforeDiskOff;
	int poweredSecsBeforeSleep;

	int batterySecsBeforeDisplayOff;
	int batterySecsBeforeDiskOff;
	int batterySecsBeforeSleep;
};

extern int secondsSinceLastUserIO;
extern PowerSettings currentPowerSettings;
extern bool powCtrlOnBattery;

void KeChangePowerSettings(PowerSettings settings);
void KeSetupPowerManager();
void KeUserIOReceived();

void KeHandlePowerButton();
void KeHandleSleepButton();

void KeRegisterShutdownHandler(void (*shutdownFunc)());
void KeRegisterRestartHandler(void (*restartFunc)());
void KeRegisterSleepHandler(void (*sleepFunc)());
void KeShutdown();
void KeSleep();
void KeRestart();

#endif