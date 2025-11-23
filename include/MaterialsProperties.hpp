#ifndef __MATERIALSPROPERTIES_H__
#define __MATERIALSPROPERTIES_H__
#include "ConfigParser.hpp"

struct Material
{
  std::string name;
  double melting_point;        /* Kelvin */
  double max_operating_temp;   /* Kelvin */
  double reflectivity;         /* ρ */
  double absorptivity;         /* α */
  double emissivity;           /* ε */
  double transmittivity;       /* τ */
  double thermal_conductivity; /*W/m.K*/
  double thickness;            /* meter */
  void DumpInfo() const
  {
    std::cout << "=============== Material Name: " << name << " ===============" << std::endl;
    std::cout << "Melting Point: " << melting_point << " [°C]" << std::endl;
    std::cout << "Max Operating Temp: " << max_operating_temp << " [°C]" << std::endl;
    std::cout << "Reflectivity: " << reflectivity << std::endl;
    std::cout << "Absorptivity: " << absorptivity << std::endl;
    std::cout << "Emissivity: " << emissivity << std::endl;
    std::cout << "Transmitivity: " << transmittivity << std::endl;
    std::cout << "Thermal Conductivity: " << thermal_conductivity << " [W/m.K]" << std::endl;
    std::cout << "Thickness: " << thickness << " [m]" << std::endl;
  }
  bool IsMaterialSuitable(const double &temp) const { return temp < melting_point; }
  Material()
      : name(""), melting_point(0.0), max_operating_temp(0.0), reflectivity(0.0), absorptivity(0.0),
        emissivity(0.0), transmittivity(0.0), thermal_conductivity(0.0), thickness(0.0) {}
  Material(const std::string &n, const double &mp, const double &mop, const double &refl,
           const double &absor, const double &emmi, const double &trans, const double &thc, const double &th)
      : name(n), melting_point(mp), max_operating_temp(mop), reflectivity(refl), absorptivity(absor),
        emissivity(emmi), transmittivity(trans), thermal_conductivity(thc), thickness(th) {}
};

class MaterialsParser : public ConfigParser<std::vector<Material>>
{
public:
  MaterialsParser(const std::string &filename = "material.conf")
      : ConfigParser<std::vector<Material>>(filename) {}
  const std::vector<Material> GetParsedData() const override
  {
    std::vector<Material> result;
    const std::regex pattern(R"([\d\.]+)");
    for (const auto &mp : m_ParsedData)
    {
      std::vector<double> val;
      const auto &props = mp.second;
      auto begin = std::sregex_iterator(props.begin(), props.end(), pattern);
      auto end = std::sregex_iterator();
      for (std::sregex_iterator it = begin; it != end; ++it)
        val.push_back(std::stod(it->str()));
      if (val.size() != 8)
        continue;
      result.push_back(Material(mp.first, val[0],
                                val[1], val[2], val[3], val[4], val[5], val[6], val[7]));
    }

    return result;
  }
};

class MaterialProperties
{
public:
  bool FetchMaterial(const std::string &name, Material &material)
  {
    if (m_MaterialCatalog.empty())
    {
      MaterialsParser m_MaterialConfig;
      if (!m_MaterialConfig.parseConfig())
        return false;
      m_MaterialCatalog = m_MaterialConfig.GetParsedData();
      if (m_MaterialCatalog.empty())
        return false;
    }

    for (const auto &mat : m_MaterialCatalog)
    {
      if (mat.name == name)
      {
        material = mat;
        return true;
      }
    }
    return false;
  }

private:
  std::vector<Material> m_MaterialCatalog;
};

#endif //__MATERIALSPROPERTIES_H__