#ifndef _NODE_H_
#define _NODE_H_

// #include <glm>

struct Node{

public:
  Node(const double &x, const double &y, const double &z){
    
  }  
  operator[](const int &i){return x[i];}

protected:
  double x[3];
};

#endif