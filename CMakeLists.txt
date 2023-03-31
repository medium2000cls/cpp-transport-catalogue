cmake_minimum_required(VERSION 3.23)
project(cpp-transport-catalogue)

set(CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wfatal-errors -O0 -Wno-unknown-pragmas -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_LIBCPP_DEBUG=1 -g")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wfatal-errors -O3")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wfatal-errors -O0")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
set(PYTHON_FILES PlantUml.py)

file(COPY transport-catalogue/tests/test_case
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

add_executable(cpp-transport-catalogue
        transport-catalogue/main.cpp
        transport-catalogue/infrastructure/stream_reader.h
        transport-catalogue/infrastructure/stream_reader.cpp
        transport-catalogue/domain/domain.h
        transport-catalogue/domain/domain.cpp
        transport-catalogue/domain/geo.h
        transport-catalogue/domain/geo.cpp
        transport-catalogue/external/json.h
        transport-catalogue/external/json.cpp
        transport-catalogue/infrastructure/json_reader.h
        transport-catalogue/infrastructure/json_reader.cpp
        transport-catalogue/infrastructure/map_renderer.h
        transport-catalogue/infrastructure/map_renderer.cpp
        transport-catalogue/infrastructure/request_handler.h
        transport-catalogue/infrastructure/request_handler.cpp
        transport-catalogue/external/svg.h
        transport-catalogue/external/svg.cpp
        transport-catalogue/business_logic/transport_catalogue.cpp
        transport-catalogue/business_logic/transport_catalogue.h
        transport-catalogue/tests/tests.h
        transport-catalogue/tests/tests.cpp
        transport-catalogue/infrastructure/io_requests_base.h
        transport-catalogue/infrastructure/io_requests_base.cpp
        transport-catalogue/external/json_builder.h
        transport-catalogue/external/json_builder.cpp)
