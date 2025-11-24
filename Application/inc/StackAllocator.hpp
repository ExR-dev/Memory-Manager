#pragma once
#include <memory>
#include <array>

#define STACK_SIZE 1 << 14
typedef std::unique_ptr<std::array<char, STACK_SIZE>> StorageType;

class StackAllocator
{
private:
	StorageType m_stack = {};
	size_t m_top = 0;

public:
	StackAllocator();

	void* At(size_t idx);
	size_t Push(void* data, size_t size);
	void Reset();

	size_t DBG_GetTop();
	StorageType& DBG_GetStack();
};