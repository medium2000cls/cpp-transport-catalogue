cmake_minimum_required(VERSION 3.23)
project(transport_catalogue)

set(CMAKE_CXX_STANDARD 17)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wfatal-errors -O0 -Wno-unknown-pragmas -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_LIBCPP_DEBUG=1 -g")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wfatal-errors -O3")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wfatal-errors -O0")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

file(COPY transport-catalogue/tests/test_case DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

set(PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/transport-catalogue/domain/transport_catalogue.proto)
#message(STATUS "PROTO_FILES: ${PROTO_FILES}")
set(MAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/transport-catalogue/main.cpp)
#message(STATUS "MAIN_FILE: ${MAIN_FILE}")
set(DOMAIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/transport-catalogue/domain)
#message(STATUS "DOMAIN_DIR: ${DOMAIN_DIR}")
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/transport-catalogue/tests)
#message(STATUS "TEST_DIR: ${TEST_DIR}")
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/transport-catalogue/external)
#message(STATUS "EXTERNAL_DIR: ${EXTERNAL_DIR}")
set(BUSINESS_LOGIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/transport-catalogue/business_logic)
#message(STATUS "BUSINESS_LOGIC_DIR: ${BUSINESS_LOGIC_DIR}")
set(INFRASTRUCTURE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/transport-catalogue/infrastructure)
#message(STATUS "INFRASTRUCTURE_DIR: ${INFRASTRUCTURE_DIR}")

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)
 
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

message(STATUS "PROTO_SRCS: ${PROTO_SRCS} PROTO_HDRS: ${PROTO_HDRS}")

add_executable(transport_catalogue
        ${PROTO_SRCS}
        ${PROTO_HDRS}
        ${MAIN_FILE}
        ${DOMAIN_DIR}/domain.h
        ${DOMAIN_DIR}/domain.cpp
        ${DOMAIN_DIR}/geo.h
        ${DOMAIN_DIR}/geo.cpp
        ${TEST_DIR}/tests.h
        ${TEST_DIR}/tests.cpp
        ${EXTERNAL_DIR}/json.h
        ${EXTERNAL_DIR}/json.cpp
        ${EXTERNAL_DIR}/svg.h
        ${EXTERNAL_DIR}/svg.cpp
        ${EXTERNAL_DIR}/json_builder.h
        ${EXTERNAL_DIR}/json_builder.cpp
        ${EXTERNAL_DIR}/router.h
        ${EXTERNAL_DIR}/graph.h
        ${EXTERNAL_DIR}/ranges.h
        ${BUSINESS_LOGIC_DIR}/transport_catalogue.cpp
        ${BUSINESS_LOGIC_DIR}/transport_catalogue.h
        ${BUSINESS_LOGIC_DIR}/transport_router.cpp
        ${BUSINESS_LOGIC_DIR}/transport_router.h
        ${INFRASTRUCTURE_DIR}/stream_reader.h
        ${INFRASTRUCTURE_DIR}/stream_reader.cpp
        ${INFRASTRUCTURE_DIR}/json_reader.h
        ${INFRASTRUCTURE_DIR}/json_reader.cpp
        ${INFRASTRUCTURE_DIR}/map_renderer.h
        ${INFRASTRUCTURE_DIR}/map_renderer.cpp
        ${INFRASTRUCTURE_DIR}/request_handler.h
        ${INFRASTRUCTURE_DIR}/request_handler.cpp
        ${INFRASTRUCTURE_DIR}/io_requests_base.h
        ${INFRASTRUCTURE_DIR}/io_requests_base.cpp
        ${INFRASTRUCTURE_DIR}/serialization.cpp
        ${INFRASTRUCTURE_DIR}/serialization.h)

target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)
