#ifndef _RESULTS_H_
#define _RESULTS_H_

class ResultPart {
public:
    std::string name;
    vtkSmartPointer<vtkDataSet> mesh; // could be UnstructuredGrid, PolyData, etc.
    vtkSmartPointer<vtkActor> actor;

    ResultPart(const std::string& name_,
               vtkSmartPointer<vtkDataSet> mesh_,
               vtkSmartPointer<vtkActor> actor_)
        : name(name_), mesh(mesh_), actor(actor_) {}
};

class ResultFrame {
public:
    double time;
    std::vector<ResultPart> parts;

    ResultFrame(double time_) : time(time_) {}

    void AddPart(const ResultPart& part) {
        parts.push_back(part);
    }
};

class Results {
public:
    std::vector<ResultFrame> frames;

    void AddFrame(const ResultFrame& frame) {
        frames.push_back(frame);
    }

    const ResultFrame& GetFrame(size_t i) const {
        return frames[i];
    }

    size_t GetFrameCount() const {
        return frames.size();
    }
};

// Results results;

// for (int i = 0; i < numTimesteps; ++i) {
    // std::ostringstream fname;
    // fname << "frame_" << i << "_partA.vtk";

    // auto reader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
    // reader->SetFileName(fname.str().c_str());
    // reader->Update();

    // auto mesh = reader->GetOutput();

    // auto mapper = vtkSmartPointer<vtkDataSetMapper>::New();
    // mapper->SetInputData(mesh);

    // auto actor = vtkSmartPointer<vtkActor>::New();
    // actor->SetMapper(mapper);

    // ResultPart part("PartA", mesh, actor);

    // ResultFrame frame(i * dt);
    // frame.AddPart(part);

    // results.AddFrame(frame);
// }

#endif