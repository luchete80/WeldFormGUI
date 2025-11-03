// Material_Db.h
#ifndef _MATERIAL_DB_H_
#define _MATERIAL_DB_H_

#include "Material.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Material_Db {
private:
    std::vector<Material_*> materials;
    bool active_db = true;

public:
    Material_Db() {}

    void addMaterial(Material_*m) {
        materials.push_back(m);
    }

    bool isActive() const { return active_db; }
    void setActive(bool val) { active_db = val; }

    const std::vector<Material_*>& getAll() const { return materials; }

    // --- Guardar a JSON ---
    bool saveToJson(const std::string& filename) const {
        json j;
        j["Materials"] = json::array();

        for (auto& m : materials) {
            json jm;
            jm["density"] = m->getDensityConstant();

            if (m->isPlastic() && m->getPlastic()) {
                jm["plastic_model"] = m->getPlastic()->getType();
                jm["plastic_constants"] = m->getPlastic()->getPlasticConstants();
            } else {
                jm["plastic_model"] = 0;
            }
            j["Materials"].push_back(jm);
        }

        std::ofstream f(filename);
        if (!f.is_open()) return false;
        f << j.dump(4);
        f.close();
        return true;
    }

    // --- Cargar desde JSON ---
    bool loadFromJson(const std::string& filename) {
        std::ifstream f(filename);
        if (!f.is_open()) return false;
        json j;
        f >> j;
        f.close();

        if (!j.contains("Materials")) return false;

        materials.clear();
        for (auto& jm : j["Materials"]) {
            Material_* m = new Material_();
            if (jm.contains("density"))
                m->setDensityConstant(jm["density"].get<double>());

            int model = jm.value("plastic_model", 0);
            if (model != 0) {
                Plastic_* p = nullptr;
                std::vector<double> c;
                if (jm.contains("plastic_constants"))
                    c = jm["plastic_constants"].get<std::vector<double>>();

                switch (model) {
                    case BILINEAR:
                        if (c.size() >= 2) p = new Bilinear(c[0], c[1]);
                        break;
                    case HOLLOMON:
                        if (c.size() >= 2) p = new Hollomon(c[0], c[1]);
                        break;
                    case _GMT_:
                        if (c.size() >= 6)
                            p = new GMT(c[0], c[1], 0, 0, c[2], c[3], c[4], c[5]);
                        break;
                    case JOHNSON_COOK:
                        p = new JohnsonCook();
                        break;
                }
                if (p) {
                    m->m_plastic = p;
                    m->m_isplastic = true;
                }
            }
            materials.push_back(m);
          delete m;
        }
        return true;
    }
};

#endif

