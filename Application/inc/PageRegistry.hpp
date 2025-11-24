// PageRegistry.h allocates, stores & manages all type-specific memory pages.

#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <cstring>

namespace MemoryInternal
{
	constexpr size_t DEFAULT_PAGE_SIZE = (1 << 12);

	struct AllocLink
	{
		size_t offset;
		size_t size;
		std::unique_ptr<AllocLink> next;

		AllocLink(size_t off, size_t sz)
			: offset(off), size(sz), next(nullptr) { }
	};

	template <typename T>
	class PageRegistry
	{
	public:
		[[nodiscard]] static T *Alloc(size_t count)
		{
			PageRegistry<T> &registry = Get();

			if (!registry.m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Default max count

			// Find first free region of sufficient size
			AllocLink *prev = nullptr;
			AllocLink *current = registry.m_freeRegions.get();

			while (current)
			{
				if (current->size >= count)
				{
					size_t allocOffset = current->offset;

					// Update free region
					current->offset += count;
					current->size -= count;

					if (current->size == 0)
					{
						// Remove the link if no space left
						if (prev)
						{
							prev->next = std::move(current->next);
						}
						else
						{
							// Update head of free regions
							registry.m_freeRegions = std::move(current->next);
						}
					}

					// Add to alloc map
					registry.m_allocMap[allocOffset] = count;

					return &registry.m_pageStorage[allocOffset];
				}

				prev = current;
				current = current->next.get();
			}

			return nullptr; // Failure: No sufficient free region
		}

		static int Free(T *ptr)
		{
			PageRegistry<T> &registry = Get();
			if (!registry.m_initialized || ptr == nullptr)
				return -1;

			size_t offset = ptr - registry.m_pageStorage.data();

			auto it = registry.m_allocMap.find(offset);
			if (it == registry.m_allocMap.end())
				return -2; // Failure: Invalid pointer or size

			size_t count = it->second;

			// Remove from alloc map
			registry.m_allocMap.erase(it);

			// Handle case where pool is full
			if (!registry.m_freeRegions)
			{
				// Add this allocation as the only free region
				registry.m_freeRegions = std::make_unique<AllocLink>(offset, count);
			}
			else
			{
				// Find correct position to insert freed region
				AllocLink *left = nullptr;
				AllocLink *right = registry.m_freeRegions.get();

				while (right)
				{
					if (offset < right->offset)
						break;

					left = right;
					right = right->next.get();
				}

				// If regions are contiguous, merge them instead of creating a new link
				if (left && (left->offset + left->size == offset))
				{
					left->size += count;

					if (right && (offset + count == right->offset))
					{
						// Merge with next region as well
						left->size += right->size;
						left->next = std::move(right->next);
					}
				}
				else if (right && (offset + count == right->offset))
				{
					// Merge with next region
					right->offset = offset;
					right->size += count;
				}
				else
				{
					// Insert new free region
					auto newLink = std::make_unique<AllocLink>(offset, count);
					newLink->next = std::move(left ? left->next : registry.m_freeRegions->next);

					if (left)
						left->next = std::move(newLink);
					else
						registry.m_freeRegions = std::move(newLink);
				}
			}
		}

		static void DBG_PrintPage(int lineWidth)
		{
			PageRegistry<T> &registry = Get();
			if (!registry.m_initialized)
			{
				printf("PageRegistry not initialized.\n");
				return;
			}

			char pageStr[DEFAULT_PAGE_SIZE + 1]{};
			memset(pageStr, '.', DEFAULT_PAGE_SIZE);
			pageStr[DEFAULT_PAGE_SIZE] = '\0';

			char allocID = 'a';
			for (const auto &alloc : registry.m_allocMap)
			{
				size_t offset = alloc.first;
				size_t size = alloc.second;

				for (size_t pos = offset; pos < offset + size; ++pos)
				{
					if (pageStr[pos] != '.')
						pageStr[pos] = '?'; // Overlap indication
					else if (pos < DEFAULT_PAGE_SIZE)
						pageStr[pos] = allocID;
				}

				allocID++;
			}

			// Print the page representation
			for (size_t i = 0; i < DEFAULT_PAGE_SIZE; i += lineWidth)
			{
				printf("%04zx: ", i);
				for (size_t j = 0; j < lineWidth && (i + j) < DEFAULT_PAGE_SIZE; ++j)
				{
					putchar(pageStr[i + j]);
				}
				putchar('\n');
			}
			putchar('\n');
		}

	private:
		std::vector<T> m_pageStorage;
		std::unique_ptr<AllocLink> m_freeRegions;
		std::unordered_map<size_t, size_t> m_allocMap; // Offset to size mapping

		bool m_initialized = false;
		size_t m_maxCount = 0;


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
			registry.m_freeRegions = std::make_unique<AllocLink>(0, maxCount);
			registry.m_initialized = true;

			return 0; // Success
		}
	};

	template <typename T>
	[[nodiscard]] inline T *Alloc(size_t count)
	{
		return PageRegistry<T>::Alloc(count);
	}

	template <typename T>
	inline int Free(T *ptr)
	{
		return PageRegistry<T>::Free(ptr);
	}
}