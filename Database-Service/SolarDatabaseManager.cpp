#include "SolarDatabaseManager.h"

SolarDatabaseManager::SolarDatabaseManager(const std::string &path) : m_DB_path(path) { open(); }
SolarDatabaseManager::~SolarDatabaseManager() { close(); }

// static int callback(void *data, int argc, char **argv, char **azColName)
// {
//   int i;
//   fprintf(stderr, "%s: ", (const char *)data);

//   for (i = 0; i < argc; i++)
//   {
//     printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
//   }

//   printf("\n");
//   return 0;
// }

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

// PRIVATES
bool SolarDatabaseManager::exec(const std::string &cmd) const
{
  char *errMsg = nullptr;
  int ret = sqlite3_exec(m_DB, cmd.c_str(), nullptr, nullptr, &errMsg);
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