#pragma once
#include "Point.h"
struct IMUSensorData
{
  Point3f accel; /* X, Y, Z (m/s^2) */
  Point3f gyro;  /* Yaw, Pitch, Roll (degree) */
  Point3f mag;   /* X, Y, Z (uT) */
  IMUSensorData() : accel(Point3f()), gyro(Point3f()), mag(Point3f()) {}
  IMUSensorData(const Point3f &a, const Point3f &g, const Point3f &m)
      : accel(a), gyro(g), mag(m) {}
};

struct GeoLocationData
{
  double Latitude;
  double Longitude;
  double Altitude;
  double Speed;
  GeoLocationData() : Latitude(0.0), Longitude(0.0), Altitude(0.0), Speed(0.0) {}
  GeoLocationData(const double &lat, const double &lon, const double &alt, const double &speed)
      : Latitude(lat), Longitude(lon), Altitude(alt), Speed(speed) {}
};

struct GeoWeatherData
{
  double temp;     /* in Celsius */
  double presure;  /* in millibar */
  double humidity; /* in % */
  GeoWeatherData(const double &t, const double &p, const double &h)
      : temp(t), presure(p), humidity(h) {}
};

template <typename ClassImp>
class ISensor
{
public:
  virtual ~ISensor() = default;
  virtual int Initialize() = 0;
  template <typename T>
  T GetServiceData() const
  {
    return static_cast<const ClassImp *>(this)->template GetServiceData<T>();
  };
};