
#include <vtkUnstructuredGridReader.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>

#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <filesystem> // C++17 for checking file existence

class ResultFrame {
public:
    std::string name;
    vtkSmartPointer<vtkDataSet> mesh; // could be UnstructuredGrid, PolyData, etc.
    vtkSmartPointer<vtkActor> actor;
    vtkSmartPointer<vtkDataSetMapper> mapper;

    ResultFrame(std::string& name_)
        //: name(name_), mesh(mesh_), actor(actor_) 
        {
          
          
          
            // auto reader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
            // reader->SetFileName(name_.c_str());
            // reader->Update();

            // mesh = reader->GetOutput();

            // mapper = vtkSmartPointer<vtkDataSetMapper>::New();
            // mapper->SetInputData(mesh);

            // actor = vtkSmartPointer<vtkActor>::New();
            // actor->SetMapper(mapper);
          
          
        }
};