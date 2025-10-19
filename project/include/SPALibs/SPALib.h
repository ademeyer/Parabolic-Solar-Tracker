#pragma once

#include "IDateTime.h"
#include "IGPSSensor.h"
#include "IWeather.h"

/*************************** USER INPUT DATA ***************************************/
struct SPA_Input
{
  DateTimeData dateTime;
  GeoLocationData pos;
  GeoWeatherData weather;
  SPA_Input(const DateTimeData &d, const GeoLocationData &p, const GeoWeatherData &w)
      : dateTime(d), pos(p), weather(w) {}
};

/*************************** END USER INPUT DATA ***********************************/

/*************************** USER OUTPUT DATA ***************************************/
struct SPA_Output
{
  int errCode;
  double elevation; // Elevation [in degrees]
  double azimuth;   // Azimuth [in degrees]
  double incidence; // Incidence [in degrees]
  Time sunrise;     // Sunrise Time [in Time]
  Time sunset;      // Sunset Time [in Time]
  SPA_Output()
      : errCode(0), elevation(0.0), azimuth(0.0), incidence(0.0),
        sunrise(Time(0, 0, 0, 0)), sunset(Time(0, 0, 0, 0)) {}
};

/*************************** END USER OUTPUT DATA ***********************************/

#ifdef __cplusplus
extern "C"
{
#endif
#include "spa.h"

  SPA_Output getSunPosition(const SPA_Input *input);
#ifdef __cplusplus
} // extern "C"
#endif
