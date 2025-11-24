#include <iostream>
#include "../inc/PageRegistry.hpp"

int main()
{
	using namespace MemoryInternal;

	std::cout << "Test 1: Allocate and free integers" << std::endl;
	{
		int *allocedInts = PageRegistry<int>::Alloc(10);

		for (int i = 0; i < 10; ++i)
			allocedInts[i] = i * 10;

		std::cout << "Allocated integers: ";
		for (int i = 0; i < 10; ++i)
			std::cout << allocedInts[i] << " ";
		std::cout << std::endl;

		PageRegistry<int>::Free(allocedInts, 10);
	}

	std::cout << "Test 2: Re-allocate same space" << std::endl;
	{
		int *allocedInts = PageRegistry<int>::Alloc(10);

		for (int i = 0; i < 10; ++i)
			allocedInts[i] = i * 10;

		std::cout << "Allocated integers: ";
		for (int i = 0; i < 10; ++i)
			std::cout << allocedInts[i] << " ";
		std::cout << std::endl;

		PageRegistry<int>::Free(allocedInts, 10);
	}

	std::cout << "Test 3: Multiple allocations" << std::endl;
	{
		int *allocArray[6]{};
		int allocSizes[6] = { 5, 10, 18, 2, 1, 64 };
		
		for (int i = 0; i < 6; ++i)
		{
			allocArray[i] = PageRegistry<int>::Alloc(allocSizes[i]);

			for (int j = 0; j < allocSizes[i]; ++j)
				allocArray[i][j] = i * 100 + j;
		}

		for (int i = 0; i < 6; ++i)
		{
			std::cout << "Allocated block " << i << ": ";
			for (int j = 0; j < allocSizes[i]; ++j)
				std::cout << allocArray[i][j] << " ";
			std::cout << std::endl;
		}

		for (int i = 0; i < 6; ++i)
		{
			PageRegistry<int>::Free(allocArray[i], allocSizes[i]);
		}
	}

    return 0;
}
