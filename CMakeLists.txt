cmake_minimum_required(VERSION 3.27)

project(conqueror
        VERSION 0.1
        DESCRIPTION "Conqueror"
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 26)

set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=thread")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 -Wall -Wextra -Wpedantic -Werror ")

set(CONQ_LIB_SRC
        src/SPSCBoundedQueue.h
        src/SPSC.h
        src/MPSCBoundedQueue.h
        src/RCU.h
        src/Definitions.h
        src/MPMC.h
        src/os/ShMem.h
        src/channel/Channel.h
        src/channel/Encoder.h
        src/os/Process.h
        src/os/LinuxError.h
        src/os/Semaphore.h
        src/LockFreeStack.h
)

add_library(libconq_libconq STATIC ${CONQ_LIB_SRC})
add_library(libconq::libconq ALIAS libconq_libconq)

set_target_properties(
        libconq_libconq PROPERTIES
        LINKER_LANGUAGE CXX
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${PROJECT_VERSION_MAJOR}"
        EXPORT_NAME libconq
        OUTPUT_NAME libconq
)

target_include_directories(
        libconq_libconq ${warning_guard}
        PUBLIC
        "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

target_compile_features(libconq_libconq PUBLIC cxx_std_23)

add_executable(conqueror main.cpp)
target_link_libraries(conqueror PUBLIC libconq::libconq)

add_subdirectory(tests)