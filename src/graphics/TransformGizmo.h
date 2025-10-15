#ifndef _TRANSFORMGIZMO_H_
#define _TRANSFORMGIZMO_H_


#include <vtkLineSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkPropPicker.h>
#include <vtkTransform.h>
#include <vtkCommand.h>
#include <vtkObject.h>  // Add this include
#include <array>
#include <vtkTransformFilter.h>
#include "../model/Part.h"
#include <vtkCellPicker.h> 
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkArrowSource.h>

#include <vtkTransformPolyDataFilter.h>
#include <vtkCylinderSource.h>
#include <vtkCoordinate.h>

using namespace std;

double ComputeGizmoScale(vtkActor* actor) {
    double bounds[6];
    actor->GetBounds(bounds);
    double maxDim = std::max({bounds[1]-bounds[0], bounds[3]-bounds[2], bounds[5]-bounds[4]});
    return maxDim * 0.5; // eje del gizmo = mitad del tamaño máximo de la pieza
}


    double DistanceToLineSegment(const double point[3], const double lineStart[3], const double lineEnd[3]) {
        double lineVec[3] = {lineEnd[0] - lineStart[0], lineEnd[1] - lineStart[1], lineEnd[2] - lineStart[2]};
        double pointVec[3] = {point[0] - lineStart[0], point[1] - lineStart[1], point[2] - lineStart[2]};
        
        double lineLengthSquared = vtkMath::Dot(lineVec, lineVec);
        
        if (lineLengthSquared == 0.0) {
            return vtkMath::Norm(pointVec); // El segmento es un punto
        }
        
        // Proyectar el punto sobre la línea
        double t = vtkMath::Dot(pointVec, lineVec) / lineLengthSquared;
        t = std::max(0.0, std::min(1.0, t)); // Clamp al segmento
        
        // Calcular el punto más cercano en el segmento
        double closestPoint[3] = {
            lineStart[0] + t * lineVec[0],
            lineStart[1] + t * lineVec[1],
            lineStart[2] + t * lineVec[2]
        };
        
        // Calcular distancia
        double diff[3] = {
            point[0] - closestPoint[0],
            point[1] - closestPoint[1],
            point[2] - closestPoint[2]
        };
        
        return vtkMath::Norm(diff);
    }


/////// CYLINDER
class TransformGizmo : public vtkObject {
public:
    vtkTypeMacro(TransformGizmo, vtkObject);
    static TransformGizmo* New();

    TransformGizmo() {
        CreateAxes();
    }

    void Hide() {
        for (int i = 0; i < 3; i++)
            Axes[i]->SetVisibility(0);
    }

    void Show() {
        for (int i = 0; i < 3; i++)
            Axes[i]->SetVisibility(1);
    }

    vtkSmartPointer<vtkActor> getActor(const int& i) { return Axes[i]; }

    void SetTargetActor(vtkSmartPointer<vtkActor> actor) {
        this->TargetActor = actor;
        //UpdateGizmoPosition();
        Rescale(actor);
    }

    std::array<vtkSmartPointer<vtkActor>, 3> GetAxes() const { return Axes; }

    void AddToRenderer(vtkSmartPointer<vtkRenderer> renderer) {
        for (auto& axis : Axes)
            renderer->AddActor(axis);
    }

    void RemoveFromRenderer(vtkSmartPointer<vtkRenderer> renderer) {
        for (auto& axis : Axes)
            renderer->RemoveActor(axis);
    }

    void UpdateGizmoPosition() {
        if (!TargetActor) return;

        double* pos = TargetActor->GetPosition();
        for (auto& axis : Axes)
            axis->SetPosition(pos);
    }

    void SetVisible(bool visible) {
        for (auto& axis : Axes)
            axis->SetVisibility(visible);
    }

    bool IsVisible() const {
        return Axes[0]->GetVisibility();
    }

