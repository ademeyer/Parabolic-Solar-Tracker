#pragma once
#pragma warning(disable : 4996)
#include <algorithm>
#include <iostream>
#include "SolarDatabaseManager.h"

SolarDatabaseManager::SolarDatabaseManager(const std::string &path) : m_DB_path(path)
{
  if (!open())
    throw std::runtime_error("Unable to open " + path + "\n");
}

SolarDatabaseManager::~SolarDatabaseManager() { close(); }

template <typename T>
bool SolarDatabaseManager::Insert(const T &db_data) const
{
  if (!exec(db_data.create_table_sql))
    return false;

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_DB, db_data.insert_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
  {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(m_DB) << std::endl;
    return false;
  }

  bool success = bind(db_data, stmt);
  if (!success)
  {
    std::cerr << "Failed to bind: " << sqlite3_errmsg(m_DB) << std::endl;
  }

  if (!(success &= (sqlite3_step(stmt) == SQLITE_DONE)))
    std::cerr << "Failed to step: " << sqlite3_errmsg(m_DB) << std::endl;
  if (!(success &= (sqlite3_finalize(stmt) == SQLITE_OK)))
    std::cerr << "Failed to finalize: " << sqlite3_errmsg(m_DB) << std::endl;

  return success;
}

int SolarDatabaseManager::GetLocationIdWithName(const std::string &location_name)
{
  const std::string sql = "SELECT location_id FROM locations WHERE name = ?;";
  sqlite3_stmt *stmt;
  int loc_id = -1;
  if (sqlite3_prepare_v2(m_DB, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
  {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(m_DB) << std::endl;
    return loc_id;
  }

  if (sqlite3_bind_text(stmt, 1, location_name.c_str(), -1, SQLITE_STATIC) != SQLITE_OK)
  {
    std::cerr << "Failed to bind text: " << sqlite3_errmsg(m_DB) << std::endl;
    sqlite3_finalize(stmt);
    return loc_id;
  }

  if (sqlite3_step(stmt) == SQLITE_ROW)
    loc_id = sqlite3_column_int(stmt, 0);
  else if (sqlite3_step(stmt) == SQLITE_DONE)
    std::cout << "No location id Found!" << std::endl;

  sqlite3_finalize(stmt);

  return loc_id;
}

std::unordered_map<std::string, dBCommon::DBLoggedData> SolarDatabaseManager::GetDailyDBLoggedData(const std::string &datestr)
{
  auto &sql = dBCommon::RetrieveDailyLoggedData;
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_DB, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
  {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(m_DB) << std::endl;
    return {};
  }

  // Bind parameters
  bool success = true;
  for (int i = 1; (i <= 2 && success); ++i)
    success = sqlite3_bind_text(stmt, i, datestr.c_str(), -1, SQLITE_STATIC) == SQLITE_OK;

  if (!success)
  {
    std::cerr << "Failed to bind text: " << sqlite3_errmsg(m_DB) << std::endl;
    sqlite3_finalize(stmt);
    return {};
  }

  using namespace dBCommon;
  std::unordered_map<std::string, DBLoggedData> results;

  while (sqlite3_step(stmt) == SQLITE_ROW)
  {
    auto loc_name = reinterpret_cast<const char *>(std::launder(sqlite3_column_text(stmt, 0)));
    auto loc_lat = sqlite3_column_double(stmt, 1);
    auto loc_lon = sqlite3_column_double(stmt, 2);
    auto loc_alt = sqlite3_column_double(stmt, 3);
    auto wea_temp = sqlite3_column_double(stmt, 4);
    auto wea_pres = sqlite3_column_double(stmt, 5);
    auto wea_humi = sqlite3_column_double(stmt, 6);
    auto wea_wind = sqlite3_column_double(stmt, 7);
    auto s_dni = sqlite3_column_double(stmt, 8);
    auto s_dhi = sqlite3_column_double(stmt, 9);
    auto s_ghi = sqlite3_column_double(stmt, 10);
    auto s_dhh = sqlite3_column_double(stmt, 11);
    auto date_cstr = reinterpret_cast<const char *>(std::launder(sqlite3_column_text(stmt, 12)));
    auto dt_str = std::string(date_cstr);

    std::replace(dt_str.begin(), dt_str.end(), ' ', 'T');
    dt_str += "TZ0";

    auto tsd = TimeSeriesData(DateTime(std::string(dt_str)),
                              GeoSolarRadiationData(s_dni, s_dhi, s_ghi, s_dhh),
                              GeoWeatherData(wea_temp, wea_pres, wea_humi, wea_wind));

    if (results.find(loc_name) == results.end())
    {
      auto dblogg = DBLoggedData(GeoLocationData(loc_lat, loc_lon, loc_alt, 0.0), tsd);
      results.emplace(std::string(loc_name), dblogg);
    }
    else
    {
      auto &dbloggeddata = results[std::string(loc_name)];
      dbloggeddata.addSeriesData(tsd);
    }
  }

  sqlite3_finalize(stmt);
  return results;
}

// PRIVATES
template <typename T>
bool SolarDatabaseManager::bind(const T &db_data, sqlite3_stmt *stmt) const
{
  bool ret = true;
  // Parse each data variants and bind to SQL types
  auto parsed_data = db_data.ParseDataStructure();
  size_t pos = 1;
  for (const auto &item : parsed_data)
  {
    std::visit([&](auto &&arg)
               { 
                  using type = std::decay_t<decltype(arg)>;
                  if constexpr (std::is_same_v<type, std::string> || std::is_same_v<type, DateTime> || std::is_same_v<type, Time>)
                    ret &= (sqlite3_bind_text(stmt, pos, arg.c_str(), -1, SQLITE_STATIC) == SQLITE_OK);
                  else if constexpr (std::is_same_v<type, double>)
                    ret &= (sqlite3_bind_double(stmt, pos, arg) == SQLITE_OK);
                  else if constexpr (std::is_same_v<type, int>)
                    ret &= (sqlite3_bind_int(stmt, pos, arg) == SQLITE_OK);
                  else
                    static_assert(sizeof(type) == 0, "Unknown DB Data type"); }, item);
    if (!ret)
      break;

    ++pos;
  }

  return ret;
}

bool SolarDatabaseManager::exec(const std::string &cmd, callback cb, void *data) const
{
  char *errMsg = nullptr;
  int ret = sqlite3_exec(m_DB, cmd.c_str(), cb, data, &errMsg);
  if (ret != SQLITE_OK)
  {
    std::cerr << "SQL Error: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    return false;
  }

  return true;
}

bool SolarDatabaseManager::open()
{
  int ret = sqlite3_open(m_DB_path.c_str(), &m_DB);
  if (ret)
  {
    std::cerr << "Can't open database: " << sqlite3_errmsg(m_DB) << std::endl;
    return false;
  }

  bool result = true;
  // Enable foreign keys
  result &= exec("PRAGMA foreign_keys = ON;");
  result &= exec("PRAGMA journal_mode = WAL;");

  return result;
}

void SolarDatabaseManager::close()
{
  if (m_DB)
  {
    sqlite3_close(m_DB);
    m_DB = nullptr;
  }
}