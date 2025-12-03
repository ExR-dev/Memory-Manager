#include "TrackedAlloc.hpp"
#include "TracyClient/public/tracy/Tracy.hpp"

#ifdef TRACY_ENABLE
#include <new>

void *operator new(size_t size)
{
	void *ptr = malloc(size);
	if (!ptr)
	{
		throw std::bad_alloc();
	}
	TracyAlloc(ptr, size);
	return ptr;
}
void *operator new(size_t size, [[maybe_unused]] int blockUse, [[maybe_unused]] char const *fileName, [[maybe_unused]] int lineNumber)
{
	void *ptr = malloc(size);
	if (!ptr)
	{
		throw std::bad_alloc();
	}
	TracyAlloc(ptr, size);
	return ptr;
}

void *operator new[](size_t size)
{
	void *ptr = malloc(size);
	if (!ptr)
	{
		throw std::bad_alloc();
	}
	TracyAlloc(ptr, size);
	return ptr;
}
void *operator new[](size_t size, [[maybe_unused]] int blockUse, [[maybe_unused]] char const *fileName, [[maybe_unused]] int lineNumber)
{
	void *ptr = malloc(size);
	if (!ptr)
	{
		throw std::bad_alloc();
	}
	TracyAlloc(ptr, size);
	return ptr;
}

void operator delete(void *ptr) noexcept
{
	if (ptr)
	{
		TracyFree(ptr);
		free(ptr);
	}
}
void operator delete(void *ptr, [[maybe_unused]] size_t size) noexcept
{
	if (ptr)
	{
		TracyFree(ptr);
		free(ptr);
	}
}

void operator delete[](void *ptr) noexcept
{
	if (ptr)
	{
		TracyFree(ptr);
		free(ptr);
	}
}
void operator delete[](void *ptr, [[maybe_unused]] size_t size) noexcept
{
	if (ptr)
	{
		TracyFree(ptr);
		free(ptr);
	}
}
#endif
