/* File : example.i */

%include "std_string.i"
%include "cpointer.i"
%include <std_vector.i>

%module model

%{
 #include "App.h"
#include "Entity.h"
#include "src/common/math/Vector.h"
#include "Model.h"
#include "Node.h"
#include "Element.h"
#include "Mesh.h"
#include "Part.h"
#include "App.h"
//#include "global.h" 
#include "../io/ModelWriter.h"
#include "geom/vtkOCCTGeom.h"
%}

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
