#include "core/main.hpp"
#include "hw/ports.hpp"
#include "hw/acpi.hpp"
#include "hw/fpu/x87.hpp"
#include <stdint.h>

extern "C" size_t x87Detect();
extern "C" void x87Init();
extern "C" void x87Save(size_t ptr);
extern "C" void x87Load(size_t ptr);
extern "C" void x87Close();

x87::x87() : FPU("x87 FPU")
{

}


int x87::open(int num, int a, void* v)
{
    kprintf("x87 FPU INIT.\n");
    x87Init();
    return 0;
}

int x87::close(int a, int b, void* c)
{
    x87Close();
    return 0;
}

bool x87::available() {
    return x87Detect();
}

void x87::save(void* ptr) {
    kprintf("x87 FPU SAVE 1.\n");
    x87Save((size_t) ptr);
    kprintf("x87 FPU SAVE 2.\n");
}

void x87::load(void* ptr) {
    kprintf("x87 FPU LOAD 1.\n");
    x87Load((size_t) ptr);
    kprintf("x87 FPU LOAD 2.\n");
}