cmake_minimum_required(VERSION 3.20)
project(labs VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(core STATIC
    src/dataset.cpp
    src/matrix.cpp
)

target_include_directories(core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(core PRIVATE -Wall -Wextra)

add_executable(app
    src/main.cpp
)

target_link_libraries(app PRIVATE core)
target_compile_options(app PRIVATE -Wall -Wextra)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

add_executable(tests
    tests/tests.cpp
)

target_link_libraries(tests PRIVATE core)
target_compile_options(tests PRIVATE -Wall -Wextra)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tests PRIVATE -UNDEBUG)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(core PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(core PRIVATE -fsanitize=address,undefined)
    
    target_compile_options(app PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(app PRIVATE -fsanitize=address,undefined)
    
    target_compile_options(tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(tests PRIVATE -fsanitize=address,undefined)
endif()

enable_testing()
add_test(NAME unit COMMAND tests)