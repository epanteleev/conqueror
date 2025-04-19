#include <gtest/gtest.h>

#include "allocation/SeqAllocator.h"

TEST(Allocation, test1) {
    conq::memory::SeqAllocator<int, 2> allocator;
    int* ptr1 = allocator.allocate(42);
    ASSERT_NE(ptr1, nullptr);
    *ptr1 = 42;
    int* ptr2 = allocator.allocate(43);
    ASSERT_NE(ptr2, nullptr);
    *ptr2 = 43;

    int* ptr3 = allocator.allocate(44);
    ASSERT_EQ(ptr3, nullptr);

    ASSERT_EQ(*ptr1, 42);
    ASSERT_EQ(*ptr2, 43);
    ASSERT_EQ(allocator.size(), 2);
    allocator.deallocate(ptr1);
    allocator.deallocate(ptr2);

    ASSERT_EQ(allocator.size(), 0);
}

TEST(ObjPool, test1) {
    conq::memory::Rc<int> p{};
    {
        conq::memory::ObjPool<int, 2> pool;
        p = pool.allocate(42);
        *p = 42;
        auto ptr2 = pool.allocate(43);
        *ptr2 = 43;

        auto ptr3 = pool.allocate(44);
        *ptr3 = 44;
        ASSERT_EQ(*p, 42);
        ASSERT_EQ(*ptr2, 43);
        ASSERT_EQ(*ptr3, 44);

        pool.deallocate(ptr2);
        ASSERT_EQ(ptr2.has_value(), false);
    }

    ASSERT_EQ(p.has_value(), false);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}