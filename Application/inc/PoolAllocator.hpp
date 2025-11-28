// PoolAllocator.h allocates, stores & manages all type-specific memory pages.

#pragma once

#include "TracyWrapper.hpp"

#include <vector>
#include <memory>

//#define FREE_REGION_CACHE

namespace MemoryInternal
{
	//typedef size_t IndexType;
	typedef unsigned int IndexType;

	constexpr IndexType NULL_INDEX = static_cast<IndexType>(-1);
	constexpr IndexType DEFAULT_PAGE_SIZE = (1 << 13);
#ifdef FREE_REGION_CACHE
	constexpr IndexType FREE_REGION_CACHE_GRANULARITY = (1 << 5);
#endif


	template <typename T>
	class PoolAllocator
	{
	public:
		struct AllocLink
		{
			IndexType offset;
			IndexType size;
			IndexType next;

			AllocLink() : offset(0), size(0), next(NULL_INDEX) {}
			AllocLink(IndexType off, IndexType sz)
				: offset(off), size(sz), next(NULL_INDEX) {
			}
		};

		static int Initialize(IndexType maxCount)
		{
			PoolAllocator<T> &registry = Get();

			if (registry.m_initialized)
				return -1; // Failure: Already initialized

			if (maxCount <= 0)
				return -2; // Failure: Invalid max count

			if ((maxCount & (maxCount - 1)) != 0)
				return -3; // Failure: Max count must be a power of two

			ZoneScopedXC(tracy::Color::DarkOrchid2);

			registry.m_pageStorage.resize(maxCount);
			registry.m_allocMap.resize(maxCount);
			registry.m_freeRegionLinkStorage.resize(maxCount);
#ifdef FREE_REGION_CACHE
			registry.m_freeRegionsCache.resize(FREE_REGION_CACHE_GRANULARITY);
#endif

			registry.m_maxCount = maxCount;
			registry.m_initialized = true;
			registry.m_freeRegionsRoot = 0;

			std::fill(registry.m_allocMap.begin(), registry.m_allocMap.end(), NULL_INDEX);
			std::fill(registry.m_freeRegionLinkStorage.begin(), registry.m_freeRegionLinkStorage.end(), AllocLink(0, 0));
#ifdef FREE_REGION_CACHE
			std::fill(registry.m_freeRegionsCache.begin(), registry.m_freeRegionsCache.end(), NULL_INDEX);
#endif

			registry.m_freeRegionLinkStorage[0] = AllocLink(0, maxCount);

			return 0; // Success
		}
		static void Reset()
		{
			ZoneScopedXC(tracy::Color::Seashell2);

			PoolAllocator<T> &registry = Get();
			registry.m_pageStorage = std::vector<T>();
			registry.m_freeRegionLinkStorage = std::vector<AllocLink>();
#ifdef FREE_REGION_CACHE
			registry.m_freeRegionsCache = std::vector<IndexType>();
#endif
			registry.m_allocMap = std::vector<IndexType>();
			registry.m_freeRegionsRoot = 0;
			registry.m_initialized = false;
			registry.m_maxCount = 0;
		}