    void Rescale(vtkActor* targetActor);



void updateAxis(vtkPolyData *m_polydata) {

    if (!TargetActor && !m_polydata) return;

    // Calcular el centro de referencia
    double center[3] = {0.0, 0.0, 0.0};

    if (m_polydata) {
        double bounds[6];
        m_polydata->GetBounds(bounds);
        center[0] = (bounds[0] + bounds[1]) / 2.0;
        center[1] = (bounds[2] + bounds[3]) / 2.0;
        center[2] = (bounds[4] + bounds[5]) / 2.0;
    } else if (TargetActor) {
        double* pos = TargetActor->GetPosition();
        center[0] = pos[0];
        center[1] = pos[1];
        center[2] = pos[2];
    }

    // Actualizar posición de cada eje
    for (int i = 0; i < 3; ++i) {
        vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();

        // Aplicar posición central
        transform->Translate(center);

        // Mantener orientación del cilindro
        //~ if (i == 0) transform->RotateZ(90);   // X
        //~ if (i == 2) transform->RotateX(90);   // Z
        //~ // Y ya apunta a Y por defecto
        double length = 2.0;
        if (i == 0) {
            //transform->RotateZ(90);
            //transform->RotateZ(90);
            //transform->Translate(length/2.0, 0, 0);
        } else if (i == 1) {
            transform->Translate(0, 1.0, 0);
        } else if (i == 2) {
            //transform->RotateX(90);
            transform->Translate(0, 2.0, 0.0);
        }

        vtkSmartPointer<vtkTransformPolyDataFilter> tf = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        tf->SetTransform(transform);
        tf->SetInputConnection(AxisSources[i]->GetOutputPort());
        tf->Update();

        Axes[i]->GetMapper()->SetInputConnection(tf->GetOutputPort());
        Axes[i]->Modified();  // fuerza actualización
    }
}


private:
void CreateAxes() {
    const std::array<std::array<double, 3>, 3> colors = {{
        {1.0, 0.0, 0.0}, // X: rojo
        {0.0, 1.0, 0.0}, // Y: verde
        {0.0, 0.0, 1.0}  // Z: azul
    }};

    double length = 2.0;
    double radius = 0.02;

    for (int i = 0; i < 3; ++i) {
        AxisSources[i] = vtkSmartPointer<vtkCylinderSource>::New();
        AxisSources[i]->SetHeight(length);
        AxisSources[i]->SetRadius(radius);
        AxisSources[i]->SetResolution(32);
        AxisSources[i]->CappingOn();
        AxisSources[i]->Update();

        vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();

        // Orientar cilindro según eje
        if (i == 0) transform->RotateZ(90); // X
        else if (i == 2) transform->RotateX(90); // Z

        // Aplicar posición inicial del TargetActor si existe
        if (TargetActor) {
            double* pos = TargetActor->GetPosition();
            transform->Translate(pos);
            
        }

        vtkSmartPointer<vtkTransformPolyDataFilter> tf = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        tf->SetTransform(transform);
        tf->SetInputConnection(AxisSources[i]->GetOutputPort());
        tf->Update();

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(tf->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(colors[i].data()[0], colors[i].data()[1], colors[i].data()[2]);
        actor->PickableOn();
        actor->DragableOn();

        Axes[i] = actor;
        
        Axes[i]->GetProperty()->SetOpacity(0.5);
    }
}


private:
    std::array<vtkSmartPointer<vtkActor>, 3> Axes;
    std::array<vtkSmartPointer<vtkCylinderSource>, 3> AxisSources;
    vtkSmartPointer<vtkActor> TargetActor;
};

TransformGizmo* TransformGizmo::New() {
    auto newGizmo = new TransformGizmo;
    return newGizmo;
}

void TransformGizmo::Rescale(vtkActor* targetActor) {
    if (!targetActor) return;

    double bounds[6];
    targetActor->GetBounds(bounds);
    double maxDim = std::max({bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]});

    double scaleLength = maxDim * 2.0;
    double scaleRadius = maxDim * 0.05;
    cout << "MaxDim: "<< maxDim<<endl;

    for (int i = 0; i < 3; i++) {
        AxisSources[i]->SetHeight(scaleLength);
        AxisSources[i]->SetRadius(scaleRadius);
        AxisSources[i]->Update();
    }
}


