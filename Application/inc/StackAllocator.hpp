#pragma once
#include <memory>
#include <array>
#include <iostream>

#define STACK_SIZE 1 << 14
typedef std::unique_ptr<std::array<char, STACK_SIZE>> StorageType;

class StackAllocator
{
private:
	StorageType m_stack;
	size_t m_top = 0;

public:
	StackAllocator()
	{
		m_stack = std::make_unique<std::array<char, STACK_SIZE>>();
		*(m_stack.get()) = {};
	}

	void* At(size_t idx)
	{
		if (idx >= m_top)
			return nullptr;

		return &m_stack.get()->at(idx);
	}

	size_t Push(void* data, size_t size)
	{
		if (m_top + size > STACK_SIZE)
			return (size_t)-1;

		size_t start = m_top;
		char* begin = m_stack.get()->data();

		for (size_t i = 0; i < size; i++)
		{
			char* foo = (char*)data;
			begin[m_top] = foo[i];
			m_top++;
		}

		return start;
	}

	void Reset()
	{
		m_top = 0;
	}

	size_t DBG_GetTop()
	{
		return m_top;
	}

	StorageType& DBG_GetStack()
	{
		return m_stack;
	}
};