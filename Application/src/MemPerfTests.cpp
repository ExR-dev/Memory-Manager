#include "TracyWrapper.hpp"

#include "MemPerfTests.hpp"
#include "PoolAllocator.hpp"
#include "NonArrayPoolAllocator.hpp"

#include <chrono>
#include <iostream>
#include <fstream>
#include <format>
#include <algorithm>

#include "BuddyAllocator.hpp"
#include "StackAllocator.hpp"

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
			int freeCount = rand() % std::max(static_cast<unsigned long long>(currAllocs.size() / 5 + 1), 2ull);

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
		int newAllocs = rand() % std::max(static_cast<unsigned long long>(maxConcurrentAllocs - currAllocs.size()) / 4 + 1, 2ull);
		for (int j = 0; j < newAllocs; ++j)
		{
			ZoneNamedNC(allocZone, "Allocate", tracy::Color::Red, true);

			if (currAllocs.size() >= static_cast<std::size_t>(maxConcurrentAllocs))
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
	for (std::size_t i = 0; i < currAllocs.size(); ++i)
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
			int freeCount = rand() % std::max(static_cast<unsigned long long>(currAllocs.size() / 5 + 1), 2ull);

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
		int newAllocs = rand() % std::max(static_cast<unsigned long long>(maxConcurrentAllocs - currAllocs.size()) / 4 + 1, 2ull);
		for (int j = 0; j < newAllocs; ++j)
		{
			ZoneNamedNC(allocZone, "Allocate", tracy::Color::Red, true);

			if (currAllocs.size() >= static_cast<std::size_t>(maxConcurrentAllocs))
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
	for (std::size_t i = 0; i < currAllocs.size(); ++i)
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

	int seed = rand();

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



template<typename T>
static float StressTestPoolNonArrayAlloc(int allocCount, int maxConcurrentAllocs)
{
	ZoneScopedC(tracy::Color::Yellow3);

	using namespace MemoryInternalNonArray;

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
			int freeCount = rand() % std::max(static_cast<unsigned long long>(currAllocs.size() / 5 + 1), 2ull);

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
		int newAllocs = rand() % std::max(static_cast<unsigned long long>(maxConcurrentAllocs - currAllocs.size()) / 4 + 1, 2ull);
		for (int j = 0; j < newAllocs; ++j)
		{
			ZoneNamedNC(allocZone, "Allocate", tracy::Color::Red, true);

			if (currAllocs.size() >= static_cast<std::size_t>(maxConcurrentAllocs))
				break;

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

			T *newAlloc = Alloc<T>();

			allocs[allocIdx] = newAlloc;
			currAllocs.push_back(allocIdx);

			++i;
		}
	}

	// Free remaining allocations
	for (std::size_t i = 0; i < currAllocs.size(); ++i)
	{
		int allocIdx = currAllocs[i];
		Free<T>(allocs[allocIdx]);
	}

	std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();

	return std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

template<typename T>
static float StressTestPoolNonArrayNew(int allocCount, int maxConcurrentAllocs)
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
			int freeCount = rand() % std::max(static_cast<unsigned long long>(currAllocs.size()) / 5 + 1, 2ull);

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
		int newAllocs = rand() % std::max(static_cast<unsigned long long>(maxConcurrentAllocs - currAllocs.size()) / 4 + 1, 2ull);
		for (int j = 0; j < newAllocs; ++j)
		{
			ZoneNamedNC(allocZone, "Allocate", tracy::Color::Red, true);

			if (currAllocs.size() >= static_cast<std::size_t>(maxConcurrentAllocs))
				break;

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

			T *newAlloc = new T();
			TracyAllocN(newAlloc, sizeof(T), "New");

			allocs[allocIdx] = newAlloc;
			currAllocs.push_back(allocIdx);

			++i;
		}
	}

	// Free remaining allocations
	for (std::size_t i = 0; i < currAllocs.size(); ++i)
	{
		int allocIdx = currAllocs[i];

		TracyFreeN(allocs[allocIdx], "New");
		delete[] allocs[allocIdx];
	}

	std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();

	return std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

template<typename T>
static TestResult StressTestPoolNonArray(TestPoolParams &params)
{
	std::vector<float> allocTimes;
	std::vector<float> newTimes;

	allocTimes.reserve(params.iterations);
	newTimes.reserve(params.iterations);

	MemoryInternalNonArray::NonArrayPoolAllocator<T>::Reset();
	MemoryInternalNonArray::NonArrayPoolAllocator<T>::Initialize(static_cast<size_t>(params.maxConcurrent));

	int seed = rand();

	for (int i = 0; i < params.iterations; ++i)
	{
		ZoneNamedNC(perfTestIterLoopZone, "Iteration Loop", tracy::Color::Aquamarine3, true);
		
		srand(seed + i);
		allocTimes.push_back(StressTestPoolNonArrayAlloc<T>(params.allocCount, params.maxConcurrent));

		srand(seed + i);
		newTimes.push_back(StressTestPoolNonArrayNew<T>(params.allocCount, params.maxConcurrent));
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

	MemoryInternalNonArray::NonArrayPoolAllocator<T>::Reset();

	return result;
}




static size_t GetTypeSizeByName(const std::string &typeName)
{
	if (typeName == "TestStructSmall")
		return sizeof(TestStructSmall);
	else if (typeName == "TestStructMed")
		return sizeof(TestStructMed);
	else if (typeName == "TestStructLarge")
		return sizeof(TestStructLarge);
	else if (typeName == "char")
		return sizeof(char);
	else if (typeName == "int")
		return sizeof(int);
	else if (typeName == "size_t")
		return sizeof(size_t);
	return 0;
}

void PerfTests::RunPoolPerfTests()
{
	ZoneScopedC(tracy::Color::Purple2);


	
	std::vector<std::string> typeNames = {
		//"TestStructSmall"
		//"TestStructMed"
		
		"char",
		"int",
		"size_t",
		"TestStructSmall",
		"TestStructMed",
		"TestStructLarge",
	};

	std::vector<int> maxConcurrent = {
		1 << 6

		//1 << 0,
		//1 << 1,
		//1 << 2,
		//1 << 3,
		//1 << 4,
		//1 << 5,
		//1 << 6,
		//1 << 7,
		//1 << 8,
		//1 << 9,
		//1 << 10,
		//1 << 11,
		//1 << 12,
		//1 << 13,
		//1 << 14,
		//1 << 15,
		//1 << 16
	};

	std::vector<int> maxAllocSizes = {
		//1 << 6

		1 << 0,
		1 << 1,
		1 << 2,
		1 << 3,
		1 << 4,
		1 << 5,
		1 << 6,
		1 << 7,
		1 << 8,
		1 << 9,
		1 << 10,
		1 << 11,
		1 << 12,
		1 << 13,
		1 << 14,
		1 << 15,
		1 << 16
	};

	size_t maxMemUsage = 1ull << 28; 
	size_t maxItemCount = 1ull << 18;
	
	
	std::vector<TestPoolParams> tests = {
		// TypeName,			Iterations, AllocCount, MaxConcurrent,	MaxAllocSize
		/*{ "TestStructSmall",	16,			5000,		1 << 3,			1 << 3	},
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
		{ "size_t",				16,			3000,		1 << 9,			1 << 6	},*/
	};

	
	for (size_t i = 0; i < typeNames.size(); i++)
	{
		std::string &typeName = typeNames[i];

		for (size_t j = 0; j < maxConcurrent.size(); j++)
		{
			int concurrent = maxConcurrent[j];

			for (size_t k = 0; k < maxAllocSizes.size(); k++)
			{
				int allocSize = maxAllocSizes[k];

				size_t totalItemCount = static_cast<size_t>(allocSize) * concurrent;

				if (totalItemCount > maxItemCount)
					continue; // Skip tests that exceed max item count

				size_t totalMemUsage = totalItemCount * GetTypeSizeByName(typeName);

				if (totalMemUsage > maxMemUsage)
					continue; // Skip tests that exceed max memory usage

				tests.push_back({ 
					typeName, 
					16, 
					1000, 
					concurrent, 
					allocSize
				});
			}
		}
	}
	

	srand(static_cast<int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
	//srand(1);

	std::vector<TestResult> results;
	results.reserve(tests.size());
	
	for (std::size_t i = 0; i < tests.size(); ++i)
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

		/*if (test.typeName == "TestStructSmall")
			results.push_back(StressTestPoolNonArray<TestStructSmall>(test));
		else if (test.typeName == "TestStructMed")
			results.push_back(StressTestPoolNonArray<TestStructMed>(test));
		else if (test.typeName == "TestStructLarge")
			results.push_back(StressTestPoolNonArray<TestStructLarge>(test));
		else if (test.typeName == "char")
			results.push_back(StressTestPoolNonArray<char>(test));
		else if (test.typeName == "int")
			results.push_back(StressTestPoolNonArray<int>(test));
		else if (test.typeName == "size_t")
			results.push_back(StressTestPoolNonArray<size_t>(test));*/
	}

	std::cout << "\nPool Allocator Performance Tests\n";
	std::cout << "Time measurements (ms)\n";

	for (std::size_t j = 0; j < results.size(); ++j)
	{
		TestPoolParams &test = tests[j];
		TestResult &result = results[j];

		std::cout << std::format("Params: Type={}, Iterations={}, AllocCount={}, MaxConcurrent={}, MaxAllocSize={}, PageSize={}\n",
			test.typeName, test.iterations, test.allocCount, test.maxConcurrent, test.maxSize, static_cast<size_t>(test.maxConcurrent) * test.maxSize);
		std::cout << "\t            \tAvg\n";

		std::cout << std::format("\tPool Alloc: \t{:2.3f}\n", result.allocAvgTimeMs);
		std::cout << std::format("\tNew/Delete: \t{:2.3f}\n", result.newAvgTimeMs);

		std::cout << std::format("\tDifference: \t{:2.3f}\n", 			
			result.newAvgTimeMs - result.allocAvgTimeMs);
		std::cout << std::format("\tSpeedup:    \t{:2.3f}\n", 			
			result.newAvgTimeMs / result.allocAvgTimeMs);

		std::cout << "\n";
	}

	// Write results to file
	{
		std::ofstream resultFile("PoolAllocPerfResults.txt");

		resultFile << 
			"Type Size\tMax Concurrent Allocs\tMax Alloc Size" 
			"\tPool" 
			"\tNew" 
			"\n";

		for (std::size_t j = 0; j < results.size(); ++j)
		{
			TestPoolParams &test = tests[j];
			TestResult &result = results[j];

			resultFile << std::format(
				"{}\t{}\t{}" 
				"\t{}" 
				"\t{}" 
				"\n",
				GetTypeSizeByName(test.typeName), test.maxConcurrent, test.maxSize,
				result.allocAvgTimeMs,
				result.newAvgTimeMs
			);

		}

		resultFile << "\n";

		resultFile.close();
	}
}

static void StressTestStackAlloc()
{

	{ // Testing 16 allocations
		StackAllocator stackAllocator;

		std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 16; ++i)
		{
			stackAllocator.Alloc(sizeof(int));
			stackAllocator.Reset();
		}
		std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
		std::cout << "Stack 16: " << std::chrono::duration<float, std::milli>(endTime - startTime).count();
	}

	{ // Testing 64 allocations
		StackAllocator stackAllocator;

		std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 64; ++i)
		{
			stackAllocator.Alloc(sizeof(int));
			stackAllocator.Reset();
		}
		std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
		std::cout << "Stack 64: " << std::chrono::duration<float, std::milli>(endTime - startTime).count();
	}

	{ // Testing 256 allocations
		StackAllocator stackAllocator;
		std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 256; ++i)
		{
			stackAllocator.Alloc(sizeof(int));
			stackAllocator.Reset();
		}
		std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
		std::cout << "Stack 256: " << std::chrono::duration<float, std::milli>(endTime - startTime).count();
	}

	{ // Testing 512 allocations
		StackAllocator stackAllocator;
		std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 512; ++i)
		{
			stackAllocator.Alloc(sizeof(int));
			stackAllocator.Reset();
		}
		std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
		std::cout << "Stack 512: " << std::chrono::duration<float, std::milli>(endTime - startTime).count();
	}

	{ // Testing 1024 allocations
		StackAllocator stackAllocator;
		std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 1024; ++i)
		{
			stackAllocator.Alloc(sizeof(int));
			stackAllocator.Reset();
		}
		std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
		std::cout << "Stack 1024: " << std::chrono::duration<float, std::milli>(endTime - startTime).count();
	}
}

