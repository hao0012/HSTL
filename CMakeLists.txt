cmake_minimum_required(VERSION 3.14)
project(HSTL)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 在CMake中，某些命令（如 include_directories）的效果是全局的。
# 如果你在顶层的CMakeLists.txt中使用了include_directories，
# 那么该目录及其子目录都会继承这个包含路径。这意味着所有在顶层之后
# 添加的目标都会自动使用该包含路径，除非在子目录中明确地覆盖了这一设置。
include_directories(${CMAKE_SOURCE_DIR}/include)

# 将测试目录添加到构建中
add_subdirectory(test)

set(CLANG_SEARCH_PATH "/usr/local/bin" "/usr/bin" "/usr/local/opt/llvm/bin" "/usr/local/opt/llvm@14/bin"
        "/opt/homebrew/opt/llvm@14/bin/")
set(CLANG_FORMAT_SUPPORT_DIR "${CMAKE_SOURCE_DIR}/build_support/clang_format")
set(CLANG_TIDY_SUPPORT_DIR "${CMAKE_SOURCE_DIR}/build_support/clang_tidy")
# clang
# 让clangd正确理解项目的编译上下文
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# clang-format

# 找到clang-format的可执行文件
find_program(CLANG_FORMAT_BIN
        NAMES clang-format clang-format-14
        HINTS ${CLANG_SEARCH_PATH})
if("${CLANG_FORMAT_BIN}" STREQUAL "CLANG_FORMAT_BIN-NOTFOUND")
        message(WARNING "BusTub/main couldn't find clang-format.")
else()
        message(STATUS "BusTub/main found clang-format at ${CLANG_FORMAT_BIN}")
endif()

# 找到clang-tidy的可执行文件
find_program(CLANG_TIDY_BIN
        NAMES clang-tidy clang-tidy-14
        HINTS ${CLANG_SEARCH_PATH})

if("${CLANG_TIDY_BIN}" STREQUAL "CLANG_TIDY_BIN-NOTFOUND")
        message(WARNING "BusTub/main couldn't find clang-tidy.")
else()
        # Output compile_commands.json
        set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
        message(STATUS "BusTub/main found clang-tidy at ${CLANG_TIDY_BIN}")
endif()

# make format
# make check-format

string(CONCAT FORMAT_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/include,"
        "${CMAKE_CURRENT_SOURCE_DIR}/src,"
        "${CMAKE_CURRENT_SOURCE_DIR}/test,"
)

add_custom_target(format ${CLANG_FORMAT_SUPPORT_DIR}/run_clang_format.py
        ${CLANG_FORMAT_BIN}
        ${CLANG_FORMAT_SUPPORT_DIR}/clang_format_exclusions.txt
        --source_dirs ${FORMAT_DIRS}
        --fix
        --quiet
)

# Runs clang format and exits with a non-zero exit code if any files need to be reformatted
add_custom_target(check-format ${CLANG_FORMAT_SUPPORT_DIR}/run_clang_format.py
        ${CLANG_FORMAT_BIN}
        ${CLANG_FORMAT_SUPPORT_DIR}/clang_format_exclusions.txt
        --source_dirs ${FORMAT_DIRS}
        --quiet
)

# make tidy

add_custom_target(tidy 
        ${CLANG_TIDY_SUPPORT_DIR}/run_clang_tidy.py
        -clang-tidy-binary ${CLANG_TIDY_BIN}
        -p ${CMAKE_BINARY_DIR}
)

set(SMART_PTR_FILES
        "include/shared_ptr.hpp"
        # "include/enable_shared.hpp"
        "test/shared_ptr_test.cpp"
)

add_custom_target(tidy-smartp
        ${CLANG_TIDY_SUPPORT_DIR}/run_clang_tidy.py
        -clang-tidy-binary ${CLANG_TIDY_BIN}
        -p ${CMAKE_BINARY_DIR}
        ${SMART_PTR_FILES}
)

# 构建tidy前 gtest需先构造完成
add_dependencies(tidy gtest)