function(add_benchmark_executable FILE_NAME BENCHMARK_SOURCE_PATH)

  add_executable(${FILE_NAME} ${BENCHMARK_SOURCE_PATH}/${FILE_NAME}.cpp)
  # include headers
  target_include_directories(${FILE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

  target_link_libraries(${FILE_NAME}
    benchmark::benchmark
    benchmark::benchmark_main
  )
endfunction()

set(BENCHMARK_EXECUTABLES
  thread_pool_benchmark
)

foreach(BENCHMARK ${BENCHMARK_EXECUTABLES})
  add_benchmark_executable(${BENCHMARK} concurrency)
endforeach()