//~ vtkSmartPointer<vtkConeSource> cone = vtkSmartPointer<vtkConeSource>::New();
//~ cone->SetHeight(scaleLength * 0.2);
//~ cone->SetRadius(scaleRadius * 2);
//~ cone->SetResolution(32);


//////////////////////////////////



class GizmoInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
    static GizmoInteractorStyle* New();
    vtkTypeMacro(GizmoInteractorStyle, vtkInteractorStyleTrackballCamera);

    void SetTargetActor(vtkSmartPointer<vtkActor> actor) { 
        this->TargetActor = actor; 
    }

    void SetPolyData(vtkSmartPointer<vtkPolyData> pd) { 
        this->m_polydata = pd; 
    }
    
    void SetPart(Part* pt){
      this->m_part = pt;
      
      }
        
    void SetGizmoAxes(std::array<vtkSmartPointer<vtkActor>, 3> axes) { 
        this->Axes = axes; 
    }

    void SetTransformGizmo(vtkSmartPointer<TransformGizmo> gizmo) {
        this->Gizmo = gizmo;
    }
    
    void OnLeftButtonDown() override {

        int closestAxis = -1;


          int* clickPos = this->GetInteractor()->GetEventPosition();
          this->ClickPos[0] = clickPos[0];  // guardar posición inicial
          this->ClickPos[1] = clickPos[1];

          vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
          picker->SetTolerance(0.05); // ajustable        
          picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());

          vtkActor* pickedActor = picker->GetActor();
          if (!pickedActor) {
              std::cout << "Not edge selected" << std::endl;
              this->SelectedAxis = -1;
          } else {
              // Comprobar cuál de los ejes es
              for (int i = 0; i < 3; ++i) {
                  if (pickedActor == Axes[i]) {
                      this->SelectedAxis = i;
                      std::cout << "Axis selected: " << (i==0?"X":i==1?"Y":"Z") << std::endl;
                      break;
                  }
              }
              
              std::cout << "Pick Position Z: " << picker->GetPickPosition()[2] << std::endl;


          for (int i = 0; i < 3; ++i) {
              if (pickedActor == Axes[i]) {
                  SelectedAxis = i;
                  Axes[i]->GetProperty()->SetLineWidth(8.0);
                  Axes[i]->GetProperty()->SetOpacity(1.0);
                  std::cout << "Axis selected: " << (i==0?"X":i==1?"Y":"Z") << std::endl;
                  break;
              }
          }

          }
 
 
          if (!pickedActor) {
              double clickWorld[3];
              //this->ComputeDisplayToWorld(clickPos, clickWorld);
              vtkRenderer* ren = this->GetDefaultRenderer();

              //~ double displayCoords[3];
              //~ displayCoords[0] = clickPos[0];
              //~ displayCoords[1] = clickPos[1];

              //~ double displayZ = ren->GetZ (clickPos[0], clickPos[1]); // <- verdadero Z de la cámara

              //~ ren->SetDisplayPoint(displayCoords[0], displayCoords[1], displayZ);
              //~ ren->DisplayToWorld();
              //~ double worldPoint[4];
              //~ ren->GetWorldPoint(worldPoint);

              //~ if (worldPoint[3] != 0.0) {
                  //~ clickWorld[0] = worldPoint[0] / worldPoint[3];
                  //~ clickWorld[1] = worldPoint[1] / worldPoint[3];
                  //~ clickWorld[2] = worldPoint[2] / worldPoint[3];
              //~ }

              //~ double minDist = 0.1; // ajustable según escala
              //~ int closest = -1;

              //~ for (int i = 0; i < 3; ++i) {
                  //~ double start[3], end[3];
                  //~ Axes[i]->GetPosition(start);
                  //~ double dir[3] = {0,0,0};
                  //~ dir[i] = 1.0; // X, Y, Z

                  //~ end[0] = start[0] + dir[0]*2.0;
                  //~ end[1] = start[1] + dir[1]*2.0;
                  //~ end[2] = start[2] + dir[2]*2.0;
                  
                  //~ double d = DistanceToLineSegment(clickWorld, start, end);
                  //~ if (d < minDist) {
                      //~ minDist = d;
                      //~ closest = i;
                  //~ }
              //~ }
              //~ cout << "Click World "<<clickWorld[0]<<", "<<clickWorld[1]<<", "<<clickWorld[2]<<endl;
              //~ if (closest != -1) {
                  //~ SelectedAxis = closest;
                  //~ std::cout << "Axis selected (approx): " << (closest==0?"X":closest==1?"Y":"Z") << std::endl;
              //~ }
              vtkSmartPointer<vtkCoordinate> coordinate = vtkSmartPointer<vtkCoordinate>::New();
              coordinate->SetCoordinateSystemToDisplay();
              coordinate->SetValue(clickPos[0], clickPos[1], 0); // z=0 plano frontal de la cámara

              double* world = coordinate->GetComputedWorldValue(this->GetDefaultRenderer());

              clickWorld[0] = world[0];
              clickWorld[1] = world[1];
              clickWorld[2] = world[2];

              std::cout << "Click World: " << clickWorld[0] << ", " 
                        << clickWorld[1] << ", " 
                        << clickWorld[2] << std::endl;
                        } 
                      
        


        ///// Si no se seleccionó ningún eje, comportamiento por defecto
        vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
    }

