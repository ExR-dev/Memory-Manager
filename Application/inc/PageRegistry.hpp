// PageRegistry.h allocates, stores & manages all type-specific memory pages.

#pragma once

#include <vector>
#include <memory>

namespace MemoryInternal
{
	template <typename T>
	class PageRegistry
	{
	public:


	private:
		std::vector<T> m_pageStorage; // TODO: Decide what storage type to use
		// TODO: Decide how to track allocated regions

		bool m_initialized;
		size_t m_maxCount;


		PageRegistry() = default;
		~PageRegistry() = default;

		[[nodiscard]] static PageRegistry<T> &Get()
		{
			static PageRegistry<T> instance;
			return instance;
		}

		static int Initialize(size_t maxCount)
		{
			PageRegistry<T> &registry = Get();

			if (registry.m_initialized)
				return -1; // Failure: Already initialized

			if (maxCount <= 0)
				return -2; // Failure: Invalid max count

			if ((maxCount & (maxCount - 1)) != 0)
				return -3; // Failure: Max count must be a power of two

			registry.m_pageStorage.resize(maxCount);
			registry.m_maxCount = maxCount;
			registry.m_initialized = true;

			return 0; // Success
		}
	};
}