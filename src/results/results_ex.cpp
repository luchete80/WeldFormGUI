#include <vtkSmartPointer.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkContourFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkProperty.h>

int openresults(std::string filename])
{


    std::string filename = argv[1];

    // Read .vtk file (legacy format)
    vtkSmartPointer<vtkUnstructuredGridReader> reader =
        vtkSmartPointer<vtkUnstructuredGridReader>::New();
    reader->SetFileName(filename.c_str());
    reader->Update();
    
    reader->GetOutput()->GetPointData()->SetActiveScalars("VonMises");

    // Create contour filter
    vtkSmartPointer<vtkContourFilter> contour =
        vtkSmartPointer<vtkContourFilter>::New();
    contour->SetInputConnection(reader->GetOutputPort());
    contour->SetValue(0, 0.5);  // set iso-value

    // Create mapper
    vtkSmartPointer<vtkPolyDataMapper> mapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(contour->GetOutputPort());
    mapper->ScalarVisibilityOn();

    // Create actor
    vtkSmartPointer<vtkActor> actor =
        vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    // Renderer and render window
    vtkSmartPointer<vtkRenderer> renderer =
        vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> renderWindow =
        vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);

    vtkSmartPointer<vtkRenderWindowInteractor> interactor =
        vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(renderWindow);

    renderer->AddActor(actor);
    renderer->SetBackground(1.0, 1.0, 1.0); // white

    renderWindow->Render();
    interactor->Start();

    return EXIT_SUCCESS;
}
