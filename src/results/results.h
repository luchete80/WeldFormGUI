#ifndef _RESULTS_H_
#define _RESULTS_H_

// Includes necesarios para VTK
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkContourFilter.h>
#include <vtkPointData.h>
#include <vtkLookupTable.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarsToColors.h>
#include <vtkTextProperty.h>

// Includes estándar de C++
#include <string>
#include <iostream>
#include <stdexcept>


#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory> // para std::unique_ptr

#include <vtkCellData.h>

#include <vtkDataSetReader.h>

using json = nlohmann::json;

namespace fs = std::filesystem;

class ResultFrame;

struct MultiResult {
    //std::vector<std::unique_ptr<ResultFrame>> frames;
    std::vector<std::unique_ptr<ResultFrame>> frames;
    void setShowEdges(bool showEdges);
};

MultiResult LoadResultsFromJson(const std::string& jsonFile);




class ResultFrame {
public:
    std::string name;
    vtkSmartPointer<vtkUnstructuredGrid> mesh;
    vtkSmartPointer<vtkActor> actor;
    vtkSmartPointer<vtkDataSetMapper> mapper;
    vtkSmartPointer<vtkContourFilter> contourFilter;
    vtkSmartPointer<vtkScalarBarActor> scalarBar;
    bool useContour;
    bool showEdges;
    
    // Constructor moderno y seguro
    explicit ResultFrame(const std::string& name_) : name(name_) {
        showEdges = false;
        loadVTKFile(name_);
        setupRenderingPipeline();
        setupScalarBar();
    }

    std::vector<std::string> getAvailableFieldNames() const {
        std::vector<std::string> names;
        if (!mesh) return names;
        
        // === Campos nodales ===
        if (mesh->GetPointData()) {
            vtkPointData* pointData = mesh->GetPointData();
            //std::cout << "PointData has " << pointData->GetNumberOfArrays() << " arrays.\n";

            for (int i = 0; i < pointData->GetNumberOfArrays(); i++) {
                vtkDataArray* array = pointData->GetArray(i);
                if (array && array->GetName()) {
                    std::string entry = "[P] ";
                    entry += array->GetName();
                    names.push_back(entry);
                }
            }
        }

        // === Campos elementales ===
        if (mesh->GetCellData()) {
            vtkCellData* cellData = mesh->GetCellData();
            //std::cout << "cellData has " << cellData->GetNumberOfArrays() << " arrays.\n";
            for (int i = 0; i < cellData->GetNumberOfArrays(); i++) {
                vtkDataArray* array = cellData->GetArray(i);
                if (array && array->GetName()) {
                    std::string entry = "[C] ";
                    entry += array->GetName();
                    names.push_back(entry);
                }
            }
        }

        return names;
    }
private:
    /////// THIS WOIRKS BADSLY
    //~ void loadVTKFile(const std::string& filename) {
        //~ auto reader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
        //~ reader->SetFileName(filename.c_str());
        //~ reader->Update();
        
        //~ // Verificar que el archivo se leyó correctamente
        //~ if (reader->GetErrorCode() != 0) {
            //~ throw std::runtime_error("Error reading VTK file: " + filename);
        //~ }
        
        //~ mesh = reader->GetOutput();
        
        //~ // Verificar que el mesh no esté vacío
        //~ if (!mesh || mesh->GetNumberOfPoints() == 0) {
            //~ throw std::runtime_error("Empty or invalid mesh in file: " + filename);
        //~ }
        
        //~ std::cout << "File loaded successfully" << std::endl;
        //~ std::cout << "Points: " << mesh->GetNumberOfPoints() << std::endl;
        //~ std::cout << "Cells: " << mesh->GetNumberOfCells() << std::endl;

        //~ if (mesh->GetNumberOfPoints() == 0) {
            //~ throw std::runtime_error("Mesh has no points");
        //~ }

    //~ }
    
void loadVTKFile(const std::string& filename) {
    auto reader = vtkSmartPointer<vtkDataSetReader>::New();
    reader->SetFileName(filename.c_str());
    
    // Fuerza lectura de todo tipo de arrays
    reader->ReadAllScalarsOn();
    reader->ReadAllVectorsOn();
    reader->ReadAllTensorsOn();
    reader->ReadAllFieldsOn();

    reader->Update();

    // Obtener el output como un unstructured grid
    mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    mesh = reader->GetUnstructuredGridOutput();
    
    if (!mesh) {
        std::cerr << "Could not read unstructured grid from " << filename << std::endl;
        return;
    }

    // std::cout << "Loaded " << mesh->GetNumberOfPoints() << " points, "
    //           << mesh->GetNumberOfCells() << " cells" << std::endl;
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
            // std::cout << "Scalar range: [" << range[0] << ", " << range[1] << "]" << std::endl;
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
        actor->GetProperty()->SetRepresentationToSurface();
        actor->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);
        actor->GetProperty()->EdgeVisibilityOff();
        
