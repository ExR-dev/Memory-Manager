#include "TracyWrapper.hpp"

#include "MemPerfTests.hpp"
#include "PoolAllocator.hpp"

#include <chrono>
#include <iostream>
#include <format>


struct TestStructSmall
{
	int a[2];
	double b[2];
	size_t c[2];
};

struct TestStructMed
{
	int a[16];
	double b[4];
	size_t c[16];
};

struct TestStructLarge
{
	int a[64];
	double b[80];
	size_t c[128];
};


struct TestResult
{
	float allocAvgTimeMs;
	float allocMinTimeMs;
	float allocMaxTimeMs;

	float newAvgTimeMs;
	float newMinTimeMs;
	float newMaxTimeMs;
};

struct TestPoolParams
{
	std::string typeName;
	int iterations;
	int allocCount;
	int maxConcurrent;
	int maxSize;
};

template<typename T>
static float StressTestPoolAlloc(int allocCount, int maxConcurrentAllocs, int maxAllocSize)
{
	ZoneScopedC(tracy::Color::Yellow3);

	using namespace MemoryInternal;

	std::vector<T *> allocs;
	std::vector<int> currAllocs;

	allocs.resize(allocCount, nullptr);
	currAllocs.reserve(maxConcurrentAllocs);

	std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < allocCount; )
	{
		ZoneNamedNC(allocIterZone, "Loop", tracy::Color::Gray72, true);

		if (currAllocs.size() > 0)
		{
			// Free a random number of current allocations
			int freeCount = rand() % (currAllocs.size() / 5 + 1);

			for (int j = 0; j < freeCount; ++j)
			{
				ZoneNamedNC(freeZone, "Free", tracy::Color::Green, true);

				if (currAllocs.size() <= 0)
					break;

				int currAllocIndex = rand() % currAllocs.size();
				int freeIdx = currAllocs[currAllocIndex];

				Free<T>(allocs[freeIdx]);

				allocs[freeIdx] = nullptr;
				currAllocs.erase(currAllocs.begin() + currAllocIndex);
			}
		}

		// Allocate a random number of floats
		int newAllocs = rand() % ((maxConcurrentAllocs - currAllocs.size()) / 4 + 1);
		for (int j = 0; j < newAllocs; ++j)
		{
			ZoneNamedNC(allocZone, "Allocate", tracy::Color::Red, true);

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

			T *newAlloc = Alloc<T>(allocSize);

			allocs[allocIdx] = newAlloc;
			currAllocs.push_back(allocIdx);

			++i;
		}
	}

	// Free remaining allocations
	for (int i = 0; i < currAllocs.size(); ++i)
	{
		int allocIdx = currAllocs[i];
		Free<T>(allocs[allocIdx]);
	}

	std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();

	return std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

template<typename T>
static float StressTestPoolNew(int allocCount, int maxConcurrentAllocs, int maxAllocSize)
{
	ZoneScopedC(tracy::Color::Blue3);

	std::vector<T *> allocs;
	std::vector<int> currAllocs;

	allocs.resize(allocCount, nullptr);
	currAllocs.reserve(maxConcurrentAllocs);

	std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < allocCount; )
	{
		ZoneNamedNC(allocIterZone, "Loop", tracy::Color::Gray16, true);

		if (currAllocs.size() > 0)
		{
			// Free a random number of current allocations
			int freeCount = rand() % (currAllocs.size() / 5 + 1);

			for (int j = 0; j < freeCount; ++j)
			{
				ZoneNamedNC(freeZone, "Free", tracy::Color::Green, true);

				if (currAllocs.size() <= 0)
					break;

				int currAllocIndex = rand() % currAllocs.size();
				int freeIdx = currAllocs[currAllocIndex];

				TracyFreeN(allocs[freeIdx], "New");
				delete[] allocs[freeIdx];

				allocs[freeIdx] = nullptr;
				currAllocs.erase(currAllocs.begin() + currAllocIndex);
			}
		}

		// Allocate a random number of floats
		int newAllocs = rand() % ((maxConcurrentAllocs - currAllocs.size()) / 4 + 1);
		for (int j = 0; j < newAllocs; ++j)
		{
			ZoneNamedNC(allocZone, "Allocate", tracy::Color::Red, true);

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

			T *newAlloc = new T[allocSize];
			TracyAllocN(newAlloc, allocSize * sizeof(T), "New");

			allocs[allocIdx] = newAlloc;
			currAllocs.push_back(allocIdx);

			++i;
		}
	}

	// Free remaining allocations
	for (int i = 0; i < currAllocs.size(); ++i)
	{
		int allocIdx = currAllocs[i];

		TracyFreeN(allocs[allocIdx], "New");
		delete[] allocs[allocIdx];
	}

	std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();

	return std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

