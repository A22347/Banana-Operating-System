#include "hw/cpu.hpp"
#include "hw/ports.hpp"
#include "core/virtmgr.hpp"
#include "core/physmgr.hpp"
#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

int lastCode = -1;
size_t lastA = 0;
size_t lastB = 0;
size_t lastC = 0;
size_t lastD = 0;

#define CPUID_VENDOR_OLDAMD       "AMDisbetter!" /* early engineering samples of AMD K5 processor */
#define CPUID_VENDOR_AMD          "AuthenticAMD"
#define CPUID_VENDOR_INTEL        "GenuineIntel"
#define CPUID_VENDOR_VIA1         "CentaurHauls"
#define CPUID_VENDOR_OLDTRANSMETA "TransmetaCPU"
#define CPUID_VENDOR_TRANSMETA    "GenuineTMx86"
#define CPUID_VENDOR_CYRIX        "CyrixInstead"
#define CPUID_VENDOR_CENTAUR      "CentaurHauls"
#define CPUID_VENDOR_NEXGEN       "NexGenDriven"
#define CPUID_VENDOR_UMC          "UMC UMC UMC "
#define CPUID_VENDOR_SIS          "SiS SiS SiS "
#define CPUID_VENDOR_NSC          "Geode by NSC"
#define CPUID_VENDOR_RISE         "RiseRiseRise"
#define CPUID_VENDOR_VORTEX       "Vortex86 SoC"
#define CPUID_VENDOR_VIA2         "VIA VIA VIA "

#define CPUID_VENDOR_VMWARE       "VMwareVMware"
#define CPUID_VENDOR_XENHVM       "XenVMMXenVMM"
#define CPUID_VENDOR_MICROSOFT_HV "Microsoft Hv"
#define CPUID_VENDOR_PARALLELS    " lrpepyh vr"

char cpuNameList[42][16] = {
	"i486 DX",		//0
	"i486 SX",		//1
	"i486 DX2",		//2
	"i486 SL",		//3
	"i486 SX2",		//4
	"i486 DX4",		//5
	"Pentium",		//6
	"Pentium MMX",	//7
	"Quark",		//8
	"Pentium Pro",	//9
	"Pentium II",	//10
	"Pentium III",	//11
	"Pentium M",	//12
	"Core",			//13
	"Core 2",		//14
	"Nehalem",		//15
	"Bonnell",		//16
	"Westmere",		//17
	"Sandy Bridge",	//18
	"Silvermont",	//19
	"Ivy Bridge",	//20
	"Haswell",		//21
	"Broadwell",	//22
	"Knights Corner",//23
	"Pentium 4",	//24
	"Nocona",		//25
	"Am486",		//26
	"Am5x86",		//27
	"K5",			//28
	"K6",			//29
	"K6-2",			//30
	"K6-3",			//31
	"K7",			//32
	"K8",			//33
	"K10",			//34
	"Bobcat",		//35
	"Bulldozer",	//36
	"Piledriver",	//37
	"Steamroller",	//38
	"Jaguar",		//39
	"Jaguar (Puma)",//40
};

