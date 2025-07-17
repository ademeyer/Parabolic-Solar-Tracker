#pragma once
#include <ctime>
#include <math.h>
#define PI 3.14159265
/*
  How to calculate JD:
  A = Y/100
  B = A/4
  C = 2-A+B
  E = 365.25x(Y+4716)
  F = 30.6001x(M+1)
  JD= C+D+E+F-1524.5
  // JD (2025-07-15) = 2460872.3633
  // ON=2460872.425162
  MJD = JD - 2400000.5
  besselian_year = 1900.0 + (JD - 2415020.31352) / 365.242198781

  UT2-UT1 = 0.022 sin(2*pi*T) - 0.012 cos(2*pi*T)
                              - 0.006 sin(4*pi*T) + 0.007 cos(4*pi*T)
    where pi = 3.14159265... and T is the date in Besselian years.

  GENERAL INFORMATION:
  TT = TAI + 32.184 seconds
  DUT1= (UT1-UTC) transmitted with time signals
      = +0.1 seconds beginning 10 July 2025 at 0000 UTC
  Beginning 1 January 2017:
    TAI-UTC = 37.000 000 seconds
*/

/*
  PREDICTIONS:
  The following formulas will not reproduce the predictions given below,
  but may be used to extend the predictions beyond the end of this table.

  x =  0.1332 + 0.0566 cos A + 0.1483 sin A - 0.0151 cos C - 0.0861 sin C
  y =  0.3838 + 0.1403 cos A - 0.0484 sin A - 0.0861 cos C + 0.0151 sin C
      UT1-UTC =  0.0590 + 0.00011 (MJD - 60874) - (UT2-UT1)

  where A = 2*pi*(MJD-60866)/365.25 and C = 2*pi*(MJD-60866)/435.
  */

struct Date
{
  int year;
  int month;
  int date;
  Date(const int &y, const int &mn, const int &d)
      : year(y), month(mn), date(d) {}
};

struct Time
{
  int hour;
  int minute;
  int second;
  double timezone;
  Time(const int &h, const int &m, const int &s, const double t)
      : hour(h), minute(m), second(s), timezone(t) {}
};

struct DateTimeData
{
  Date dt;
  Time tt;
  DateTimeData(const Date &d, const Time &t) : dt(d), tt(t) {}
  DateTimeData(const int &y, const int &mn, const int &d,
               const int &h, const int &m, const int &s, const double t)
      : dt(Date(y, mn, d)), tt(Time(h, m, s, t)) {}

  double GetDelta_UT1() const
  {
    /* Calculate the Julian Date */
    auto JD = [&] -> double
    {
      double A = dt.year / 100.0;
      double B = A / 4.0;
      double C = 2 - A + B;
      double E = 365.25 * (dt.year + 4716);
      double F = 30.6001 * (dt.month + 1);
      return C + dt.date + E + F - 1524.5;
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

    double A = 2 * PI * (MJD - 60866) / 365.25;
    double C = 2 * PI * (MJD - 60866) / 435.0;

    double x = 0.1332 + 0.0566 * cos(A) + 0.1483 * sin(A) -
               0.0151 * cos(C) - 0.0861 * sin(C);

    double y = 0.3838 + (0.1403 * cos(A)) - (0.0484 * sin(A)) -
               (0.0861 * cos(C)) + (0.0151 * sin(C));

    /* return UT1_UTC */
    return (0.0590 + (0.00011 * (MJD - 60874)) - diff_UT2_UT1);
  }

  double GetDelta_T() const
  {
    // (TAI - UTC) = 37.0
    return (32.184 + 37.0 - GetDelta_UT1());
  }
};

class IDateTime
{
public:
  virtual DateTimeData GetDateTimeDate() const
  {
    std::time_t now = std::time(nullptr);
    std::tm *ltm = std::localtime(&now);
    double offset_hours = 0.0;
#ifdef __GLIBC__ // Check if glibc extensions are available
    if (ltm->tm_gmtoff != 0)
    {
      // tm_gmtoff is in seconds, convert to hours
      offset_hours = static_cast<double>(ltm->tm_gmtoff) / 3600.0;
    }
#endif
    return DateTimeData((1900 + ltm->tm_year), ltm->tm_mon, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec, offset_hours);
  }
  virtual double GetDecimalYear() const
  {
    std::time_t now = std::time(nullptr);
    std::tm *ltm = std::localtime(&now);
    return static_cast<double>(1900 + ltm->tm_year + (ltm->tm_yday + 1) / 365.0);
  }
};
