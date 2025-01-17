#ifndef _SYSCALLS_HPP_
#define _SYSCALLS_HPP_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
struct regs;
uint64_t KeSystemCall(regs* r, void* context);
uint64_t KeSystemCallFromUsermode(size_t a, size_t b, size_t c, size_t d);
#endif

#define WSBE_FORCE_INIT_EBX		0xA5347896
#define WSBE_FORCE_INIT_ECX		0x4F777FF7
#define WSBE_FORCE_INIT_EDX		0x11235555

#define _APPSETTINGS_VALIDATION_V1			'N'

#define _APPSETTINGS_MODE_UNKNOWN			0
#define _APPSETTINGS_MODE_SET_NAME			1
#define _APPSETTINGS_MODE_SET_DESCRIPTION	2
#define _APPSETTINGS_MODE_SET_KEYWORDS		3
#define _APPSETTINGS_MODE_SET_AUTHOR		4
#define _APPSETTINGS_MODE_SET_COPYRIGHT		5
#define _APPSETTINGS_MODE_SET_VERSION		6
#define _APPSETTINGS_MODE_SET_FLAGS			7
#define _APPSETTINGS_MODE_SET_TITLECOLOUR	8

typedef struct AppSettings_t
{
	union
	{
		char string[256];
		char keywords[16][16];
	};

	uint32_t mode				: 5;
	uint32_t valid				: 8;
	uint32_t noConsole			: 1;
	uint32_t unbufferedKeyboard : 1;

	uint32_t intData;

} AppSettings_t;

enum
#ifdef __cplusplus
class
#endif
SystemCallNumber
{
	Yield,
	Exit,
	Sbrk,
	Write,
	Read,
	GetPID,
	GetCwd,
	SetCwd,
	Open,
	Close,
	OpenDir,
	ReadDir,
	SeekDir,
	TellDir,
	MakeDir,
	CloseDir,
	Seek,
	Tell,
	Size,
	Verify,
	Wait,
	Fork,
	Execve,
	Rmdir,
	Unlink,
	GetArgc,
	GetArgv,
	Realpath,
	TTYName,
	IsATTY,
	USleep,
	SizeFromFilename,
	Spawn,
	GetEnv,
	AppSettings,
	FormatDisk,
	SetDiskVolumeLabel,
	GetDiskVolumeLabel,
	SetFATAttrib,
	Panic,
	Shutdown,
	Pipe,
	GetUnixTime,
	LoadDLL,
	SetTime,
	Timezone,
	Eject,
	WSBE,
	GetRAMData,
	GetVGAPtr,
	RegisterSignal,
	Kill,
	RegistryGetTypeFromPath,
	RegistryReadExtent,
	RegistryPathToExtentLookup,
	RegistryEnterDirectory,
	RegistryGetNext,
	RegistryGetNameAndTypeFromExtent,
	RegistryOpen,
	RegistryClose,
	Truncate,
	SizeFromFilenameNoSymlink,
	Symlink,
	RegistryEasyReadString,
	RegistryEasyReadInteger,
	Alarm,
	Pause,
	PthreadCreate,
	PthreadJoin,
	PthreadExit,
	PthreadGetTID,
	InternalPthreadGetContext,
	InternalPthreadGetStartLocation,
};


#endif