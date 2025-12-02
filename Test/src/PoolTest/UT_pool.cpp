#undef TRACY_ENABLE

#include "../../../Application/inc/PoolAllocator.hpp"
#include <gtest/gtest.h>

// Types

struct TestStruct
{
	int a;
	double b;
	char c;
};


// Helper functions

template <typename T>
bool IsAddressAllocated(typename MemoryInternal::PoolPtr<T> addr)
{
	using namespace MemoryInternal;

	const auto &pageStorage = PoolAllocator<T>::DBG_GetPageStorage();
	const auto &freeRegionStorage = PoolAllocator<T>::DBG_GetFreeRegions();
	
	size_t addrOffset = addr.get() - pageStorage.data();
	IndexType addrSize = addr.size();

	IndexType regionIndex = PoolAllocator<T>::DBG_GetFreeRegionRoot();

	// Ensure no free region overlaps with addr
	while (regionIndex != NULL_INDEX)
	{
		const auto &freeRegion = freeRegionStorage[regionIndex];
		if (addrOffset + addrSize <= freeRegion.offset)
		{
			// addr is completely before this free region
			regionIndex = freeRegion.next;
			break;
		}
		else if (addrOffset >= freeRegion.offset + freeRegion.size)
		{
			// addr is completely after this free region
			regionIndex = freeRegion.next;
		}
		else
		{
			// Overlaps with a free region
			return false;
		}
	}

	return true;
}


// Tests

TEST(PoolTest, AllocFree)
{
	using namespace MemoryInternal;

	PoolAllocator<int>::Reset();

	auto allocInt = Alloc<int>(1);
	
	*allocInt = 69;

	ASSERT_EQ(*allocInt, 69);

	Free<int>(allocInt);
}

TEST(PoolTest, DuplicateAlloc)
{
	using namespace MemoryInternal;

	PoolAllocator<int>::Reset();

	PoolPtr<int> allocArray[3]{};
	int allocSizes[3] = { 5, 10, 18 };
		
	for (int i = 0; i < 3; ++i)
	{
		allocArray[i] = Alloc<int>(allocSizes[i]);

		for (int j = 0; j < allocSizes[i]; ++j)
			allocArray[i][j] = i * 100 + j;
	}

	for (int i = 1; i < 3; ++i)
	{
		ASSERT_EQ(allocArray[i - 1].get() + allocArray[i - 1].size(), allocArray[i].get());
	}

	for (int i = 1; i < 3; ++i)
	{
		Free<int>(allocArray[i]);
	}
}

TEST(PoolTest, UnorderedAlloc)
{
	using namespace MemoryInternal;

	PoolAllocator<int>::Reset();

	PoolPtr<int> allocArray[3]{};
	int allocSizes[3]{ 5, 10, 18 };
		
	for (int i = 0; i < 3; ++i)
	{
		allocArray[i] = Alloc<int>(allocSizes[i]);

		ASSERT_TRUE(allocArray[i]);

		for (int j = 0; j < allocSizes[i]; ++j)
			allocArray[i][j] = i * 100 + j;
	}

	ASSERT_TRUE(IsAddressAllocated<int>(allocArray[1]));

	Free<int>(allocArray[1]);
	
	ASSERT_FALSE(IsAddressAllocated<int>(allocArray[1]));
	
	Free<int>(allocArray[0]);
	Free<int>(allocArray[2]);
}

TEST(PoolTest, ReuseFreedSpace)
{
	using namespace MemoryInternal;

	PoolAllocator<int>::Reset();

	PoolPtr<int> alloc1 = Alloc<int>(10);
	PoolPtr<int> alloc2 = Alloc<int>(20);

	ASSERT_TRUE(IsAddressAllocated<int>(alloc1));
	ASSERT_TRUE(IsAddressAllocated<int>(alloc2));

	ASSERT_EQ(Free<int>(alloc1), 0);

	ASSERT_FALSE(IsAddressAllocated<int>(alloc1));
	ASSERT_TRUE(IsAddressAllocated<int>(alloc2));

	PoolPtr<int> alloc3 = Alloc<int>(5);

	ASSERT_EQ(alloc3.get(), alloc1.get()); // Should reuse freed space

	ASSERT_EQ(Free<int>(alloc2), 0);
	ASSERT_EQ(Free<int>(alloc3), 0);
}

