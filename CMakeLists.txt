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

add_library(Compiler_lib STATIC
    src/token.cpp
    src/grammer.cpp
    src/lexer.cpp
    src/symbol_table.cpp
    src/visitor.cpp
    src/error.cpp
    src/evaluate.cpp
    src/ir/AsmWriter.cpp
    src/ir/BasicBlock.cpp
    src/ir/Constants.cpp
    src/ir/Function.cpp
    src/ir/GlobalValue.cpp
    src/ir/Instructions.cpp
    src/ir/LLVMContext.cpp
    src/ir/LLVMContextImpl.cpp
    src/ir/Module.cpp
    src/ir/Type.cpp
    src/ir/Use.cpp
    src/ir/Value.cpp
    src/ir/ValueSymbolTable.cpp
)
# target_link_libraries(Compiler_lib PRIVATE ${LLVM_LINK_LIBS})

add_executable(Compiler src/main.cpp)
target_link_libraries(Compiler Compiler_lib)

option(BUILD_TESTING "Build unit tests" OFF) # 定义选项，默认为 OFF（不构建测试）
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