char* CPU::lookupAMDName(uint8_t a, uint8_t b)
{
	if (a == 0x04) {
		if (b == 0x03) return cpuNameList[26];
		if (b == 0x07) return cpuNameList[26];
		if (b == 0x08) return cpuNameList[26];
		if (b == 0x09) return cpuNameList[26];
		if (b == 0x0A) return cpuNameList[26];
		if (b == 0x0E) return cpuNameList[27];
		if (b == 0x0F) return cpuNameList[27];
	}
	
	if (a == 0x05) {
		if (b == 0x00) return cpuNameList[28];
		if (b == 0x01) return cpuNameList[28];
		if (b == 0x02) return cpuNameList[28];
		if (b == 0x03) return cpuNameList[28];
		if (b == 0x06) return cpuNameList[29];
		if (b == 0x07) return cpuNameList[29];
		if (b == 0x08) return cpuNameList[30];
		if (b == 0x09) return cpuNameList[31];
		if (b == 0x0D) return cpuNameList[31];
	}

	if (a == 0x06) {
		if (b == 0x01) return cpuNameList[32];
		if (b == 0x02) return cpuNameList[32];
		if (b == 0x03) return cpuNameList[32];
		if (b == 0x04) return cpuNameList[32];
		if (b == 0x06) return cpuNameList[32];
		if (b == 0x07) return cpuNameList[32];
		if (b == 0x08) return cpuNameList[32];
		if (b == 0x0A) return cpuNameList[32];
	}
	

	for (int i = 0; i < 0xD0; ++i) {
		if (a == 0x0F && b == i) return cpuNameList[33];
	}

	if (a == 0x10) {
		if (b == 0x02) return cpuNameList[34];
		if (b == 0x04) return cpuNameList[34];
		if (b == 0x05) return cpuNameList[34];
		if (b == 0x06) return cpuNameList[34];
		if (b == 0x08) return cpuNameList[34];
		if (b == 0x09) return cpuNameList[34];
		if (b == 0x0A) return cpuNameList[34];
	}
	

	if (a == 0x11 && b == 0x03) return cpuNameList[33];

	if (a == 0x12 && b == 0x01) return cpuNameList[34];

	if (a == 0x14 && b == 0x01) return cpuNameList[35];
	if (a == 0x14 && b == 0x02) return cpuNameList[35];

	if (a == 0x15) {
		if (b == 0x01) return cpuNameList[36];
		if (b == 0x02) return cpuNameList[37];
		if (b == 0x10) return cpuNameList[37];
		if (b == 0x13) return cpuNameList[37];
		if (b == 0x30) return cpuNameList[38];
	}


	if (a == 0x16 && b == 0x00) return cpuNameList[39];
	if (a == 0x16 && b == 0x30) return cpuNameList[40];

	return (char*) "CPU";
}

char* CPU::lookupIntelName(uint8_t a, uint8_t b)
{
	if (a == 0x04) {
		if (b == 0x00) return cpuNameList[0];
		if (b == 0x01) return cpuNameList[0];
		if (b == 0x02) return cpuNameList[1];
		if (b == 0x03) return cpuNameList[2];
		if (b == 0x04) return cpuNameList[3];
		if (b == 0x05) return cpuNameList[4];
		if (b == 0x07) return cpuNameList[2];
		if (b == 0x08) return cpuNameList[5];
		if (b == 0x09) return cpuNameList[5];
	}
	
	if (a == 0x05) {
		if (b == 0x01) return cpuNameList[6];
		if (b == 0x02) return cpuNameList[6];
		if (b == 0x03) return cpuNameList[6];
		if (b == 0x04) return cpuNameList[7];
		if (b == 0x07) return cpuNameList[6];
		if (b == 0x08) return cpuNameList[7];
		if (b == 0x09) return cpuNameList[8];
	}

	if (a == 0x06) {
		if (b == 0x01) return cpuNameList[9];
		if (b == 0x03) return cpuNameList[10];
		if (b == 0x05) return cpuNameList[10];
		if (b == 0x06) return cpuNameList[10];
		if (b == 0x07) return cpuNameList[11];
		if (b == 0x08) return cpuNameList[11];
		if (b == 0x09) return cpuNameList[12];
		if (b == 0x0A) return cpuNameList[11];
		if (b == 0x0B) return cpuNameList[11];
		if (b == 0x0D) return cpuNameList[12];
		if (b == 0x0E) return cpuNameList[13];
		if (b == 0x0F) return cpuNameList[14];
		if (b == 0x15) return cpuNameList[12];
		if (b == 0x16) return cpuNameList[14];
		if (b == 0x17) return cpuNameList[14];
		if (b == 0x1A) return cpuNameList[15];
		if (b == 0x1C) return cpuNameList[16];
		if (b == 0x1D) return cpuNameList[14];
		if (b == 0x25) return cpuNameList[17];
		if (b == 0x26) return cpuNameList[16];
		if (b == 0x27) return cpuNameList[16];
		if (b == 0x2A) return cpuNameList[18];
		if (b == 0x2C) return cpuNameList[17];
		if (b == 0x2D) return cpuNameList[18];
		if (b == 0x2E) return cpuNameList[15];
		if (b == 0x2F) return cpuNameList[17];
		if (b == 0x35) return cpuNameList[16];
		if (b == 0x36) return cpuNameList[16];
		if (b == 0x37) return cpuNameList[19];
		if (b == 0x3A) return cpuNameList[20];
		if (b == 0x3C) return cpuNameList[21];
		if (b == 0x3D) return cpuNameList[22];
		if (b == 0x3E) return cpuNameList[20];
		if (b == 0x3F) return cpuNameList[21];
		if (b == 0x45) return cpuNameList[21];
		if (b == 0x46) return cpuNameList[21];
		if (b == 0x4A) return cpuNameList[19];
		if (b == 0x4D) return cpuNameList[19];
	}

	if (a == 0x0B && b == 0x01) return cpuNameList[23];

	if (a == 0x0F && b == 0x00) return cpuNameList[24];
	if (a == 0x0F && b == 0x01) return cpuNameList[24];
	if (a == 0x0F && b == 0x02) return cpuNameList[24];
	if (a == 0x0F && b == 0x03) return cpuNameList[25];
	if (a == 0x0F && b == 0x04) return cpuNameList[25];
	if (a == 0x0F && b == 0x06) return cpuNameList[25];

	return (char*) "CPU";
}