//~ void OnLeftButtonDown() override {
    //~ int* clickPos = this->GetInteractor()->GetEventPosition();
    //~ this->ClickPos[0] = clickPos[0];
    //~ this->ClickPos[1] = clickPos[1];

    //~ // SOLUTION 1 & 2: Use vtkCellPicker with adjusted tolerance
    //~ vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    //~ picker->SetTolerance(0.1); // Increased tolerance for easier picking

    //~ // SOLUTION 2: Configure picker behavior
    //~ picker->PickFromListOn(); // Optional: Restrict picking to a specific list
    //~ // InitializePickList and AddPickList would be used if you enable PickFromList

    //~ picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());
    //~ vtkActor* pickedActor = picker->GetActor();

    //~ if (!pickedActor) {
        //~ std::cout << "No actor selected" << std::endl;
        //~ this->SelectedAxis = -1;
    //~ } else {
        //~ // Identify which axis was picked
        //~ this->SelectedAxis = -1;
        //~ for (int i = 0; i < 3; ++i) {
            //~ if (pickedActor == this->Axes[i].Get()) { // Use Get() for raw pointer comparison
                //~ this->SelectedAxis = i;
                //~ // Visual feedback for selection
                //~ this->Axes[i]->GetProperty()->SetColor(1.0, 1.0, 0.0); // Highlight in yellow
                //~ std::cout << "Axis selected: " << (i==0?"X":i==1?"Y":"Z") << std::endl;
                //~ break;
            //~ }
        //~ }
        //~ // If the picked actor wasn't a gizmo axis, do nothing or reset selection.
        //~ if (this->SelectedAxis == -1) {
            //~ // Optionally reset any previous axis highlighting here
            //~ std::cout << "Selected actor is not a gizmo axis." << std::endl;
        //~ }
    //~ }

    //~ vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
