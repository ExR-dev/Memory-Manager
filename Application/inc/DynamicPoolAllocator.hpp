#pragma once

#include "TracyWrapper.hpp"

#include <vector>
#include <memory>

namespace MemoryInternalDynamic
{
	typedef unsigned int IndexType;

	const IndexType NULL_INDEX = static_cast<IndexType>(-1);
	const IndexType DEFAULT_PAGE_SIZE = (1 << 13);


	template <typename T>
	class DynamicPoolAllocator
	{
	public:
		struct AllocLink
		{
			IndexType offset;
			IndexType size;
			std::unique_ptr<AllocLink> next;

			AllocLink() : offset(0), size(0), next(nullptr) {}
			AllocLink(IndexType off, IndexType sz)
				: offset(off), size(sz), next(nullptr) {}
		};

		static int Initialize(IndexType maxCount)
		{
			DynamicPoolAllocator<T> &registry = Get();

			if (registry.m_initialized)
				return -1; // Failure: Already initialized

			if (maxCount <= 0)
				return -2; // Failure: Invalid max count

			if ((maxCount & (maxCount - 1)) != 0)
				return -3; // Failure: Max count must be a power of two

			ZoneScopedXC(tracy::Color::DarkOrchid2);

			registry.m_pageStorage.resize(maxCount);
			registry.m_allocMap.resize(maxCount);
			registry.m_freeRegionsRoot = std::make_unique<AllocLink>(0, maxCount);

			registry.m_maxCount = maxCount;
			registry.m_initialized = true;

			std::fill(registry.m_allocMap.begin(), registry.m_allocMap.end(), NULL_INDEX);

			return 0; // Success
		}
		static void Reset()
		{
			ZoneScopedXC(tracy::Color::Seashell2);

			DynamicPoolAllocator<T> &registry = Get();

			registry.m_pageStorage = std::vector<T>();
			registry.m_allocMap = std::vector<IndexType>();
			registry.m_freeRegionsRoot = nullptr;

			registry.m_initialized = false;
			registry.m_maxCount = 0;
		}

		[[nodiscard]] static T *Alloc(IndexType count)
		{
			ZoneScopedXC(tracy::Color::Goldenrod2);

			DynamicPoolAllocator<T> &registry = Get();

			if (!registry.m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Default max count

			if (count == 0 || count > registry.m_maxCount)
				return nullptr; // Failure: Invalid count

			// Find first free region of sufficient size
			AllocLink *prev = nullptr;
			AllocLink *curr = registry.m_freeRegionsRoot.get();

			while (curr)
			{
				if (curr->size >= count)
				{
					ZoneNamedXNC(allocateRegionZone, "Allocate Region", tracy::Color::Gold, true);

					IndexType allocOffset = curr->offset;

					// Update free region
					curr->offset += count;
					curr->size -= count;

					// Remove the link if no space left
					if (curr->size == 0)
					{
						if (prev)
						{
							prev->next = std::move(curr->next);
						}
						else
						{
							// Update head of free regions
							registry.m_freeRegionsRoot = std::move(curr->next);
						}
					}

					// Add to alloc map
					registry.m_allocMap[allocOffset] = count;

					// Register allocation in tracy
					TracyAllocN(&registry.m_pageStorage[allocOffset], count * sizeof(T), "PoolDyn");

					return &registry.m_pageStorage[allocOffset];
				}

				prev = curr;
				curr = curr->next.get();
			}

			return nullptr; // Failure: No sufficient free region
		}
		static int Free(T *ptr)
		{
			ZoneScopedXC(tracy::Color::LavenderBlush1);

			DynamicPoolAllocator<T> &registry = Get();
			if (!registry.m_initialized || ptr == nullptr) [[unlikely]]
				return -1;

			IndexType offset = static_cast<IndexType>(ptr - registry.m_pageStorage.data());

			if (offset >= registry.m_maxCount) [[unlikely]]
				return -2; // Failure: Invalid pointer

			IndexType count = registry.m_allocMap[offset];
			if (count == NULL_INDEX) [[unlikely]]
				return -3; // Failure: Not allocated

			// Unregister allocation in tracy
			TracyFreeN(ptr, "PoolDyn");

			// Remove from alloc map
			registry.m_allocMap[offset] = NULL_INDEX;

			// Handle case where pool is full
			if (!registry.m_freeRegionsRoot) [[unlikely]]
			{
				ZoneNamedXNC(poolFullZone, "Pool Full", tracy::Color::Moccasin, true);

				// Add this allocation as the only free region
				registry.m_freeRegionsRoot = std::make_unique<AllocLink>(offset, count);
			}
			else
			{
				ZoneNamedXNC(findInsertRegionZone, "Insert Region", tracy::Color::Thistle2, true);

				// Find correct position to insert freed region
				// such that the insertion point falls after 'left' and before 'right'
				AllocLink *left = nullptr;
				AllocLink *right = registry.m_freeRegionsRoot.get();

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
					ZoneNamedXNC(mergeLeftZone, "Merge Left", tracy::Color::Brown2, true);

					left->size += count;

					if (right && (offset + count == right->offset))
					{
						ZoneNamedXNC(mergeRightZone, "Merge Right", tracy::Color::Brown2, true);

						// Merge with next region as well
						left->size += right->size;
						left->next = std::move(right->next);
					}
				}
				else if (right && (offset + count == right->offset))
				{
					ZoneNamedXNC(mergeRightZone, "Merge Right", tracy::Color::Brown2, true);

					// Merge with next region
					right->offset = offset;
					right->size += count;
				}
				else // Region is not contiguous with either side, insert new link
				{
					ZoneNamedXNC(insertNewRegionZone, "New Region", tracy::Color::Coral2, true);

					// Insert new free region
					std::unique_ptr<AllocLink> newLink = std::make_unique<AllocLink>(offset, count);

					if (right && !left)
					{
						// Inserting at head
						newLink->next = std::move(registry.m_freeRegionsRoot);
						registry.m_freeRegionsRoot = std::move(newLink);
					}
					else if (left)
					{
						// Inserting in middle or end
						newLink->next = std::move(left->next);
						left->next = std::move(newLink);
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
		const static AllocLink *DBG_GetFreeRegionRoot()
		{
			if (!Get().m_initialized)
				Initialize(DEFAULT_PAGE_SIZE); // Ensure initialized for debugging

			return Get().m_freeRegionsRoot.get();
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
		std::unique_ptr<AllocLink> m_freeRegionsRoot = nullptr;

		bool m_initialized = false;
		IndexType m_maxCount = 0;


		DynamicPoolAllocator() = default;
		~DynamicPoolAllocator() = default;

		[[nodiscard]] static DynamicPoolAllocator<T> &Get()
		{
			static DynamicPoolAllocator<T> instance;
			return instance;
		}
	};


	template <typename T>
	[[nodiscard]] inline T *Alloc(IndexType count)
	{
		return DynamicPoolAllocator<T>::Alloc(count);
	}

	template <typename T>
	inline int Free(T *ptr)
	{
		return DynamicPoolAllocator<T>::Free(ptr);
	}
};