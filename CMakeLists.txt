
cmake_minimum_required(VERSION 3.20)
project(labs VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(core STATIC
    src/dataset.cpp
    src/matrix.cpp
    src/decision_stump.cpp
)

target_include_directories(core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(core PRIVATE -Wall -Wextra)

# Лабораторная №1
add_executable(app
    src/main.cpp
)

target_link_libraries(app PRIVATE core)
target_compile_options(app PRIVATE -Wall -Wextra)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# Лабораторная №2
add_executable(lab2
    src/lab2_main.cpp
)

target_link_libraries(lab2 PRIVATE core)
target_compile_options(lab2 PRIVATE -Wall -Wextra)

# Тесты
add_executable(tests
    tests/tests.cpp
)

target_link_libraries(tests PRIVATE core)
target_compile_options(tests PRIVATE -Wall -Wextra)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tests PRIVATE -UNDEBUG)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    foreach(target core app lab2 tests)
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(${target} PRIVATE
            -fsanitize=address,undefined
        )
    endforeach()
endif()

enable_testing()
add_test(NAME unit COMMAND tests)

# Документация не входит в обычную сборку и не требует Graphviz.
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    configure_file(Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(docs
        COMMAND Doxygen::doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating HTML documentation with Doxygen"
        VERBATIM
    )
else()
    message(STATUS "Doxygen not found: install it with 'sudo apt install doxygen' to build HTML documentation; app, lab2 and tests remain available.")
    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E echo "Doxygen is required for docs. Install: sudo apt install doxygen. Then configure CMake again."
        COMMAND ${CMAKE_COMMAND} -E false
        VERBATIM
    )
endif()
