#ifndef _NODE_H_
#define _NODE_H_

// #include <glm>

struct Node{

public:
  Node(){}
  Node(const double &x, const double &y, const double &z){
    
  }  
  const double & operator[](const int &i)const{return m_x[i];}

protected:
  double  m_x[3];
  int     m_id;
};

#endif