		[[nodiscard]] static T *Alloc(IndexType count)
		{
			ZoneScopedXC(tracy::Color::Goldenrod2);

			PoolAllocator<T> &registry = Get();

			if (!registry.m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Default max count

			if (count == 0 || count > registry.m_maxCount)
				return nullptr; // Failure: Invalid count

			// Find first free region of sufficient size
			IndexType prev = NULL_INDEX;
			IndexType current = registry.m_freeRegionsRoot;

			auto &freeRegions = registry.m_freeRegionLinkStorage;

			while (current != NULL_INDEX)
			{
				if (freeRegions[current].size >= count)
				{
					ZoneNamedXNC(allocateRegionZone, "Allocate Region", tracy::Color::Gold, true);

					IndexType allocOffset = freeRegions[current].offset;

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
					TracyAllocN(&registry.m_pageStorage[allocOffset], count * sizeof(T), "Pool");

#ifdef FREE_REGION_CACHE
					// Relace cache within the allocated region with the previous free region
					registry.RemoveIndexFromCache(allocOffset, count, current);
#endif

					return &registry.m_pageStorage[allocOffset];
				}

				prev = current;
				current = freeRegions[current].next;
			}

			return nullptr; // Failure: No sufficient free region
		}
		static int Free(T *ptr)
		{
			ZoneScopedXC(tracy::Color::LavenderBlush1);

			PoolAllocator<T> &registry = Get();
			if (!registry.m_initialized || ptr == nullptr) [[unlikely]]
				return -1;

			IndexType offset = static_cast<IndexType>(ptr - registry.m_pageStorage.data());

			if (offset >= registry.m_maxCount) [[unlikely]]
				return -2; // Failure: Invalid pointer

			IndexType count = registry.m_allocMap[offset];
			if (count == NULL_INDEX) [[unlikely]]
				return -3; // Failure: Not allocated

			// Unregister allocation in tracy
			TracyFreeN(ptr, "Pool");

			// Remove from alloc map
			registry.m_allocMap[offset] = NULL_INDEX;

			auto &freeRegions = registry.m_freeRegionLinkStorage;

			// Handle case where pool is full
			if (registry.m_freeRegionsRoot == NULL_INDEX) [[unlikely]]
			{
				ZoneNamedXNC(poolFullZone, "Pool Full", tracy::Color::Moccasin, true);

				// Add this allocation as the only free region
				IndexType newLinkIndex = registry.FindFreeRegion();
				registry.m_freeRegionsRoot = newLinkIndex;
				freeRegions[newLinkIndex] = AllocLink(offset, count);

#ifdef FREE_REGION_CACHE
				// Update the cache in the new region
				registry.UpdateCacheInRange(offset, count, newLinkIndex);
#endif
			}
			else
			{
				ZoneNamedXNC(findInsertRegionZone, "Insert Region", tracy::Color::Thistle2, true);

				// Find correct position to insert freed region
				// such that the insertion point falls after 'left' and before 'right'
				IndexType left = NULL_INDEX;
				IndexType right = registry.m_freeRegionsRoot;

#ifdef FREE_REGION_CACHE
				// First check if we can start from the closest cached region
				// For this to be the case, the cached region before this one must be valid
				IndexType cachedRegion = registry.GetClosestCachedRegion(offset);

				if (cachedRegion != NULL_INDEX)
				{
					ZoneNamedXNC(checkInCacheZone, "Check Cache", tracy::Color::LightSeaGreen, true);

					AllocLink &cachedLink = freeRegions[cachedRegion];

					if (cachedLink.offset < offset && cachedLink.size != 0) // Valid cached region
					{
						if (cachedLink.offset + cachedLink.size <= offset)
						{
							ZoneNamedXNC(foundInCacheZone, "Cache Hit", tracy::Color::MediumPurple2, true);
							ZoneValue(cachedLink.offset);

							// Start from cached region
							left = cachedRegion;
							right = cachedLink.next;
						}
					}
				}
#endif

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
					ZoneNamedXNC(mergeLeftZone, "Merge Left", tracy::Color::Brown2, true);

#ifdef FREE_REGION_CACHE
					IndexType growSize = count;
					IndexType growStart = offset;
#endif

					freeRegions[left].size += count;

					if (right != NULL_INDEX && (offset + count == freeRegions[right].offset))
					{
						ZoneNamedXNC(mergeRightZone, "Merge Right", tracy::Color::Brown2, true);

						// Merge with next region as well
						freeRegions[left].size += freeRegions[right].size;
						freeRegions[left].next = freeRegions[right].next;
#ifdef FREE_REGION_CACHE
						growSize += freeRegions[right].size;
#endif

						freeRegions[right] = AllocLink(0, 0); // Mark as unused
					}

#ifdef FREE_REGION_CACHE
					// Update the cache in the grown region
					registry.UpdateCacheInRange(growStart, growSize, left);
#endif
				}
				else if (right != NULL_INDEX && (offset + count == freeRegions[right].offset))
				{
					ZoneNamedXNC(mergeRightZone, "Merge Right", tracy::Color::Brown2, true);

					// Merge with next region
					freeRegions[right].offset = offset;
					freeRegions[right].size += count;

#ifdef FREE_REGION_CACHE
					// Update the cache in the grown region
					registry.UpdateCacheInRange(offset, freeRegions[right].size, right);
#endif
				}
				else // Region is not contiguous with either side, insert new link
				{
					ZoneNamedXNC(insertNewRegionZone, "New Region", tracy::Color::Coral2, true);

					// Insert new free region
					IndexType newLinkIndex = registry.FindFreeRegion();
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

#ifdef FREE_REGION_CACHE
					// Update the cache in the new region
					registry.UpdateCacheInRange(offset, count, newLinkIndex);
#endif
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
		static IndexType DBG_GetFreeRegionRoot()
		{
			if (!Get().m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Ensure initialized for debugging

			return Get().m_freeRegionsRoot;
		}
		const static std::vector<IndexType> &DBG_GetAllocMap()
		{
			if (!Get().m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Ensure initialized for debugging

			return Get().m_allocMap;
		}

	private:
		std::vector<T> m_pageStorage;
		std::vector<IndexType> m_allocMap; // Offset to size mapping
		std::vector<AllocLink> m_freeRegionLinkStorage;
#ifdef FREE_REGION_CACHE
		std::vector<IndexType> m_freeRegionsCache;
#endif

		bool m_initialized = false;
		IndexType m_maxCount = 0;
		IndexType m_freeRegionsRoot = NULL_INDEX;


		PoolAllocator() = default;
		~PoolAllocator() = default;

		[[nodiscard]] static PoolAllocator<T> &Get()
		{
			static PoolAllocator<T> instance;
			return instance;
		}

		[[nodiscard]] IndexType FindFreeRegion()
		{
			ZoneScopedXC(tracy::Color::Sienna2);

			// Look through free region links to find first with size of 0, meaning unused
			for (IndexType i = 0; i < m_freeRegionLinkStorage.size(); ++i)
			{
				if (m_freeRegionLinkStorage[i].size == 0)
					return i;
			}

			// No free link found
			return NULL_INDEX;
		}

#ifdef FREE_REGION_CACHE
		[[nodiscard]] IndexType GetFreeRegionCacheIndex(IndexType offset) const
		{
			return (static_cast<size_t>(offset) * FREE_REGION_CACHE_GRANULARITY) / m_maxCount;
		}

		void ReplaceCacheIfCloser(IndexType offset, IndexType newIndex)
		{
			ZoneScopedXC(tracy::Color::Firebrick1);

			IndexType cacheIndex = GetFreeRegionCacheIndex(offset);
			IndexType cachedRegion = m_freeRegionsCache[cacheIndex];

			if (cachedRegion == NULL_INDEX)
				return;

			AllocLink &cachedLink = m_freeRegionLinkStorage[cachedRegion];

			if (offset < cachedLink.offset)
				cachedRegion = offset;
		}

		void UpdateCacheInRange(IndexType offset, IndexType count, IndexType newIndex)
		{
			ZoneScopedXC(tracy::Color::DarkOrange2);

			IndexType startCacheIndex = GetFreeRegionCacheIndex(offset);
			IndexType endCacheIndex = GetFreeRegionCacheIndex(offset + count - 1);

			AllocLink &newLink = m_freeRegionLinkStorage[newIndex];

			for (IndexType i = startCacheIndex; i <= endCacheIndex; ++i)
			{
				if (m_freeRegionsCache[i] == NULL_INDEX)
				{
					m_freeRegionsCache[i] = newIndex;
				}
				else
				{
					AllocLink &cachedLink = m_freeRegionLinkStorage[m_freeRegionsCache[i]];
					if (newLink.offset < cachedLink.offset)
						m_freeRegionsCache[i] = newIndex;
				}
			}
		}

		void RemoveIndexFromCache(IndexType offset, IndexType size, IndexType removedIndex)
		{
			ZoneScopedXC(tracy::Color::Magenta2);

			IndexType startCacheIndex = GetFreeRegionCacheIndex(offset);
			IndexType endCacheIndex = GetFreeRegionCacheIndex(offset + size - 1);

			IndexType prevCacheIndex = m_freeRegionsCache[startCacheIndex];
			if (prevCacheIndex == removedIndex && startCacheIndex > 0)
				prevCacheIndex = m_freeRegionsCache[static_cast<size_t>(startCacheIndex) - 1];

			for (IndexType i = startCacheIndex; i <= endCacheIndex; ++i)
			{
				if (i != startCacheIndex && m_freeRegionsCache[i] != removedIndex)
					break;

				m_freeRegionsCache[i] = prevCacheIndex;
			}
		}

		IndexType GetClosestCachedRegion(IndexType offset)
		{
			// Step back from the cache index to find the closest valid cached region
			IndexType cacheIndex = GetFreeRegionCacheIndex(offset);

			while (true)
			{
				if (cacheIndex != NULL_INDEX)
				{
					IndexType cachedRegion = m_freeRegionsCache[cacheIndex];
					if (cachedRegion != NULL_INDEX)
					{
						AllocLink &cachedLink = m_freeRegionLinkStorage[cachedRegion];

						if (cachedLink.offset < offset)
							return cachedRegion;
					}
				}

				if (cacheIndex == 0)
					break;

				cacheIndex--;
			}

			return NULL_INDEX;
		}
#endif
	};


	template <typename T>
	[[nodiscard]] inline T *Alloc(IndexType count)
	{
		return PoolAllocator<T>::Alloc(count);
	}

	template <typename T>
	inline int Free(T *ptr)
	{
		return PoolAllocator<T>::Free(ptr);
	}
};