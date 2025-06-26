// Includes necesarios para VTK
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkContourFilter.h>
#include <vtkPointData.h>

// Includes estándar de C++
#include <string>
#include <iostream>
#include <stdexcept>

class ResultFrame {
public:
    std::string name;
    vtkSmartPointer<vtkUnstructuredGrid> mesh;
    vtkSmartPointer<vtkActor> actor;
    vtkSmartPointer<vtkDataSetMapper> mapper;
    vtkSmartPointer<vtkContourFilter> contourFilter;
    bool useContour;
    
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
        
        std::cout << "File loaded successfully" << std::endl;
std::cout << "Points: " << mesh->GetNumberOfPoints() << std::endl;
std::cout << "Cells: " << mesh->GetNumberOfCells() << std::endl;

if (mesh->GetNumberOfPoints() == 0) {
    throw std::runtime_error("Mesh has no points");
}

    }
    
    void setupRenderingPipeline() {
        // Verificar que el mesh no sea null
        if (!mesh) {
            throw std::runtime_error("Mesh is null");
        }
        
        // PIPELINE SIMPLE Y SEGURO
        mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInputData(mesh);
        
        // Verificar si tenemos datos escalares
        bool hasScalars = (mesh->GetPointData() && mesh->GetPointData()->GetScalars());
        
        if (hasScalars) {
            double* range = mesh->GetScalarRange();
            std::cout << "Scalar range: [" << range[0] << ", " << range[1] << "]" << std::endl;
            mapper->SetScalarRange(range[0], range[1]);
            mapper->ScalarVisibilityOn();
        } else {
            mapper->ScalarVisibilityOff();
        }
        
        // Forzar update del mapper antes de crear actor
        mapper->Update();
        
        // Crear actor
        actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        
        // Configuración básica
        if (!hasScalars) {
            actor->GetProperty()->SetColor(0.8, 0.8, 0.9);
        }
        actor->GetProperty()->SetOpacity(1.0);
        
        useContour = false;
        std::cout << "Created basic pipeline" << std::endl;
    }
    
    void setupContourPipeline() {
        // Crear contour filter
        contourFilter = vtkSmartPointer<vtkContourFilter>::New();
        contourFilter->SetInputData(mesh);
        
        // Obtener rango de escalares
        double* range = mesh->GetScalarRange();
        std::cout << "Scalar range: [" << range[0] << ", " << range[1] << "]" << std::endl;
        
        // Crear múltiples contornos
        int numContours = 10;
        contourFilter->GenerateValues(numContours, range[0], range[1]);
        contourFilter->Update();
        
        // IMPORTANTE: Usar SetInputConnection para filters
        mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInputConnection(contourFilter->GetOutputPort());  // Connection, no Data
        mapper->SetScalarRange(range[0], range[1]);
        mapper->ScalarVisibilityOn();
        
        // Crear actor
        actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        
        // Propiedades para contornos
        actor->GetProperty()->SetLineWidth(2.0);
        actor->GetProperty()->EdgeVisibilityOff(); // Los contornos ya son líneas
        
        useContour = true;
        std::cout << "Created contour with " << numContours << " levels" << std::endl;
    }
    
    void setupSimplePipeline() {
        // Pipeline simple sin contornos
        mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInputData(mesh);
        mapper->ScalarVisibilityOff();
        
        // Crear actor
        actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        
        // Configuración estándar
        actor->GetProperty()->SetColor(0.8, 0.8, 0.9);
        actor->GetProperty()->SetOpacity(1.0);
        actor->GetProperty()->EdgeVisibilityOn();
        actor->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);
        
        useContour = false;
    }