static void StressTestBuddyAlloc()
{

	{ // Testing 16 allocations
		BuddyAllocator buddyAllocator;

		std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 16; ++i)
		{
			int* test = static_cast<int*>(buddyAllocator.Alloc(sizeof(int)));
			buddyAllocator.Free(test);
		}

		std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
		std::cout << "Buddy 16: " << std::chrono::duration<float, std::milli>(endTime - startTime).count();
	}

	{ // Testing 64 allocations
		BuddyAllocator buddyAllocator;

		std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 64; ++i)
		{
			int* test = static_cast<int*>(buddyAllocator.Alloc(sizeof(int)));
			buddyAllocator.Free(test);
		}

		std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
		std::cout << "Buddy 64: " << std::chrono::duration<float, std::milli>(endTime - startTime).count();
	}

	{ // Testing 256 allocations
		BuddyAllocator buddyAllocator;

		std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 256; ++i)
		{
			int* test = static_cast<int*>(buddyAllocator.Alloc(sizeof(int)));
			buddyAllocator.Free(test);
		}

		std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
		std::cout << "Buddy 256: " << std::chrono::duration<float, std::milli>(endTime - startTime).count();
	}

	{ // Testing 512 allocations
		BuddyAllocator buddyAllocator;

		std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 512; ++i)
		{
			int* test = static_cast<int*>(buddyAllocator.Alloc(sizeof(int)));
			buddyAllocator.Free(test);
		}

		std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
		std::cout << "Buddy 512: " << std::chrono::duration<float, std::milli>(endTime - startTime).count();
	}

	{
		// Testing 1024 allocations
		BuddyAllocator buddyAllocator;

		std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 1024; ++i)
		{
			int* test = static_cast<int*>(buddyAllocator.Alloc(sizeof(int)));
			buddyAllocator.Free(test);
		}

		std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
		std::cout << "Buddy 1024: " << std::chrono::duration<float, std::milli>(endTime - startTime).count();
	}
}
