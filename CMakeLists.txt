cmake_minimum_required(VERSION 3.20)
project(loomlog VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LOOMLOG_BUILD_TESTS "Build tests" ON)
option(LOOMLOG_BUILD_BENCH "Build benches" ON)
option(LOOMLOG_BUILD_CLI "Build CLI" ON)

add_library(loomlog
  src/thread_pool.cpp
  src/logger.cpp
  src/text_analyzer.cpp
  src/file_processor.cpp
  src/directory_scanner.cpp
  src/inverted_index.cpp
  src/indexer.cpp
  src/index_store.cpp
  src/query_engine.cpp
)
add_library(loomlog::loomlog ALIAS loomlog)
target_compile_features(loomlog PUBLIC cxx_std_20)
target_include_directories(loomlog
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(LOOMLOG_BUILD_CLI)
  add_executable(loomsearch src/cli.cpp)
  target_link_libraries(loomsearch PRIVATE loomlog)
endif()

if(LOOMLOG_BUILD_TESTS)
    enable_testing()

    if(WIN32)
        include(FetchContent)

        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.15.2
        )

        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    else()
        find_package(GTest REQUIRED)
    endif()

    add_executable(test_thread_pool tests/test_thread_pool.cpp)
    target_link_libraries(test_thread_pool PRIVATE loomlog GTest::gtest_main)

    add_executable(test_logger tests/test_logger.cpp)
    target_link_libraries(test_logger PRIVATE loomlog GTest::gtest_main)

    add_executable(test_text_analyzer tests/test_text_analyzer.cpp)
    target_link_libraries(test_text_analyzer PRIVATE loomlog GTest::gtest_main)

    add_executable(test_file_processor tests/test_file_processor.cpp)
    target_link_libraries(test_file_processor PRIVATE loomlog GTest::gtest_main)

    add_executable(test_directory_scanner tests/test_directory_scanner.cpp)
    target_link_libraries(test_directory_scanner PRIVATE loomlog GTest::gtest_main)

    add_executable(test_inverted_index tests/test_inverted_index.cpp)
    target_link_libraries(test_inverted_index PRIVATE loomlog GTest::gtest_main)

    add_executable(test_indexer tests/test_indexer.cpp)
    target_link_libraries(test_indexer PRIVATE loomlog GTest::gtest_main)

    add_executable(test_index_store tests/test_index_store.cpp)
    target_link_libraries(test_index_store PRIVATE loomlog GTest::gtest_main)

    add_executable(test_query_engine tests/test_query_engine.cpp)
    target_link_libraries(test_query_engine PRIVATE loomlog GTest::gtest_main)

    if(LOOMLOG_BUILD_CLI)
        add_executable(test_cli tests/test_cli.cpp)
        target_link_libraries(test_cli PRIVATE GTest::gtest_main)
        target_compile_definitions(test_cli PRIVATE LOOMSEARCH_CLI_PATH="$<TARGET_FILE:loomsearch>")
        add_dependencies(test_cli loomsearch)
    endif()

    include(GoogleTest)
    gtest_discover_tests(test_thread_pool)
    gtest_discover_tests(test_logger)
    gtest_discover_tests(test_text_analyzer)
    gtest_discover_tests(test_file_processor)
    gtest_discover_tests(test_directory_scanner)
    gtest_discover_tests(test_inverted_index)
    gtest_discover_tests(test_indexer)
    gtest_discover_tests(test_index_store)
    gtest_discover_tests(test_query_engine)
    if(LOOMLOG_BUILD_CLI)
        gtest_discover_tests(test_cli)
    endif()
endif()

if(LOOMLOG_BUILD_BENCH)
  add_executable(bench_pool benches/bench_pool.cpp)
  target_link_libraries(bench_pool PRIVATE loomlog)

  add_executable(bench_log benches/bench_log.cpp)
  target_link_libraries(bench_log PRIVATE loomlog)
endif()
