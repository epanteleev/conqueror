include(../cmake/GoogleTest.cmake)

enable_testing()
include(GoogleTest)

function(add_test_executable target)
    add_executable(${target} ${ARGN})
    target_compile_features(${target} PUBLIC cxx_std_23)

    target_link_libraries(${target} PRIVATE libconq::libconq GTest::gtest GTest::gtest_main)
    gtest_discover_tests(${target})

    add_test(NAME ${target} COMMAND ${target})
endfunction()

add_test_executable(scsp_test scsp_test scsp_test.cpp)
add_test_executable(mpsc_test mpsc_test mpsc_test.cpp)
add_test_executable(mpmc_test mpmc_test mpmc_test.cpp)
add_test_executable(channel_test channel_test channel_test.cpp)
add_test_executable(semaphore_test semaphore_test semaphore_test.cpp)
add_test_executable(rcu_test rcu_test rcu_test.cpp)
add_test_executable(lock_free_stack_test lock_free_stack_test lock_free_stack_test.cpp)
add_test_executable(perf_test perf_test perf_test.cpp)
add_test_executable(allocation_test allocation_test allocation_test.cpp)