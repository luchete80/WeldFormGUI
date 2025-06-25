#include <vtkSmartPointer.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkContourFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCallbackCommand.h>
#include <vtkUnstructuredGrid.h>

#include <vector>
#include <string>
#include <sstream>
#include <iostream>

class AnimationCallback : public vtkCallbackCommand
{
public:
    static AnimationCallback* New()
    {
        return new AnimationCallback;
    }

    void SetFiles(const std::vector<std::string>& files)
    {
        this->vtkFiles = files;
        this->frame = 0;
    }

    void SetPipeline(vtkUnstructuredGridReader* reader, vtkContourFilter* contour)
    {
        this->reader = reader;
        this->contour = contour;
    }

    void Execute(vtkObject* caller, unsigned long eventId, void* callData) override
    {
        if (frame >= vtkFiles.size()) {
            frame = 0;  // loop or exit
            // reinterpret_cast<vtkRenderWindowInteractor*>(caller)->ExitCallback(); // to stop instead
        }

        std::cout << "Frame " << frame << ": " << vtkFiles[frame] << std::endl;

        reader->SetFileName(vtkFiles[frame].c_str());
        reader->Update();
        contour->Update();

        auto iren = static_cast<vtkRenderWindowInteractor*>(caller);
        iren->Render();

        frame++;
    }

private:
    vtkUnstructuredGridReader* reader = nullptr;
    vtkContourFilter* contour = nullptr;
    std::vector<std::string> vtkFiles;
    size_t frame = 0;
};

int main()
{
    // Generate list of file names (frame_0.vtk, frame_1.vtk, ...)
    std::vector<std::string> vtkFiles;
    for (int i = 0; i < 100; ++i) {
        std::ostringstream oss;
        oss << "frame_" << i << ".vtk";
        vtkFiles.push_back(oss.str());
    }

    // Read first file
    auto reader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
    reader->SetFileName(vtkFiles[0].c_str());
    reader->Update();

    // Contour filter
    auto contour = vtkSmartPointer<vtkContourFilter>::New();
    contour->SetInputConnection(reader->GetOutputPort());
    contour->SetValue(0, 0.5);  // adjust

    // Mapper and actor
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(contour->GetOutputPort());

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    // Rendering
    auto renderer = vtkSmartPointer<vtkRenderer>::New();
    auto renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);

    auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(renderWindow);

    renderer->AddActor(actor);
    renderer->SetBackground(1, 1, 1);

    // Setup animation callback
    auto cb = vtkSmartPointer<AnimationCallback>::New();
    cb->SetFiles(vtkFiles);
    cb->SetPipeline(reader, contour);

    interactor->AddObserver(vtkCommand::TimerEvent, cb);
    interactor->CreateRepeatingTimer(100); // milliseconds per frame

    renderWindow->Render();
    interactor->Start();

    return 0;
}