        useContour = false;
        // std::cout << "Created basic pipeline" << std::endl;
    }

    void setupScalarBar() {
        scalarBar = vtkSmartPointer<vtkScalarBarActor>::New();
        scalarBar->SetLookupTable(mapper ? mapper->GetLookupTable() : nullptr);
        scalarBar->SetNumberOfLabels(5);
        scalarBar->SetUnconstrainedFontSize(true);
        scalarBar->SetMaximumWidthInPixels(80);
        scalarBar->SetMaximumHeightInPixels(300);
        scalarBar->SetPosition(0.88, 0.1);
        scalarBar->SetWidth(0.1);
        scalarBar->SetHeight(0.8);
        scalarBar->SetVisibility(false);

        scalarBar->GetTitleTextProperty()->SetColor(1.0, 1.0, 1.0);
        scalarBar->GetTitleTextProperty()->BoldOff();
        scalarBar->GetTitleTextProperty()->ItalicOff();
        scalarBar->GetTitleTextProperty()->ShadowOff();
        scalarBar->GetLabelTextProperty()->SetColor(1.0, 1.0, 1.0);
        scalarBar->GetLabelTextProperty()->BoldOff();
        scalarBar->GetLabelTextProperty()->ItalicOff();
        scalarBar->GetLabelTextProperty()->ShadowOff();
    }
    
    void setupContourPipeline() {
        // Crear contour filter
        contourFilter = vtkSmartPointer<vtkContourFilter>::New();
        contourFilter->SetInputData(mesh);
        
        // Obtener rango de escalares
        double* range = mesh->GetScalarRange();
        // std::cout << "Scalar range: [" << range[0] << ", " << range[1] << "]" << std::endl;
        
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
        // std::cout << "Created contour with " << numContours << " levels" << std::endl;
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
        actor->GetProperty()->EdgeVisibilityOff();
        actor->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);
        
        useContour = false;
    }

