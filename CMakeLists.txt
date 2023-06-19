cmake_minimum_required(VERSION 3.22)
project(app CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CheckIPOSupported)
check_ipo_supported()
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
add_executable(esp2net
    main.cpp
    uart.cpp
    telnet.cpp
)
target_include_directories(esp2net SYSTEM PRIVATE
    asio/asio/include
)
install(TARGETS esp2net RUNTIME DESTINATION bin)
