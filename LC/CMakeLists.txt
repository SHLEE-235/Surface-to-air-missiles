cmake_minimum_required(VERSION 3.10)
project(LC_Project)

set(CMAKE_CXX_STANDARD 17)

# Debug build 설정 추가
set(CMAKE_BUILD_TYPE Debug)  # 기본 빌드 타입을 Debug로 설정
set(CMAKE_CXX_FLAGS_DEBUG "-g")  # -g 플래그로 디버깅 정보 포함

find_package(Eigen3 CONFIG REQUIRED)

if (NOT Eigen3_FOUND)
    message(FATAL_ERROR "Eigen3 not found! Please check Eigen3_DIR path in CMakeLists.txt: ${Eigen3_DIR}")
endif()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/CalcAlgo
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

    # Communication modules
    comm/ecc/TcpECC.cpp
    comm/ls/SerialLS.cpp
    comm/mfr/TcpMFR.cpp
    core/LCCommandHandler.cpp

    # Common utilities
    comm/common/MessageParser.cpp
    comm/common/Serializer.cpp

    # ECEF 변환 유틸리티 추가
    CalcAlgo/GeolocationConverter.cpp # <--- 이 줄을 추가

    # External library
    inih/ini.c
)
target_link_libraries(lc PUBLIC Eigen3::Eigen)


