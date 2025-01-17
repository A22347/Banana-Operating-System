#ifndef _LIBK_STRING_H
#define _LIBK_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

	int memcmp(const void*, const void*, size_t);

	static inline __attribute__((always_inline)) void* memcpy(void* destination, const void* source, size_t n)
	{
		void* originalDest = destination;
		size_t clbr1, clbr2, clbr3;
		asm volatile("rep movsb" : "=S" (clbr1), "=D" (clbr2), "=c" (clbr3) : "S"(source), "D"(destination), "c"(n) : "cc", "memory");
		return originalDest;
	}


	//https://stackoverflow.com/questions/49131175/recreate-the-strstr-function
	char* strstr(const char* str, const char* substring);


	char* strtok(char* str, const char* delim);


	void strncpy(char* _dst, const char* _src, size_t _n);
	void* memmove(void*, const void*, size_t);
	void* memset(void*, int, size_t);
	size_t strlen(const char*);
	char* strcpy(char* strDest, const char* strSrc);
	int strcmp(const char* s1, const char* s2);
	int strncmp(const char* s1, const char* s2, size_t num);
	char* strcat(char* dest, const char* src);
	char* strchr(char* str, int ch);

#ifdef __cplusplus
}
#endif

#endif