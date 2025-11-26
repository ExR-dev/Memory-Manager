// PageRegistry.h allocates, stores & manages all type-specific memory pages.

#pragma once

#include <vector>
#include <memory>

#include "TracyClient/public/tracy/Tracy.hpp"

namespace MemoryInternal
{
	constexpr size_t NULL_INDEX = static_cast<size_t>(-1);

	constexpr size_t DEFAULT_PAGE_SIZE = (1 << 13);

	struct AllocLink
	{
		size_t offset;
		size_t size;
		size_t next;

		AllocLink() : offset(0), size(0), next(NULL_INDEX) { }
		AllocLink(size_t off, size_t sz)
			: offset(off), size(sz), next(NULL_INDEX) { }
	};

	template <typename T>
	class PageRegistry
	{
	public:
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
			registry.m_freeRegionLinkStorage.resize(maxCount);

			registry.m_maxCount = maxCount;
			registry.m_initialized = true;
			registry.m_freeRegionsRoot = 0;

			std::fill(registry.m_allocMap.begin(), registry.m_allocMap.end(), NULL_INDEX);
			std::fill(registry.m_freeRegionLinkStorage.begin(), registry.m_freeRegionLinkStorage.end(), AllocLink(0, 0));

			registry.m_freeRegionLinkStorage[0] = AllocLink(0, maxCount);

			return 0; // Success
		}
		static void Reset()
		{
			PageRegistry<T> &registry = Get();
			registry.m_pageStorage.clear();
			registry.m_freeRegionLinkStorage.clear();
			registry.m_allocMap.clear();
			registry.m_freeRegionsRoot = 0;
			registry.m_initialized = false;
			registry.m_maxCount = 0;
		}

		[[nodiscard]] static T *Alloc(size_t count)
		{
			PageRegistry<T> &registry = Get();

			if (!registry.m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Default max count

			if (count == 0 || count > registry.m_maxCount)
				return nullptr; // Failure: Invalid count

			// Find first free region of sufficient size
			size_t prev = NULL_INDEX;
			size_t current = registry.m_freeRegionsRoot;

			auto &freeRegions = registry.m_freeRegionLinkStorage;

			while (current != NULL_INDEX)
			{
				if (freeRegions[current].size >= count)
				{
					size_t allocOffset = freeRegions[current].offset;

					// Update free region
					freeRegions[current].offset += count;
					freeRegions[current].size -= count;

					// Remove the link if no space left
					if (freeRegions[current].size == 0)
					{
						if (prev != NULL_INDEX)
						{
							freeRegions[prev].next = freeRegions[current].next;
						}
						else
						{
							// Update head of free regions
							registry.m_freeRegionsRoot = freeRegions[current].next;
						}

						freeRegions[current] = AllocLink(0, 0); // Mark as unused
					}

					// Add to alloc map
					registry.m_allocMap[allocOffset] = count;

					// Register allocation in tracy
					TracyAlloc(&registry.m_pageStorage[allocOffset], count * sizeof(T));

					return &registry.m_pageStorage[allocOffset];
				}

				prev = current;
				current = freeRegions[current].next;
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
			if (count == NULL_INDEX)
				return -3; // Failure: Not allocated

			// Unregister allocation in tracy
			TracyFree(ptr);

			// Remove from alloc map
			registry.m_allocMap[offset] = NULL_INDEX;

			auto &freeRegions = registry.m_freeRegionLinkStorage;

			// Handle case where pool is full
			if (registry.m_freeRegionsRoot == NULL_INDEX)
			{
				// Add this allocation as the only free region
				size_t newLinkIndex = registry.FindFreeRegion();
				registry.m_freeRegionsRoot = newLinkIndex;
				freeRegions[newLinkIndex] = AllocLink(offset, count);
			}
			else
			{
				// Find correct position to insert freed region
				// such that the insertion point falls after 'left' and before 'right'
				size_t left = NULL_INDEX;
				size_t right = registry.m_freeRegionsRoot;

				while (right != NULL_INDEX)
				{
					if (offset < freeRegions[right].offset)
						break;

					left = right;
					right = freeRegions[right].next;
				}

				// If regions are contiguous, merge them instead of creating a new link
				if (left != NULL_INDEX && (freeRegions[left].offset + freeRegions[left].size == offset))
				{
					freeRegions[left].size += count;

					if (right && (offset + count == freeRegions[right].offset))
					{
						// Merge with next region as well
						freeRegions[left].size += freeRegions[right].size;
						freeRegions[left].next = freeRegions[right].next;

						freeRegions[right] = AllocLink(0, 0); // Mark as unused
					}
				}
				else if (right != NULL_INDEX && (offset + count == freeRegions[right].offset))
				{
					// Merge with next region
					freeRegions[right].offset = offset;
					freeRegions[right].size += count;
				}
				else // Region is not contiguous with either side, insert new link
				{
					// Insert new free region
					size_t newLinkIndex = registry.FindFreeRegion();
					freeRegions[newLinkIndex] = AllocLink(offset, count);

					if (right != NULL_INDEX && left == NULL_INDEX)
					{
						// Inserting at head
						freeRegions[newLinkIndex].next = registry.m_freeRegionsRoot;
						registry.m_freeRegionsRoot = newLinkIndex;
					}
					else if (left != NULL_INDEX)
					{
						// Inserting in middle or end
						freeRegions[newLinkIndex].next = freeRegions[left].next;
						freeRegions[left].next = newLinkIndex;
					}
				}
			}

			return 0; // Success
		}

		const static std::vector<T> &DBG_GetPageStorage()
		{
			if (!Get().m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Ensure initialized for debugging

			return Get().m_pageStorage;
		}
		const static std::vector<AllocLink> &DBG_GetFreeRegions()
		{
			if (!Get().m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Ensure initialized for debugging

			return Get().m_freeRegionLinkStorage;
		}
		const static size_t DBG_GetFreeRegionRoot()
		{
			if (!Get().m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Ensure initialized for debugging

			return Get().m_freeRegionsRoot;
		}
		const static std::vector<size_t> &DBG_GetAllocMap()
		{
			if (!Get().m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Ensure initialized for debugging

			return Get().m_allocMap;
		}

	private:
		std::vector<AllocLink> m_freeRegionLinkStorage;

		std::vector<T> m_pageStorage;
		std::vector<size_t> m_allocMap; // Offset to size mapping
		size_t m_freeRegionsRoot = NULL_INDEX;

		bool m_initialized = false;
		size_t m_maxCount = 0;


		PageRegistry() = default;
		~PageRegistry() = default;

		[[nodiscard]] static PageRegistry<T> &Get()
		{
			static PageRegistry<T> instance;
			return instance;
		}

		[[nodiscard]] size_t FindFreeRegion()
		{
			// Look through free region links to find first with size of 0, meaning unused
			for (size_t i = 0; i < m_freeRegionLinkStorage.size(); ++i)
			{
				if (m_freeRegionLinkStorage[i].size == 0)
					return i;
			}

			// No free link found
			return NULL_INDEX;
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