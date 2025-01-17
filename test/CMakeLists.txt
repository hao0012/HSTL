# 当修改了CMakeLists.txt文件后，需要重新生成build目录(vscode中自动生成)
# cmake ..
# make

include(FetchContent)
# 引入google test
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

include(GoogleTest)

set(TEST_EXECUTABLES
  vector_test
  shared_ptr_test
  unique_ptr_test
  forward_test
  is_reference_test
  add_reference_test
  declval_test
)

foreach(TEST ${TEST_EXECUTABLES})
  add_executable(${TEST} ${TEST}.cpp)
  target_link_libraries(${TEST} GTest::gtest_main)
  target_link_libraries(${TEST} asan)
  gtest_discover_tests(${TEST})
endforeach()