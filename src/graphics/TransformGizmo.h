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

///// ARROWS: WORKING TRICKY 

//~ class TransformGizmo : public vtkObject {
//~ public:
    //~ vtkTypeMacro(TransformGizmo, vtkObject);
    //~ static TransformGizmo* New();

    //~ TransformGizmo() { CreateAxes(); }
    //~ std::array<vtkSmartPointer<vtkActor>, 3> GetAxes() const { 
        //~ return Axes; 
    //~ }
    
    //~ void Hide() {
        //~ for (int i = 0; i < 3; i++) Axes[i]->SetVisibility(0);
    //~ }

    //~ void Show() {
        //~ for (int i = 0; i < 3; i++) Axes[i]->SetVisibility(1);
    //~ }

    //~ vtkSmartPointer<vtkActor> getActor(const int &i) { return Axes[i]; }

    //~ void SetTargetActor(vtkSmartPointer<vtkActor> actor) { 
        //~ this->TargetActor = actor; 
        //~ UpdateGizmoPosition();
        //~ Rescale(actor);
    //~ }

    //~ void AddToRenderer(vtkSmartPointer<vtkRenderer> renderer) {
        //~ for (auto& axis : Axes) renderer->AddActor(axis);
    //~ }

    //~ void RemoveFromRenderer(vtkSmartPointer<vtkRenderer> renderer) {
        //~ for (auto& axis : Axes) renderer->RemoveActor(axis);
    //~ }

    //~ void UpdateGizmoPosition() {
        //~ if (!TargetActor) return;
        //~ double* pos = TargetActor->GetPosition();
        //~ for (auto& axis : Axes) axis->SetPosition(pos);
    //~ }

    //~ void SetVisible(bool visible) {
        //~ for (auto& axis : Axes) axis->SetVisibility(visible);
    //~ }

    //~ bool IsVisible() const { return Axes[0]->GetVisibility(); }

    //~ void Rescale(vtkActor* targetActor) {
        //~ if (!targetActor) return;

        //~ double bounds[6];
        //~ targetActor->GetBounds(bounds);
        //~ double maxDim = std::max({bounds[1]-bounds[0], bounds[3]-bounds[2], bounds[5]-bounds[4]});
        //~ double scale = maxDim * 1.5; // escala relativa al tamaño del objeto

        //~ for (auto& axis : Axes) {
            //~ axis->SetScale(scale);
        //~ }
    //~ }

//~ private:
    //~ void CreateAxes() {
        //~ const std::array<std::array<double, 3>, 3> colors = {
            //~ std::array<double, 3>{1.0, 0.0, 0.0}, // X (rojo)
            //~ std::array<double, 3>{0.0, 1.0, 0.0}, // Y (verde)
            //~ std::array<double, 3>{0.0, 0.0, 1.0}  // Z (azul)
        //~ };

        //~ // Crear una sola flecha base
        //~ vtkSmartPointer<vtkArrowSource> arrow = vtkSmartPointer<vtkArrowSource>::New();
        //~ arrow->SetTipLength(0.25);
        //~ arrow->SetTipRadius(0.05);
        //~ arrow->SetShaftRadius(0.02);
        //~ arrow->Update();

        //~ // Crear transformaciones para cada eje
        //~ std::array<vtkSmartPointer<vtkTransform>, 3> transforms;

        //~ // X: no se rota
        //~ transforms[0] = vtkSmartPointer<vtkTransform>::New();

        //~ // Y: rotar flecha de +X a +Y
        //~ transforms[1] = vtkSmartPointer<vtkTransform>::New();
        //~ transforms[1]->RotateZ(90);

        //~ // Z: rotar flecha de +X a +Z
        //~ transforms[2] = vtkSmartPointer<vtkTransform>::New();
        //~ transforms[2]->RotateY(-90);

        //~ // Crear actores
        //~ for (int i = 0; i < 3; ++i) {
            //~ vtkSmartPointer<vtkTransformPolyDataFilter> tf = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
            //~ tf->SetTransform(transforms[i]);
            //~ tf->SetInputConnection(arrow->GetOutputPort());
            //~ tf->Update();

            //~ vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            //~ mapper->SetInputConnection(tf->GetOutputPort());

            //~ vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            //~ actor->SetMapper(mapper);
            //~ actor->GetProperty()->SetColor(colors[i][0], colors[i][1], colors[i][2]);
            //~ actor->SetScale(1.0);

            //~ Axes[i] = actor;
        //~ }
    //~ }

