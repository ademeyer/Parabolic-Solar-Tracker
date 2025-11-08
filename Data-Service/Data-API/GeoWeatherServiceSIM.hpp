#ifndef __GeoWeatherServiceSIM_H__
#define __GeoWeatherServiceSIM_H__
#include "ISensor.h"
#include "HTTPHelper.hpp"

class GeoWeatherServiceSIM : public ISensor<GeoWeatherData>
{
public:
  GeoWeatherServiceSIM(const std::string &lat, const std::string &lon) : m_Lat(lat), m_Lon(lon) {}

  int Initialize() override
  {
    std::string weacurl = "https://api.open-meteo.com/v1/forecast?latitude=" + m_Lat + "&longitude=" + m_Lon +
                          "&current=temperature_2m,relative_humidity_2m,surface_pressure,wind_speed_10m&timezone=auto";

    if (!m_http.getAPI(weacurl))
      return 1;

    // Parse Data
    std::pair<std::string, std::vector<std::string>> keyPair = {"current", {"temperature_2m", "surface_pressure", "relative_humidity_2m", "wind_speed_10m"}};
    auto weadata = m_http.parseJsonResponse<float>(keyPair);
    if (weadata.size() < 4)
      return 1;

    m_WeatherData = std::make_unique<GeoWeatherData>(weadata[0].second,
                                                     weadata[1].second,
                                                     weadata[2].second,
                                                     weadata[3].second);
    /* Format we need: yyyy-mn-ddThh:mm:ssTZ-06
      Data we got: 2025-10-26T07:45 */

    /* Grab Date & Time for the location */
    keyPair = {"current", {"time"}};
    auto loc_time = m_http.parseJsonResponse<std::string>(keyPair);
    if (loc_time.empty())
      return 0;

    /* Fix missing second by adding : 00TZ */
    auto &tr = loc_time[0].second;
    tr.insert(tr.size(), std::string(":00TZ"));

    /* Grab Timezone for the location */
    keyPair = {"", {"timezone_abbreviation"}};
    auto zone_time = m_http.parseJsonResponse<std::string>(keyPair);
    if (zone_time.empty())
      return 0;

    auto zntime = zone_time[0].second.substr(3);
    datetime_str = loc_time[0].second + zntime;

    return 0;
  }

  GeoWeatherData GetServiceData() const override
  {
    return m_WeatherData != nullptr ? *m_WeatherData : GeoWeatherData();
  }

  const std::string GetDateTimeStr() const { return datetime_str; }

private:
  HTTPClient m_http;
  std::string m_Lat;
  std::string m_Lon;
  std::string datetime_str;
  std::unique_ptr<GeoWeatherData> m_WeatherData = nullptr;
};

#endif // __GeoWeatherServiceSIM_H__