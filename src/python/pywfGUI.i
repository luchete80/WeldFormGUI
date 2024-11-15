/* File : example.i */

%include "std_string.i"
%include "cpointer.i"
%include <std_vector.i>

%module model

%{
 #include "App.h"
#include "Entity.h"

#include "Model.h"
#include "Node.h"
#include "Element.h"
#include "Mesh.h"
#include "Part.h"
#include "App.h"
//#include "global.h" 
 
%}

//%inline %{
//extern Model *curr_Model;
//%}

%template () std::vector<Part*>;

/* Let's just grab the original header file here */
%include "Model.h"
%include "Node.h"
%include "Element.h"
%include "Entity.h"
%include "Mesh.h"
%include "Part.h"
%include "App.h"
