#include "axis.h"

Axis::Axis()
:colors(nullptr)
{
  init();
  /*
  colors = vtkSmartPointer<vtkNamedColors>::New();

  // Create an arrow.
  arrowSource = vtkSmartPointer<vtkArrowSource>::New();
  // arrowSource->SetShaftRadius(1.0);
  // arrowSource->SetTipLength(1.0);
  arrowSource->Update();

  // Create a mapper and actor
  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(arrowSource->GetOutputPort());
  //vtkNew<vtkActor> actor;
  
  axisactor = vtkSmartPointer<vtkActor>::New();
  axisactor->SetMapper(mapper);

  */
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
