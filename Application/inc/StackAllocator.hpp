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

	void* Alloc(size_t size)
	{
		ZoneScopedC(tracy::Color::CornflowerBlue);

		if (m_top + size > STACK_SIZE)
			return nullptr;

		char* ptr = m_stack.get()->data();
		ptr += m_top;
		m_top += size;

#ifdef TRACY_ENABLE
		TracyAllocN(ptr, size, "Stack");
		m_dbgTrackedAllocs.push(ptr);
#endif

#ifdef DBG_STACK_TRACK_SIZE
		m_dbgTrackedSizes.push_back(size);
#endif

		return ptr;
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

	size_t DBG_GetTop() const
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