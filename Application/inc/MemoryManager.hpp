#pragma once

namespace Memory
{
	class MemoryManager
	{
	public:
		~MemoryManager();

		MemoryManager &Get()
		{
			static MemoryManager instance;
			return instance;
		}

	private:
		MemoryManager();
	};
}