void CPU::AMD_K6_writeback(int family, int model, int stepping)
{
	/* mem_end == top of memory in bytes */
	int mem = (Phys::highestMem >> 20) / 4; /* turn into 4mb aligned pages */
	int c;
	REGS regs;

	if (family == 5) {
		c = model;

		/* model 8 stepping 0-7 use old style, 8-F use new style */
		if (model == 8) {
			if (stepping < 8)
				c = 7;
			else
				c = 9;
		}

		switch (c) {
			/* old style write back */
		case 6:
		case 7:
			AMD_K6_read_msr(0xC0000082, &regs);
			AMD_K6_write_msr(0xC0000082, ((mem << 1) & 0x7F), 0, &regs);
			break;

			/* new style write back */
		case 9:
			AMD_K6_read_msr(0xC0000082, &regs);
			AMD_K6_write_msr(0xC0000082, ((mem << 22) & 0x3FF), 0, &regs);
			break;
		default:    /* dont set it on Unknowns + k5's */
			break;
		}
	}
}

void CPU::AMD_K6_write_msr(uint32_t msr, uint32_t v1, uint32_t v2, REGS* regs)
{
	asm __volatile__("pusha");
	asm __volatile__(
		"pushfl\n"
		"cli\n"
		"wbinvd\n"
		"wrmsr\n"
		"popfl\n"
		: "=a" (regs->eax)
		: "a" (v1),
		"d" (v2),
		"c" (msr)
	);
	asm __volatile__("popa");

}

void CPU::AMD_K6_read_msr(uint32_t msr, REGS* regs)
{
	asm __volatile__("pusha");
	asm __volatile__(
		"pushfl\n"
		"cli\n"
		"wbinvd\n"
		"xorl %%eax, %%eax\n"
		"xorl %%edx, %%edx\n"
		"rdmsr\n"
		"popfl\n"
		: "=a" (regs->eax)
		: "c" (msr)
	);
	asm __volatile__("popa");
}

