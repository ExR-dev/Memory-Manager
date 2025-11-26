#include "MemPerfTests.hpp"
#include "PageRegistry.hpp"

#include <chrono>
#include <iostream>


constexpr int allocCount = 10000;
constexpr int maxConcurrentAllocs = 64;
constexpr int maxAllocSize = 1 << 10;
constexpr size_t pageSize = 1ull << 16;


static float StressTestAlloc()
{
	using namespace MemoryInternal;

	PageRegistry<float>::Reset();
	PageRegistry<float>::Initialize(pageSize);

	std::vector<float *> allocs;
	std::vector<int> currAllocs;

	allocs.resize(allocCount, nullptr);
	currAllocs.reserve(maxConcurrentAllocs);

	std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

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

				Free<float>(allocs[freeIdx]);

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

			float *newAlloc = Alloc<float>(allocSize);

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
		Free<float>(allocs[allocIdx]);
	}

	std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();

	return std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

static float StressTestNew()
{
	std::vector<float *> allocs;
	std::vector<int> currAllocs;

	allocs.resize(allocCount, nullptr);
	currAllocs.reserve(maxConcurrentAllocs);

	std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

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

			float *newAlloc = new float[allocSize];

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

	std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();

	return std::chrono::duration<float, std::milli>(endTime - startTime).count();
}


void PerfTests::RunPoolPerfTests()
{
	int iterations = 32;

	std::vector<float> allocTimes;
	std::vector<float> newTimes;

	allocTimes.reserve(iterations);
	newTimes.reserve(iterations);

	for (int i = 0; i < iterations; ++i)
	{
		allocTimes.push_back(StressTestAlloc());
		newTimes.push_back(StressTestNew());
	}

	float avgAllocTime = 0.0f;
	float avgNewTime = 0.0f;

	for (int i = 0; i < iterations; ++i)
	{
		avgAllocTime += allocTimes[i];
		avgNewTime += newTimes[i];
	}

	avgAllocTime /= static_cast<float>(iterations);
	avgNewTime /= static_cast<float>(iterations);

	std::cout << "Pool Alloc Average Time: " << avgAllocTime << " ms\n";
	std::cout << "New/Delete Average Time: " << avgNewTime << " ms\n";
}