//~ private:
    //~ std::array<vtkSmartPointer<vtkActor>, 3> Axes;
    //~ vtkSmartPointer<vtkActor> TargetActor;
//~ };


// Implement the New() method and Initialize()
//~ TransformGizmo* TransformGizmo::New() {
    //~ auto newGizmo = new TransformGizmo;
    //~ //newGizmo->Initialize();
    //~ return newGizmo;
//~ }




//////// LINES:THIS WORKS

    
//~ class TransformGizmo : public vtkObject  // Inherit from vtkObject
//~ {
//~ public:
    //~ // Add VTK type macros
    //~ vtkTypeMacro(TransformGizmo, vtkObject);
    
    //~ // Replace constructor with static New() method
    //~ static TransformGizmo* New();
    
    //~ // Initialize method for setup
    //~ void Initialize();
    
    //~ TransformGizmo() {
        //~ CreateAxes();
    //~ }
    
    //~ void Hide(){
      //~ for (int i=0;i<3;i++)Axes[i]->SetVisibility(0);
      
      //~ }

    //~ void Show(){
      //~ for (int i=0;i<3;i++)Axes[i]->SetVisibility(1);
      
      //~ }
          
    //~ vtkSmartPointer<vtkActor> getActor(const int &i){return Axes[i];}
    

    //~ void SetTargetActor(vtkSmartPointer<vtkActor> actor) { 
        //~ this->TargetActor = actor; 
        //~ UpdateGizmoPosition();
    //~ }

    //~ std::array<vtkSmartPointer<vtkActor>, 3> GetAxes() const { 
        //~ return Axes; 
    //~ }

    //~ void AddToRenderer(vtkSmartPointer<vtkRenderer> renderer) {
        //~ for (auto& axis : Axes) {
            //~ renderer->AddActor(axis);
        //~ }
    //~ }

    //~ void RemoveFromRenderer(vtkSmartPointer<vtkRenderer> renderer) {
        //~ for (auto& axis : Axes) {
            //~ renderer->RemoveActor(axis);
        //~ }
    //~ }

    //~ void UpdateGizmoPosition() {
        //~ if (!TargetActor) return;
        
        //~ double* position = TargetActor->GetPosition();
        //~ for (auto& axis : Axes) {
            //~ axis->SetPosition(position);
        //~ }
    //~ }

    //~ void SetVisible(bool visible) {
        //~ for (auto& axis : Axes) {
            //~ axis->SetVisibility(visible);
        //~ }
    //~ }

    //~ bool IsVisible() const {
        //~ return Axes[0]->GetVisibility();
    //~ }
    
    //~ void Rescale(vtkActor* targetActor);

