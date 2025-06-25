
#include <vtkUnstructuredGridReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>

#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <filesystem> // C++17 for checking file existence
class ResultFrame {
public:
    std::string name;
    vtkSmartPointer<vtkUnstructuredGrid> mesh;
    vtkSmartPointer<vtkActor> actor;
    vtkSmartPointer<vtkDataSetMapper> mapper;
    
    // Constructor moderno y seguro
    explicit ResultFrame(const std::string& name_) : name(name_) {
        loadVTKFile(name_);
        setupRenderingPipeline();
    }

private:
    void loadVTKFile(const std::string& filename) {
        auto reader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
        reader->SetFileName(filename.c_str());
        reader->Update();
        
        // Verificar que el archivo se leyó correctamente
        if (reader->GetErrorCode() != 0) {
            throw std::runtime_error("Error reading VTK file: " + filename);
        }
        
        mesh = reader->GetOutput();
        
        // Verificar que el mesh no esté vacío
        if (!mesh || mesh->GetNumberOfPoints() == 0) {
            throw std::runtime_error("Empty or invalid mesh in file: " + filename);
        }
    }
    
    void setupRenderingPipeline() {
        // Crear mapper - no necesitas cast, vtkUnstructuredGrid ES un vtkDataSet
        mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInputData(mesh);  // Sin cast necesario
        
        // Crear actor
        actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        
        // Configuración inicial del actor
        actor->GetProperty()->SetColor(0.8, 0.8, 0.9);  // Color por defecto
        actor->GetProperty()->SetOpacity(1.0);
    }

public:
    // Métodos útiles adicionales
    void setOpacity(double opacity) {
        if (actor) {
            actor->GetProperty()->SetOpacity(opacity);
        }
    }
    
    void setColor(double r, double g, double b) {
        if (actor) {
            actor->GetProperty()->SetColor(r, g, b);
        }
    }
    
    void setWireframe(bool wireframe) {
        if (actor) {
            if (wireframe) {
                actor->GetProperty()->SetRepresentationToWireframe();
            } else {
                actor->GetProperty()->SetRepresentationToSurface();
            }
        }
    }
    
    // Información del mesh
    void printInfo() const {
        if (mesh) {
            std::cout << "Mesh info for " << name << ":\n";
            std::cout << "  Points: " << mesh->GetNumberOfPoints() << "\n";
            std::cout << "  Cells: " << mesh->GetNumberOfCells() << "\n";
        }
    }
};

// Ejemplo de uso:
/*
try {
    ResultFrame frame("mi_archivo.vtk");
    frame.setOpacity(0.7);
    frame.setColor(1.0, 0.5, 0.0);  // Naranja
    frame.printInfo();
    
    // Agregar al renderer
    renderer->AddActor(frame.actor);
    
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
*/