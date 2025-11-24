#include <iostream>
#include "../inc/PageRegistry.hpp"

int main()
{
	using namespace MemoryInternal;

	std::cout << "Test 1: Allocate and free integers" << std::endl;
	{
		int *allocedInts = Alloc<int>(10);

		std::cout << "Allocated: (" << allocedInts << ", " << 10 << ")" << std::endl;

		for (int i = 0; i < 10; ++i)
			allocedInts[i] = i * 10;

		std::cout << "Contents: ";
		for (int i = 0; i < 10; ++i)
			std::cout << allocedInts[i] << " ";
		std::cout << std::endl;

		Free<int>(allocedInts);
	}
	std::cout << std::endl;

	std::cout << "Test 2: Re-allocate same space" << std::endl;
	{
		int *allocedInts = Alloc<int>(10);

		std::cout << "Allocated: (" << allocedInts << ", " << 10 << ")" << std::endl;

		for (int i = 0; i < 10; ++i)
			allocedInts[i] = i * 10;

		std::cout << "Contents: ";
		for (int i = 0; i < 10; ++i)
			std::cout << allocedInts[i] << " ";
		std::cout << std::endl;

		Free<int>(allocedInts);
	}
	std::cout << std::endl;

	std::cout << "Test 3: Multiple allocations" << std::endl;
	{
		int *allocArray[6]{};
		int allocSizes[6] = { 5, 10, 18, 2, 1, 32 };
		
		for (int i = 0; i < 6; ++i)
		{
			allocArray[i] = Alloc<int>(allocSizes[i]);

			std::cout << "Allocated: (" << allocArray[i] << ", " << allocSizes[i] << ")" << std::endl;

			for (int j = 0; j < allocSizes[i]; ++j)
				allocArray[i][j] = i * 100 + j;
		}

		for (int i = 0; i < 6; ++i)
		{
			std::cout << "Contents[" << i << "]: ";
			for (int j = 0; j < allocSizes[i]; ++j)
				std::cout << allocArray[i][j] << " ";
			std::cout << std::endl;
		}

		for (int i = 0; i < 6; ++i)
		{
			Free<int>(allocArray[i]);
		}
	}
	std::cout << std::endl;

	std::cout << "Test 4: Unordered free" << std::endl;
	{
		int *allocArray[6]{};
		int allocSizes[6] = { 5, 10, 18, 2, 1, 32 };

		for (int i = 0; i < 6; ++i)
		{
			allocArray[i] = Alloc<int>(allocSizes[i]);

			std::cout << "Allocated: (" << allocArray[i] << ", " << allocSizes[i] << ")" << std::endl;

			for (int j = 0; j < allocSizes[i]; ++j)
				allocArray[i][j] = i * 100 + j;
		}

		for (int i = 0; i < 6; ++i)
		{
			std::cout << "Contents[" << i << "]: ";
			for (int j = 0; j < allocSizes[i]; ++j)
				std::cout << allocArray[i][j] << " ";
			std::cout << std::endl;
		}


		Free<int>(allocArray[0]);
		Free<int>(allocArray[2]);
		allocArray[2] = Alloc<int>(7);

		PageRegistry<int>::DBG_PrintPage(64);

		Free<int>(allocArray[1]);
		Free<int>(allocArray[3]);
		Free<int>(allocArray[4]);
		Free<int>(allocArray[5]);
	}
	std::cout << std::endl;

    return 0;
}
