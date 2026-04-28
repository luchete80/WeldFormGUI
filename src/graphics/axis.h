#ifndef _AXIS_H_
#define _AXIS_H_

#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCaptionActor2D.h>
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
#include <vtkObjectFactory.h>
#include <vtkTextProperty.h>

#include "imgui.h"

//https://examples.vtk.org/site/Cxx/Visualization/DisplayCoordinateAxes/

class HoverableOrientationMarkerWidget : public vtkOrientationMarkerWidget {
public:
  static HoverableOrientationMarkerWidget* New();
  vtkTypeMacro(HoverableOrientationMarkerWidget, vtkOrientationMarkerWidget);

  vtkRenderer* GetOverlayRenderer() const { return this->Renderer; }
};

class Axis{

public:
  Axis();
  ~Axis();
	/*IMGUI_IMPL_API*/ void init();

  void setInteractor(vtkSmartPointer <vtkRenderWindowInteractor>, vtkSmartPointer<vtkRenderer> sceneRenderer);
  vtkSmartPointer<vtkNamedColors> colors;

  void onMouseMove();
  void onLeftButtonPress();

//protected:
  vtkSmartPointer <vtkAxesActor>                actor;
  vtkSmartPointer <HoverableOrientationMarkerWidget>  widget;
  //vtkRenderWindowInteractor                     rwi;
  protected:
  //vtkSmartPointer<vtkNamedColors> colors;

  // Create an arrow.
  //std::vector <vtkSmartPointer<vtkArrowSource> > arrow_ax;
  

  // Create a mapper and actor
  vtkSmartPointer<vtkPolyDataMapper> mapper;  
  vtkSmartPointer<vtkRenderWindowInteractor> interactor;
  vtkSmartPointer<vtkRenderer> sceneRenderer;
  vtkSmartPointer<vtkCallbackCommand> mouseMoveCallback;
  vtkSmartPointer<vtkCallbackCommand> leftButtonPressCallback;
  int hoveredAxis = -1;

  static void HandleMouseMove(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void HandleLeftButtonPress(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  int pickAxisAtEventPosition() const;
  void setAxisHighlight(int axis);
  void orientCameraToAxis(int axis);
  bool eventInsideWidgetViewport() const;
  };

#endif
