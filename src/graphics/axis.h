#ifndef _AXIS_H_
#define _AXIS_H_

#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

#include "imgui.h"

//https://examples.vtk.org/site/Cxx/Visualization/DisplayCoordinateAxes/

class Axis{

public:
  Axis();
  ~Axis();
//private:
	/*IMGUI_IMPL_API*/ void init();

  void setInteractor(vtkSmartPointer <vtkRenderWindowInteractor>);
  vtkSmartPointer<vtkNamedColors> colors;


//protected:
  vtkSmartPointer <vtkAxesActor>                actor;
  vtkSmartPointer <vtkOrientationMarkerWidget>  widget;
  //vtkRenderWindowInteractor                     rwi;
  protected:
  //vtkSmartPointer<vtkNamedColors> colors;

  // Create an arrow.
  //std::vector <vtkSmartPointer<vtkArrowSource> > arrow_ax;
  

  // Create a mapper and actor
  vtkSmartPointer<vtkPolyDataMapper> mapper;  
  
  
  
  };

#endif
