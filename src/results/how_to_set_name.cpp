vtkSmartPointer<vtkUnstructuredGrid> output = reader->GetOutput();
vtkPointData* pointData = output->GetPointData();

std::cout << "Available scalar arrays:" << std::endl;
for (int i = 0; i < pointData->GetNumberOfArrays(); ++i) {
    const char* name = pointData->GetArrayName(i);
    std::cout << "  [" << i << "] " << (name ? name : "(unnamed)") << std::endl;
}

// Option 1: by name
output->GetPointData()->SetActiveScalars("PlasticStrain");

// Option 2: by index
vtkDataArray* arr = pointData->GetArray(1);
if (arr) {
    output->GetPointData()->SetActiveScalars(arr->GetName());
}

contour->SetInputData(output);
contour->SetValue(0, 0.25);  // based on the chosen scalar's range


//QUERY SCALAR RANGE
vtkDataArray* activeArray = output->GetPointData()->GetScalars();
double range[2];
activeArray->GetRange(range);

std::cout << "Active scalar: " << activeArray->GetName()
          << " | Range: [" << range[0] << ", " << range[1] << "]" << std::endl;
          