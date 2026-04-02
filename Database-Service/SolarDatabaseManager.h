#ifndef __SOLARDATABASEMANAGER_H__
#define __SOLARDATABASEMANAGER_H__
#include <vector>
#include <unordered_map>
#include <sqlite3.h>
#include "database-common.hpp"

using callback = int (*)(void *, int, char **, char **);

class SolarDatabaseManager
{
public:
  SolarDatabaseManager(const std::string &path = "solarDB.db");
  ~SolarDatabaseManager();

  template <typename T>
  bool Insert(const T &db_data) const;
  int GetLocationIdWithName(const std::string &location_name);
  std::unordered_map<std::string, dBCommon::DBLoggedData> GetDBLoggedData(const std::string &start_datestr,
                                                                          const std::string &end_datestr = "",
                                                                          const std::string &loc_name = "%");

private:
  std::string m_DB_path;
  sqlite3 *m_DB = nullptr;

  template <typename T>
  bool bind(const T &db_data, sqlite3_stmt *stmt) const;
  bool exec(const std::string &cmd, callback cb = nullptr, void *data = nullptr) const;
  bool open();
  void close();
};

#include "SolarDatabaseManager.cpp"

#endif //__SOLARDATABASEMANAGER_H__