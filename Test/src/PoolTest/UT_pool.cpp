#include "../../../Application/inc/PageRegistry.hpp"
#include <gtest/gtest.h>

TEST(PoolTest, AllocFree)
{
	using namespace MemoryInternal;

	{
		int *allocedInts = Alloc<int>(10);

		for (int i = 0; i < 10; ++i)
			allocedInts[i] = i * 10;

		Free<int>(allocedInts);
	}

	{
		int *allocArray[6]{};
		int allocSizes[6] = { 5, 10, 18, 2, 1, 32 };
		
		for (int i = 0; i < 6; ++i)
		{
			allocArray[i] = Alloc<int>(allocSizes[i]);

			for (int j = 0; j < allocSizes[i]; ++j)
				allocArray[i][j] = i * 100 + j;
		}

		for (int i = 0; i < 6; ++i)
		{
			Free<int>(allocArray[i]);
		}
	}

    ASSERT_EQ(true, true);
}