#include "../../../Application/inc/StackAllocator.hpp"
#include <gtest/gtest.h>
#include <vector>

TEST(StackTest, InitiallyEmpty)
{
    StackAllocator stackAllocator;

    ASSERT_EQ(stackAllocator.DBG_GetTop(), 0ULL);
}

TEST(StackTest, PushData)
{
    StackAllocator stackAllocator;

    int intA = 1;
    int intB = 2;
    int intC = 3;

    stackAllocator.Push(&intA, sizeof(intA));
    stackAllocator.Push(&intB, sizeof(intB));
    stackAllocator.Push(&intC, sizeof(intC));

    std::string strA = "AAAAAAAA";
    std::string strB = "BBBBBBBB";
    std::string strC = "CCCCCCCC";
    std::string strD = "DDDDDDDD";

    stackAllocator.Push(&strA, sizeof(strA));
    stackAllocator.Push(&strB, sizeof(strB));
    stackAllocator.Push(&strC, sizeof(strC));
    stackAllocator.Push(&strD, sizeof(strD));

    size_t expectedSize = sizeof(intA) + sizeof(intB) + sizeof(intC) +
        sizeof(strA) + sizeof(strB) + sizeof(strC) + sizeof(strD);

    ASSERT_EQ(stackAllocator.DBG_GetTop(), expectedSize);
}

TEST(StackTest, OriginalCopyGoesOutOfScope)
{
    StackAllocator stackAllocator;
    std::vector<size_t> ptrs;
    for (int i = 0; i < 5; i++)
    {
        int foo = i;
        ptrs.push_back(stackAllocator.Push(&foo, sizeof(foo)));
    }

    for (int i = 0; i < 5; i++)
    {
        int* foo = (int*)stackAllocator.At(ptrs[i]);
        ASSERT_EQ(*foo, i);
    }
}

TEST(StackTest, RetrieveData)
{
    StackAllocator stackAllocator;
    std::vector<size_t> intPtrs;
    std::vector<size_t> strPtrs;

    // Push data to the stack
    {
        int intA = 1;
        int intB = 2;
        int intC = 3;

        intPtrs.push_back(stackAllocator.Push(&intA, sizeof(intA)));
        intPtrs.push_back(stackAllocator.Push(&intB, sizeof(intB)));
        intPtrs.push_back(stackAllocator.Push(&intC, sizeof(intC)));

        std::string strA = "AAAAAAAA";
        std::string strB = "BBBBBBBB";
        std::string strC = "CCCCCCCC";
        std::string strD = "DDDDDDDD";

        strPtrs.push_back(stackAllocator.Push(&strA, sizeof(strA)));
        strPtrs.push_back(stackAllocator.Push(&strB, sizeof(strB)));
        strPtrs.push_back(stackAllocator.Push(&strC, sizeof(strC)));
        strPtrs.push_back(stackAllocator.Push(&strD, sizeof(strD)));
    }

    int* intA = (int*)stackAllocator.At(intPtrs[0]);
    int* intB = (int*)stackAllocator.At(intPtrs[1]);
    int* intC = (int*)stackAllocator.At(intPtrs[2]);

    std::string* strA = (std::string*)stackAllocator.At(strPtrs[0]);
    std::string* strB = (std::string*)stackAllocator.At(strPtrs[1]);
    std::string* strC = (std::string*)stackAllocator.At(strPtrs[2]);
    std::string* strD = (std::string*)stackAllocator.At(strPtrs[3]);

    ASSERT_EQ(*intA, 1);
    ASSERT_EQ(*intB, 2);
    ASSERT_EQ(*intC, 3);

    ASSERT_EQ(*strA, "AAAAAAAA");
    ASSERT_EQ(*strB, "BBBBBBBB");
    ASSERT_EQ(*strC, "CCCCCCCC");
    ASSERT_EQ(*strD, "DDDDDDDD");
}

TEST(StackTest, NoOverflow)
{
    StackAllocator stackAllocator;

    constexpr size_t arrSize = stackAllocator.DBG_GetMaxSize() - 5;
    std::array<char, arrSize> arr = {};
    arr.fill('a');

    stackAllocator.Push(&arr, sizeof(arr));

    ASSERT_EQ(stackAllocator.DBG_GetTop(), arrSize);

    char a = 'a';
    char b = 'b';
    char c = 'c';
    char d = 'd';
    char e = 'e';
    char f = 'f';

    stackAllocator.Push(&a, sizeof(a));
    stackAllocator.Push(&b, sizeof(b));
    stackAllocator.Push(&c, sizeof(c));
    stackAllocator.Push(&d, sizeof(d));
    
    ASSERT_NE(stackAllocator.DBG_GetTop(), (size_t)-1);
    
    stackAllocator.Push(&e, sizeof(e));
    
    ASSERT_EQ(stackAllocator.DBG_GetTop(), stackAllocator.DBG_GetMaxSize());

    size_t ptr = stackAllocator.Push(&f, sizeof(f));

    ASSERT_EQ(ptr, (size_t)-1);
}
