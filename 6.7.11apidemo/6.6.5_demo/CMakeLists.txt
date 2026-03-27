cmake_minimum_required(VERSION 3.10)
project(ctp_demo_linux LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")

if(NOT UNIX)
    message(FATAL_ERROR "This CMakeLists.txt is for Linux/Unix builds.")
endif()

find_package(Threads REQUIRED)

set(CTP_TRADER_SO "${CMAKE_CURRENT_SOURCE_DIR}/thosttraderapi_se.so")
set(CTP_MD_SO "${CMAKE_CURRENT_SOURCE_DIR}/thostmduserapi_se.so")
set(DATA_COLLECT_SO "${CMAKE_CURRENT_SOURCE_DIR}/LinuxDataCollect.so")

if(NOT EXISTS "${CTP_TRADER_SO}")
    message(FATAL_ERROR "Missing shared library: ${CTP_TRADER_SO}")
endif()

if(NOT EXISTS "${CTP_MD_SO}")
    message(FATAL_ERROR "Missing shared library: ${CTP_MD_SO}")
endif()

if(NOT EXISTS "${DATA_COLLECT_SO}")
    message(FATAL_ERROR "Missing shared library: ${DATA_COLLECT_SO}")
endif()

message(STATUS "Using trader so: ${CTP_TRADER_SO}")
message(STATUS "Using md so: ${CTP_MD_SO}")
message(STATUS "Using data collect so: ${DATA_COLLECT_SO}")

add_library(ctp_trader_se SHARED IMPORTED)
set_target_properties(ctp_trader_se PROPERTIES IMPORTED_LOCATION "${CTP_TRADER_SO}")

add_library(ctp_md_se SHARED IMPORTED)
set_target_properties(ctp_md_se PROPERTIES IMPORTED_LOCATION "${CTP_MD_SO}")

add_library(data_collect_so SHARED IMPORTED)
set_target_properties(data_collect_so PROPERTIES IMPORTED_LOCATION "${DATA_COLLECT_SO}")

set(SRC_FILES
    main.cpp
    traderApi.cpp
    traderSpi.cpp
    getconfig.cpp
    stdafx.cpp
    data_collect_stub.cpp
)

add_executable(testprogram ${SRC_FILES})

target_include_directories(testprogram PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(testprogram PRIVATE -fPIC -Wall -Wextra)
target_link_options(testprogram PRIVATE -static-libstdc++ -static-libgcc)
target_link_libraries(testprogram PRIVATE
    ctp_trader_se
    ctp_md_se
    data_collect_so
    Threads::Threads
    dl
)

set_target_properties(testprogram PROPERTIES
    BUILD_RPATH "\$ORIGIN;\$ORIGIN/.."
    INSTALL_RPATH "\$ORIGIN"
)

install(TARGETS testprogram RUNTIME DESTINATION .)
install(FILES
    "${CTP_TRADER_SO}"
    "${CTP_MD_SO}"
    "${DATA_COLLECT_SO}"
    DESTINATION .
)