void CPU::cpuid(int code, size_t* a, size_t* b, size_t* c, size_t* d)
{
	if (code == lastCode) {
		*a = lastA;
		*b = lastB;
		*c = lastC;
		*d = lastD;
		return;
	}
	asm volatile ("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(code), "c"(0) : "cc");
	lastCode = code;
	lastA = *a;
	lastB = *b;
	lastC = *c;
	lastD = *d;
}

bool CPU::cpuidCheckEDX(uint32_t check)
{
	size_t eax, ebx, ecx, edx;
	cpuid(CPUID_GETFEATURES, &eax, &ebx, &ecx, &edx);
	return edx & check;
}

bool CPU::cpuidCheckECX(uint32_t check)
{
	size_t eax, ebx, ecx, edx;
	cpuid(CPUID_GETFEATURES, &eax, &ebx, &ecx, &edx);
	return ecx & check;
}

bool CPU::cpuidCheckExtendedEBX(uint32_t check)
{
	size_t eax, ebx, ecx, edx;
	cpuid(CPUID_GETEXTENDED, &eax, &ebx, &ecx, &edx);
	return ebx & check;
}

bool CPU::cpuidCheckExtendedECX(uint32_t check)
{
	size_t eax, ebx, ecx, edx;
	cpuid(CPUID_GETEXTENDED, &eax, &ebx, &ecx, &edx);
	return ecx & check;
}

CPU::CPU(): Device("CPU")
{
	deviceType = DeviceType::CPU;
}

int CPU::open(int num, int b, void* vas_)
{
	cpuNum = num;

	Krnl::setBootMessage("Setting up GDT...");
	gdt.setup();
	Krnl::setBootMessage("Setting up TSS...");
	tss.setup(0xDEADBEEF);
	Krnl::setBootMessage("Setting up TSS 2...");
	tss.flush();
	Krnl::setBootMessage("Setting up IDT...");
	idt.setup();
	Krnl::setBootMessage("Setting up debug registers...");
	writeDR7(0x400);

	Krnl::setBootMessage("Setting up CPU specific paging...");
	cpuSpecificData = (CPUSpecificData*) VIRT_CPU_SPECIFIC;

	cpuSpecificPhysAddr = (CPUSpecificData*) Phys::allocatePage();
	cpuSpecificPhysAddr->cpuNumber = num;
	cpuSpecificPhysAddr->cpuPointer = this;

	Krnl::setBootMessage("Still setting up CPU specific paging...");
	VAS* vas = (VAS*) vas_;
	vas->setCPUSpecific((size_t) cpuSpecificPhysAddr);
	
	Krnl::setBootMessage("Detecting CPU specific features...");
	//here so APIC can be disabled on dodgy K5 CPUs
	detectFeatures();

	Krnl::setBootMessage("Configuring interrupt controller...");
	intCtrl = setupInterruptController();
	Krnl::setBootMessage("Setting up system timer...");
	timer = setupTimer(sysBootSettings & 16 ? 30 : 100);

	if (sysBootSettings & 32) {
		Krnl::setBootMessage("Setting up CPU features...");
		setupFeatures();
	}
	Krnl::setBootMessage("Displaying CPU features...");
	displayFeatures();

	return 0;
}

void CPU::displayFeatures()
{
	kprintf("CPU %d Features\n", cpuNum);
	kprintf("    Vendor : %s\n", vendorIDString);
	kprintf("    Family : %d\n", familyID);
	kprintf("    Model  : %d\n", model);
	kprintf("\n");
	kprintf("    RDRAND       - %s\n", features.hasRDRAND ? "Yes" : "No");
	kprintf("    RDSEED       - %s\n", features.hasRDSEED ? "Yes" : "No");
	kprintf("    TSC          - %s\n", features.hasTSC ? "Yes" : "No");
	kprintf("    PAE          - %s\n", features.hasPAE ? "Yes" : "No");
	kprintf("    PSE          - %s\n", features.hasPSE ? "Yes" : "No");
	kprintf("    Global pages - %s\n", features.hasGlobalPages ? "Yes" : "No");
	kprintf("    PAT          - %s\n", features.hasPAT ? "Yes" : "No");
	kprintf("    SMEP         - %s\n", features.hasSMEP ? "Yes" : "No");
	kprintf("    SMAP         - %s\n", features.hasSMAP ? "Yes" : "No");
	kprintf("    MTRR         - %s\n", features.hasMTRR ? "Yes" : "No");
	kprintf("    UMIP         - %s\n", features.hasUMIP ? "Yes" : "No");
	kprintf("    CR8          - %s\n", features.hasCR8 ? "Yes" : "No");
	kprintf("    Sysenter     - %s\n", features.hasSysenter ? "Yes" : "No");
	kprintf("    Syscall      - %s\n", features.hasSyscall ? "Yes" : "No");
	kprintf("    TPAUSE       - %s\n", features.hasTPAUSE ? "Yes" : "No");
}

extern "C" size_t is486();
extern "C" size_t hasLegacyFPU();

void CPU::detectFeatures()
{
	opcodeDetectionMode = true;

	features.hasCR8 = false;
	features.hasSMAP = false;
	features.hasSysenter = false;
	features.hasSyscall = false;
	features.hasRDRAND = false;
	features.hasTSC = false;
	features.hasPAE = false;
	features.hasPSE = false;
	features.hasGlobalPages = false;
	features.hasPAT = false;
	features.hasSMEP = false;
	features.hasRDSEED = false;
	features.hasUMIP = false;
	features.hasMTRR = false;
	features.onboardFPU = false;
	features.hasTPAUSE = false;

	memset(vendorIDString, 0, 13);

	if (computer->features.hasCPUID) {
		features.hasTSC = cpuidCheckEDX(CPUID_FEAT_EDX_TSC);
		features.hasPAE = cpuidCheckEDX(CPUID_FEAT_EDX_PAE);
		features.hasPSE = cpuidCheckEDX(CPUID_FEAT_EDX_PSE);
		features.hasGlobalPages = cpuidCheckEDX(CPUID_FEAT_EDX_PGE);
		features.hasPAT = cpuidCheckEDX(CPUID_FEAT_EDX_PAT);
		features.onboardFPU = cpuidCheckEDX(CPUID_FEAT_EDX_FPU);
		features.hasSysenter = cpuidCheckEDX(CPUID_FEAT_EDX_SEP);

		size_t eax, ebx, ecx, edx;
		cpuid(0, &eax, &ebx, &ecx, &edx);
		memcpy(vendorIDString + 0, &ebx, 4);
		memcpy(vendorIDString + 4, &edx, 4);
		memcpy(vendorIDString + 8, &ecx, 4);

		if (!strcmp(vendorIDString, CPUID_VENDOR_OLDAMD) || !strcmp(vendorIDString, CPUID_VENDOR_AMD)) vendor = CPUVendor::AMD;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_VIA1) || !strcmp(vendorIDString, CPUID_VENDOR_VIA2)) vendor = CPUVendor::VIA;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_OLDTRANSMETA) || !strcmp(vendorIDString, CPUID_VENDOR_TRANSMETA)) vendor = CPUVendor::TransMeta;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_INTEL)) vendor = CPUVendor::Intel;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_CYRIX)) vendor = CPUVendor::Cyrix;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_CENTAUR)) vendor = CPUVendor::Centaur;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_NEXGEN)) vendor = CPUVendor::Nexgen;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_UMC)) vendor = CPUVendor::UMC;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_SIS)) vendor = CPUVendor::SIS;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_NSC)) vendor = CPUVendor::NSC;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_RISE)) vendor = CPUVendor::Rise;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_VORTEX)) vendor = CPUVendor::Vortex;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_VMWARE)) vendor = CPUVendor::VirtualMachine;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_XENHVM)) vendor = CPUVendor::VirtualMachine;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_MICROSOFT_HV)) vendor = CPUVendor::VirtualMachine;
		else if (!strcmp(vendorIDString, CPUID_VENDOR_PARALLELS)) vendor = CPUVendor::VirtualMachine;
		else vendor = CPUVendor::Unknown;

		if (eax >= 7) {
			features.hasSMEP = cpuidCheckExtendedEBX(1 << 7);
			features.hasSMAP = cpuidCheckExtendedEBX(1 << 20);
			features.hasRDSEED = cpuidCheckExtendedEBX(1 << 18);
			features.hasUMIP = cpuidCheckExtendedECX(1 << 2);
			features.hasTPAUSE = cpuidCheckExtendedECX(1 << 5);
		}

		cpuid(1, &eax, &ebx, &ecx, &edx);
		steppingID = eax & 0xF;
		model = (eax >> 4) & 0xF;
		familyID = (eax >> 8) & 0xF;
		processorType = (eax >> 12) & 3;
		if (familyID == 6 || familyID == 15) {
			model |= (((eax >> 16) & 0xF) << 4);
		}
		if (familyID == 15) {
			familyID += ((eax >> 20) & 0xFF);
		}

		bool ecxCanReturnFeatures = false;
		if (ecxCanReturnFeatures) {
			features.hasRDRAND = cpuidCheckECX(1 << 30);
		}

		features.hasINVLPG = 1;
		features.hasINVD = 1;
		features.hasWBINVD = 1;

		cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
		if (eax >= 0x80000001) {
			cpuid(0x80000001, &eax, &ebx, &ecx, &edx);

			features.hasCR8 = ecx & (1 << 4);
			features.hasMTRR = edx & (1 << 12);
			features.hasSyscall = edx & (1 << 11);
		}

		if (vendor == CPUVendor::Intel) {
			char n[32];
			strcpy(n, "Intel ");
			strcat(n, lookupIntelName(familyID, model));
			setName(n);

			if (!strcmp(humanName, "Intel Pentium Pro")) {
				//the Pentium Pro has some dodgy CPUID bits
				features.hasSyscall = false;
				features.hasSysenter = false;
			}

		} else if (vendor == CPUVendor::AMD) {
			char n[32];
			strcpy(n, "AMD ");
			strcat(n, lookupAMDName(familyID, model));
			setName(n);

			//more dodgy CPUID return values
			if (!strcmp(humanName, "AMD K5")) {
				if (computer->features.hasAPIC) {
					computer->features.hasAPIC = false;
					features.hasGlobalPages = true;
				}
			}

			//K6 optimisations
			if (familyID == 5) {
				AMD_K6_writeback(familyID, model, steppingID);
			}

		} else if (vendor == CPUVendor::Centaur || vendor == CPUVendor::VIA || vendor == CPUVendor::Cyrix) {
			if (familyID == 5) {
				if (model == 4) setName("Centaur WinChip C6");
				else if (model == 8) setName("Centaur WinChip 2");
				else if (model == 9) setName("Centaur WinChip 3");
				else {
					if (vendor == CPUVendor::Centaur) setName("Centaur CPU");
					else if (vendor == CPUVendor::VIA) setName("VIA CPU");
					else if (vendor == CPUVendor::Cyrix) {
						setName("Cyrix 5x86");
					}
				}
			} else if (familyID == 6) {
				if (model == 6) setName("Centaur WinChip");
				else if (model == 7) setName("VIA C3");
				else if (model == 8) setName("VIA C3");
				else if (model == 9) setName("VIA C3-2");
				else if (model == 0xA) setName("VIA C7");
				else if (model == 0xD) setName("VIA C7");
				else if (model == 0xF) setName("VIA Nano");
				else {
					if (vendor == CPUVendor::Centaur) setName("Centaur CPU");
					else if (vendor == CPUVendor::VIA) setName("VIA CPU");
					else if (vendor == CPUVendor::Cyrix) {
						setName("Cyrix 6x86");

						outb(0x22, 0xC1);
						uint8_t val = inb(0x23) | 0x10;
						outb(0x22, 0xC1);
						outb(0x23, val);
						
						ports[noPorts].rangeStart = 0x22;
						ports[noPorts].rangeLength = 2;
						ports[noPorts++].width = 0;
					}
				}
			} else {
				if (vendor == CPUVendor::Centaur) setName("Centaur CPU");
				else if (vendor == CPUVendor::VIA) setName("VIA CPU");
				else if (vendor == CPUVendor::Cyrix) setName("Cyrix CPU");
			}

		} else if (vendor == CPUVendor::Nexgen) {
			setName("Nexgen CPU");

		} else if (vendor == CPUVendor::NSC) {
			setName("NSC CPU");

		} else if (vendor == CPUVendor::Rise) {
			setName("Rise CPU");

		} else if (vendor == CPUVendor::UMC) {
			setName("UMC CPU");

		} else if (vendor == CPUVendor::SIS) {
			setName("SIS CPU");

		} else if (vendor == CPUVendor::TransMeta) {
			setName("TransMeta CPU");

		} else if (vendor == CPUVendor::Vortex) {
			setName("Vortex CPU");

		} else if (vendor == CPUVendor::VirtualMachine) {
			setName("Virtual Machine CPU");

		} else {
			setName("Unknown CPU");

		}

	} else {
		//either an i386 or an i486

		strcpy(vendorIDString, CPUID_VENDOR_INTEL);
		vendor = CPUVendor::Intel;

		if (is486()) {
			setName("Intel i486");
			features.hasINVLPG = 1;
			features.hasINVD = 1;
			features.hasWBINVD = 1;

			size_t cr0 = readCR0();
			if ((cr0 & (1 << 2)) && !(cr0 & (1 << 4))) {
				features.onboardFPU = false;

			} else {
				features.onboardFPU = hasLegacyFPU();
			}

		} else {
			setName("Intel i386");
			features.hasINVLPG = 0;
			features.hasINVD = 0;
			features.hasWBINVD = 0;

			size_t cr0 = readCR0();
			if ((cr0 & (1 << 2)) && !(cr0 & (1 << 4))) {
				features.onboardFPU = false;

			} else {
				features.onboardFPU = hasLegacyFPU();
			}
		}
	}

	opcodeDetectionMode = false;
}

