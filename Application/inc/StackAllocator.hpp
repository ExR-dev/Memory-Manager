#pragma once

#include "TracyWrapper.hpp"

#include <memory>
#include <array>
#include <iostream>
#include <stack>

//#define DBG_STACK_TRACK_SIZE

constexpr size_t STACK_SIZE = 1 << 14;
typedef std::unique_ptr<std::array<char, STACK_SIZE>> StorageType;

class StackAllocator
{
private:
	StorageType m_stack;
	size_t m_top = 0;
#ifdef TRACY_ENABLE
	std::stack<void*> m_dbgTrackedAllocs;
#endif
#ifdef DBG_STACK_TRACK_SIZE
	std::vector<size_t> m_dbgTrackedSizes;
#endif

public:
	StackAllocator()
	{
		m_stack = std::make_unique<std::array<char, STACK_SIZE>>();

		*(m_stack.get()) = {};
	}

	void* At(size_t idx)
	{
		ZoneScopedXC(tracy::Color::DarkGreen);

		if (idx >= m_top)
			return nullptr;

		return &m_stack.get()->at(idx);
	}

	size_t Push(void* data, size_t size)
	{
		ZoneScopedC(tracy::Color::CornflowerBlue);

		if (m_top + size > STACK_SIZE)
			return (size_t)-1;

		size_t start = m_top;
		char* begin = m_stack.get()->data();

#ifdef TRACY_ENABLE
		TracyAllocN(begin + m_top, size, "Stack");
		m_dbgTrackedAllocs.push(begin + m_top);
#endif

#ifdef DBG_STACK_TRACK_SIZE
		m_dbgTrackedSizes.push_back(size);
#endif

		for (size_t i = 0; i < size; i++)
		{
			begin[m_top] = ((char*)data)[i];
			m_top++;
		}

		return start;
	}

	void Reset()
	{
		ZoneScopedC(tracy::Color::Chartreuse2);

#ifdef TRACY_ENABLE
		while (!m_dbgTrackedAllocs.empty())
		{
			TracyFreeN(m_dbgTrackedAllocs.top(), "Stack");
			m_dbgTrackedAllocs.pop();
		}
#endif

#ifdef DBG_STACK_TRACK_SIZE
		m_dbgTrackedSizes.clear();
#endif
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

	constexpr size_t DBG_GetMaxSize()
	{
		return STACK_SIZE;
	}

#ifdef DBG_STACK_TRACK_SIZE
	const std::vector<size_t> &DBG_GetAllocSizes()
	{
		return m_dbgTrackedSizes;
	}
#endif
};