//~ }

    void OnMouseMove() override {
    // Highlight del eje bajo el cursor (antes de la selección)
    if (this->SelectedAxis == -1) {
        int* pos = this->GetInteractor()->GetEventPosition();
        //~ int closestAxis = FindClosestAxis(pos[0], pos[1]);
        
        //~ // Resetear colores anteriores
        //~ for (int i = 0; i < 3; ++i) {
            //~ if (this->Axes[i]) {
                //~ double color[3] = {i==0?1.0:0.0, i==1?1.0:0.0, i==2?1.0:0.0};
                //~ this->Axes[i]->GetProperty()->SetColor(color);
            //~ }
        //~ }
        
        //~ // Highlight del eje más cercano
        //~ if (closestAxis != -1 && this->Axes[closestAxis]) {
            //~ this->Axes[closestAxis]->GetProperty()->SetColor(1.0, 1.0, 0.0); // Amarillo
            //~ this->GetInteractor()->GetRenderWindow()->Render();
        //~ }
        //
    }
    //std::cout << "Selected Axis: "<< SelectedAxis<<std::endl;
    
    if (this->SelectedAxis >= 0 && this->TargetActor) {
        int* currPos = this->GetInteractor()->GetEventPosition();
        double dx = currPos[0] - this->ClickPos[0];
        double dy = currPos[1] - this->ClickPos[1];

        double translate[3] = {0, 0, 0};
        
        
                
        //~ double movement = (dx + dy) * 0.001;
        //~ translate[this->SelectedAxis] = movement;

          
          
          const std::array<std::array<double, 3>, 3> directions = {{
              {{1.0, 0.0, 0.0}}, // X
              {{0.0, 1.0, 0.0}}, // Y
              {{0.0, 0.0, 1.0}}  // Z
          }};

          vtkSmartPointer<vtkRenderer> renderer = this->GetDefaultRenderer();

          // Obtener vector de movimiento en cámara
          double worldDelta[3];
          renderer->SetDisplayPoint(currPos[0], currPos[1], 0);
          renderer->DisplayToWorld();
          double worldPos1[4]; renderer->GetWorldPoint(worldPos1);

          renderer->SetDisplayPoint(ClickPos[0], ClickPos[1], 0);
          renderer->DisplayToWorld();
          double worldPos0[4]; renderer->GetWorldPoint(worldPos0);

          // Normalizar y proyectar sobre dirección del eje
          double moveVec[3] = {worldPos1[0]-worldPos0[0],
                               worldPos1[1]-worldPos0[1],
                               worldPos1[2]-worldPos0[2]};
          double dot = vtkMath::Dot(moveVec, directions[SelectedAxis]);
          translate[0] = directions[SelectedAxis][0]*dot;
          translate[1] = directions[SelectedAxis][1]*dot;
          translate[2] = directions[SelectedAxis][2]*dot;

        //~ // Aplicar transformación
        vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
        transform->Translate(translate[0],translate[1],translate[2]);
        
        
        //// A. Transformation over the actor
        ///this->TargetActor->SetUserTransform(transform);

        //// B. If moving polyata
        vtkNew<vtkTransformFilter> tf;
        if (m_polydata){

        //IF MOVE THE POLYDATA
        tf->SetInputData(m_polydata);
        tf->SetTransform(transform);
        tf->Update();
        m_polydata->ShallowCopy(tf->GetOutput());


        //this->TargetActor->GetMapper()->SetInputData(m_polydata);
        
        //m_part->getGeom()->Move(movement,0.,0.);
        m_part->getGeom()->Move(translate[0],translate[1],translate[2]);
      } else {
        
        std::cout << "ERROR: NO POLYDATA"<<endl;
        }

        // DEBUG: Verificar después de mover
        double* posAfter = this->TargetActor->GetPosition();
        //~ std::cout << "Position after: " << posAfter[0] << ", " << posAfter[1] << ", " << posAfter[2] << std::endl;
        //~ std::cout << "Transform applied: " << translate[0] << ", " << translate[1] << ", " << translate[2] << std::endl;


        //~ std::cout << TargetActor->GetMapper()->GetClassName() << std::endl;
        //~ std::cout << TargetActor->GetMapper()->GetInput()->GetClassName() << std::endl;

      //~ double origin[3] = {0,0,0};
      //~ double transformed[3];
      //~ TargetActor->GetUserTransform()->TransformPoint(origin, transformed);
      //~ std::cout << "Actual visual position: " 
                //~ << transformed[0] << ", " 
                //~ << transformed[1] << ", " 
                //~ << transformed[2] << std::endl;
                

        this->GetInteractor()->GetRenderWindow()->Render();
    } else {
        vtkInteractorStyleTrackballCamera::OnMouseMove();
    } 
        

    }

    void OnLeftButtonUp() override {
        this->SelectedAxis = -1;
        vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
    }

private:

    vtkSmartPointer<vtkPolyData> m_polydata;
    Part* m_part;
    
    vtkSmartPointer<vtkActor> TargetActor;
    std::array<vtkSmartPointer<vtkActor>, 3> Axes;
    vtkSmartPointer<TransformGizmo> Gizmo;
    int SelectedAxis = -1;
    int LastPos[2];
    int ClickPos[2];
    
};

vtkStandardNewMacro(GizmoInteractorStyle);

#endif