int CPU::close(int a, int b, void* ptr)
{
	return 0;
}

void CPU::setupSMEP()
{
	writeCR4(readCR4() | (1 << 20));
}

void CPU::setupSMAP()
{
	writeCR4(readCR4() | (1 << 21));
	prohibitUsermodeDataAccess();
}

void CPU::setupUMIP()
{
	writeCR4(readCR4() | (1 << 11));
}

void CPU::setupTSC()
{
	writeCR4(readCR4() | (1 << 2));
}

void CPU::setupLargePages()
{
	writeCR4(readCR4() | (1 << 4));
}

void CPU::setupPAT()
{
	if (computer->features.hasMSR) {
		uint64_t pat = computer->rdmsr(0x277);

		//first 4 entries
		uint32_t lowPat = pat & 0xFFFFFFFF;

		//next 4 entries
		uint32_t highPat = pat >> 32;

		//clear first entry of high dword (entry 4)
		highPat &= ~7;

		//set to write combining
		highPat |= 1;

		//write back the PAT
		pat = (((uint64_t) highPat) << 32) | ((uint64_t) lowPat);
		computer->wrmsr(0x277, pat);
	}
}

void CPU::setupMTRR()
{

}

void CPU::setupGlobalPages()
{
	writeCR4(readCR4() | (1 << 7));
}

