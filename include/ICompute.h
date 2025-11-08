#pragma once

/*************************** SPA USER INPUT DATA ***************************************/
struct SPA_Input
{
  GeoDateTimeData dateTime;
  GeoLocationData pos;
  GeoWeatherData weather;
  SPA_Input(const GeoDateTimeData &d, const GeoLocationData &p, const GeoWeatherData &w)
      : dateTime(d), pos(p), weather(w) {}
};

/*************************** END SPA USER INPUT DATA ***********************************/

/*************************** SPA USER OUTPUT DATA ***************************************/
struct SunPositionData
{
  int errCode;
  double elevation; // Elevation [in degrees]
  double azimuth;   // Azimuth [in degrees]
  double incidence; // Incidence [in degrees]
  Time sunrise;     // Sunrise Time [in Time]
  Time sunset;      // Sunset Time [in Time]
  bool IsValid() const { return errCode == 0; }
  SunPositionData()
      : errCode(-1), elevation(0.0), azimuth(0.0), incidence(0.0),
        sunrise(Time(0, 0, 0, 0)), sunset(Time(0, 0, 0, 0)) {}
};

/*************************** END SPA USER OUTPUT DATA ***********************************/

/*************************** MAG USER INPUT DATA ***************************************/
struct InData
{
  double decimalYear;
  struct GeoLocationData pos;
};
/*************************** END MAG USER INPUT DATA ***********************************/

/*************************** MAG USER OUTPUT DATA **************************************/

struct MagComponents
{
  /**
   * @brief: Usage not yet defined for application
   *
  double F; // Total Intensity of the geomagnetic field
  double H; // Horizontal Intensity of the geomagnetic field
  double I; // Geomagnetic Inclination*/
  double X; // North Component of the geomagnetic field
  double Y; // East Component of the geomagnetic field
  double Z; // Vertical Component of the geomagnetic field
  double D; // Geomagnetic Declination (Magnetic Variation)
};

struct GeoMagneticData
{
  int errCode;
  double sv; /* Secular variable / Annual Changes (in nT) */
  MagComponents magData;
  MagComponents magDataErr;
  bool IsValid() const { return errCode == 0; }
  constexpr double GetLocalMagneticNorth() const { return magData.X == 0 ? 0.0 : atan(magData.Y / magData.X); }
  constexpr double GetGeoMagneticNorth() const { return GetLocalMagneticNorth() - magData.D; }
};

/*************************** END MAG USER OUTPUT DATA ***********************************/
