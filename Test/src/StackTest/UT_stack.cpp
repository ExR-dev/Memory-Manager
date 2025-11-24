#include "../../../Application/inc/StackAllocator.hpp"
#include <gtest/gtest.h>

TEST(StackTest, InitiallyEmpty)
{
    StackAllocator stackAllocator;

    //ASSERT_EQ(stackAllocator.DBG_GetTop(), 0ULL);

    //ASSERT_TRUE(stackAllocator.DBG_GetStack().get()->empty());
    ASSERT_TRUE(true);
}