void CPU::allowUsermodeDataAccess()
{
	if (features.hasSMAP) {
		asm volatile ("stac" ::: "cc");
	}
}

void CPU::prohibitUsermodeDataAccess()
{
	if (features.hasSMAP) {
		asm volatile ("clac" ::: "cc");
	}
}

void CPU::setupFeatures()
{
	if (features.hasSMEP) {
		//kprintf("SMEP on.\n");
		//setupSMEP();
	}

	if (features.hasSMAP) {
		//kprintf("SMAP on.\n");
		//setupSMAP();
	}

	if (features.hasUMIP) {
		kprintf("UMIP on.\n");
		setupUMIP();
	}

	if (features.hasTSC) {
		kprintf("TSC on.\n");
		setupTSC();
	}
	
	if (features.hasPSE) {
		kprintf("PSE on.\n");
		setupLargePages();
	}
	
	if (features.hasGlobalPages) {
		kprintf("GLOBAL PAGES on.\n");
		setupGlobalPages();
	}

	if (features.hasPAT) {
		kprintf("PAT on.\n");
		setupPAT();
	}
	
	if (features.hasMTRR) {
		kprintf("MTRR on.\n");
		setupMTRR();
	}
}


uint8_t* CPU::decodeAddress(regs* r, int* instrLenOut, bool* registerOnlyOut, uint8_t* middleDigitOut)
{
	uint32_t segmentInfo;
	asm volatile ("lar %1, %0" : "=r"(segmentInfo) : "r"(r->cs));
	bool fromKernel = ((segmentInfo >> 13) & 3) == 0;

	uint8_t* eip = (uint8_t*) r->eip;

	uint8_t middleDigit = (eip[1] >> 3) & 7;		//0
	uint8_t mod = (eip[1] >> 6) & 3;
	uint8_t rm = (eip[1] >> 0) & 7;					//4

	*middleDigitOut = middleDigit;

	uint8_t* ptr = 0;
	bool registerOnly = false;
	bool ptrIllegal = false;
	int instrLen = 2;

	// d9 1c 24                fstp   DWORD PTR[esp]

	if (mod != 3 && rm != 4 && !(mod == 0 && rm == 5)) {
		//[reg]
		//[reg + disp8]
		//[reg + disp32]
		if (rm == 0) ptr = (uint8_t*) r->eax;
		if (rm == 1) ptr = (uint8_t*) r->ecx;
		if (rm == 2) ptr = (uint8_t*) r->edx;
		if (rm == 3) ptr = (uint8_t*) r->ebx;
		if (rm == 5) ptr = (uint8_t*) r->ebp;
		if (rm == 6) ptr = (uint8_t*) r->esi;
		if (rm == 7) ptr = (uint8_t*) r->edi;

		if (mod == 1) {
			ptr += *((int8_t*) (eip + 2));
			instrLen += 1;

		} else if (mod == 2) {
			ptr += *((int32_t*) (eip + 2));
			instrLen += 4;
		}

	} else if (mod == 0 && rm == 5) {
		//32 bit displacement
		ptr = (uint8_t*) (*((uint32_t*) (eip + 2)));
		instrLen += 4;

	} else if (mod == 3) {
		//register only mode
		registerOnly = true;

	} else if (rm == 4) {
		//SIB mode
		uint8_t sib = eip[2];
		uint8_t scale = (sib >> 6) & 3;		//0
		uint8_t index = (sib >> 3) & 7;		//4
		uint8_t base = (sib >> 0) & 7;		//4
		instrLen += 1;

		uint32_t actBase;
		uint32_t actIndex;

		if (base == 0) actBase = r->eax;
		else if (base == 1) actBase = r->ecx;
		else if (base == 2) actBase = r->edx;
		else if (base == 3) actBase = r->ebx;
		else if (base == 4) actBase = fromKernel ? r->esp + (5 * 4) : r->useresp;		//plus 5 * 4 because error code, irq num, EIP, EFLAGS, CS get pushed before this
		else if (base == 5) actBase = r->ebp;
		else if (base == 6) actBase = r->esi;
		else if (base == 7) actBase = r->edi;

		if (index == 0) actIndex = r->eax;
		else if (index == 1) actIndex = r->ecx;
		else if (index == 2) actIndex = r->edx;
		else if (index == 3) actIndex = r->ebx;
		else if (index == 4) actIndex = 0;
		else if (index == 5) actIndex = r->ebp;
		else if (index == 6) actIndex = r->esi;
		else if (index == 7) actIndex = r->edi;

		if (mod == 0 && base == 5) {
			//displacement only
			ptr = (uint8_t*) (actIndex << scale);
			ptr += *((int32_t*) (eip + 3));
			instrLen += 4;

		} else if (mod == 0) {
			//SIB
			ptr = (uint8_t*) (actBase + (actIndex << scale));

		} else if (mod == 1) {
			//SIB + disp8
			ptr = (uint8_t*) (actBase + (actIndex << scale));
			ptr += *((int8_t*) (eip + 3));
			instrLen += 1;

		} else if (mod == 2) {
			//SIB + disp32
			ptr = (uint8_t*) (actBase + (actIndex << scale));
			ptr += *((int32_t*) (eip + 3));
			instrLen += 4;
		}
	}

	*instrLenOut = instrLen;
	*registerOnlyOut = registerOnly;

	return ptr;
}