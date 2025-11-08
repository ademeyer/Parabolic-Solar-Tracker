#ifndef __GEOSSOLARRADIATIONSERVICESIM_H__
#define __GEOSSOLARRADIATIONSERVICESIM_H__
#include "ISensor.h"
#include "HTTPHelper.hpp"

class GeoSolarRadiationServiceSIM : public ISensor<GeoSolarRadiationData>
{
public:
  GeoSolarRadiationServiceSIM(const std::string &lat, const std::string &lon) : m_Lat(lat), m_Lon(lon) {}

  int Initialize() override
  {
    std::string solarUrl = "https://api.open-meteo.com/v1/forecast?latitude=" + m_Lat + "&longitude=" + m_Lon +
                           "&current=direct_normal_irradiance,diffuse_radiation,shortwave_radiation,direct_radiation&timezone=auto";

    if (!m_http.getAPI(solarUrl))
      return 1;

    // Parse Data
    std::pair<std::string, std::vector<std::string>> keyPair = {"current", {"direct_normal_irradiance", "diffuse_radiation", "shortwave_radiation", "direct_radiation"}};
    auto solarData = m_http.parseJsonResponse<float>(keyPair);

    if (solarData.size() < 4)
      return 1;

    m_SolarData = std::make_unique<GeoSolarRadiationData>(
        solarData[0].second,
        solarData[1].second,
        solarData[2].second,
        solarData[3].second);
    return 0;
  }

  GeoSolarRadiationData GetServiceData() const override
  {
    return m_SolarData != nullptr ? *m_SolarData : GeoSolarRadiationData();
  };

private:
  HTTPClient m_http;
  std::string m_Lat;
  std::string m_Lon;
  std::unique_ptr<GeoSolarRadiationData> m_SolarData = nullptr;
};
#endif //__GEOSSOLARRADIATIONSERVICESIM_H__