/* File : example.i */
%module model

%{
#include "Model.h"
%}

/* Let's just grab the original header file here */
%include "Model.h"

