#include "GeolocationConverter.h"

// WGS84 상수를 전역 또는 네임스페이스 내에 정의
namespace GeolocationConverter
{
    // WGS84 Ellipsoid constants
    const double a = 6378137.0;       // Semi-major axis (meters)
    const double f = 1.0 / 298.257223563; // Flattening
    const double b = a * (1.0 - f);   // Semi-minor axis (meters)
    const double e_sq = f * (2.0 - f); // Eccentricity squared

    Eigen::Vector3d geodeticToEcef(double lat_rad, double lon_rad, double h)
    {
        double N = a / std::sqrt(1.0 - e_sq * std::sin(lat_rad) * std::sin(lat_rad));
        double X = (N + h) * std::cos(lat_rad) * std::cos(lon_rad);
        double Y = (N + h) * std::cos(lat_rad) * std::sin(lon_rad);
        double Z = (N * (1.0 - e_sq) + h) * std::sin(lat_rad);
        return Eigen::Vector3d(X, Y, Z);
    }

    Eigen::Vector3d ecefToEnu(const Eigen::Vector3d& ecef_point,
                              const Eigen::Vector3d& ecef_origin,
                              double lat_origin_rad, double lon_origin_rad)
    {
        // Vector from origin to point in ECEF
        Eigen::Vector3d delta_ecef = ecef_point - ecef_origin;

        // ECEF to ENU rotation matrix (based on origin's lat/lon)
        double sin_lat = std::sin(lat_origin_rad);
        double cos_lat = std::cos(lat_origin_rad);
        double sin_lon = std::sin(lon_origin_rad);
        double cos_lon = std::cos(lon_origin_rad);

        // This matrix transforms a vector in ECEF coordinates to ENU coordinates
        // relative to the origin (lat_origin_rad, lon_origin_rad).
        Eigen::Matrix3d R_enu;
        R_enu << -sin_lon,           cos_lon,          0,
                 -sin_lat * cos_lon, -sin_lat * sin_lon, cos_lat,
                  cos_lat * cos_lon,  cos_lat * sin_lon, sin_lat;

        // Apply rotation
        return R_enu * delta_ecef;
    }

    double degreesToRadians(double degrees)
    {
        return degrees * M_PI / 180.0;
    }

    double radiansToDegrees(double radians)
    {
        return radians * 180.0 / M_PI;
    }
}