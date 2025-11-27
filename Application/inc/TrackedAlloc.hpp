#pragma once
#include "TracyWrapper.hpp"

#ifdef TRACY_ENABLE
void *operator new(size_t size);
void *operator new(size_t size, int blockUse, char const *fileName, int lineNumber);
void *operator new[](size_t size);
void *operator new[](size_t size, int blockUse, char const *fileName, int lineNumber);

void operator delete(void *ptr) noexcept;
void operator delete(void *ptr, size_t size) noexcept;
void operator delete[](void *ptr) noexcept;
void operator delete[](void *ptr, size_t size) noexcept;

#define DEBUG_TRACE_DEF
#define DEBUG_TRACE(ptr)	TracyAlloc(ptr, sizeof(*ptr));
#define DEBUG_TRACE_ARR(ptr, count)	TracyAlloc(ptr, sizeof(*ptr) * count);
#endif

#ifndef DEBUG_TRACE_DEF
#define DEBUG_TRACE_DEF
#define DEBUG_TRACE(x);
#define DEBUG_TRACE_ARR(x, y);
#endif