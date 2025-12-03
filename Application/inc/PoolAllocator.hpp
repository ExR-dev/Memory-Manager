// PoolAllocator.h allocates, stores & manages all type-specific memory pages.

#pragma once

#include "TracyWrapper.hpp"

#include <vector>
#include <memory>

namespace MemoryInternal
{
	//typedef size_t IndexType;
	typedef unsigned int IndexType;

	constexpr IndexType NULL_INDEX = static_cast<IndexType>(-1);
	constexpr IndexType DEFAULT_PAGE_SIZE = (1 << 13);


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

			ZoneScopedXC(tracy::Color::DarkOrchid2);

			registry.m_pageStorage.resize(maxCount);
			registry.m_allocMap.resize(maxCount);
			registry.m_freeRegionLinkStorage.resize(maxCount / 2 + 1);

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
			PoolAllocator<T> &registry = Get();

			if (!registry.m_initialized)
				return; // Not initialized

			ZoneScopedXC(tracy::Color::Seashell2);

			registry.m_pageStorage = std::vector<T>();
			registry.m_allocMap = std::vector<IndexType>();
			registry.m_freeRegionLinkStorage = std::vector<AllocLink>();

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

			bool foundRegion = false;

			auto &freeRegions = registry.m_freeRegionLinkStorage;

			while (current != NULL_INDEX)
			{
				ZoneNamedXNC(checkRegionZone, "Check Region", tracy::Color::MediumOrchid, true);

				if (freeRegions[current].size >= count)
				{
					foundRegion = true;
					break;
				}

				prev = current;
				current = freeRegions[current].next;
			}

			if (foundRegion && current != NULL_INDEX)
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

					// Mark as unused
					freeRegions[current].next = NULL_INDEX;
					freeRegions[current].size = 0;
				}

				// Add to alloc map
				registry.m_allocMap[allocOffset] = count;

				// Register allocation in tracy
				TracyAllocN(&registry.m_pageStorage[allocOffset], count * sizeof(T), "Pool");

				return &registry.m_pageStorage[allocOffset];
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
			}
			else
			{
				ZoneNamedXNC(findInsertRegionZone, "Insert Region", tracy::Color::Thistle2, true);

				// Find correct position to insert freed region
				// such that the insertion point falls after 'left' and before 'right'
				IndexType left = NULL_INDEX;
				IndexType right = registry.m_freeRegionsRoot;

				while (right != NULL_INDEX)
				{
					ZoneNamedXNC(traverseRegionsZone, "Traverse Regions", tracy::Color::Plum, true);

					if (offset < freeRegions[right].offset)
						break;

					left = right;
					right = freeRegions[right].next;
				}

				// If regions are contiguous, merge them instead of creating a new link
				if (left != NULL_INDEX && (freeRegions[left].offset + freeRegions[left].size == offset))
				{
					ZoneNamedXNC(mergeLeftZone, "Merge Left", tracy::Color::Brown2, true);

					freeRegions[left].size += count;

					if (right != NULL_INDEX && (offset + count == freeRegions[right].offset))
					{
						ZoneNamedXNC(mergeRightZone, "Merge Right", tracy::Color::Brown2, true);

						// Merge with next region as well
						freeRegions[left].size += freeRegions[right].size;
						freeRegions[left].next = freeRegions[right].next;

						freeRegions[right] = AllocLink(0, 0); // Mark as unused
					}
				}
				else if (right != NULL_INDEX && (offset + count == freeRegions[right].offset))
				{
					ZoneNamedXNC(mergeRightZone, "Merge Right", tracy::Color::Brown2, true);

					// Merge with next region
					freeRegions[right].offset = offset;
					freeRegions[right].size += count;
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
		const static std::vector<IndexType> &DBG_GetAllocMap()
		{
			if (!Get().m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Ensure initialized for debugging

			return Get().m_allocMap;
		}
		const static std::vector<AllocLink> &DBG_GetFreeRegions()
		{
			if (!Get().m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Ensure initialized for debugging

			return Get().m_freeRegionLinkStorage;
		}
		const static IndexType DBG_GetFreeRegionRoot()
		{
			if (!Get().m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Ensure initialized for debugging

			return Get().m_freeRegionsRoot;
		}

	private:
		std::vector<T> m_pageStorage;
		std::vector<IndexType> m_allocMap; // Offset to size mapping
		std::vector<AllocLink> m_freeRegionLinkStorage;

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