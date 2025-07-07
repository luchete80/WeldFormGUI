/* File : example.i */

%include "std_string.i"
%include "cpointer.i"
%include <std_vector.i>
%include <std_string.i>  // Required for std::string mapping

%module model

%{
#include "geom/vtkOCCTGeom.h"
//#include "geom/Geom.h"
#include "App.h"
#include "Entity.h"
#include "src/common/math/Vector.h"
#include "Model.h"
#include "Node.h"
#include "Element.h"
#include "Mesh.h"
#include "Part.h"
//#include "global.h" 
#include "../io/ModelWriter.h"


%}

// Or forward declare it as an opaque type
class TopoDS_Shape;
//%ignore TopoDS_Shape;
//%ignore TopoDS_Shape::*;
//%opaque TopoDS_Shape;

//%inline %{
//extern Model *curr_Model;
//%}

%template () std::vector<Part*>;

/* Let's just grab the original header file here */
%include "src/common/math/Vector.h"
%include "Model.h"
%include "Node.h"
%include "Element.h"
%include "Entity.h"
%include "Mesh.h"
%include "Part.h"
%include "App.h"
%include "../io/ModelWriter.h"  // ✅ This is what exposes ModelWriter to Python
%newobject ModelWriter::ModelWriter;
%include "geom/vtkOCCTGeom.h"
//%include "geom/Geom.h"
  
class Geom {
public:
    Geom();
    Geom(std::string fname);
    ~Geom();

    void readFile(std::string file);
};


// Force destructor wrapper generation if needed
//%destructor Geom;