public:
    // Método para seleccionar qué campo mostrar
    //~ void setActiveScalarField(const std::string& fieldName) {
        //~ if (!mesh || !mesh->GetPointData()) {
            //~ std::cerr << "No point data available" << std::endl;
            //~ return;
        //~ }
        
        //~ vtkPointData* pointData = mesh->GetPointData();
        
        //~ // Buscar el campo por nombre
        //~ vtkDataArray* field = pointData->GetArray(fieldName.c_str());
        //~ if (!field) {
            //~ std::cerr << "Field '" << fieldName << "' not found" << std::endl;
            //~ printAvailableFields(); // Helper para debug
            //~ return;
        //~ }
        
        //~ // Establecer como campo activo según el tipo
        //~ if (field->GetNumberOfComponents() == 1) {
            //~ // Campo escalar
            //~ pointData->SetActiveScalars(fieldName.c_str());
            //~ if (mapper) {
                //~ mapper->SetScalarModeToUsePointFieldData();
                //~ mapper->SelectColorArray(fieldName.c_str());
                //~ mapper->ScalarVisibilityOn();
                
                //~ double* range = field->GetRange();
                //~ mapper->SetScalarRange(range[0], range[1]);
                //~ std::cout << "Set scalar field '" << fieldName << "' range: [" 
                         //~ << range[0] << ", " << range[1] << "]" << std::endl;
            //~ }
        //~ } else if (field->GetNumberOfComponents() == 3) {
            //~ // Campo vectorial (como DISP)
            //~ pointData->SetActiveVectors(fieldName.c_str());
            
            //~ // Para vectores, puedes mostrar la magnitud
            //~ if (mapper) {
                //~ mapper->SetScalarModeToUsePointFieldData();
                //~ mapper->SelectColorArray(fieldName.c_str());
                //~ mapper->ScalarVisibilityOn();
                
                //~ // Calcular rango de magnitud
                //~ double* range = field->GetRange(-1); // -1 = magnitud
                //~ mapper->SetScalarRange(range[0], range[1]);
                //~ std::cout << "Set vector field '" << fieldName << "' magnitude range: [" 
                         //~ << range[0] << ", " << range[1] << "]" << std::endl;
            //~ }
        //~ }
        
        //~ // Forzar actualización
        //~ if (mapper) mapper->Modified();
        //~ if (actor) actor->Modified();
    //~ }

      void setActiveScalarField(const std::string& fieldName) {
          if (!mesh) {
              std::cerr << " No mesh available" << std::endl;
              return;
          }

          vtkPointData* pointData = mesh->GetPointData();
          vtkCellData*  cellData  = mesh->GetCellData();
          vtkDataArray* field = nullptr;
          bool isCellField = false;

          // --- Buscar en PointData primero ---
          if (pointData && pointData->HasArray(fieldName.c_str())) {
              field = pointData->GetArray(fieldName.c_str());
              isCellField = false;
          }
          // --- Si no está, buscar en CellData ---
          else if (cellData && cellData->HasArray(fieldName.c_str())) {
              field = cellData->GetArray(fieldName.c_str());
              isCellField = true;
          }

          if (!field) {
              std::cerr << "WARNING: Field '" << fieldName << "' not found in point or cell data." << std::endl;
              printAvailableFields();
              return;
          }

          // --- Configurar mapper según tipo de campo ---
          if (mapper) {
              if (isCellField) {
                  mapper->SetScalarModeToUseCellFieldData();
                  cellData->SetActiveScalars(fieldName.c_str());
              } else {
                  mapper->SetScalarModeToUsePointFieldData();
                  pointData->SetActiveScalars(fieldName.c_str());
              }

              mapper->SelectColorArray(fieldName.c_str());
              mapper->ScalarVisibilityOn();

              // --- Rango dinámico ---
              double range[2];
              if (field->GetNumberOfComponents() == 3)
                  field->GetRange(range, -1); // magnitud
              else
                  field->GetRange(range);

              mapper->SetScalarRange(range);
              // std::cout << "Set field '" << fieldName << "' ("
              //           << (isCellField ? "CellData" : "PointData") << ") range: ["
              //           << range[0] << ", " << range[1] << "]" << std::endl;
          }

          // --- Forzar actualización ---
          mesh->Modified();
          if (mapper) mapper->Modified();
          if (actor) actor->Modified();
      }
    
    // Helper para ver campos disponibles
    void printAvailableFields() const {
        if (!mesh || !mesh->GetPointData()) return;
        
        vtkPointData* pointData = mesh->GetPointData();
        // std::cout << "Available point data fields:" << std::endl;
        
        for (int i = 0; i < pointData->GetNumberOfArrays(); i++) {
            vtkDataArray* array = pointData->GetArray(i);
            // std::cout << "  [" << i << "] " << array->GetName() 
            //          << " (" << array->GetNumberOfComponents() << " components, "
            //          << array->GetNumberOfTuples() << " values)" << std::endl;
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
            // std::cout << "Set " << fieldName << " component " << compName[component] 
            //          << " range: [" << range[0] << ", " << range[1] << "]" << std::endl;
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

    void setShowEdges(bool enabled) {
        showEdges = enabled;
        if (!actor || useContour) return;

        actor->GetProperty()->SetRepresentationToSurface();
        if (enabled) {
            actor->GetProperty()->EdgeVisibilityOn();
            actor->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);
        } else {
            actor->GetProperty()->EdgeVisibilityOff();
        }
        actor->Modified();
    }

    bool getShowEdges() const { return showEdges; }

    bool getFieldRange(const std::string& fieldName, bool isCellField, double& minValue, double& maxValue) const {
        if (!mesh) return false;

        vtkDataArray* field = isCellField
            ? mesh->GetCellData()->GetArray(fieldName.c_str())
            : mesh->GetPointData()->GetArray(fieldName.c_str());

        if (!field) return false;

        double range[2];
        if (field->GetNumberOfComponents() == 3)
            field->GetRange(range, -1);
        else
            field->GetRange(range);

        minValue = range[0];
        maxValue = range[1];
        return true;
    }

    void updateScalarBar(const std::string& fieldName, double minValue, double maxValue) {
        if (!scalarBar || !mapper) return;

        auto newLut = vtkSmartPointer<vtkLookupTable>::New();
        newLut->SetRange(minValue, maxValue);
        newLut->SetHueRange(0.66667, 0.0); // azul -> rojo
        newLut->Build();
        mapper->SetLookupTable(newLut);

        vtkScalarsToColors* lut = mapper->GetLookupTable();

        scalarBar->SetLookupTable(lut);
        scalarBar->SetTitle(fieldName.c_str());
        scalarBar->SetVisibility(true);
        scalarBar->Modified();
    }

    void hideScalarBar() {
        if (scalarBar) scalarBar->SetVisibility(false);
    }

    vtkSmartPointer<vtkScalarBarActor> getScalarBarActor() const { return scalarBar; }
    
    // Información del mesh
    void printInfo() const {
        if (mesh) {
            // std::cout << "Mesh info for " << name << ":\n";
            // std::cout << "  Points: " << mesh->GetNumberOfPoints() << "\n";
            // std::cout << "  Cells: " << mesh->GetNumberOfCells() << "\n";
        }
    }
};

inline void MultiResult::setShowEdges(bool showEdges) {
    for (auto& frame : frames) {
        if (frame) frame->setShowEdges(showEdges);
    }
}

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


#endif
