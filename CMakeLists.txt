cmake_minimum_required(VERSION 3.16)

project(kv_server LANGUAGES CXX)

find_package(Threads REQUIRED)

add_executable(kv_server
    main.cpp
    src/server/server.cpp
    src/session/session.cpp
    src/protocol/parser.cpp
    src/protocol/command_processor.cpp
)

target_include_directories(kv_server
    PRIVATE
    include
)

target_link_libraries(kv_server
    PRIVATE
    Threads::Threads
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