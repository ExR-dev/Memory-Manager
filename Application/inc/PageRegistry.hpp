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

			if (count == 0 || count > registry.m_maxCount)
				return nullptr; // Failure: Invalid count

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

			if (offset >= registry.m_maxCount)
				return -2; // Failure: Invalid pointer

			size_t count = registry.m_allocMap[offset];
			if (count == (size_t)-1)
				return -3; // Failure: Not allocated

			// Remove from alloc map
			registry.m_allocMap[offset] = (size_t)-1;

			// Handle case where pool is full
			if (!registry.m_freeRegions)
			{
				// Add this allocation as the only free region
				registry.m_freeRegions = std::make_unique<AllocLink>(offset, count);
			}
			else
			{
				// Find correct position to insert freed region
				// such that the insertion point falls after 'left' and before 'right'
				std::unique_ptr<AllocLink> *left = nullptr;
				std::unique_ptr<AllocLink> *right = &registry.m_freeRegions;

				while (*right)
				{
					if (offset < right->get()->offset)
						break;

					left = right;
					right = &right->get()->next;
				}

				// If regions are contiguous, merge them instead of creating a new link
				if (left && *left && (left->get()->offset + left->get()->size == offset))
				{
					left->get()->size += count;

					if (right && (offset + count == right->get()->offset))
					{
						// Merge with next region as well
						left->get()->size += right->get()->size;
						left->get()->next = std::move(right->get()->next);
					}
				}
				else if (*right && (offset + count == right->get()->offset))
				{
					// Merge with next region
					right->get()->offset = offset;
					right->get()->size += count;
				}
				else // Region is not contiguous with either side, insert new link
				{
					// Insert new free region
					auto newLink = std::make_unique<AllocLink>(offset, count);

					if (*right && !left)
					{
						// Inserting at head
						newLink->next = std::move(registry.m_freeRegions);
						registry.m_freeRegions = std::move(newLink);
					}
					else if (left && *left)
					{
						// Inserting in middle or end
						newLink->next = std::move(left->get()->next);
						left->get()->next = std::move(newLink);
					}
				}
			}

			return 0; // Success
		}

		static void DBG_Reset()
		{
			PageRegistry<T> &registry = Get();
			registry.m_pageStorage.clear();
			registry.m_freeRegions.reset();
			registry.m_allocMap.clear();
			registry.m_initialized = false;
			registry.m_maxCount = 0;
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
			for (int i = 0; i < registry.m_allocMap.size(); ++i)
			{
				const auto &alloc = registry.m_allocMap[i];

				if (alloc == (size_t)-1 || alloc == (size_t)0)
					continue; // Not allocated

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
				i += size - 1; // Skip to end of this allocation
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
		const static std::vector<T> &DBG_GetPageStorage()
		{
			if (!Get().m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Ensure initialized for debugging

			return Get().m_pageStorage;
		}
		const static std::unique_ptr<AllocLink> &DBG_GetFreeRegions()
		{
			if (!Get().m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Ensure initialized for debugging

			return Get().m_freeRegions;
		}
		const static std::vector<size_t> &DBG_GetAllocMap()
		{
			if (!Get().m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Ensure initialized for debugging

			return Get().m_allocMap;
		}

	private:
		std::vector<T> m_pageStorage;
		std::unique_ptr<AllocLink> m_freeRegions;
		std::vector<size_t> m_allocMap; // Offset to size mapping

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
			registry.m_allocMap.resize(maxCount);
			registry.m_maxCount = maxCount;
			registry.m_freeRegions = std::make_unique<AllocLink>(0, maxCount);
			registry.m_initialized = true;

			std::fill(registry.m_allocMap.begin(), registry.m_allocMap.end(), (size_t)-1);

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