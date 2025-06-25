#include "Results.h"
#include <vtkUnstructuredGridReader.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>

#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <filesystem> // C++17 for checking file existence

namespace fs = std::filesystem;

void LoadResults(Results& results,
                 const std::vector<std::string>& partNames,
                 const std::string& filePrefix,
                 const std::string& fileSuffix,
                 int numFrames,
                 double dt)
{
    for (int frameId = 0; frameId < numFrames; ++frameId) {
        ResultFrame frame(frameId * dt);

        for (const auto& partName : partNames) {
            std::ostringstream fname;
            fname << filePrefix << frameId << "_" << partName << fileSuffix;

            if (!fs::exists(fname.str())) {
                std::cerr << "Warning: file " << fname.str() << " not found." << std::endl;
                continue;
            }

            auto reader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
            reader->SetFileName(fname.str().c_str());
            reader->Update();

            auto mesh = reader->GetOutput();

            auto mapper = vtkSmartPointer<vtkDataSetMapper>::New();
            mapper->SetInputData(mesh);

            auto actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);

            ResultPart part(partName, mesh, actor);
            frame.AddPart(part);
        }

        results.AddFrame(frame);
    }
}


int test()
{
    Results results;

    std::vector<std::string> partNames = {"partA", "partB", "partC"};
    std::string prefix = "frame_";
    std::string suffix = ".vtk"; // or ".vtu"
    int numFrames = 100;
    double dt = 0.01;

    LoadResults(results, partNames, prefix, suffix, numFrames, dt);

    std::cout << "Loaded " << results.GetFrameCount() << " frames." << std::endl;
    std::cout << "First frame has " << results.GetFrame(0).parts.size() << " parts." << std::endl;

    // TODO: Use in animation/rendering
}