TEST(PoolTest, AllocFreeEdgeCases)
{
	using namespace MemoryInternal;

	PoolAllocator<int>::Reset();

	PoolPtr<int> allocInt = Alloc<int>(1);
	ASSERT_TRUE(allocInt);

	// Freeing nullptr
	PoolPtr<int> nullPtr;
	int result = Free<int>(nullPtr);
	ASSERT_EQ(result, -1);

	// Allocating zero size
	PoolPtr<int> allocZero = Alloc<int>(0);
	ASSERT_EQ(allocZero.get(), nullptr);

	// Allocating more than max size
	PoolPtr<int> allocTooLarge = Alloc<int>((MemoryInternal::IndexType)MemoryInternal::PoolAllocator<int>::DBG_GetPageStorage().size() + 1);
	ASSERT_EQ(allocTooLarge.get(), nullptr);

	ASSERT_EQ(Free<int>(allocInt), 0);

	// Double free check
	ASSERT_EQ(Free<int>(allocInt), -4);
}

TEST(PoolTest, AllocFreeMultipleTypes)
{
	using namespace MemoryInternal;

	PoolAllocator<int>::Reset();
	PoolAllocator<double>::Reset();
	PoolAllocator<char>::Reset();

	PoolPtr<int> allocInt = Alloc<int>(10);
	PoolPtr<double> allocDouble = Alloc<double>(5);
	PoolPtr<char> allocChar = Alloc<char>(20);

	ASSERT_TRUE(allocInt);
	ASSERT_TRUE(allocDouble);
	ASSERT_TRUE(allocChar);

	for (int i = 0; i < 10; ++i)
		allocInt[i] = i * 10;
	for (int i = 0; i < 5; ++i)
		allocDouble[i] = i * 0.5;
	for (int i = 0; i < 20; ++i)
		allocChar[i] = 'A' + (char)i;

	for (int i = 0; i < 10; ++i)
		ASSERT_EQ(allocInt[i], i * 10);
	for (int i = 0; i < 5; ++i)
		ASSERT_EQ(allocDouble[i], i * 0.5);
	for (int i = 0; i < 20; ++i)
		ASSERT_EQ(allocChar[i], 'A' + (char)i);

	ASSERT_EQ(Free<int>(allocInt), 0);
	ASSERT_EQ(Free<double>(allocDouble), 0);
	ASSERT_EQ(Free<char>(allocChar), 0);
}

TEST(PoolTest, StructAlloc)
{
	using namespace MemoryInternal;

	PoolAllocator<TestStruct>::Reset();

	PoolPtr<TestStruct> allocStruct = Alloc<TestStruct>(10);

	ASSERT_TRUE(allocStruct);

	for (int i = 0; i < 10; ++i)
	{
		allocStruct[i].a = i;
		allocStruct[i].b = i * 0.1;
		allocStruct[i].c = 'A' + (char)i;
	}

	for (int i = 0; i < 10; ++i)
	{
		ASSERT_EQ(allocStruct[i].a, i);
		ASSERT_EQ(allocStruct[i].b, i * 0.1);
		ASSERT_EQ(allocStruct[i].c, 'A' + (char)i);
	}

	ASSERT_EQ(Free<TestStruct>(allocStruct), 0);
}


constexpr int allocCount = 10000;
constexpr int maxConcurrentAllocs = 8;
constexpr int maxAllocSize = 1 << 11;

