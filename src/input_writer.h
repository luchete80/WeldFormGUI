#ifndef _INPUT_WRITER_
#define _INPUT_WRITER_

#include <string>
#include "Domain.h"

//TODO: TEMPLATE AS MODEL TYPE, SPH; FEM
class InputWriter {
  //Check if write particles as geometry or with ext file
public:
  InputWriter(const std::string &, const SPHModel &);
  
};


#endif