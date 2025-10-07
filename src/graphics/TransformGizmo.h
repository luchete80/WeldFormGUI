#ifndef _TRANSFORMGIZMO_H_
#define _TRANSFORMGIZMO_H_


#include <vtkLineSource.h>
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
    
class TransformGizmo : public vtkObject  // Inherit from vtkObject
{
public:
    // Add VTK type macros
    vtkTypeMacro(TransformGizmo, vtkObject);
    
    // Replace constructor with static New() method
    static TransformGizmo* New();
    
    // Initialize method for setup
    void Initialize();
    
    TransformGizmo() {
        CreateAxes();
    }
    
    vtkSmartPointer<vtkActor> getActor(const int &i){return Axes[i];}
    

    void SetTargetActor(vtkSmartPointer<vtkActor> actor) { 
        this->TargetActor = actor; 
        UpdateGizmoPosition();
    }

    std::array<vtkSmartPointer<vtkActor>, 3> GetAxes() const { 
        return Axes; 
    }

    void AddToRenderer(vtkSmartPointer<vtkRenderer> renderer) {
        for (auto& axis : Axes) {
            renderer->AddActor(axis);
        }
    }

    void RemoveFromRenderer(vtkSmartPointer<vtkRenderer> renderer) {
        for (auto& axis : Axes) {
            renderer->RemoveActor(axis);
        }
    }

    void UpdateGizmoPosition() {
        if (!TargetActor) return;
        
        double* position = TargetActor->GetPosition();
        for (auto& axis : Axes) {
            axis->SetPosition(position);
        }
    }

    void SetVisible(bool visible) {
        for (auto& axis : Axes) {
            axis->SetVisibility(visible);
        }
    }

    bool IsVisible() const {
        return Axes[0]->GetVisibility();
    }

private:
    void CreateAxes() {
        const std::array<std::array<double, 3>, 3> colors = {
            std::array<double, 3>{1.0, 0.0, 0.0}, // Rojo (X)
            std::array<double, 3>{0.0, 1.0, 0.0}, // Verde (Y)  
            std::array<double, 3>{0.0, 0.0, 1.0}  // Azul (Z)
        };

        const std::array<std::array<double, 3>, 3> directions = {
            std::array<double, 3>{1.0, 0.0, 0.0}, // X
            std::array<double, 3>{0.0, 1.0, 0.0}, // Y
            std::array<double, 3>{0.0, 0.0, 1.0}  // Z
        };

        for (int i = 0; i < 3; ++i) {
            vtkSmartPointer<vtkLineSource> axisSource = vtkSmartPointer<vtkLineSource>::New();
            axisSource->SetPoint1(0.0, 0.0, 0.0);
            axisSource->SetPoint2(directions[i][0] * 2.0, directions[i][1] * 2.0, directions[i][2] * 2.0); // Ejes más largos

            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(axisSource->GetOutputPort());

            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            actor->GetProperty()->SetColor(colors[i][0], colors[i][1], colors[i][2]);
            actor->GetProperty()->SetLineWidth(3.0); // Líneas más gruesas

            Axes[i] = actor;
        }
    }

    std::array<vtkSmartPointer<vtkActor>, 3> Axes;
    vtkSmartPointer<vtkActor> TargetActor;
};


// Implement the New() method and Initialize()
TransformGizmo* TransformGizmo::New() {
    auto newGizmo = new TransformGizmo;
    newGizmo->Initialize();
    return newGizmo;
}

void TransformGizmo::Initialize() {
    CreateAxes();
}

class GizmoInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
    static GizmoInteractorStyle* New();
    vtkTypeMacro(GizmoInteractorStyle, vtkInteractorStyleTrackballCamera);

    void SetTargetActor(vtkSmartPointer<vtkActor> actor) { 
        this->TargetActor = actor; 
    }
    
    void SetGizmoAxes(std::array<vtkSmartPointer<vtkActor>, 3> axes) { 
        this->Axes = axes; 
    }

    void SetTransformGizmo(vtkSmartPointer<TransformGizmo> gizmo) {
        this->Gizmo = gizmo;
    }
    
    void OnLeftButtonDown() override {
        int* clickPos = this->GetInteractor()->GetEventPosition();
        this->SelectedAxis = -1;

        // Convertir coordenadas de pantalla a mundo 3D
        vtkRenderer* renderer = this->GetDefaultRenderer();
        if (!renderer) {
            vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
            return;
        }

        // Obtener el rayo desde la cámara
        double worldPos[4];
        double displayPos[3] = {static_cast<double>(clickPos[0]), 
                               static_cast<double>(clickPos[1]), 0.0};
        
        renderer->SetDisplayPoint(displayPos);
        renderer->DisplayToWorld();
        renderer->GetWorldPoint(worldPos);

        if (worldPos[3] != 0.0) {
            worldPos[0] /= worldPos[3];
            worldPos[1] /= worldPos[3];
            worldPos[2] /= worldPos[3];
        }

        // Encontrar el eje más cercano al rayo
        double minDistance = 0.1; // Umbral de distancia (ajustable)
        int closestAxis = -1;

        for (int i = 0; i < 3; ++i) {
            if (!this->Axes[i]) continue;

            // Obtener posición y dirección del eje
            double* axisStart = this->Axes[i]->GetPosition();
            double axisEnd[3];
            
            // Calcular el punto final del eje (asumiendo longitud 2.0 como en tu código)
            const std::array<std::array<double, 3>, 3> directions = {
                std::array<double, 3>{2.0, 0.0, 0.0}, // X
                std::array<double, 3>{0.0, 2.0, 0.0}, // Y
                std::array<double, 3>{0.0, 0.0, 2.0}  // Z
            };
            
            axisEnd[0] = axisStart[0] + directions[i][0];
            axisEnd[1] = axisStart[1] + directions[i][1];
            axisEnd[2] = axisStart[2] + directions[i][2];

            // Calcular distancia del rayo al segmento del eje
            double distance = DistanceToLineSegment(worldPos, axisStart, axisEnd);
            
            if (distance < minDistance) {
                minDistance = distance;
                closestAxis = i;
            }
        }

        if (closestAxis != -1) {
            this->SelectedAxis = closestAxis;
            std::cout << "Eje seleccionado (proximidad): " << (closestAxis==0?"X":closestAxis==1?"Y":"Z") 
                      << " - Distancia: " << minDistance << std::endl;
            
            this->LastPos[0] = clickPos[0];
            this->LastPos[1] = clickPos[1];
            
            // No llamar al comportamiento por defecto - estamos manipulando el gizmo
            return;
        }else {
          
          std::cout << "Not edge selected"<<std::endl;
          SelectedAxis = 0;
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
            double dx = currPos[0] - this->LastPos[0];
            double dy = currPos[1] - this->LastPos[1];
            
            cout << "dx: "<<dx<<endl;

            // Movimiento más suave
            double movement = (dx + dy) * 0.005;

            double translate[3] = {0, 0, 0};
            translate[this->SelectedAxis] = movement;

            // Aplicar transformación
            vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
            transform->Translate(translate);
            this->TargetActor->SetUserTransform(transform);

            // Actualizar posición del gizmo
            if (this->Gizmo) {
                this->Gizmo->UpdateGizmoPosition();
            }

            this->LastPos[0] = currPos[0];
            this->LastPos[1] = currPos[1];
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

    
    vtkSmartPointer<vtkActor> TargetActor;
    std::array<vtkSmartPointer<vtkActor>, 3> Axes;
    vtkSmartPointer<TransformGizmo> Gizmo;
    int SelectedAxis = -1;
    int LastPos[2];
};

vtkStandardNewMacro(GizmoInteractorStyle);

#endif

///// USAGE 

//~ // Crear y configurar el gizmo
//~ vtkSmartPointer<TransformGizmo> gizmo = vtkSmartPointer<TransformGizmo>::New();
//~ gizmo->SetTargetActor(mesh_actor);
//~ gizmo->AddToRenderer(renderer);

//~ // Configurar el estilo de interacción
//~ vtkSmartPointer<GizmoInteractorStyle> style = vtkSmartPointer<GizmoInteractorStyle>::New();
//~ style->SetDefaultRenderer(renderer);
//~ style->SetTargetActor(mesh_actor);
//~ style->SetGizmoAxes(gizmo->GetAxes());
//~ style->SetTransformGizmo(gizmo); // Nueva integración

//~ interactor->SetInteractorStyle(style);
