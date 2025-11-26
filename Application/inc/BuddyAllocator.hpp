#pragma once
#include <memory>
#include <vector>
#include <array>
#include <bitset>
#include <math.h>

class BuddyAllocator
{
private:
	struct Block
	{
		bool isFree = true;
		size_t size = 0;
		size_t offset = 0;

		Block* left = nullptr;
		Block* right = nullptr;
		Block* parent = nullptr;
	};

	std::unique_ptr<std::array<char, 4096 * 1024>> m_memory;
	size_t m_minimumSize = 32 * 1024;

	size_t m_numRows = (size_t)(log2(4096 * 1024) - log2(m_minimumSize));
	size_t m_numBlocks = (size_t)pow(2, m_numRows + 1) - 1;

	std::unique_ptr<std::vector<Block>> m_blocks;

	Block* FindBlock(Block* block, size_t size, int parentIndex)
	{
		// If the block we are checking is free, and if the data could fit in the block
		if (block->isFree && block->size >= size)
		{
			// There are no child blocks
			if (block->left == nullptr && block->right == nullptr)
			{
				// If the half size of the block is larger than the minimum,
				// and the data can fit in the half size
				if (block->size / 2 >= m_minimumSize && block->size / 2 >= size)
				{
					block->left = &m_blocks.get()->at((2 * parentIndex) + 1);
					block->right = &m_blocks.get()->at(2 * (parentIndex + 1));

					block->left->size = block->size / 2;
					block->right->size = block->size / 2;

					block->left->offset = block->offset;
					block->right->offset = block->offset + block->left->size;

					block->left->parent = block->right->parent = block;
				}
				// Either the half block would hit the minimum, or the data would not fit so we stop
				else
				{
					block->isFree = false;
					return block;
				}

			}
			Block* left = FindBlock(block->left, size, (2 * parentIndex) + 1);
			if (left != nullptr)
			{
				return left;
			}
			return FindBlock(block->right, size, 2 * (parentIndex + 1));
		}
		else
		{
			return nullptr;
		}
	}

	Block* FindBlockByOffset(Block* block, size_t offset)
	{
		if (block->left == nullptr || block->right == nullptr)
		{
			if (block->offset == offset)
				return block;

			return nullptr;
		}

		if (block->right->offset > offset)
			return FindBlockByOffset(block->left, offset);

		return FindBlockByOffset(block->right, offset);
	}

public:
	BuddyAllocator()
	{
		m_memory = std::make_unique<std::array<char, 4096 * 1024>>();
		m_blocks = std::make_unique<std::vector<Block>>();
		m_blocks.get()->resize(m_numBlocks);
		//Do we need this resize? We need to keep the blocks at constant memory places, so yes?
		// Change this to reserve.
		//m_blocks.reserve((1024 * 1000) / m_minimumSize);
		//m_blocks[0].size = 1024 * 1000;

		m_blocks.get()->at(0).size = 4096 * 1024;
		//m_blocks.push_back(baseBlock);
	}

	
	void* Alloc(size_t size)
	{
		Block* allocated = FindBlock(&m_blocks.get()->at(0), size, 0);
		if (allocated == nullptr)
		{
			return nullptr;
		}

		return m_memory.get()->data() + allocated->offset;
	}

	void Free(void* mem)
	{
		ptrdiff_t offset = (char*)mem - m_memory.get()->data();
		Block* block = FindBlockByOffset(&m_blocks.get()->at(0), offset);

		block->isFree = true;
		Block* parent = block->parent;
		
		if (parent->left->isFree && parent->right->isFree)
		{
			parent->left = nullptr;
			parent->right = nullptr;
		}

	}

	void PrintAllocatedIndices()
	{
		for (size_t i = 0; i < m_blocks.get()->size(); i++)
		{
			if (!m_blocks.get()->at(i).isFree)
				std::cout << i << '\n';
		}
		std::cout << std::endl;
	}
};