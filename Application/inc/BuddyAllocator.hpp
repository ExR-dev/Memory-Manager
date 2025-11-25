#pragma once
#include <memory>
#include <vector>
#include <array>

class BuddyAllocator
{
private:
	struct Block
	{
		Block* buddy = nullptr;
		bool isFree = true;
		size_t size = 0;
		size_t offset = 0;
	};

	std::unique_ptr<std::array<char, 1024 * 1000>> m_memory;
	std::vector<Block> m_blocks;
	size_t m_minimumSize = 64 * 1000;

public:
	BuddyAllocator()
	{
		m_memory = std::make_unique<std::array<char, 1024 * 1000>>();
		//Do we need this resize? We need to keep the blocks at constant memory places, so yes?
		//m_blocks.resize((1024 * 1000) / m_minimumSize);
		m_blocks[0].size = 1024 * 1000;
	}

	void* Alloc(size_t size)
	{
		// Find a free block
		for (size_t i = 0; i < m_blocks.size(); i++)
		{
			if (m_blocks[i].isFree && m_blocks[i].size >= size)
			{
				// Attempt to split the block
				while (m_blocks[i].size / 2 >= size && m_blocks[i].size / 2 >= m_minimumSize)
				{
					size_t halfSize = m_blocks[i].size / 2;

					Block buddyBlock;
					buddyBlock.size = halfSize;
					buddyBlock.offset = m_blocks[i].offset + halfSize;

					m_blocks.push_back(buddyBlock);
					m_blocks[i].size = halfSize;
					m_blocks[i].buddy = &m_blocks.back();
					m_blocks.back().buddy = &m_blocks[i];
				}

				m_blocks[i].isFree = false;
				return m_memory.get()->data() + m_blocks[i].offset;
			}
		}

		// No suitable block was found
		return nullptr;
	}

	void Free(void* mem)
	{
		// Find the memory block to be freed
		Block block;
		for (size_t i = 0; i < m_blocks.size(); i++)
		{
			if (m_memory.get()->data() + m_blocks[i].offset == mem)
			{
				block = m_blocks[i];
				break;
			}
		}

		// Check if the buddy is free, and combine them if that is the case
	}
};