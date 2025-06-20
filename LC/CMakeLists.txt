cmake_minimum_required(VERSION 3.10)
project(LC_Project)

set(CMAKE_CXX_STANDARD 17)

# Debug build 설정 추가
set(CMAKE_BUILD_TYPE Debug)  # 기본 빌드 타입을 Debug로 설정
set(CMAKE_CXX_FLAGS_DEBUG "-g")  # -g 플래그로 디버깅 정보 포함

# Define include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/comm/common
    ${CMAKE_CURRENT_SOURCE_DIR}/comm/ecc
    ${CMAKE_CURRENT_SOURCE_DIR}/comm/ls
    ${CMAKE_CURRENT_SOURCE_DIR}/comm/mfr
    ${CMAKE_CURRENT_SOURCE_DIR}/status
    ${CMAKE_CURRENT_SOURCE_DIR}/inih
    ${CMAKE_CURRENT_SOURCE_DIR}/Config
)

# Set the output directory for the executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Define the executable and its source files
add_executable(lc
    main.cpp

    Config/LCConfig.cpp

    # Core components
    core/LCManager.cpp
    core/StatusLoader.cpp
    core/timeTrans.cpp
    # Communication modules
    comm/ecc/TcpECC.cpp
    comm/ls/SerialLS.cpp
    comm/mfr/TcpMFR.cpp
    core/LCCommandHandler.cpp

    # Common utilities
    comm/common/MessageParser.cpp
    comm/common/Serializer.cpp

    # External library
    inih/ini.c
)