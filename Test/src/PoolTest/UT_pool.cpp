#include "../../../Application/inc/PageRegistry.hpp"
#include <gtest/gtest.h>

#pragma warning(disable: 6262) // Disable stack size warning

// Types

struct TestStruct
{
	int a;
	double b;
	char c;
};


// Helper functions

template <typename T>
bool IsAddressAllocated(T *addr)
{
	using namespace MemoryInternal;

	const auto &allocMap = PageRegistry<T>::DBG_GetAllocMap();
	const auto &pageStorage = PageRegistry<T>::DBG_GetPageStorage();

	size_t addrOffset = addr - pageStorage.data();

	// Step back from addrOffset to find the allocation start
	size_t i = addrOffset;

	while (i >= 0 && i < allocMap.size())
	{
		if (allocMap[i] == (size_t)-1)
		{
			--i;
			continue;
		}

		if (addrOffset < i + allocMap[i])
		{
			return true;
		}
		else
		{
			return false; // Not found
		}
	}

	return false;
}

template <typename T>
size_t GetAllocatedSize(T *addr)
{
	using namespace MemoryInternal;

	const auto &allocMap = PageRegistry<T>::DBG_GetAllocMap();
	const auto &pageStorage = PageRegistry<T>::DBG_GetPageStorage();

	size_t addrOffset = addr - pageStorage.data();

	// Step back from addrOffset to find the allocation start
	size_t i = addrOffset;

	while (i >= 0 && i < allocMap.size())
	{
		if (allocMap[i] == (size_t)-1)
		{
			--i;
			continue;
		}

		if (addrOffset < i + allocMap[i])
		{
			return allocMap[i];
		}
		else
		{
			return 0; // Not found
		}
	}

	return 0;
}


// Tests

TEST(PoolTest, AllocFree)
{
	using namespace MemoryInternal;

	PageRegistry<int>::Reset();

	int *allocInt = Alloc<int>(1);
	
	(*allocInt) = 69;

	ASSERT_EQ(*allocInt, 69);

	Free<int>(allocInt);
}

TEST(PoolTest, DuplicateAlloc)
{
	using namespace MemoryInternal;

	PageRegistry<int>::Reset();

	int *allocArray[3]{};
	int allocSizes[3] = { 5, 10, 18 };
		
	for (int i = 0; i < 3; ++i)
	{
		allocArray[i] = Alloc<int>(allocSizes[i]);

		for (int j = 0; j < allocSizes[i]; ++j)
			allocArray[i][j] = i * 100 + j;
	}

	for (int i = 1; i < 3; ++i)
	{
		ASSERT_EQ(allocArray[i - 1] + allocSizes[i - 1], allocArray[i]);
	}

	for (int i = 1; i < 3; ++i)
	{
		Free<int>(allocArray[i]);
	}
}

TEST(PoolTest, UnorderedAlloc)
{
	using namespace MemoryInternal;

	PageRegistry<int>::Reset();

	int *allocArray[3]{ nullptr, nullptr, nullptr };
	int allocSizes[3]{ 5, 10, 18 };
		
	for (int i = 0; i < 3; ++i)
	{
		allocArray[i] = Alloc<int>(allocSizes[i]);

		ASSERT_TRUE(allocArray[i] != nullptr);

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

	PageRegistry<int>::Reset();

	int *alloc1 = Alloc<int>(10);
	int *alloc2 = Alloc<int>(20);

	ASSERT_EQ(IsAddressAllocated<int>(alloc1), true);
	ASSERT_EQ(IsAddressAllocated<int>(alloc2), true);

	ASSERT_EQ(Free<int>(alloc1), 0);

	ASSERT_EQ(IsAddressAllocated<int>(alloc1), false);
	ASSERT_EQ(IsAddressAllocated<int>(alloc2), true);

	int *alloc3 = Alloc<int>(5);

	ASSERT_EQ(alloc3, alloc1); // Should reuse freed space

	ASSERT_EQ(Free<int>(alloc2), 0);
	ASSERT_EQ(Free<int>(alloc3), 0);
}

TEST(PoolTest, AllocFreeEdgeCases)
{
	using namespace MemoryInternal;

	PageRegistry<int>::Reset();

	int *allocInt = Alloc<int>(1);
	ASSERT_TRUE(allocInt != nullptr);

	// Freeing nullptr
	int result = Free<int>(nullptr);
	ASSERT_EQ(result, -1);

	// Freeing unallocated pointer
	int dummy;
	result = Free<int>(&dummy);
	ASSERT_EQ(result, -2);

	// Allocating zero size
	int *allocZero = Alloc<int>(0);
	ASSERT_EQ(allocZero, nullptr);

	// Allocating more than max size
	int *allocTooLarge = Alloc<int>(MemoryInternal::PageRegistry<int>::DBG_GetPageStorage().size() + 1);
	ASSERT_EQ(allocTooLarge, nullptr);

	ASSERT_EQ(Free<int>(allocInt), 0);

	// Double free check
	ASSERT_EQ(Free<int>(allocInt), -3);
}

TEST(PoolTest, AllocFreeMultipleTypes)
{
	using namespace MemoryInternal;

	PageRegistry<int>::Reset();
	PageRegistry<double>::Reset();
	PageRegistry<char>::Reset();

	int *allocInt = Alloc<int>(10);
	double *allocDouble = Alloc<double>(5);
	char *allocChar = Alloc<char>(20);

	ASSERT_TRUE(allocInt != nullptr);
	ASSERT_TRUE(allocDouble != nullptr);
	ASSERT_TRUE(allocChar != nullptr);

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

	PageRegistry<TestStruct>::Reset();

	TestStruct *allocStruct = Alloc<TestStruct>(10);

	ASSERT_TRUE(allocStruct != nullptr);

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

	PageRegistry<float>::Reset();
	PageRegistry<float>::Initialize(1ull << 16);

	float *allocs[allocCount]{ nullptr };
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
				size_t allocSize = GetAllocatedSize<float>(allocs[freeIdx]) / sizeof(float);
				for (size_t k = 0; k < allocSize; ++k)
				{
					if (allocs[freeIdx])
						allocs[freeIdx][k] = 0.0f;
				}

				ASSERT_EQ(Free<float>(allocs[freeIdx]), 0);

				allocs[freeIdx] = nullptr;
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

			if (allocIdx == -1)
				continue;

			ASSERT_TRUE(allocIdx != -1);
			float *newAlloc = Alloc<float>(allocSize);
			ASSERT_TRUE(newAlloc != nullptr);

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
	for (int i = 0; i < currAllocs.size(); ++i)
	{
		int allocIdx = currAllocs[i];
		ASSERT_EQ(Free<float>(allocs[allocIdx]), 0);
	}
}

TEST(PoolTest, UnorderedAllocFreeStress_New)
{
	float *allocs[allocCount]{ nullptr };
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
				size_t allocSize = GetAllocatedSize<float>(allocs[freeIdx]) / sizeof(float);
				for (size_t k = 0; k < allocSize; ++k)
				{
					if (allocs[freeIdx])
						allocs[freeIdx][k] = 0.0f;
				}

				delete[] allocs[freeIdx];

				allocs[freeIdx] = nullptr;
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

			if (allocIdx == -1)
				continue;

			ASSERT_TRUE(allocIdx != -1);
			float *newAlloc = new float[allocSize];
			ASSERT_TRUE(newAlloc != nullptr);

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
	for (int i = 0; i < currAllocs.size(); ++i)
	{
		int allocIdx = currAllocs[i];
		delete[] allocs[allocIdx];
	}
}

#pragma warning(default: 6262) // Reset stack size warning