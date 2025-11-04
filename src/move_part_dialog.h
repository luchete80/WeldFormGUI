#ifndef _MOVE_PART_DIALOG_H_
#define _MOVE_PART_DIALOG_H_


struct MoveCommand {
    int axis;      // 0=X, 1=Y, 2=Z
    double delta;  // movimiento
    bool active;   // true si hay acción
};

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
struct MovePartDialog{
  
  //void    AddLog(const char* fmt, ...);
  int m_id; //TODO; CHANGE TO VECTOR (TEMP FUNCTION)
  double m_elastic_const;
  double m_poisson_const;

  
  bool m_initialized = false;
  
  bool cancel_action;
  bool create_part;
  
  //const bool & isModelCreated()const{return create_part;}
  MoveCommand   Draw(double &step, double* pos, bool *open );  
};


#endif
