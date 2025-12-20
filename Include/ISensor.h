#pragma once
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <cstdio>
#include <stdexcept>
#include <array>
#include "Point.h"
#define PI 3.14159265

struct IMUSensorData
{
  Point3f accel; /* X, Y, Z (m/s^2) */
  Point3f gyro;  /* Yaw, Pitch, Roll (degree) */
  Point3f mag;   /* X, Y, Z (uT) */
  operator bool() const { return (accel && gyro && mag); }
  IMUSensorData() : accel(Point3f()), gyro(Point3f()), mag(Point3f()) {}
  IMUSensorData(const Point3f &a, const Point3f &g, const Point3f &m)
      : accel(a), gyro(g), mag(m) {}
};

struct GeoLocationData
{
  double Latitude;  /* degree */
  double Longitude; /* degree */
  double Altitude;  /* degree */
  double Speed;     /* degree */
  auto operator<=>(const GeoLocationData &) const = default;
  operator bool() const { return !(Latitude == -MAXFLOAT && Longitude == -MAXFLOAT && Altitude == -MAXFLOAT && Speed == -MAXFLOAT); }
  GeoLocationData() : Latitude(-MAXFLOAT), Longitude(-MAXFLOAT), Altitude(-MAXFLOAT), Speed(-MAXFLOAT) {}
  GeoLocationData(const double &lat, const double &lon, const double &alt, const double &speed)
      : Latitude(lat), Longitude(lon), Altitude(alt), Speed(speed) {}
};

struct GeoWeatherData
{
  double temp;       /* in Celsius */
  double pressure;   /* in millibar */
  double humidity;   /* in % */
  double wind_speed; /* m/s */
  GeoWeatherData operator-(const GeoWeatherData &rhs)
  {
    temp -= rhs.temp;
    pressure -= rhs.pressure;
    humidity -= rhs.humidity;
    wind_speed -= rhs.wind_speed;
    return *this;
  }

  GeoWeatherData operator+(const GeoWeatherData &rhs)
  {
    temp += rhs.temp;
    pressure += rhs.pressure;
    humidity += rhs.humidity;
    wind_speed += rhs.wind_speed;
    return *this;
  }

  GeoWeatherData operator/(const GeoWeatherData &rhs)
  {
    temp /= rhs.temp;
    pressure /= rhs.pressure;
    humidity /= rhs.humidity;
    wind_speed /= rhs.wind_speed;
    return *this;
  }

  GeoWeatherData operator/(const double &rhs)
  {
    temp /= rhs;
    pressure /= rhs;
    humidity /= rhs;
    wind_speed /= rhs;
    return *this;
  }

  GeoWeatherData operator/(const ssize_t &rhs)
  {
    temp = static_cast<double>(temp / rhs);
    pressure = static_cast<double>(pressure / rhs);
    humidity = static_cast<double>(humidity / rhs);
    wind_speed = static_cast<double>(wind_speed / rhs);
    return *this;
  }

  auto operator<=>(const GeoWeatherData &) const = default;

  constexpr operator bool() const
  {
    return !((temp == -MAXFLOAT || pressure == -MAXFLOAT || humidity == -MAXFLOAT || wind_speed == -MAXFLOAT) ||
             (std::isnan(temp) || std::isnan(pressure) || std::isnan(humidity) || std::isnan(wind_speed)) ||
             (std::isinf(temp) || std::isinf(pressure) || std::isinf(humidity) || std::isinf(wind_speed)));
  }
  GeoWeatherData() : temp(-MAXFLOAT), pressure(-MAXFLOAT), humidity(-MAXFLOAT), wind_speed(-MAXFLOAT) {}
  GeoWeatherData(const double &t, const double &p, const double &h, const double &w)
      : temp(t), pressure(p), humidity(h), wind_speed(w) {}
};

struct GeoSolarRadiationData
{
  double DNI; // Direct Normal Irradiance (W/m²)
  double DHI; // Diffuse Horizontal Irradiance (W/m²)
  double GHI; // Global Horizontal Irradiance (W/m²)
  double DHH; // Direct Horizontal Irradiance (W/m²)

  GeoSolarRadiationData operator-(const GeoSolarRadiationData &rhs)
  {
    DNI -= rhs.DNI;
    DHI -= rhs.DHI;
    GHI -= rhs.GHI;
    DHH -= rhs.DHH;
    return *this;
  }

  GeoSolarRadiationData operator+(const GeoSolarRadiationData &rhs)
  {
    DNI += rhs.DNI;
    DHI += rhs.DHI;
    GHI += rhs.GHI;
    DHH += rhs.DHH;
    return *this;
  }

  GeoSolarRadiationData operator/(const GeoSolarRadiationData &rhs)
  {
    DNI /= rhs.DNI;
    DHI /= rhs.DHI;
    GHI /= rhs.GHI;
    DHH /= rhs.DHH;
    return *this;
  }

  GeoSolarRadiationData operator/(const double &rhs)
  {
    DNI /= rhs;
    DHI /= rhs;
    GHI /= rhs;
    DHH /= rhs;
    return *this;
  }
  auto operator<=>(const GeoSolarRadiationData &) const = default;

  constexpr operator bool() const
  {
    return !((DNI == -MAXFLOAT || DHI == -MAXFLOAT || GHI == -MAXFLOAT || DHH == -MAXFLOAT) ||
             (std::isnan(DNI) || std::isnan(DHI) || std::isnan(GHI) || std::isnan(DHH)) ||
             (std::isinf(DNI) || std::isinf(DHI) || std::isinf(GHI) || std::isinf(DHH)));
  }

