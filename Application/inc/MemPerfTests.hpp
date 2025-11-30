#pragma once

namespace PerfTests
{
	void RunPoolPerfTests();
	void RunStackPerfTests1();

	void StressTestStackAlloc();
	void StressTestBuddyAlloc();
	void StressTestNew();
}