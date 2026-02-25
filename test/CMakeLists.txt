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

function(add_test_executable TEST_NAME TEST_SOURCE_PATH)

  add_executable(${TEST_NAME} ${TEST_SOURCE_PATH}/${TEST_NAME}.cpp)
  # include headers
  target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

  target_link_libraries(${TEST_NAME}
    GTest::gtest_main
    asan
  )
  
  gtest_discover_tests(${TEST_NAME})
endfunction()

# test/xxx_test.cpp
set(TEST_EXECUTABLES
  vector_test
  shared_ptr_test
  unique_ptr_test
  forward_test
  declval_test
  function_test
)

foreach(TEST ${TEST_EXECUTABLES})
  add_test_executable(${TEST} .)
endforeach()

# test/type_traits/xxx_test.cpp
set(TEST_TYPE_TRAITS_EXECUTABLES
  is_reference_test
  add_reference_test
)

foreach(TEST ${TEST_TYPE_TRAITS_EXECUTABLES})
  add_test_executable(${TEST} type_traits)
endforeach()

# test/concurrency/xxx_test.cpp
set(TEST_CONCURRENCY_EXECUTABLES
  thread_pool_test
  # parallel_test
)

foreach(TEST ${TEST_CONCURRENCY_EXECUTABLES})
  add_test_executable(${TEST} concurrency)
endforeach()