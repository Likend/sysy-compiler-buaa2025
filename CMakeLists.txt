cmake_minimum_required(VERSION 3.15)

if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

project(Compiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(src)

# find_package(LLVM REQUIRED CONFIG)

# include_directories(${LLVM_INCLUDE_DIRS})
# link_directories(${LLVM_LIBRARY_DIRS})

# llvm_map_components_to_libnames(
#     LLVM_LINK_LIBS
#     Core IRReader Support Analysis Target
# )

file(GLOB_RECURSE SRCS src/*.cpp)
list(REMOVE_ITEM SRCS src/main.cpp)

add_library(Compiler_lib STATIC ${SRCS})
target_compile_definitions(Compiler_lib PRIVATE DEBUG_TOKEN_TYPE_NAME)
# target_link_libraries(Compiler_lib PRIVATE ${LLVM_LINK_LIBS})

add_executable(Compiler src/main.cpp)
target_link_libraries(Compiler Compiler_lib)

target_compile_definitions(Compiler PRIVATE DEBUG_TOKEN_TYPE_NAME)

option(BUILD_TESTING "Build unit tests" OFF) # 定义选项，默认为 OFF（不构建测试）
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
