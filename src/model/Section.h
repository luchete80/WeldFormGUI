#ifndef _SECTION_H_
#define _SECTION_H_

#include <string>

class Section {
public:
  int getId() const { return m_id; }
  void setId(int id) { m_id = id; }

  const std::string& getName() const { return m_name; }
  void setName(const std::string& name) { m_name = name; }

  int getMaterialIndex() const { return m_material_index; }
  void setMaterialIndex(int material_index) { m_material_index = material_index; }

  const std::string& getIntendedElementType() const { return m_intended_element_type; }
  void setIntendedElementType(const std::string& intended_element_type) {
    m_intended_element_type = intended_element_type;
  }

  double getThickness() const { return m_thickness; }
  void setThickness(double thickness) { m_thickness = thickness; }

private:
  int m_id = -1;
  std::string m_name;
  int m_material_index = -1;
  std::string m_intended_element_type = "Auto";
  double m_thickness = 1.0;
};

#endif
