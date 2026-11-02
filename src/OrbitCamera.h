#pragma once

#include <cmath>

namespace pool {

inline constexpr double kCameraPi = 3.14159265358979323846;
inline constexpr double kCameraOrbitRadius = 4.4;
inline constexpr double kCameraTargetY = 0.02;
inline constexpr double kCameraElevationDegrees = 40.0;
inline constexpr double kCameraOrbitSpeed = kCameraPi / 4.0; // 45 degrees/second.

struct CameraPosition { double x, y, z; };

inline double wrappedCameraAzimuth(double angle) {
    if(!std::isfinite(angle)) return 0.0;
    double wrapped=std::fmod(angle,2.0*kCameraPi);
    if(wrapped<0.0) wrapped+=2.0*kCameraPi;
    return wrapped;
}

inline double advanceCameraAzimuth(double angle,double seconds,bool reverse) {
    if(!std::isfinite(seconds)||seconds<=0.0) return wrappedCameraAzimuth(angle);
    const double direction=reverse ? -1.0 : 1.0;
    return wrappedCameraAzimuth(angle+direction*kCameraOrbitSpeed*seconds);
}

// Radius and height stay constant while azimuth changes around the Y axis.
inline CameraPosition cameraOrbitPosition(double azimuth) {
    const double angle=wrappedCameraAzimuth(azimuth);
    const double height=kCameraOrbitRadius*std::tan(kCameraElevationDegrees*kCameraPi/180.0);
    return {kCameraOrbitRadius*std::sin(angle),kCameraTargetY+height,
            -kCameraOrbitRadius*std::cos(angle)};
}

} // namespace pool
