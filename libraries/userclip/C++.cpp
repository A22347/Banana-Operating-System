
#include <stddef.h>
#include <stdlib.h>

void* operator new(size_t size)
{
    return malloc(size);
}

void* operator new[](size_t size)
{
    return malloc(size);
}

void operator delete(void* p)
{
    free(p);
}

void operator delete[](void* p)
{
    free(p);
}

void operator delete(void* p, unsigned long)
{

}

void operator delete[](void* p, unsigned long)
{

}