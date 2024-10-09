/* File : example.i */

%include "std_string.i"
%include "cpointer.i"
%include <std_vector.i>

%module model

%{
#include "Model.h"
#include "Part.h"
%}

%template () std::vector<Part*>;

/* Let's just grab the original header file here */
%include "Model.h"
%include "Part.h"

