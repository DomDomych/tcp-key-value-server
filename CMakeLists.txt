cmake_minimum_required(VERSION 3.16)

project(kv_server LANGUAGES CXX)

include(FetchContent)

find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

set(SKIP_BUILD_TEST ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    libpqxx
    GIT_REPOSITORY https://github.com/jtv/libpqxx.git
    GIT_TAG 7.10.1
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(libpqxx)


add_executable(kv_server
    main.cpp
    src/server/server.cpp
    src/session/session.cpp
    src/protocol/parser.cpp
    src/protocol/command_processor.cpp
    src/storage/storage.cpp
    src/storage/postgres_storage.cpp
    src/storage/lru_cache.cpp
)

target_include_directories(kv_server
    PRIVATE
    include
)

target_link_libraries(kv_server
    PRIVATE
    Threads::Threads
    pqxx
)

target_compile_features(kv_server
    PRIVATE
    cxx_std_20
)

target_compile_options(kv_server
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)


add_executable(kv_client
    src/client/client.cpp
)

target_compile_features(kv_client
    PRIVATE
    cxx_std_20
)

target_compile_options(kv_client
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)


enable_testing()

add_executable(kv_tests
    tests/parser_tests.cpp
    tests/processor_tests.cpp
    src/protocol/command_processor.cpp
    src/protocol/parser.cpp
    src/storage/storage.cpp
    src/storage/postgres_storage.cpp
    src/storage/lru_cache.cpp
)

target_include_directories(kv_tests
    PRIVATE
    include
)

target_link_libraries(kv_tests
    PRIVATE
    GTest::gtest_main
    Threads::Threads
    pqxx
)

target_compile_features(kv_tests
    PRIVATE
    cxx_std_20
)

target_compile_options(kv_tests
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

include(GoogleTest)

gtest_discover_tests(kv_tests)