#ifndef _AXIS_H_
#define _AXIS_H_

#include <vtkProp.h>
#include <vtkPropCollection.h>
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include "imgui.h"


class Axis{

public:
  Axis();
  ~Axis();
private:
	/*IMGUI_IMPL_API*/ void init();

vtkSmartPointer<vtkNamedColors> colors;

/*
//protected:
  vtkSmartPointer <vtkActor> axisactor;
  protected:
  vtkSmartPointer<vtkNamedColors> colors;

  // Create an arrow.
  vtkSmartPointer<vtkArrowSource> arrowSource;


  // Create a mapper and actor
  vtkSmartPointer<vtkPolyDataMapper> mapper;  
  
  */
  
  };

#endif
