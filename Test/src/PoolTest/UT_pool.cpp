#include "../../../Application/inc/PageRegistry.hpp"
#include <gtest/gtest.h>

template <typename T>
bool IsAddressAllocated(T *addr)
{
	using namespace MemoryInternal;

	const auto &allocMap = PageRegistry<T>::DBG_GetAllocMap();
	const auto &pageStorage = PageRegistry<T>::DBG_GetPageStorage();

	for (const auto &alloc : allocMap)
	{
		const T *beginAddr = &pageStorage[alloc.first];
		const T *endAddr = beginAddr + alloc.second;

		if (addr >= beginAddr && addr < endAddr)
			return true;
	}

	return false;
}

template <typename T>
size_t GetAllocatedSize(T *addr)
{
	using namespace MemoryInternal;

	const auto &allocMap = PageRegistry<T>::DBG_GetAllocMap();
	const auto &pageStorage = PageRegistry<T>::DBG_GetPageStorage();

	for (const auto &alloc : allocMap)
	{
		const T *beginAddr = &pageStorage[alloc.first];
		const T *endAddr = beginAddr + alloc.second;

		if (addr >= beginAddr && addr < endAddr)
			return alloc.second * sizeof(T);
	}

	return 0;
}

TEST(PoolTest, AllocFree)
{
	using namespace MemoryInternal;

	PageRegistry<int>::DBG_Reset();

	int *allocInt = Alloc<int>(1);
	
	(*allocInt) = 69;

	ASSERT_EQ(*allocInt, 69);

	Free<int>(allocInt);
}

TEST(PoolTest, DuplicateAlloc)
{
	using namespace MemoryInternal;

	PageRegistry<int>::DBG_Reset();

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

	PageRegistry<int>::DBG_Reset();

	int *allocArray[3]{ nullptr, nullptr, nullptr };
	int allocSizes[3] = { 5, 10, 18 };
		
	for (int i = 0; i < 3; ++i)
	{
		allocArray[i] = Alloc<int>(allocSizes[i]);

		ASSERT_EQ(allocArray[i] != nullptr, true);

		for (int j = 0; j < allocSizes[i]; ++j)
			allocArray[i][j] = i * 100 + j;
	}

	ASSERT_EQ(IsAddressAllocated<int>(allocArray[1]), true);

	Free<int>(allocArray[1]);
	
	ASSERT_EQ(IsAddressAllocated<int>(allocArray[1]), false);
	
	Free<int>(allocArray[0]);
	Free<int>(allocArray[2]);
}

TEST(PoolTest, ReuseFreedSpace)
{
	using namespace MemoryInternal;

	PageRegistry<int>::DBG_Reset();

	int *alloc1 = Alloc<int>(10);
	int *alloc2 = Alloc<int>(20);

	ASSERT_EQ(IsAddressAllocated<int>(alloc1), true);
	ASSERT_EQ(IsAddressAllocated<int>(alloc2), true);

	Free<int>(alloc1);

	ASSERT_EQ(IsAddressAllocated<int>(alloc1), false);
	ASSERT_EQ(IsAddressAllocated<int>(alloc2), true);

	int *alloc3 = Alloc<int>(5);

	ASSERT_EQ(alloc3, alloc1); // Should reuse freed space

	Free<int>(alloc2);
	Free<int>(alloc3);
}

TEST(PoolTest, AllocFreeEdgeCases)
{
	using namespace MemoryInternal;

	PageRegistry<int>::DBG_Reset();

	int *allocInt = Alloc<int>(1);

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
	ASSERT_EQ(Free<int>(allocInt), -2);
}

TEST(PoolTest, AllocFreeMultipleTypes)
{
	using namespace MemoryInternal;

	PageRegistry<int>::DBG_Reset();
	PageRegistry<double>::DBG_Reset();
	PageRegistry<char>::DBG_Reset();

	int *allocInt = Alloc<int>(10);
	double *allocDouble = Alloc<double>(5);
	char *allocChar = Alloc<char>(20);

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
