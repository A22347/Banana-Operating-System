#include "krnl/main.hpp"
#include "hal/device.hpp"
#include "hal/diskctrl.hpp"
#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

HardDiskController::HardDiskController(const char* name) : Device(name)
{
	deviceType = DeviceType::HardDiskController;
}

HardDiskController::~HardDiskController()
{

}