public:
    // Método para seleccionar qué campo mostrar
    void setActiveScalarField(const std::string& fieldName) {
        if (!mesh || !mesh->GetPointData()) {
            std::cerr << "No point data available" << std::endl;
            return;
        }
        
        vtkPointData* pointData = mesh->GetPointData();
        
        // Buscar el campo por nombre
        vtkDataArray* field = pointData->GetArray(fieldName.c_str());
        if (!field) {
            std::cerr << "Field '" << fieldName << "' not found" << std::endl;
            printAvailableFields(); // Helper para debug
            return;
        }
        
        // Establecer como campo activo según el tipo
        if (field->GetNumberOfComponents() == 1) {
            // Campo escalar
            pointData->SetActiveScalars(fieldName.c_str());
            if (mapper) {
                mapper->SetScalarModeToUsePointFieldData();
                mapper->SelectColorArray(fieldName.c_str());
                mapper->ScalarVisibilityOn();
                
                double* range = field->GetRange();
                mapper->SetScalarRange(range[0], range[1]);
                std::cout << "Set scalar field '" << fieldName << "' range: [" 
                         << range[0] << ", " << range[1] << "]" << std::endl;
            }
        } else if (field->GetNumberOfComponents() == 3) {
            // Campo vectorial (como DISP)
            pointData->SetActiveVectors(fieldName.c_str());
            
            // Para vectores, puedes mostrar la magnitud
            if (mapper) {
                mapper->SetScalarModeToUsePointFieldData();
                mapper->SelectColorArray(fieldName.c_str());
                mapper->ScalarVisibilityOn();
                
                // Calcular rango de magnitud
                double* range = field->GetRange(-1); // -1 = magnitud
                mapper->SetScalarRange(range[0], range[1]);
                std::cout << "Set vector field '" << fieldName << "' magnitude range: [" 
                         << range[0] << ", " << range[1] << "]" << std::endl;
            }
        }
        
        // Forzar actualización
        if (mapper) mapper->Modified();
        if (actor) actor->Modified();
    }
    
    // Helper para ver campos disponibles
    void printAvailableFields() const {
        if (!mesh || !mesh->GetPointData()) return;
        
        vtkPointData* pointData = mesh->GetPointData();
        std::cout << "Available point data fields:" << std::endl;
        
        for (int i = 0; i < pointData->GetNumberOfArrays(); i++) {
            vtkDataArray* array = pointData->GetArray(i);
            std::cout << "  [" << i << "] " << array->GetName() 
                     << " (" << array->GetNumberOfComponents() << " components, "
                     << array->GetNumberOfTuples() << " values)" << std::endl;
        }
    }
    
    // Para mostrar solo la magnitud de un vector
    void setVectorMagnitude(const std::string& fieldName) {
        setActiveScalarField(fieldName); // Usa el mismo método
    }
    
    // Para mostrar componente específica de un vector
    void setVectorComponent(const std::string& fieldName, int component) {
        if (!mesh || !mesh->GetPointData()) return;
        
        vtkDataArray* field = mesh->GetPointData()->GetArray(fieldName.c_str());
        if (!field || component >= field->GetNumberOfComponents()) {
            std::cerr << "Invalid field or component" << std::endl;
            return;
        }
        
        if (mapper) {
            mapper->SetScalarModeToUsePointFieldData();
            mapper->SelectColorArray(fieldName.c_str());
            mapper->SetArrayComponent(component); // 0=X, 1=Y, 2=Z
            mapper->ScalarVisibilityOn();
            
            double* range = field->GetRange(component);
            mapper->SetScalarRange(range[0], range[1]);
            
            std::string compName[] = {"X", "Y", "Z"};
            std::cout << "Set " << fieldName << " component " << compName[component] 
                     << " range: [" << range[0] << ", " << range[1] << "]" << std::endl;
        }
    }
    void setContourLevels(int numLevels) {
        if (useContour && contourFilter) {
            double* range = mesh->GetScalarRange();
            contourFilter->GenerateValues(numLevels, range[0], range[1]);
            contourFilter->Update();
        }
    }
    
    void setContourValue(double value) {
        if (useContour && contourFilter) {
            contourFilter->SetValue(0, value);
            contourFilter->Update();
        }
    }
    
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
    
    // Ver qué campos están disponibles
    frame.printAvailableFields();
    
    // Mostrar el campo DISP (magnitud del vector)
    frame.setActiveScalarField("DISP");
    
    // O mostrar solo componente X del desplazamiento
    // frame.setVectorComponent("DISP", 0); // 0=X, 1=Y, 2=Z
    
    renderer->AddActor(frame.actor);
    
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
*/
