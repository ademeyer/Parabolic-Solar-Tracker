#ifndef __APIMANAGER_H__
#define __APIMANAGER_H__
#include "GeoLocationServiceSIM.hpp"
#include "GeoDateTimeServiceSIM.hpp"
#include "GeoWeatherServiceSIM.hpp"
#include "GeoSolarRadiationServiceSIM.hpp"

enum class APIERROR
{
  NOERROR,
  GEOLOCATIONINITERROR,
  GEOLOCATIONDATAERROR,
  GEOWEATHERINITERROR,
  GEOWEATHERDATAERROR,
  GEODATETIMEINITERROR,
  GEODATETIMEDATAERROR,
  GEOSOLARINITERROR,
  GEOSOLARDATAERROR,
};

struct APIGeoData
{
  std::string loc_name;
  GeoLocationData location;
  GeoDateTimeData datetime;
  GeoWeatherData weather;
  GeoSolarRadiationData solar;
  APIGeoData() {}
};

class APIManager
{
public:
  APIERROR InitializeServices();
  std::vector<APIGeoData> GetServicesData() const;

private:
  std::vector<APIGeoData> m_ServiceData;
};

#endif