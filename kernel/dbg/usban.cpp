#include <krnl/panic.hpp>
#include <krnl/common.hpp>
#include <stdint.h>

// Alignment must be a power of 2.
#define is_aligned(value, alignment) !(value & (alignment - 1))

struct source_location
{
	const char* file;
	uint32_t line;
	uint32_t column;
};

struct type_descriptor
{
	uint16_t kind;
	uint16_t info;
	char* name;
};

struct type_mismatch_info
{
	struct source_location location;
	struct type_descriptor* type;
	uintptr_t alignment;
	uint8_t type_check_kind;
};

const char* KiUsbanTypeMismatchTypes[] = {
	"load of",
	"store to",
	"reference binding to",
	"member access within",
	"member call on",
	"constructor call on",
	"downcast of",
	"downcast of",
	"upcast of",
	"cast to virtual base of",
};

void KiUbsanPrintDetails(const char* str, void* ptr)
{
	kprintf("UBSAN DETECTED: %s\n", str);

	source_location* src = (source_location*) ptr;
	kprintf("file: %s\n", src->file);
	kprintf("line: %d\n", src->line);
	kprintf("clmn: %d\n", src->column);
}

extern "C" void __attribute__((no_sanitize("undefined"))) __ubsan_handle_type_mismatch_v1(struct type_mismatch_info* type_mismatch, uintptr_t pointer)
{
	KiUbsanPrintDetails("__ubsan_handle_type_mismatch_v1", type_mismatch);
	
	if (pointer == 0) {
		kprintf("Null pointer access\n");

	} else if (type_mismatch->alignment != 0 &&
		is_aligned(pointer, type_mismatch->alignment)) {
		kprintf("ubsan: Unaligned memory access to 0x%X\n", pointer);

	} else {
		kprintf("ubsan: Insufficient size\n");
		kprintf("ubsan: %d\n", type_mismatch->type_check_kind);
		kprintf("ubsan: %s address 0x%X with insufficient space for object of type %s\n",
			KiUsbanTypeMismatchTypes[type_mismatch->type_check_kind], (void*) pointer,
			type_mismatch->type->name);
	}

	KePanic("__ubsan_handle_type_mismatch_v1");
}

extern "C" void __ubsan_handle_pointer_overflow(void* ptr)
{
	KiUbsanPrintDetails("__ubsan_handle_pointer_overflow", ptr);
	KePanic("__ubsan_handle_pointer_overflow");
}

extern "C" void __ubsan_handle_out_of_bounds(void* ptr)
{
	KiUbsanPrintDetails("__ubsan_handle_out_of_bounds", ptr);
	KePanic("__ubsan_handle_out_of_bounds");
}

extern "C" void __ubsan_handle_add_overflow(void* ptr)
{
	KiUbsanPrintDetails("__ubsan_handle_add_overflow", ptr);
	KePanic("__ubsan_handle_add_overflow");
}

extern "C" void __ubsan_handle_load_invalid_value(void* ptr)
{
	KiUbsanPrintDetails("__ubsan_handle_load_invalid_value", ptr);
	KePanic("__ubsan_handle_load_invalid_value");
}

extern "C" void __ubsan_handle_divrem_overflow(void* ptr)
{
	KiUbsanPrintDetails("__ubsan_handle_divrem_overflow", ptr);
	KePanic("__ubsan_handle_divrem_overflow");
}

extern "C" void __ubsan_handle_mul_overflow(void* ptr)
{
	KiUbsanPrintDetails("__ubsan_handle_mul_overflow", ptr);
	KePanic("__ubsan_handle_mul_overflow");
}

extern "C" void __ubsan_handle_sub_overflow(void* ptr)
{
	KiUbsanPrintDetails("__ubsan_handle_sub_overflow", ptr);
	KePanic("__ubsan_handle_sub_overflow");
}

extern "C" void __ubsan_handle_shift_out_of_bounds(void* ptr)
{
	KiUbsanPrintDetails("__ubsan_handle_shift_out_of_bounds", ptr);
	KePanic("__ubsan_handle_shift_out_of_bounds");
}

extern "C" void __ubsan_handle_negate_overflow(void* ptr)
{
	KiUbsanPrintDetails("__ubsan_handle_negate_overflow", ptr);
	KePanic("__ubsan_handle_negate_overflow");
}