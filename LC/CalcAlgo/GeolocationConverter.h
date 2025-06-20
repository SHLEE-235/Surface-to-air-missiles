#pragma once

#include <Eigen/Dense> // Eigen 라이브러리 포함
#include <cmath>       // M_PI 정의를 위해

// C++17 이상에서 M_PI를 사용하려면 _USE_MATH_DEFINES 같은 매크로를 정의하거나,
// C++20부터 표준화된 <numbers> 헤더를 사용해야 할 수 있습니다.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GeolocationConverter
{
    // WGS84 Geodetic to ECEF (Earth-Centered, Earth-Fixed)
    // lat_rad, lon_rad: 라디안 단위의 위도, 경도
    // h: 고도 (meters)
    Eigen::Vector3d geodeticToEcef(double lat_rad, double lon_rad, double h);

    // ECEF to ENU (East-North-Up) local coordinates
    // ecef_point: 변환하려는 ECEF 좌표
    // ecef_origin: 지역 ENU의 원점이 될 ECEF 좌표 (예: 발사대 위치)
    // lat_origin_rad, lon_origin_rad: 원점의 라디안 위도, 경도
    Eigen::Vector3d ecefToEnu(const Eigen::Vector3d& ecef_point,
                              const Eigen::Vector3d& ecef_origin,
                              double lat_origin_rad, double lon_origin_rad);

    // ECEF to Geodetic (WGS84)
    // (선택 사항: 디버깅이나 역변환이 필요할 때 사용)
    // Eigen::Vector3d ecefToGeodetic(const Eigen::Vector3d& ecef);

    // 각도 계산 유틸리티
    double degreesToRadians(double degrees);
    double radiansToDegrees(double radians);
}