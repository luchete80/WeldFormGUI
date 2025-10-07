///// BOUNDING BOX SYSTEM

class ActorCollisionSystem {
public:
    static bool CheckBoundingBoxCollision(vtkActor* actor1, vtkActor* actor2) {
        double bounds1[6], bounds2[6];
        actor1->GetBounds(bounds1);
        actor2->GetBounds(bounds2);
        
        return (bounds1[0] <= bounds2[1] && bounds1[1] >= bounds2[0] &&
                bounds1[2] <= bounds2[3] && bounds1[3] >= bounds2[2] &&
                bounds1[4] <= bounds2[5] && bounds1[5] >= bounds2[4]);
    }
    
    static bool CheckBoundingBoxCollision(vtkActor* movingActor, vtkActor* staticActor, 
                                         const Vector3f& displacement) {
        double bounds1[6], bounds2[6];
        movingActor->GetBounds(bounds1);
        staticActor->GetBounds(bounds2);
        
        // Aplicar desplazamiento a los bounds del actor en movimiento
        double displacedBounds1[6] = {
            bounds1[0] + displacement.x, bounds1[1] + displacement.x,
            bounds1[2] + displacement.y, bounds1[3] + displacement.y,
            bounds1[4] + displacement.z, bounds1[5] + displacement.z
        };
        
        return (displacedBounds1[0] <= bounds2[1] && displacedBounds1[1] >= bounds2[0] &&
                displacedBounds1[2] <= bounds2[3] && displacedBounds1[3] >= bounds2[2] &&
                displacedBounds1[4] <= bounds2[5] && displacedBounds1[5] >= bounds2[4]);
    }
};



class PolyDataCollisionSystem {
public:
    static bool CheckPolyDataCollision(vtkActor* actor1, vtkActor* actor2, 
                                      double tolerance = 0.1) {
        vtkPolyData* polyData1 = GetPolyDataFromActor(actor1);
        vtkPolyData* polyData2 = GetPolyDataFromActor(actor2);
        
        if (!polyData1 || !polyData2) return false;
        
        // Verificar distancia entre polydatas
        vtkSmartPointer<vtkDistancePolyDataFilter> distanceFilter = 
            vtkSmartPointer<vtkDistancePolyDataFilter>::New();
        distanceFilter->SetInputData(0, polyData1);
        distanceFilter->SetInputData(1, polyData2);
        distanceFilter->Update();
        
        vtkPolyData* output = distanceFilter->GetOutput();
        vtkDataArray* distanceArray = output->GetPointData()->GetArray("Distance");
        
        // Verificar si alguna distancia es menor que la tolerancia
        for (vtkIdType i = 0; i < distanceArray->GetNumberOfTuples(); i++) {
            double distance = distanceArray->GetTuple1(i);
            if (distance < tolerance) {
                return true;
            }
        }
        return false;
    }
    
    static bool CheckPolyDataCollision(vtkActor* movingActor, vtkActor* staticActor,
                                      const Vector3f& displacement, double tolerance = 0.1) {
        // Crear una copia temporal del polydata con el desplazamiento aplicado
        vtkSmartPointer<vtkPolyData> displacedPolyData = 
            CreateDisplacedPolyData(movingActor, displacement);
        
        vtkPolyData* staticPolyData = GetPolyDataFromActor(staticActor);
        
        if (!displacedPolyData || !staticPolyData) return false;
        
        vtkSmartPointer<vtkDistancePolyDataFilter> distanceFilter = 
            vtkSmartPointer<vtkDistancePolyDataFilter>::New();
        distanceFilter->SetInputData(0, displacedPolyData);
        distanceFilter->SetInputData(1, staticPolyData);
        distanceFilter->Update();
        
        vtkDataArray* distanceArray = distanceFilter->GetOutput()->GetPointData()->GetArray("Distance");
        
        for (vtkIdType i = 0; i < distanceArray->GetNumberOfTuples(); i++) {
            if (distanceArray->GetTuple1(i) < tolerance) {
                return true;
            }
        }
        return false;
    }
    
private:
    static vtkPolyData* GetPolyDataFromActor(vtkActor* actor) {
        vtkMapper* mapper = actor->GetMapper();
        if (!mapper) return nullptr;
        
        return vtkPolyData::SafeDownCast(mapper->GetInput());
    }
    
    static vtkSmartPointer<vtkPolyData> CreateDisplacedPolyData(vtkActor* actor, 
                                                               const Vector3f& displacement) {
        vtkPolyData* originalPolyData = GetPolyDataFromActor(actor);
        if (!originalPolyData) return nullptr;
        
        vtkSmartPointer<vtkPolyData> displacedPolyData = 
            vtkSmartPointer<vtkPolyData>::New();
        displacedPolyData->DeepCopy(originalPolyData);
        
        // Aplicar desplazamiento a todos los puntos
        vtkPoints* points = displacedPolyData->GetPoints();
        for (vtkIdType i = 0; i < points->GetNumberOfPoints(); i++) {
            double point[3];
            points->GetPoint(i, point);
            point[0] += displacement.x;
            point[1] += displacement.y;
            point[2] += displacement.z;
            points->SetPoint(i, point);
        }
        
        return displacedPolyData;
    }
};


class ActorGizmoInteractorStyle : public vtkInteractorStyleTrackballCamera {
public:
    void OnMouseMove() override {
        if (this->SelectedAxis >= 0 && this->TargetActor) {
            int* currPos = this->GetInteractor()->GetEventPosition();
            
            double dx = currPos[0] - this->DragStartPos[0];
            double dy = currPos[1] - this->DragStartPos[1];
            
            double movement = (dx + dy) * 0.005;
            Vector3f displacement(0, 0, 0);
            displacement[this->SelectedAxis] = movement;
            
            // Verificar colisión antes de mover
            if (!CheckCollisionWithOtherActors(displacement)) {
                // Mover el actor
                vtkSmartPointer<vtkTransform> transform = this->TargetActor->GetUserTransform();
                if (!transform) {
                    transform = vtkSmartPointer<vtkTransform>::New();
                    this->TargetActor->SetUserTransform(transform);
                }
                
                double translate[3] = {0, 0, 0};
                translate[this->SelectedAxis] = movement;
                transform->Translate(translate);
                
                this->GetInteractor()->GetRenderWindow()->Render();
            }
        } else {
            vtkInteractorStyleTrackballCamera::OnMouseMove();
        }
    }
    
    bool CheckCollisionWithOtherActors(const Vector3f& displacement) {
        if (!TargetActor || OtherActors.empty()) return false;
        
        for (vtkActor* otherActor : OtherActors) {
            if (otherActor == TargetActor) continue;
            
            // Elegir la estrategia de colisión:
            
            // 1. Bounding Box (más rápido)
            if (ActorCollisionSystem::CheckBoundingBoxCollision(TargetActor, otherActor, displacement)) {
                return true;
            }
            
            // 2. Esferas (balance velocidad/precisión)
            // if (SphereCollisionSystem::CheckSphereCollision(TargetActor, otherActor)) {
            //     return true;
            // }
            
            // 3. Polydata (más preciso pero lento)
            // if (PolyDataCollisionSystem::CheckPolyDataCollision(TargetActor, otherActor, displacement)) {
            //     return true;
            // }
        }
        return false;
    }
    
    void SetTargetActor(vtkActor* actor) { 
        this->TargetActor = actor; 
    }
    
    void SetOtherActors(const std::vector<vtkActor*>& actors) {
        this->OtherActors = actors;
    }

private:
    vtkActor* TargetActor = nullptr;
    std::vector<vtkActor*> OtherActors;
};