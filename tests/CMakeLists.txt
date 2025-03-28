include(../cmake/GoogleTest.cmake)

enable_testing()

add_executable(scsp_test scsp_test.cpp)
target_compile_features(scsp_test PUBLIC cxx_std_23)
target_link_libraries(scsp_test PRIVATE libconq::libconq GTest::gtest GTest::gtest_main)

add_executable(mpsc_test mpsc_test.cpp)
target_compile_features(mpsc_test PUBLIC cxx_std_23)
target_link_libraries(mpsc_test PRIVATE libconq::libconq GTest::gtest GTest::gtest_main)

add_executable(mpmc_test mpmc_test.cpp)
target_compile_features(mpmc_test PUBLIC cxx_std_23)
target_link_libraries(mpmc_test PRIVATE libconq::libconq GTest::gtest GTest::gtest_main)

add_executable(channel_test channel_test.cpp)
target_compile_features(channel_test PUBLIC cxx_std_23)
target_link_libraries(channel_test PRIVATE libconq::libconq GTest::gtest GTest::gtest_main)

include(GoogleTest)

gtest_discover_tests(scsp_test)
gtest_discover_tests(mpsc_test)
gtest_discover_tests(mpmc_test)
gtest_discover_tests(channel_test)

add_test(NAME scsp_test COMMAND scsp_test)
add_test(NAME mpsc_test COMMAND mpsc_test)
add_test(NAME mpmc_test COMMAND mpmc_test)