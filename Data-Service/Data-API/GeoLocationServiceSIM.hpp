#ifndef __GeoLocationServiceSIM_H__
#define __GeoLocationServiceSIM_H__

#include "HTTPHelper.hpp"
#include "ConfigParser.hpp"
#include "ISensor.h"

class GeoLocationServiceSIM : public ISensor<std::unordered_map<std::string, GeoLocationData>>
{
public:
  GeoLocationServiceSIM(const std::string &filename = "sim.conf") : m_config(filename) {}
  ~GeoLocationServiceSIM() override {}

  int Initialize() override
  {
    m_addresses.clear();
    if (!m_config.parseConfig())
      throw std::runtime_error("Unable to parse config file");

    auto addresses = m_config.GetParsedData();

    /* Fetch GeoLocation Data for each address */
    for (const auto &addr : addresses)
    {
      // std::cout << "Fetching longitude and latitude for " << addr.first << std::endl;
      std::string locurl = "https://nominatim.openstreetmap.org/search?format=json&q=" +
                           m_http.urlEncode(addr.second) + "&limit=1";

      if (!m_http.getAPI(locurl))
      {
        std::cerr << "Unable to Fetch lon and lat for " << addr.first << std::endl;
        continue;
      }
      // Parse the lon and lat
      std::pair<std::string, std::vector<std::string>>
          keyPair = {"", {"lat", "lon"}};
      auto latlon = m_http.parseJsonResponse<std::string>(keyPair);
      if (latlon.size() < 2)
      {
        std::cerr << "Parsing lat and lon return empty for " << addr.first << std::endl;
        continue;
      }

      // std::cout << "Fetching Altitude for " << addr.first << std::endl;
      std::string alturl = "https://api.open-meteo.com/v1/elevation?latitude=" +
                           latlon[0].second + "&longitude=" + latlon[1].second;

      if (!m_http.getAPI(alturl))
      {
        std::cerr << "Unable to Fetch altitude for " << addr.first << std::endl;
        continue;
      }

      // Parse the elevation
      keyPair = {"", {"elevation"}};
      auto altitude = m_http.parseJsonResponse<float>(keyPair);

      if (altitude.empty())
      {
        std::cerr << "Parsing Elevation return empty for " << addr.first << std::endl;
        continue;
      }

      m_addresses[addr.first] = GeoLocationData(std::atof(latlon[0].second.c_str()),
                                                std::atof(latlon[1].second.c_str()),
                                                double(altitude[0].second), 0.0);
    }

    return m_addresses.empty() ? 1 : 0;
  }

  std::unordered_map<std::string, GeoLocationData> GetServiceData() const override
  {
    return m_addresses;
  };

private:
  std::unordered_map<std::string, GeoLocationData> m_addresses; // Name, GeoLocation
  HTTPClient m_http;
  AddressParser m_config;
};

#endif // __GeoLocationServiceSIM_H__