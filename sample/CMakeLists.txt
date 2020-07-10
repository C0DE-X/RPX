cmake_minimum_required(VERSION 3.9)

project(rpx-sample LANGUAGES CXX)

include_directories(../include)
link_directories(../lib)


add_executable(rpx-client
                client/main.cpp
)

add_executable(rpx-server
                server/main.cpp
)

set_target_properties(rpx-client PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    )

set_target_properties(rpx-server PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    )

target_link_libraries(rpx-server pthread rpx)
target_link_libraries(rpx-client pthread rpx)