  GeoSolarRadiationData() : DNI(-MAXFLOAT), DHI(-MAXFLOAT), GHI(-MAXFLOAT), DHH(-MAXFLOAT) {}
  GeoSolarRadiationData(const double &dni, const double &dhi, const double &ghi, const double &dhh)
      : DNI(dni), DHI(dhi), GHI(ghi), DHH(dhh) {}
};

struct Time
{
  int hour;
  int minute;
  int second;
  int timezone;
  auto operator<=>(const Time &) const = default;
  constexpr operator bool() const { return !(hour <= -1 && minute <= -1 && second <= -1); }
  const char *c_str() const
  {
    static thread_local std::string str;
    std::stringstream cstream;
    cstream << std::setw(2) << std::setfill('0') << hour << ":"
            << std::setw(2) << std::setfill('0') << minute << ":"
            << std::setw(2) << std::setfill('0') << second;
    str.clear();
    str = cstream.str();
    return str.c_str();
  }
  Time() : hour(-1), minute(-1), second(-1), timezone(-1) {}
  Time(const int &h, const int &m, const int &s, const double t)
      : hour(h), minute(m), second(s), timezone(t) {}
};

struct Date
{
  int year;
  int month;
  int day;
  auto operator<=>(const Date &) const = default;
  constexpr operator bool() const { return !(year <= -1 && month <= -1 && day <= -1); }
  const char *c_str() const
  {
    static thread_local std::string str;
    std::stringstream cstream;
    cstream << year << "-" << std::setw(2) << std::setfill('0') << month
            << "-" << std::setw(2) << std::setfill('0') << day;
    str.clear();
    str = cstream.str();
    return str.c_str();
  }
  Date() : year(-1), month(-1), day(-1) {}
  Date(const int &y, const int &m, const int &d) : year(y), month(m), day(d) {}
};

struct DateTime
{
  Date date;
  Time time;
  auto operator<=>(const DateTime &) const = default;
  constexpr operator bool() const { return (date && time); }
  bool isLeapYear() const { return (date.year % 4 == 0) || (date.year % 100 != 0) || (date.year % 400 == 0); }
  int GetDaysInYear() const { return isLeapYear() ? 366 : 365; }
  int GetDayOfYear() const
  {
    static constexpr std::array<int, 13> daysInMonth = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int dayOfYear = date.day;

    for (int i = 1; i < date.month; ++i)
      dayOfYear += daysInMonth[i];

    if (date.month > 2 && isLeapYear())
      dayOfYear += 1;

    return dayOfYear;
  }

  const char *c_str() const
  {
    static thread_local std::string str = "";
    str.clear();
    str += date.c_str();
    str += " ";
    str += time.c_str();
    return str.c_str();
  }

  /* Constructors */
  DateTime() : date(), time() {}
  DateTime(const std::tm &tm_s)
      : date((1900 + tm_s.tm_year), tm_s.tm_mon, tm_s.tm_mday),
        time(tm_s.tm_hour, tm_s.tm_min, tm_s.tm_sec, static_cast<int>(tm_s.tm_gmtoff / 3600)) {}
  DateTime(const std::string &datetimestr) // yyyy-mn-ddThh:mm:ssTZ-06
  {
    int n = sscanf(datetimestr.c_str(), "%d-%d-%dT%d:%d:%dTZ%d", &date.year,
                   &date.month, &date.day, &time.hour, &time.minute,
                   &time.second, &time.timezone);

    if (n != 7)
      throw std::out_of_range(std::string("DateTime str malformed: " + std::to_string(n)));
  }
};

struct GeoDateTimeData
{
  DateTime dt;
  constexpr operator bool() const { return dt; }
  GeoDateTimeData() {}
  GeoDateTimeData(const std::tm &tm_s) : dt(tm_s) {}
  GeoDateTimeData(const std::string &datetimestr) : dt(datetimestr) {}
  double GetDelta_UT1() const
  {
    /* Calculate the Julian Date */
    auto JD = [&]() -> double
    {
      double A = dt.date.year / 100.0;
      double B = A / 4.0;
      double C = 2 - A + B;
      double E = 365.25 * (dt.date.year + 4716);
      double F = 30.6001 * (dt.date.month + 1);
      return C + dt.date.day + E + F - 1524.5;
    };
    /* Calculate the Modified Julian Date */
    double MJD = JD() - 2400000.5;
    /* Calculate the Besselian Year */
    double T = 1900.0 + (JD() - 2415020.31352) / 365.242198781;
    /* Calculate the UT2 - UT1 */
    double diff_UT2_UT1 = (0.022 * sin(2 * PI * T)) -
                          (0.012 * cos(2 * PI * T)) -
                          (0.006 * sin(4 * PI * T)) +
                          (0.007 * cos(4 * PI * T));

    /* return UT1_UTC */
    return (0.0590 + (0.00011 * (MJD - 60874)) - diff_UT2_UT1);
  }
  double GetDelta_T() const
  {
    // (TAI - UTC) = 37.0
    return (32.184 + 37.0 - GetDelta_UT1());
  }
  double GetDecimalYear() const
  {
    return static_cast<double>(dt.date.year + (dt.GetDayOfYear() + 1) / 365.0);
  }
};

template <typename T>
class ISensor
{
public:
  virtual ~ISensor() = default;
  virtual int Initialize() = 0;
  virtual T GetServiceData() const = 0;
};