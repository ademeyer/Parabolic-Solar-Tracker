#pragma once
#include <unordered_map>
#include <string>
#include <fstream>
#include <filesystem>

template <typename T>
class ConfigParser
{
protected:
  std::unordered_map<std::string, std::string> m_ParsedData;
  std::string configFile;

public:
  ConfigParser(const std::string &filename) : configFile(filename) {}

  bool parseConfig()
  {
    if (configFile.empty())
    {
      std::cerr << "Error: Config file name is empty" << std::endl;
      return false;
    }

    auto resolveCandidate = [&](const std::filesystem::path &path) -> std::filesystem::path
    {
      if (std::filesystem::exists(path))
        return path;

      auto candidate = std::filesystem::current_path() / path;
      if (std::filesystem::exists(candidate))
        return candidate;

      if (std::filesystem::exists("/proc/self/exe"))
      {
        auto exePath = std::filesystem::read_symlink("/proc/self/exe");
        candidate = exePath.parent_path() / path;
        if (std::filesystem::exists(candidate))
          return candidate;
      }

      return {};
    };

    auto searchConfigDirs = [&](const std::filesystem::path &base) -> std::filesystem::path
    {
      auto path = base;
      while (true)
      {
        const auto candidate = path / "Configs" / configFile;
        if (std::filesystem::exists(candidate))
          return candidate;

        if (!path.has_parent_path())
          break;
        path = path.parent_path();
      }
      return {};
    };

    std::filesystem::path configPath = resolveCandidate(configFile);
    if (configPath.empty())
    {
      configPath = searchConfigDirs(std::filesystem::current_path());
    }

    if (configPath.empty() && std::filesystem::exists("/proc/self/exe"))
    {
      auto exePath = std::filesystem::read_symlink("/proc/self/exe");
      configPath = searchConfigDirs(exePath.parent_path());
    }

    if (configPath.empty())
    {
      std::cerr << "Error: Cannot find file " << configFile << std::endl;
      return false;
    }

    std::ifstream file(configPath);
    if (!file.is_open())
    {
      std::cerr << "Error: Cannot open config file " << configPath.string() << std::endl;
      return false;
    }

    m_ParsedData.clear();
    std::string line;
    int lineNum = 0;

    while (std::getline(file, line))
    {
      ++lineNum;

      // Skip empty lines and comment
      if (line.empty() || line[0] == '#' || line.find('=') == std::string::npos)
        continue;

      // Parse key-value pair
      size_t equalPos = line.find('=');
      std::string key = line.substr(0, equalPos);
      std::string value = line.substr(equalPos + 1);

      // Trim whitespaces
      key.erase(0, key.find_first_not_of(" \t"));
      key.erase(key.find_last_not_of(" \t") + 1);
      value.erase(0, value.find_first_not_of(" \t"));
      value.erase(value.find_last_not_of(" \t") + 1);

      if (!key.empty() && !value.empty())
      {
        m_ParsedData[key] = value;
      }
    }
    file.close();
    std::cout << "Parsed " << m_ParsedData.size() << " detail(s) from config file.\n";

    return true;
  }

  virtual const T GetParsedData() const = 0;
};

class AddressParser : public ConfigParser<std::unordered_map<std::string, std::string>>
{
public:
  AddressParser(const std::string &filename) : ConfigParser<std::unordered_map<std::string, std::string>>(filename) {}
  const std::unordered_map<std::string, std::string> GetParsedData() const override
  {
    return m_ParsedData;
  }
};

/***************************************************************************************************************************************/
