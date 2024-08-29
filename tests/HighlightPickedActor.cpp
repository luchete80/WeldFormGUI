// FROM https://examples.vtk.org/site/Cxx/Picking/HighlightPickedActor/
// Execute application.
int main(int argc, char* argv[])
{
  vtkNew<vtkNamedColors> colors;

  int numberOfSpheres = 10;
  if (argc > 1)
  {
    numberOfSpheres = atoi(argv[1]);
  }
  // A renderer and render window.
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(640, 480);
  renderWindow->AddRenderer(renderer);
  renderWindow->SetWindowName("HighlightPickedActor");

  // An interactor.
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Set the custom type to use for interaction.
  vtkNew<MouseInteractorHighLightActor> style;
  style->SetDefaultRenderer(renderer);

  renderWindowInteractor->SetInteractorStyle(style);

  vtkNew<vtkMinimalStandardRandomSequence> randomSequence;
  randomSequence->SetSeed(8775070);
  for (int i = 0; i < numberOfSpheres; ++i)
  {
    vtkNew<vtkSphereSource> source;
    double x, y, z, radius;
    // random position and radius
    x = randomSequence->GetRangeValue(-5.0, 5.0);
    randomSequence->Next();
    y = randomSequence->GetRangeValue(-5.0, 5.0);
    randomSequence->Next();
    z = randomSequence->GetRangeValue(-5.0, 5.0);
    randomSequence->Next();
    radius = randomSequence->GetRangeValue(0.5, 1.0);
    randomSequence->Next();
    source->SetRadius(radius);
    source->SetCenter(x, y, z);
    source->SetPhiResolution(11);
    source->SetThetaResolution(21);
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(source->GetOutputPort());
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    double r, g, b;
    r = randomSequence->GetRangeValue(0.4, 1.0);
    randomSequence->Next();
    g = randomSequence->GetRangeValue(0.4, 1.0);
    randomSequence->Next();
    b = randomSequence->GetRangeValue(0.4, 1.0);
    randomSequence->Next();
    actor->GetProperty()->SetDiffuseColor(r, g, b);
    actor->GetProperty()->SetDiffuse(0.8);
    actor->GetProperty()->SetSpecular(0.5);
    actor->GetProperty()->SetSpecularColor(
        colors->GetColor3d("White").GetData());
    actor->GetProperty()->SetSpecularPower(30.0);
    renderer->AddActor(actor);
  }

  renderer->SetBackground(colors->GetColor3d("SteelBlue").GetData());

  // Render and interact.
  renderWindow->Render();
  renderWindowInteractor->Initialize();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}

