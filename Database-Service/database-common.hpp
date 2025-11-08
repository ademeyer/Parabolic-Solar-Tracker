#pragma once
#include <variant>
#include <vector>
#include <string>
#include "ISensor.h"
#include "ICompute.h"

namespace dBCommon
{
  // Tables
  static const std::string LocationTableName = "locations";
  static const std::string WeatherTableName = "weather_data";
  static const std::string SolarRadiationTableName = "solar_radiation";

  static const std::string CreateLocationTableSQL = R"(
CREATE TABLE IF NOT EXISTS )" + LocationTableName +
                                                    R"( (
    location_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    latitude REAL NOT NULL, -- degrees
    longitude REAL NOT NULL, -- degrees
    altitude REAL DEFAULT 0.0, -- meters
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
)";

  static const std::string CreateWeatherTableSQL = R"(
CREATE TABLE IF NOT EXISTS )" + WeatherTableName +
                                                   R"( (
    weather_id INTEGER PRIMARY KEY AUTOINCREMENT,
    location_id INTEGER NOT NULL,
    temperature REAL, -- °C
    pressure REAL, -- hPa
    humidity REAL, -- %
    wind_speed REAL, -- m/s
    logged_at TIMESTAMP NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (location_id) REFERENCES )" + LocationTableName +
                                                   R"((location_id),
    UNIQUE(location_id, logged_at)
);
)";

  static const std::string CreateSolarRadiationTableSQL = R"(
CREATE TABLE IF NOT EXISTS )" + SolarRadiationTableName +
                                                          R"( (
    radiation_id INTEGER PRIMARY KEY AUTOINCREMENT,
    location_id INTEGER NOT NULL,
    dni REAL, -- Direct Normal Irradiance (W/m²)
    dhi REAL, -- Diffuse Horizontal Irradiance (W/m²)
    ghi REAL, -- Global Horizontal Irradiance (W/m²)
    direct_horizontal REAL, -- Direct Horizontal Irradiance (W/m²)
    -- clear_sky_dni REAL,
    -- clear_sky_ghi REAL,
    logged_at TIMESTAMP NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (location_id) REFERENCES )" + LocationTableName +
                                                          R"((location_id),
    UNIQUE(location_id, logged_at)
);
)";

  const std::string InsertLocationTableSQL = "INSERT OR IGNORE INTO " + LocationTableName +
                                             " (name, latitude, longitude, altitude) "
                                             "VALUES (?, ?, ?, ?);";

  const std::string InsertWeatherTableSQL = "INSERT INTO " + WeatherTableName +
                                            " (location_id, temperature, pressure, humidity, "
                                            "wind_speed,  logged_at) "
                                            "VALUES (?, ?, ?, ?, ?, ?);";

  const std::string InsertRadiationTableSQL = "INSERT INTO " + SolarRadiationTableName +
                                              " (location_id, dni, dhi, ghi, direct_horizontal, logged_at) "
                                              "VALUES (?, ?, ?, ?, ?, ?);";

  // DataBase Structured Datatypes
  class DBDataTypes
  {
  public:
    using DBVariantTypes = std::variant<int, double, std::string, DateTime, Time>;
    virtual std::vector<DBVariantTypes> ParseDataStructure() const = 0;
  };

  struct DBLocationData : public DBDataTypes
  {
    std::string name;
    const std::string create_table_sql = CreateLocationTableSQL;
    const std::string insert_sql = InsertLocationTableSQL;
    GeoLocationData locs;
    std::vector<DBVariantTypes> ParseDataStructure() const override
    {
      return {name, locs.Latitude, locs.Longitude, locs.Altitude};
    }
  };

  struct DBWeatherData : public DBDataTypes
  {
    int location_id;
    const std::string create_table_sql = CreateWeatherTableSQL;
    const std::string insert_sql = InsertWeatherTableSQL;
    DateTime logTime;
    GeoWeatherData weatherData;
    std::vector<DBVariantTypes> ParseDataStructure() const override
    {
      return {location_id, weatherData.temp, weatherData.pressure,
              weatherData.humidity, weatherData.wind_speed, logTime};
    }
  };

  struct DBSolarData : public DBDataTypes
  {
    int location_id;
    const std::string create_table_sql = CreateSolarRadiationTableSQL;
    const std::string insert_sql = InsertRadiationTableSQL;
    DateTime logTime;
    GeoSolarRadiationData srd;
    std::vector<DBVariantTypes> ParseDataStructure() const override
    {
      return {location_id, srd.DNI, srd.DHI, srd.GHI, srd.DHH, logTime};
    }
  };
}