//~ private:
    //~ void CreateAxes() {
        //~ const std::array<std::array<double, 3>, 3> colors = {
            //~ std::array<double, 3>{1.0, 0.0, 0.0}, // Rojo (X)
            //~ std::array<double, 3>{0.0, 1.0, 0.0}, // Verde (Y)  
            //~ std::array<double, 3>{0.0, 0.0, 1.0}  // Azul (Z)
        //~ };

        //~ const std::array<std::array<double, 3>, 3> directions = {
            //~ std::array<double, 3>{1.0, 0.0, 0.0}, // X
            //~ std::array<double, 3>{0.0, 1.0, 0.0}, // Y
            //~ std::array<double, 3>{0.0, 0.0, 1.0}  // Z
        //~ };

        //~ for (int i = 0; i < 3; ++i) {
            //~ AxisSources[i] = vtkSmartPointer<vtkLineSource>::New();
            //~ AxisSources[i]->SetPoint1(0.0, 0.0, 0.0);
            //~ AxisSources[i]->SetPoint2(directions[i][0] * 2.0,
                                      //~ directions[i][1] * 2.0,
                                      //~ directions[i][2] * 2.0);

            //~ vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            //~ mapper->SetInputConnection(AxisSources[i]->GetOutputPort());

            //~ vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            //~ actor->SetMapper(mapper);
            //~ actor->GetProperty()->SetColor(colors[i][0], colors[i][1], colors[i][2]);
            //~ actor->GetProperty()->SetLineWidth(10.0);

            //~ Axes[i] = actor;
        //~ }

    //~ }

    //~ std::array<vtkSmartPointer<vtkActor>, 3> Axes;
    //~ vtkSmartPointer<vtkActor> TargetActor;
//~ std::array<vtkSmartPointer<vtkLineSource>, 3> AxisSources;

//~ };


//~ void TransformGizmo::Rescale(vtkActor* targetActor) {
    //~ if (!targetActor) return;

    //~ double bounds[6];
    //~ targetActor->GetBounds(bounds);
    //~ double maxDim = std::max({bounds[1]-bounds[0], bounds[3]-bounds[2], bounds[5]-bounds[4]});
    //~ double scale = maxDim * 0.5;

    //~ std::array<std::array<double, 3>, 3> directions = {{
        //~ {{1.0,0.0,0.0}}, {{0.0,1.0,0.0}}, {{0.0,0.0,1.0}}
    //~ }};

    //~ for (int i = 0; i < 3; i++) {
        //~ AxisSources[i]->SetPoint2(
            //~ directions[i][0]*scale,
            //~ directions[i][1]*scale,
            //~ directions[i][2]*scale
        //~ );
        //~ AxisSources[i]->Update();
    //~ }
//~ }


// Implement the New() method and Initialize()
//~ TransformGizmo* TransformGizmo::New() {
    //~ auto newGizmo = new TransformGizmo;
    //~ //newGizmo->Initialize();
    //~ return newGizmo;
//~ }

//~ void TransformGizmo::Initialize() {
    //~ CreateAxes();
//~ }


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
        UpdateGizmoPosition();
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