template<typename T>
static TestResult StressTestPool(TestPoolParams &params)
{
	std::vector<float> allocTimes;
	std::vector<float> newTimes;

	allocTimes.reserve(params.iterations);
	newTimes.reserve(params.iterations);

	MemoryInternal::PoolAllocator<T>::Reset();
	MemoryInternal::PoolAllocator<T>::Initialize(static_cast<size_t>(params.maxConcurrent) * params.maxSize);

	int seed = static_cast<int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());

	for (int i = 0; i < params.iterations; ++i)
	{
		ZoneNamedNC(perfTestIterLoopZone, "Iteration Loop", tracy::Color::Aquamarine3, true);
		
		srand(seed + i);
		allocTimes.push_back(StressTestPoolAlloc<T>(params.allocCount, params.maxConcurrent, params.maxSize));

		srand(seed + i);
		newTimes.push_back(StressTestPoolNew<T>(params.allocCount, params.maxConcurrent, params.maxSize));
	}

	TestResult result{};

	for (int i = 0; i < params.iterations; ++i)
	{
		result.allocAvgTimeMs += allocTimes[i];
		result.newAvgTimeMs += newTimes[i];

		if (i == 0 || allocTimes[i] < result.allocMinTimeMs)
			result.allocMinTimeMs = allocTimes[i];
		if (i == 0 || newTimes[i] < result.newMinTimeMs)
			result.newMinTimeMs = newTimes[i];

		if (i == 0 || allocTimes[i] > result.allocMaxTimeMs)
			result.allocMaxTimeMs = allocTimes[i];
		if (i == 0 || newTimes[i] > result.newMaxTimeMs)
			result.newMaxTimeMs = newTimes[i];
	}

	result.allocAvgTimeMs /= static_cast<float>(params.iterations);
	result.newAvgTimeMs /= static_cast<float>(params.iterations);

	MemoryInternal::PoolAllocator<T>::Reset();

	return result;
}


