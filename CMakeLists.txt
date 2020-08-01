cmake_minimum_required(VERSION 3.9)

project(rpx LANGUAGES CXX)

add_library(rpx SHARED
    include/rpx/Globals.h
    include/rpx/Utils.h
    include/rpx/RemoteManager.h
    include/rpx/RemoteObject.h
    include/rpx/ICommunication.h
    include/rpx/TCPServer.h
    include/rpx/TCPClient.h
    src/RemoteManager.cpp
    src/RemoteObject.cpp
    src/TCPServer.cpp
    src/TCPClient.cpp
)

set_target_properties(rpx PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY "bin/"
    RUNTIME_OUTPUT_DIRECTORY "bin/"
    LIBRARY_OUTPUT_DIRECTORY "lib/"
    )

target_include_directories(rpx PRIVATE src)
target_include_directories(rpx PUBLIC include)
target_link_libraries(rpx pthread)
    
