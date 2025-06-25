void LoadResultsWithModel(
    const Model& model,
    Results& results,
    const std::string& resultFolder,
    int numFrames,
    double dt)
{
    for (int frameId = 0; frameId < numFrames; ++frameId) {
        std::ostringstream filename;
        filename << resultFolder << "/frame_" << frameId << ".vtk";

        auto reader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
        reader->SetFileName(filename.str().c_str());
        reader->Update();

        auto resultGrid = reader->GetOutput();
        auto displacementArray = resultGrid->GetPointData()->GetArray("Displacement");
        auto stressArray = resultGrid->GetPointData()->GetArray("VonMises"); // etc.

        ResultFrame frame(frameId * dt);

        for (const auto& [partName, geometry] : model.parts) {
            // Create a shallow copy of the topology
            auto partMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
            partMesh->DeepCopy(geometry.topology);

            // Replace nodal positions if displacement exists
            if (displacementArray) {
                auto points = vtkSmartPointer<vtkPoints>::New();
                points->SetNumberOfPoints(geometry.nodeIndices.size());

                for (vtkIdType i = 0; i < geometry.nodeIndices.size(); ++i) {
                    int globalId = geometry.nodeIndices[i];
                    double x[3];
                    resultGrid->GetPoint(globalId, x);

                    double* disp = displacementArray->GetTuple3(globalId);
                    x[0] += disp[0];
                    x[1] += disp[1];
                    x[2] += disp[2];

                    points->SetPoint(i, x);
                }

                partMesh->SetPoints(points);
            }

            // Assign scalar fields if needed
            if (stressArray) {
                auto partStress = vtkSmartPointer<vtkDoubleArray>::New();
                partStress->SetName("VonMises");
                partStress->SetNumberOfComponents(1);
                partStress->SetNumberOfTuples(geometry.nodeIndices.size());

                for (vtkIdType i = 0; i < geometry.nodeIndices.size(); ++i) {
                    int globalId = geometry.nodeIndices[i];
                    double val = stressArray->GetTuple1(globalId);
                    partStress->SetTuple1(i, val);
                }

                partMesh->GetPointData()->AddArray(partStress);
                partMesh->GetPointData()->SetScalars(partStress);
            }

            // Mapper and actor
            auto mapper = vtkSmartPointer<vtkDataSetMapper>::New();
            mapper->SetInputData(partMesh);
            mapper->ScalarVisibilityOn();

            auto actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);

            frame.AddPart(ResultPart(partName, partMesh, actor));
        }

        results.AddFrame(frame);
    }
}