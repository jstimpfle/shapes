#ifndef SHAPES_SHAPES_H_INCLUDED
#define SHAPES_SHAPES_H_INCLUDED

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>  // NULL
#include <stdint.h>  // uint32_t and friends
#include <inttypes.h>  // PRId64 and others


#ifdef _MSC_VER
#pragma warning( disable : 4200)  // nonstandard extension used: zero-sized array in struct
#pragma warning( disable : 4204)  // nonstandard extension used: non-constant aggregate initializer
#define NORETURN __declspec(noreturn)
#define UNUSEDFUNC
static __declspec(noreturn) void UNREACHABLE(void) {}
#else  // assume gcc or clang
#define NORETURN __attribute__((noreturn))
#define UNUSEDFUNC __attribute__((unused))
#define UNREACHABLE() __builtin_unreachable()
#endif

#define LENGTH(a) ((int) (sizeof (a) / sizeof (a)[0]))
#define ENSURE(a) assert(a)
#define UNUSED(arg) (void)(arg)

#define NOT_IMPLEMENTED() fatalf("In %s:%ld : Not implemented", __FILE__, (long) __LINE__);

#ifdef SHAPES_IMPLEMENT_DATA
#define DATA
#else
#define DATA extern
#endif

#endif
