
#ifndef _PRCSSTHR_HPP_
#define _PRCSSTHR_HPP_

#include <stdint.h>
#include <stddef.h>
#include <krnl/linkedlist.hpp>
#include <krnl/cpplist.hpp>
#include "krnl/main.hpp"
#include "krnl/pipe.hpp"
#include "krnl/virtmgr.hpp"
#include "krnl/env.hpp"
#include <krnl/signal.hpp>

struct Process;

enum class TaskState: size_t
{
	ReadyToRun = 0,
	Running = 1,
	Paused = 2,
	Sleeping = 3,
	Terminated = 4,
	WaitingForLock = 5,
	WaitingForKeyboard = 6,
	WaitPID = 7,
	PausedForSignal,
};

#pragma pack(push,1)
struct ThreadControlBlock
{
	volatile size_t cr3;					//MUST BE AT OFFSET 0x0			INTO THE STRUCT
	volatile size_t esp;					//MUST BE AT OFFSET 0x4/0x8		INTO THE STRUCT
	volatile size_t signalStateHandler;		//MUST BE AT OFFSET 0x8/0x10	INTO THE STRUCT		(USELESS)
	volatile size_t firstTimeEIP;			//MUST BE AT OFFSET 0xC/0x18	INTO THE STRUCT
	volatile uint64_t timeKeeping;			//MUST BE AT OFFSET 0x10/0x20	INTO THE STRUCT
	volatile enum TaskState state;			//MUST BE AT OFFSET 0x18/0x28	INTO THE STRUCT

	volatile size_t magicForkyParentSavePoint;			//MUST BE AT OFFSET 0x1C/0x30	INTO THE STRUCT

	volatile ThreadControlBlock* volatile next = nullptr;
	volatile ThreadControlBlock* volatile prev = nullptr;
	volatile ThreadControlBlock* volatile nextForNonSchedulingThings = nullptr;

	volatile uint64_t sleepExpiry;
	int forkret;

	size_t timeSliceRemaining;

	int rtid;		//relative thread ID to the process (e.g. thread 0, thread 1, etc.)
	Process* processRelatedTo = nullptr;

	uint8_t priority;

	void* startContext;

	int waitingPID;						//as specified here: http://man7.org/linux/man-pages/man2/waitid.2.html
	int* wstatus;						//same link, we're not using it at the moment
	int waitingThreadReturnCode;

	int returnCodeForUseOnTerminationList;

	union
	{
		struct
		{
			uint16_t vm86IP;
			uint16_t vm86CS;
			uint16_t vm86SP;
			uint16_t vm86SS;
		};

		void* fpuState = nullptr;
	};
	
	bool vm86VIE = false;
	bool vm86Task = false;

	uint64_t alarm : 63;
	uint64_t guiTask : 1;
	size_t pthreadStartLocation;
	void* pthreadContext;
};

struct Process
{
	int pid;

	size_t* pointersToDelete;
	int nextDeletablePointer;

	char taskname[256];
	char cwd[256];

	ThreadControlBlock threads[8];
	uint16_t threadUsage;			//one bit per thread

	VgaText* terminal;

	Process* parent;

	size_t usermodeEntryPoint;

	VAS* vas;

	int argc;
	char* argv[128];

	EnvVarContainer* env;

	bool failedToLoadProgram = false;

	void addArgs(char** _argv);
	Process(bool kernel, const char* name, Process* parent = nullptr, char** argv = nullptr);
	Process(const char* filepath, Process* parent = nullptr, char** argv = nullptr);

	ThreadControlBlock* createThread(void (*where)(void*), void* context = nullptr, int pri = 128);
	ThreadControlBlock* createUserThread();

	bool gotCtrlC = false;

	SigHandlerBlock* signals;
};

void userModeEntryPoint(void* ignored);

#pragma pack(pop)

#include <krnl/semaphore.hpp>
#include <krnl/mutex.hpp>

//JUST FOR NOW	- NEEDS TO BE UPDATED IN hardware.asm IF THIS IS CHANGED
#define currentTaskTCB (*((ThreadControlBlock**) 0xC2002000))		

extern "C" size_t taskStartupFunction();
extern "C" void taskReturned();

void KeDisablePreemption();
void KeRestorePreemption();

void switchToThread(ThreadControlBlock* nextThreadToRun);
void setupMultitasking(void (*where)());
void schedule();
void blockTask(enum TaskState reason);
void blockTaskWithSchedulerLockAlreadyHeld(enum TaskState reason);
void unblockTask(ThreadControlBlock* task);
void sleep(uint64_t seconds);
void milliTenthSleep(uint64_t mtenth);
void cleanerTaskFunction(void* context);
int waitTask(int pid, int* wstatus, int options);

extern int irqDisableCounter;
extern int postponeTaskSwitchesCounter;
extern int taskSwitchesPostponedFlag;

#include <krnl/hal.hpp>

static inline __attribute__((always_inline)) void disableIRQs(void)
{
	HalDisableInterrupts();
	irqDisableCounter++;
}

static inline __attribute__((always_inline)) int getIRQNestingLevel(void)
{
	return irqDisableCounter;
}

static inline __attribute__((always_inline)) void enableIRQs(void)
{
	__sync_add_and_fetch(&irqDisableCounter, -1);
	if (irqDisableCounter == 0) {
		HalEnableInterrupts();
	}
}

static inline __attribute__((always_inline)) void lockScheduler(void)
{
#ifndef SMP
	disableIRQs();
#endif
}

static inline __attribute__((always_inline)) void unlockScheduler(void)
{
#ifndef SMP
	enableIRQs();
#endif
}

static inline __attribute__((always_inline)) void lockStuff(void)
{
#ifndef SMP
	disableIRQs();
	postponeTaskSwitchesCounter++;
#endif
}

static inline __attribute__((always_inline)) void unlockStuff(void)
{
#ifndef SMP
	postponeTaskSwitchesCounter--;
	if (postponeTaskSwitchesCounter == 0) {
		if (taskSwitchesPostponedFlag != 0) {
			taskSwitchesPostponedFlag = 0;
			schedule();
		}
	}

	enableIRQs();
#endif
}

extern Process* kernelProcess;
extern ThreadControlBlock* cleanerThread;
extern LinkedList<volatile ThreadControlBlock> sleepingTaskList;
extern LinkedList<volatile ThreadControlBlock> taskList;

void KeTerminateCurrentThread(int returnCode = 0);
Process* KeProcessFromPID(int pid);

#endif