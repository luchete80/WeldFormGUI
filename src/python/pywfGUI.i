/* File : example.i */

%include "std_string.i"
%include "cpointer.i"
%include <std_vector.i>
%include <std_string.i>  // Required for std::string mapping

%module model

%{
#include "Entity.h"
#include "src/common/math/Vector.h"
#include "Model.h"
#include "Node.h"
#include "Element.h"
#include "Face.h"
#include "Set.h"
#include "Material.h"
#include "Condition.h"
#include "BoundaryCondition.h"
#include "Mesh.h"
#include "Part.h"
#include "WorkflowAPI.h"
#include "../io/ModelWriter.h"
%}

%template () std::vector<Part*>;
%template(IntVector) std::vector<int>;

/* Let's just grab the original header file here */
%include "src/common/math/Vector.h"
%include "Model.h"
%include "Entity.h"
%include "Node.h"
%include "Element.h"
%include "Face.h"
%include "Set.h"
%template(NodeEntitySet) Set<Node>;
%template(ElementEntitySet) Set<Element>;
%include "Material.h"
%include "Condition.h"
%include "BoundaryCondition.h"
%include "Mesh.h"
%include "Part.h"
%include "WorkflowAPI.h"
%include "../io/ModelWriter.h"  // ✅ This is what exposes ModelWriter to Python
%newobject ModelWriter::ModelWriter;
%newobject create_empty_mesh_part;
%newobject create_rectangle_part;
%newobject import_step_part;
%newobject import_step_part_at;
