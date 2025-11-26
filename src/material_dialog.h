#ifndef _MATERIAL_DIALOG_H_
#define _MATERIAL_DIALOG_H_


#include "Material.h"
#include "Dialog.h"
#include "Material_Db.h"

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
struct MaterialDialog:
public Dialog{
  
  bool m_initiated = false; /// FOR FIRST CREATED
  
  //void    AddLog(const char* fmt, ...);
  double m_density_const; //TODO; CHANGE TO VECTOR (TEMP FUNCTION)
  double m_elastic_const;
  double m_poisson_const;
  
  bool cancel_action;
  bool create_material;
  int m_selected_model; //PLASTIC
  
  Plastic_ *m_pl = nullptr;
  void InitFromMaterial(Material_* mat);
  
  //// TO EDIT INTENALLY WHEN OPEN
  double bilinear_sy0;
  double bilinear_Et;

  double hollomon_K;
  double hollomon_n;
  bool m_thermal_coupling_flag;
  double m_k_T, m_cp_T;

  double gmt_n1, gmt_n2, gmt_m1, gmt_m2, gmt_I1, gmt_I2;

  std::vector<double> m_pl_const;
  GMT m_gmt;
  
  Material_ *m_temp_mat = nullptr;  // material temporal
  
  const bool & isMaterialCreated()const{return create_material;}
  void   Draw(const char* title, bool* p_open = NULL, Material_* mat = NULL,Material_Db *mat_db = nullptr);  
};


//Returns true if NEW material is created or if changes are saved, if no
//if no material is created, pointer is null
Material_ ShowCreateMaterialDialog(bool* p_open, MaterialDialog *, bool* ret, Material_Db * = nullptr);
bool ShowEditMaterialDialog(bool* p_open, MaterialDialog *, Material_ *);


#endif