void PerfTests::RunPoolPerfTests()
{
	ZoneScopedC(tracy::Color::Purple2);

	std::vector<TestPoolParams> tests = {
		// TypeName,			Iterations, AllocCount, MaxConcurrent,	MaxAllocSize
		{ "TestStructSmall",	16,			5000,		1 << 3,			1 << 3	},
		{ "TestStructMed",		16,			5000,		1 << 3,			1 << 3	},
		{ "TestStructLarge",	16,			5000,		1 << 3,			1 << 3	},

		{ "TestStructSmall",	16,			2000,		1 << 5,			1 << 5	},
		{ "TestStructMed",		16,			2000,		1 << 5,			1 << 5	},
		{ "TestStructLarge",	16,			2000,		1 << 5,			1 << 5	},

		{ "TestStructSmall",	16,			2000,		1 << 5,			1 << 9	},
		{ "TestStructMed",		16,			2000,		1 << 5,			1 << 9	},
		{ "TestStructLarge",	16,			2000,		1 << 5,			1 << 9	},

		{ "TestStructSmall",	16,			2000,		1 << 4,			1 << 10 },
		{ "TestStructMed",		16,			2000,		1 << 4,			1 << 10 },
		{ "TestStructLarge",	16,			2000,		1 << 4,			1 << 10 },

		{ "TestStructSmall",	16,			2000,		1 << 3,			1 << 11	},
		{ "TestStructMed",		16,			2000,		1 << 3,			1 << 11	},
		{ "TestStructLarge",	16,			2000,		1 << 3,			1 << 11	},
																			   
		{ "TestStructSmall",	16,			2000,		1 << 8,			1 << 4	},
		{ "TestStructMed",		16,			2000,		1 << 8,			1 << 4	},
		{ "TestStructLarge",	16,			2000,		1 << 8,			1 << 4	},

		{ "TestStructSmall",	16,			2000,		1 << 11,		1 << 3  },
		{ "TestStructMed",		16,			2000,		1 << 11,		1 << 3  },
		{ "TestStructLarge",	16,			2000,		1 << 11,		1 << 3  },
																			   
		{ "char",				16,			1000,		1 << 5,			1 << 11	},
		{ "int",				16,			1000,		1 << 5,			1 << 11	},
		{ "size_t",				16,			1000,		1 << 5,			1 << 11	},
																			   
		{ "char",				16,			3000,		1 << 9,			1 << 6	},
		{ "int",				16,			3000,		1 << 9,			1 << 6	},
		{ "size_t",				16,			3000,		1 << 9,			1 << 6	},
	};

	std::vector<TestResult> results;
	results.reserve(tests.size());
	
	for (int i = 0; i < tests.size(); ++i)
	{
		ZoneNamedNC(allocTestsZone, "Test", tracy::Color::Burlywood2, true);
		ZoneValue(i);

		TestPoolParams &test = tests[i];

		if (test.typeName == "TestStructSmall")
			results.push_back(StressTestPool<TestStructSmall>(test));
		else if (test.typeName == "TestStructMed")
			results.push_back(StressTestPool<TestStructMed>(test));
		else if (test.typeName == "TestStructLarge")
			results.push_back(StressTestPool<TestStructLarge>(test));
		else if (test.typeName == "char")
			results.push_back(StressTestPool<char>(test));
		else if (test.typeName == "int")
			results.push_back(StressTestPool<int>(test));
		else if (test.typeName == "size_t")
			results.push_back(StressTestPool<size_t>(test));
	}

	std::cout << "\nPool Allocator Performance Tests\n";
	std::cout << "Time measurements (ms)\n";

	for (int j = 0; j < results.size(); ++j)
	{
		TestPoolParams &test = tests[j];
		TestResult &result = results[j];

		std::cout << std::format("Params: Type={}, Iterations={}, AllocCount={}, MaxConcurrent={}, MaxAllocSize={}, PageSize={}\n",
			test.typeName, test.iterations, test.allocCount, test.maxConcurrent, test.maxSize, static_cast<size_t>(test.maxConcurrent) * test.maxSize);
		std::cout << "\t            \tAvg \tMin \tMax\n";
		std::cout << std::format("\tPool Alloc: \t{:2.3f} \t{:2.3f} \t{:2.3f}\n", result.allocAvgTimeMs, result.allocMinTimeMs, result.allocMaxTimeMs);
		std::cout << std::format("\tNew/Delete: \t{:2.3f} \t{:2.3f} \t{:2.3f}\n", result.newAvgTimeMs, result.newMinTimeMs, result.newMaxTimeMs);
		std::cout << std::format("\tDifference: \t{:2.3f} \t{:2.3f} \t{:2.3f}\n", 			
			result.newAvgTimeMs - result.allocAvgTimeMs,
			result.newMinTimeMs - result.allocMinTimeMs,
			result.newMaxTimeMs - result.allocMaxTimeMs);
		std::cout << std::format("\tSpeedup:    \t{:2.3f} \t{:2.3f} \t{:2.3f}\n", 			
			result.newAvgTimeMs / result.allocAvgTimeMs,
			result.newMinTimeMs / result.allocMinTimeMs,
			result.newMaxTimeMs / result.allocMaxTimeMs);
		std::cout << "\n";
	}
}
