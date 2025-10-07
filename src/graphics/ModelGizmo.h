#ifndef _MODELGIZMO_H_
#define _MODELGIZMO_H_


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

#include "../graphicmesh/GraphicMesh.h"
#include "../model/Model.h"
#include "../model/Node.h"

class ModelAwareGizmo : public vtkObject {
public:
    vtkTypeMacro(ModelAwareGizmo, vtkObject);
    static ModelAwareGizmo* New();
    
    void SetTargetGraphicMesh(GraphicMesh* graphicMesh) {
        this->TargetGraphicMesh = graphicMesh;
        UpdateGizmoPosition();
    }
    
    void SetModel(Model* model) {
        this->m_model = model;
    }
    
    void UpdateGizmoPosition() {
        if (!TargetGraphicMesh) return;
        
        Vector3f position = TargetGraphicMesh->GetPosition();
        for (auto& axis : Axes) {
            axis->SetPosition(position.x, position.y, position.z);
        }
    }
    
    // Método para verificar colisiones con todas las partes del modelo
    bool CheckCollisionWithModel(const Vector3f& displacement) {
        if (!TargetGraphicMesh || !m_model) return false;
        
        Mesh* movingMesh = TargetGraphicMesh->getMesh();
        if (!movingMesh) return false;
        
        // Verificar colisión con todas las otras partes
        for (int i = 0; i < m_model->getPartCount(); i++) {
            Part* otherPart = m_model->getPart(i);
            if (!otherPart->isMeshed()) continue;
            
            // Obtener el GraphicMesh correspondiente a esta parte
            GraphicMesh* otherGraphicMesh = FindGraphicMeshForPart(otherPart);
            if (!otherGraphicMesh || otherGraphicMesh == TargetGraphicMesh) continue;
            
            if (CheckMeshCollision(movingMesh, otherGraphicMesh->getMesh(), displacement)) {
                return true;
            }
        }
        return false;
    }
    
    GraphicMesh* FindGraphicMeshForPart(Part* part) {
        // Necesitas implementar esta función para mapear Part -> GraphicMesh
        // Esto depende de cómo manejes tu vector de GraphicMesh
        for (auto graphicMesh : m_graphicMeshes) {
            if (graphicMesh->getMesh() == part->getMesh()) {
                return graphicMesh;
            }
        }
        return nullptr;
    }
    
    void SetGraphicMeshes(const std::vector<GraphicMesh*>& graphicMeshes) {
        m_graphicMeshes = graphicMeshes;
    }

private:
    GraphicMesh* TargetGraphicMesh;
    Model* m_model;
    std::vector<GraphicMesh*> m_graphicMeshes;
    std::array<vtkSmartPointer<vtkActor>, 3> Axes;
    
    bool CheckMeshCollision(Mesh* mesh1, Mesh* mesh2, const Vector3f& displacement) {
        // Implementar detección de colisión mesh-mesh
        // Versión simplificada: verificar distancia entre nodos
        for (int i = 0; i < mesh1->getNodeCount(); i++) {
            Node* node1 = mesh1->getNode(i);
            Vector3f futurePos = node1->getPos() + displacement;
            
            for (int j = 0; j < mesh2->getNodeCount(); j++) {
                Node* node2 = mesh2->getNode(j);
                float distance = (futurePos - node2->getPos()).norm();
                if (distance < 0.01f) { // Umbral de colisión
                    return true;
                }
            }
        }
        return false;
    }
};

class ModelGizmoInteractorStyle : public vtkInteractorStyleTrackballCamera {
public:
    static ModelGizmoInteractorStyle* New();
    vtkTypeMacro(ModelGizmoInteractorStyle, vtkInteractorStyleTrackballCamera);

    void SetModelAwareGizmo(vtkSmartPointer<ModelAwareGizmo> gizmo) {
        this->Gizmo = gizmo;
    }
    
    void SetGraphicMeshes(const std::vector<GraphicMesh*>& graphicMeshes) {
        m_graphicMeshes = graphicMeshes;
        if (Gizmo) {
            Gizmo->SetGraphicMeshes(graphicMeshes);
        }
    }
    
    void SetModel(Model* model) {
        m_model = model;
        if (Gizmo) {
            Gizmo->SetModel(model);
        }
    }
    
    void OnLeftButtonDown() override {
        int* clickPos = this->GetInteractor()->GetEventPosition();

        
        // Primero verificar si se clickeó en un GraphicMesh
        GraphicMesh* clickedMesh = FindGraphicMeshAtPosition(clickPos);
        if (clickedMesh) {
            this->SelectedGraphicMesh = clickedMesh;
            this->Gizmo->SetTargetGraphicMesh(clickedMesh);
            
            // Resto de la lógica de selección de eje...
            this->ClickPos[0] = clickPos[0];  // guardar posición inicial
            this->ClickPos[1] = clickPos[1];
            this->SelectedAxis = FindClosestAxis(clickPos);
            return;
        }
        
        vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
    }
    
    void OnMouseMove() override {
        if (this->SelectedAxis >= 0 && this->SelectedGraphicMesh && this->Gizmo) {
            int* currPos = this->GetInteractor()->GetEventPosition();
            
            double dx = currPos[0] - this->ClickPos[0];
            double dy = currPos[1] - this->ClickPos[1];
            
            double movement = (dx + dy) * 0.005;
            
            Vector3f displacement(0, 0, 0);
            displacement[this->SelectedAxis] = movement;
            
            // Verify colisión ANTES de mover
            if (!this->Gizmo->CheckCollisionWithModel(displacement)) {
                // Aplicar transformación al GraphicMesh
                vtkSmartPointer<vtkTransform> transform = this->SelectedGraphicMesh->GetTransform();
                if (!transform) {
                    transform = vtkSmartPointer<vtkTransform>::New();
                    this->SelectedGraphicMesh->SetTransform(transform);
                }
                
                double translate[3] = {0, 0, 0};
                translate[this->SelectedAxis] = movement;
                transform->Translate(translate);
                
                // Actualizar la representación visual
                this->SelectedGraphicMesh->SetTransform(transform);
                
                // Actualizar gizmo
                this->Gizmo->UpdateGizmoPosition();
                
                // Acumular desplazamiento
                this->TotalDisplacement[this->SelectedAxis] += movement;
                
                this->GetInteractor()->GetRenderWindow()->Render();
            }
        } else {
            vtkInteractorStyleTrackballCamera::OnMouseMove();
        }
    }

private:
    GraphicMesh* FindGraphicMeshAtPosition(int* screenPos) {
        // Implementar picking para encontrar qué GraphicMesh fue clickeado
        vtkRenderer* renderer = this->GetDefaultRenderer();
        vtkSmartPointer<vtkPropPicker> picker = vtkSmartPointer<vtkPropPicker>::New();
        
        picker->Pick(screenPos[0], screenPos[1], 0, renderer);
        vtkActor* pickedActor = picker->GetActor();
        
        if (pickedActor) {
            for (auto graphicMesh : m_graphicMeshes) {
                if (graphicMesh->getActor() == pickedActor) {
                    return graphicMesh;
                }
            }
        }
        return nullptr;
    }
    
    vtkSmartPointer<ModelAwareGizmo> Gizmo;
    GraphicMesh* SelectedGraphicMesh = nullptr;
    std::vector<GraphicMesh*> m_graphicMeshes;
    Model* m_model;
    double TotalDisplacement[3] = {0, 0, 0};
    int SelectedAxis = -1;
    int ClickPos[2];
    int LastPos[2];
    
};


#endif
