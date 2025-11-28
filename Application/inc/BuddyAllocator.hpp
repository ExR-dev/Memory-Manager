#pragma once

#include "TracyWrapper.hpp"

#include <memory>
#include <vector>
#include <array>
#include <bitset>
#include <cmath>

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

	size_t m_numRows = static_cast<size_t>(log2(4096 * 1024) - log2(static_cast<double>(m_minimumSize)));
	size_t m_numBlocks = static_cast<size_t>(pow(2, static_cast<double>(m_numRows) + 1) - 1);

	std::unique_ptr<std::vector<Block>> m_blocks;

	Block* FindBlock(Block* block, const size_t size, const int parentIndex)
	{
		ZoneScopedXC(tracy::Color::Fuchsia);

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
					block->left = &m_blocks->at((2 * parentIndex) + 1);
					block->right = &m_blocks->at(2 * (parentIndex + 1));

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

			if (Block* left = FindBlock(block->left, size, (2 * parentIndex) + 1); left != nullptr)
			{
				return left;
			}
			return FindBlock(block->right, size, 2 * (parentIndex + 1));
		}

		return nullptr;
	}

	Block* FindBlockByOffset(Block* block, const size_t offset)
	{
		ZoneScopedXC(tracy::Color::LightSalmon);

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

	bool HasChildren(Block* block)
	{
		return block->left != nullptr || block->right != nullptr;
	}

	bool CanMerge(Block* parent)
	{
		// If at least one child is allocated, we can't merge
		if (!parent->left->isFree || !parent->right->isFree)
			return false;

		// If the left child has children, we can't merge
		if (HasChildren(parent->left))
			return false;
		
		// If the right child has children, we can't merge
		if (HasChildren(parent->right))
			return false;

		// If both children are free and don't have any children of their own, we CAN merge
		return true;
	}

public:
	BuddyAllocator()
	{
		m_memory = std::make_unique<std::array<char, 4096 * 1024>>();
		m_blocks = std::make_unique<std::vector<Block>>();
		m_blocks->resize(m_numBlocks);
		//Do we need this resize? We need to keep the blocks at constant memory places, so yes?
		// Change this to reserve.
		//m_blocks.reserve((1024 * 1000) / m_minimumSize);
		//m_blocks[0].size = 1024 * 1000;

		m_blocks->at(0).size = 4096 * 1024;
		//m_blocks.push_back(baseBlock);
	}

	
	void* Alloc(const size_t size)
	{
		ZoneScopedC(tracy::Color::Red);

		const Block* allocated = FindBlock(&m_blocks->at(0), size, 0);
		if (allocated == nullptr)
		{
			return nullptr;
		}

		TracyAllocN(m_memory->data() + allocated->offset, size, "Buddy");

		return m_memory->data() + allocated->offset;
	}

	void Free(void* mem)
	{
		ZoneScopedC(tracy::Color::Green);

		const ptrdiff_t offset = static_cast<char*>(mem) - m_memory->data();
		Block* block = FindBlockByOffset(&m_blocks->at(0), offset);

		if (block == nullptr)
			throw std::runtime_error("Block was nullptr when searching by offset. Could not free the memory."); // ITS BAD GET MOM

		TracyFreeN(mem, "Buddy");

		block->isFree = true;

		if (Block* parent = block->parent; CanMerge(parent))
		{
			parent->left = nullptr;
			parent->right = nullptr;
		}
	}

	void PrintAllocatedIndices()
	{
		ZoneScopedC(tracy::Color::RoyalBlue1);

		for (size_t i = 0; i < m_blocks->size(); i++)
		{
			if (!m_blocks->at(i).isFree)
				std::cout << i << '\n';
		}

		std::cout << std::flush;
	}
};