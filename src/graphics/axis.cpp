#include "axis.h"

Axis::Axis()
:colors(nullptr)
{
  init();
  /*
  colors = vtkSmartPointer<vtkNamedColors>::New();
  
  for (int i=0;i<3;i++){
    arrow_ax.push_back(vtkSmartPointer<vtkArrowSource>::New());
  // Create an arrow.
  //arrowSource = vtkSmartPointer<vtkArrowSource>::New();
  // arrowSource->SetShaftRadius(1.0);
  // arrowSource->SetTipLength(1.0);
    arrow_ax[i]->Update();
  }
  // Create a mapper and actor
  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(arrow_ax[0]->GetOutputPort());
  //vtkNew<vtkActor> actor;
  */
  actor = vtkSmartPointer<vtkAxesActor>::New();
  //axisactor->SetMapper(mapper);
  
  
  widget = vtkSmartPointer <vtkOrientationMarkerWidget>::New();
    
  double rgba[4]{0.0, 0.0, 0.0, 0.0};
  
  widget->SetOutlineColor(rgba[0], rgba[1], rgba[2]);
  widget->SetOrientationMarker(actor);
  //widget->SetInteractor(renderWindowInteractor);
  //widget->SetViewport(0.0, 0.0, 0.4, 0.4);
  //widget->SetEnabled(1);
  //widget->InteractiveOn();


  
  /*
  // Visualize
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetWindowName("Arrow");
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->SetBackground(colors->GetColor3d("MidnightBlue").GetData());

  renderWindow->SetWindowName("Arrow");
  renderWindow->Render();
  renderWindowInteractor->Start();
  */
  //return EXIT_SUCCESS;
}
Axis::~Axis(){
  colors = nullptr;}

IMGUI_IMPL_API void Axis::init(){
  colors = vtkSmartPointer<vtkNamedColors> ::New();
  }
  
void Axis::setInteractor(vtkSmartPointer <vtkRenderWindowInteractor> rwi){
  
widget->SetInteractor(rwi);
widget->SetViewport(0.0, 0.0, 0.4, 0.4);
widget->SetEnabled(1);
widget->InteractiveOn();
}
