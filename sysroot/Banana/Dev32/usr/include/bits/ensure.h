
#ifndef MLIBC_ENSURE_H
#define MLIBC_ENSURE_H

#ifdef __cplusplus
extern "C" {
#endif

void __ensure_fail(const char *assertion, const char *file, unsigned int line,
		const char *function);

void __ensure_warn(const char *assertion, const char *file, unsigned int line,
		const char *function);

#define __ensure(assertion) do { if(!(assertion)) \
		__ensure_fail(#assertion, __FILE__, __LINE__, __func__); } while(0)

#define MLIBC_UNIMPLEMENTED() __ensure_fail("Functionality is not implemented", \
		__FILE__, __LINE__, __func__)

#define MLIBC_MISSING_SYSDEP() __ensure_warn("Library function fails due to missing sysdep", \
		__FILE__, __LINE__, __func__)

#define MLIBC_STUB_BODY { MLIBC_UNIMPLEMENTED(); __builtin_unreachable(); }

#ifdef __cplusplus
}
#endif

#endif // MLIBC_ENSURE_H