TEST(PoolTest, UnorderedAllocFreeStress_Alloc)
{
	using namespace MemoryInternal;

	PoolAllocator<float>::Reset();
	PoolAllocator<float>::Initialize(1ull << 16);

	PoolPtr<float> allocs[allocCount]{};
	std::vector<int> currAllocs;
	currAllocs.reserve(maxConcurrentAllocs);

	for (int i = 0; i < allocCount; )
	{
		if (currAllocs.size() > 0)
		{
			// Free a random number of current allocations
			int freeCount = rand() % (currAllocs.size() / 5 + 1);

			for (int j = 0; j < freeCount; ++j)
			{
				if (currAllocs.size() <= 0)
					break;

				int currAllocIndex = rand() % currAllocs.size();
				int freeIdx = currAllocs[currAllocIndex];

				// Before freeing, set the memory to 0 for verification
				size_t allocSize = (size_t)allocs[freeIdx].size();
				for (size_t k = 0; k < allocSize; ++k)
				{
					if (allocs[freeIdx])
						allocs[freeIdx][k] = 0.0f;
				}

				ASSERT_EQ(Free<float>(allocs[freeIdx]), 0);

				allocs[freeIdx] = {};
				currAllocs.erase(currAllocs.begin() + currAllocIndex);
			}
		}

		// Allocate a random number of floats
		int newAllocs = rand() % ((maxConcurrentAllocs - currAllocs.size()) / 4 + 1);
		for (int j = 0; j < newAllocs; ++j)
		{
			if (currAllocs.size() >= maxConcurrentAllocs)
				break;

			int allocSize = (rand() % maxAllocSize) + 1;
			int allocIdx = -1;

			// Find a free slot
			for (int k = 0; k < allocCount; ++k)
			{
				if (allocs[k] == nullptr)
				{
					allocIdx = k;
					break;
				}
			}

			ASSERT_TRUE(allocIdx != -1);
			PoolPtr<float> newAlloc = Alloc<float>(allocSize);
			ASSERT_TRUE(newAlloc.get() != nullptr);

			allocs[allocIdx] = newAlloc;
			currAllocs.push_back(allocIdx);
			
			++i;

			// Fill allocation with the allocation index for verification
			for (int k = 0; k < allocSize; ++k)
			{
				newAlloc[k] = static_cast<float>(i);
			}
		}
	}

	// Free remaining allocations
	for (std::size_t i = 0; i < currAllocs.size(); ++i)
	{
		int allocIdx = currAllocs[i];
		ASSERT_EQ(Free<float>(allocs[allocIdx]), 0);
	}
}

TEST(PoolTest, UnorderedAllocFreeStress_New)
{
	float *allocs[allocCount]{ nullptr };
	std::vector<int> currAllocs, allocSizes;
	currAllocs.reserve(maxConcurrentAllocs);
	allocSizes.reserve(maxConcurrentAllocs);

	for (int i = 0; i < allocCount; )
	{
		if (currAllocs.size() > 0)
		{
			// Free a random number of current allocations
			int freeCount = rand() % (currAllocs.size() / 5 + 1);

			for (int j = 0; j < freeCount; ++j)
			{
				if (currAllocs.size() <= 0)
					break;

				int currAllocIndex = rand() % currAllocs.size();
				int freeIdx = currAllocs[currAllocIndex];
				int allocSize = allocSizes[currAllocIndex];

				// Before freeing, set the memory to 0 for verification
				for (size_t k = 0; k < (size_t)allocSize; ++k)
				{
					if (allocs[freeIdx])
						allocs[freeIdx][k] = 0.0f;
				}

				delete[] allocs[freeIdx];

				allocs[freeIdx] = nullptr;
				currAllocs.erase(currAllocs.begin() + currAllocIndex);
				allocSizes.erase(allocSizes.begin() + currAllocIndex);
			}
		}

		// Allocate a random number of floats
		int newAllocs = rand() % ((maxConcurrentAllocs - currAllocs.size()) / 4 + 1);
		for (int j = 0; j < newAllocs; ++j)
		{
			if (currAllocs.size() >= maxConcurrentAllocs)
				break;

			int allocSize = (rand() % maxAllocSize) + 1;
			int allocIdx = -1;

			// Find a free slot
			for (int k = 0; k < allocCount; ++k)
			{
				if (allocs[k] == nullptr)
				{
					allocIdx = k;
					break;
				}
			}

			if (allocIdx == -1)
				continue;

			ASSERT_TRUE(allocIdx != -1);
			float *newAlloc = new float[allocSize];
			ASSERT_TRUE(newAlloc != nullptr);

			allocs[allocIdx] = newAlloc;
			currAllocs.push_back(allocIdx);
			allocSizes.push_back(allocSize);

			++i;

			// Fill allocation with the allocation index for verification
			for (int k = 0; k < allocSize; ++k)
			{
				newAlloc[k] = static_cast<float>(i);
			}
		}
	}

	// Free remaining allocations
	for (std::size_t i = 0; i < currAllocs.size(); ++i)
	{
		int allocIdx = currAllocs[i];
		delete[] allocs[allocIdx];
	}
}