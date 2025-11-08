#ifndef __SOLARDATABASEMANAGER_H__
#define __SOLARDATABASEMANAGER_H__
#include <bits/stdc++.h>
#include <sqlite3.h>
#include "database-common.hpp"

class SolarDatabaseManager
{
private:
  std::string m_DB_path;
  sqlite3 *m_DB = nullptr;

  template <typename T>
  bool bind(const T &db_data, sqlite3_stmt *stmt) const;
  bool exec(const std::string &cmd) const;
  bool open();
  void close();

public:
  SolarDatabaseManager(const std::string &path = "solarDB.db");
  ~SolarDatabaseManager();

  template <typename T>
  bool Insert(const T &db_data) const;
  int GetLocationIdWithName(const std::string &location_name);
};

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

#endif //__SOLARDATABASEMANAGER_H__