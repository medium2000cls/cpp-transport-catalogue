cmake_minimum_required(VERSION 3.23)
project(cpp-transport-catalogue)

set(CMAKE_CXX_STANDARD 17)
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wfatal-errors -O0 -Wno-unknown-pragmas -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_LIBCPP_DEBUG=1 -g")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wfatal-errors -O3")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wfatal-errors -O0")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -static")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/transport-catalogue/test_case
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

add_executable(cpp-transport-catalogue
        transport-catalogue/geo.h
        transport-catalogue/input_reader.cpp
        transport-catalogue/input_reader.h
        transport-catalogue/main.cpp
        transport-catalogue/stat_reader.cpp
        transport-catalogue/stat_reader.h
        transport-catalogue/transport_catalogue.cpp
        transport-catalogue/transport_catalogue.h transport-catalogue/tests.h transport-catalogue/tests.cpp)
