#ifndef _NODE_H_
#define _NODE_H_

// #include <glm>

struct Node{

public:
  Node(const double &x, const double &y, const double &z){
    
  }  
  operator[](const int &i){return m_x[i];}

protected:
  double  m_x[3];
  int     m_id;
};

#endif