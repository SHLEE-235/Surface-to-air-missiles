cmake_minimum_required(VERSION 3.10)
project(RadarSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 소스 파일들
set(SOURCES
    main.cpp
    Mfr.cpp
    MfrLcCommManager.cpp
    MfrSimCommManager.cpp
    StepMotorController.cpp
)

# 실행 파일 생성
add_executable(radar ${SOURCES})

# pthread 사용 시 연결 (sleep, thread 등)
target_link_libraries(radar pthread)
