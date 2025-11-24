#include "APIManager.hpp"

APIERROR APIManager::InitializeServices()
{
  // Geo Location service
  auto geo_service = std::make_unique<GeoLocationServiceSIM>();
  if (geo_service->Initialize() != 0)
    return APIERROR::GEOLOCATIONINITERROR;

  auto address_loc = geo_service->GetServiceData();
  for (const auto &[addr, gLoc] : address_loc)
  {
    if (!gLoc)
      return APIERROR::GEOLOCATIONDATAERROR;

    // Geo Weather service
    std::unique_ptr<GeoWeatherServiceSIM> geo_weather =
        std::make_unique<GeoWeatherServiceSIM>(std::to_string(gLoc.Latitude),
                                               std::to_string(gLoc.Longitude));

    if (geo_weather->Initialize() != 0)
      return APIERROR::GEOWEATHERINITERROR;

    auto weather = geo_weather->GetServiceData();
    if (!weather)
      return APIERROR::GEOWEATHERDATAERROR;

    // Geo Date Time service
    std::unique_ptr<GeoDateTimeServiceSIM>
        geo_datetime = std::make_unique<GeoDateTimeServiceSIM>(geo_weather->GetDateTimeStr());

    if (geo_datetime->Initialize() != 0)
      return APIERROR::GEODATETIMEINITERROR;

    auto datetime = geo_datetime->GetServiceData();
    if (!datetime)
      return APIERROR::GEODATETIMEDATAERROR;

    // Get Solar Radiation service
    std::unique_ptr<GeoSolarRadiationServiceSIM> geo_solar =
        std::make_unique<GeoSolarRadiationServiceSIM>(std::to_string(gLoc.Latitude),
                                                      std::to_string(gLoc.Longitude));
    if (geo_solar->Initialize() != 0)
      return APIERROR::GEOSOLARINITERROR;

    auto solar = geo_solar->GetServiceData();
    if (!datetime)
      return APIERROR::GEOSOLARDATAERROR;

    // save data
    APIGeoData result;
    result.loc_name = addr;
    result.location = gLoc;
    result.datetime = datetime;
    result.weather = weather;
    result.solar = solar;
    m_ServiceData.push_back(result);
  }

  return APIERROR::NOERROR;
}

std::vector<APIGeoData> APIManager::GetServicesData() const
{
  return m_ServiceData;
}