private:
    void CreateAxes() {
        const std::array<std::array<double, 3>, 3> colors = {{
            {1.0, 0.0, 0.0}, // X: rojo
            {0.0, 1.0, 0.0}, // Y: verde
            {0.0, 0.0, 1.0}  // Z: azul
        }};

        // Longitud inicial de los ejes
        double length = 2.0;
        double radius = 0.05; // grosor visible

        for (int i = 0; i < 3; ++i) {
            AxisSources[i] = vtkSmartPointer<vtkCylinderSource>::New();
            AxisSources[i]->SetHeight(length);
            AxisSources[i]->SetRadius(radius);
            AxisSources[i]->SetResolution(32);
            AxisSources[i]->CappingOn();
            AxisSources[i]->Update();

            // Crear transformaciones para orientar cada cilindro
            vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();

            if (i == 0) { // X
                transform->RotateZ(90); // cilindro apunta a X
            } else if (i == 1) { // Y
                // Ya apunta a Y por defecto
            } else if (i == 2) { // Z
                transform->RotateX(90);
            }

            vtkSmartPointer<vtkTransformPolyDataFilter> tf = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
            tf->SetTransform(transform);
            tf->SetInputConnection(AxisSources[i]->GetOutputPort());
            tf->Update();

            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(tf->GetOutputPort());

            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            actor->GetProperty()->SetColor(colors[i].data()[0],colors[i].data()[1],colors[i].data()[2]);
            actor->PickableOn();
            actor->DragableOn();

            Axes[i] = actor;
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

    double scaleLength = maxDim * 1.5;
    double scaleRadius = maxDim * 0.06;

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
        //~ int *clickPos = this->GetInteractor()->GetEventPosition();
        //~ this->ClickPos[0] = clickPos[0];  // ✅ guardar posición inicial
        //~ this->ClickPos[1] = clickPos[1];
        
        //~ this->SelectedAxis = -1;

        //~ // Convertir coordenadas de pantalla a mundo 3D
        //~ vtkRenderer* renderer = this->GetDefaultRenderer();
        //~ if (!renderer) {
            //~ vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
            //~ return;
        //~ }

        //~ // Obtener el rayo desde la cámara
        //~ double worldPos[4];
        //~ double displayPos[3] = {static_cast<double>(clickPos[0]), 
                               //~ static_cast<double>(clickPos[1]), 0.0};
        
        //~ renderer->SetDisplayPoint(displayPos);
        //~ renderer->DisplayToWorld();
        //~ renderer->GetWorldPoint(worldPos);

        //~ if (worldPos[3] != 0.0) {
            //~ worldPos[0] /= worldPos[3];
            //~ worldPos[1] /= worldPos[3];
            //~ worldPos[2] /= worldPos[3];
        //~ }

        //~ // Encontrar el eje más cercano al rayo
        //~ double minDistance = 0.1; // Umbral de distancia (ajustable)
        int closestAxis = -1;

        //~ for (int i = 0; i < 3; ++i) {
            //~ if (!this->Axes[i]) continue;

            //~ // Obtener posición y dirección del eje
            //~ double* axisStart = this->Axes[i]->GetPosition();
            //~ double axisEnd[3];
            
            //~ // Calcular el punto final del eje (asumiendo longitud 2.0 como en tu código)
            //~ const std::array<std::array<double, 3>, 3> directions = {
                //~ std::array<double, 3>{2.0, 0.0, 0.0}, // X
                //~ std::array<double, 3>{0.0, 2.0, 0.0}, // Y
                //~ std::array<double, 3>{0.0, 0.0, 2.0}  // Z
            //~ };
            
            //~ axisEnd[0] = axisStart[0] + directions[i][0];
            //~ axisEnd[1] = axisStart[1] + directions[i][1];
            //~ axisEnd[2] = axisStart[2] + directions[i][2];

            //~ // Calcular distancia del rayo al segmento del eje
            //~ double distance = DistanceToLineSegment(worldPos, axisStart, axisEnd);
            
            //~ if (distance < minDistance) {
                //~ minDistance = distance;
                //~ closestAxis = i;
            //~ }
        //~ }
        
        
        
          vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
          picker->SetTolerance(0.1); // ajustable

          int* clickPos = this->GetInteractor()->GetEventPosition();
          this->ClickPos[0] = clickPos[0];  // guardar posición inicial
          this->ClickPos[1] = clickPos[1];
        
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
          }
        
        


        // Si no se seleccionó ningún eje, comportamiento por defecto
        vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
    }

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

        //~ double dx = currPos[0] - this->LastPos[0];
        //~ double dy = currPos[1] - this->LastPos[1];
                
        double movement = (dx + dy) * 0.001;
        double translate[3] = {0, 0, 0};
        translate[this->SelectedAxis] = movement;

        // DEBUG: Verificar el estado antes de mover
        //~ std::cout << "=== BEFORE TRANSFORM ===" << std::endl;
        //~ std::cout << "TargetActor: " << this->TargetActor << std::endl;
        //~ std::cout << "Actor visibility: " << this->TargetActor->GetVisibility() << std::endl;
        

        // Aplicar transformación
        vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
        transform->Translate(translate);
        
        //// A. Transformation over the actor
        ///this->TargetActor->SetUserTransform(transform);

        //// B. If moving polyata
        vtkNew<vtkTransformFilter> tf;
        if (m_polydata){

        tf->SetInputData(m_polydata);
        tf->SetTransform(transform);
        tf->Update();

        m_polydata->ShallowCopy(tf->GetOutput());
        //this->TargetActor->GetMapper()->SetInputData(m_polydata);
        
        m_part->getGeom()->Move(movement,0.,0.);
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
