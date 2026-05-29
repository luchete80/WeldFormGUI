#ifndef _TRANSFORMGIZMO_H_
#define _TRANSFORMGIZMO_H_

#include <array>
#include <algorithm>
#include <cmath>

#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkCylinderSource.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObject.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

#include "../geom/Geom.h"
#include "../graphicmesh/GraphicMesh.h"
#include "../model/Part.h"

class TransformGizmo : public vtkObject {
public:
    vtkTypeMacro(TransformGizmo, vtkObject);
    static TransformGizmo* New();

    TransformGizmo() {
        CreateActors();
    }

    void SetDimension(int dimension) {
        Dimension = (dimension == 2) ? 2 : 3;
        ApplyDimensionVisibility();
    }

    void Hide() {
        IsVisibleFlag = false;
        ApplyDimensionVisibility();
    }

    void Show() {
        IsVisibleFlag = true;
        ApplyDimensionVisibility();
    }

    void AddToRenderer(vtkRenderer* renderer) {
        if (!renderer) return;

        for (auto& actor : DragAxes)
            renderer->AddActor(actor);
        for (auto& actor : DragTips)
            renderer->AddActor(actor);
        for (auto& actor : PickAxes)
            renderer->AddActor(actor);
        for (auto& actor : OriginAxes)
            renderer->AddActor(actor);
    }

    void RemoveFromRenderer(vtkRenderer* renderer) {
        if (!renderer) return;

        for (auto& actor : DragAxes)
            renderer->RemoveActor(actor);
        for (auto& actor : DragTips)
            renderer->RemoveActor(actor);
        for (auto& actor : PickAxes)
            renderer->RemoveActor(actor);
        for (auto& actor : OriginAxes)
            renderer->RemoveActor(actor);
    }

    void SetTargetActor(vtkSmartPointer<vtkActor> actor) {
        this->TargetActor = actor;
        UpdatePlacementFromTargetActor();
    }

    void SetReferenceRenderer(vtkRenderer* renderer) {
        this->ReferenceRenderer = renderer;
        UpdatePlacementFromTargetActor();
    }

    void SetViewCenteredPlacementEnabled(bool enabled) {
        UseViewCenteredPlacement = enabled;
        UpdatePlacementFromTargetActor();
    }

    void SetDragCenterPosition(const double center[3]) {
        if (!center) return;
        HasCustomDragCenter = true;
        for (int i = 0; i < 3; ++i)
            CustomDragCenter[i] = center[i];
        UpdatePlacementFromTargetActor();
    }

    void ClearDragCenterPositionOverride() {
        HasCustomDragCenter = false;
        UpdatePlacementFromTargetActor();
    }

    void SetOriginPosition(const double origin[3]) {
        if (!origin) return;
        for (int i = 0; i < 3; ++i)
            OriginPosition[i] = origin[i];
        UpdateOriginPlacement();
    }

    void SetOriginPosition(double x, double y, double z) {
        double origin[3] = {x, y, z};
        SetOriginPosition(origin);
    }

    void UpdatePlacementFromTargetActor() {
        if (!TargetActor) return;

        double bounds[6];
        TargetActor->GetBounds(bounds);

        const double actorCenter[3] = {
            0.5 * (bounds[0] + bounds[1]),
            0.5 * (bounds[2] + bounds[3]),
            0.5 * (bounds[4] + bounds[5])
        };
        double placementCenter[3] = {
            actorCenter[0],
            actorCenter[1],
            actorCenter[2]
        };
        if (HasCustomDragCenter) {
            placementCenter[0] = CustomDragCenter[0];
            placementCenter[1] = CustomDragCenter[1];
            placementCenter[2] = CustomDragCenter[2];
        } else if (UseViewCenteredPlacement) {
            ComputeViewCenteredPlacement(actorCenter, placementCenter);
        }
        DragCenter = {placementCenter[0], placementCenter[1], placementCenter[2]};

        const double maxDim = (std::max)(
            bounds[1] - bounds[0],
            (std::max)(bounds[3] - bounds[2], bounds[5] - bounds[4]));
        const double effectiveDim = (std::max)(maxDim, 1e-9);

        const double worldUnitsPerPixel = ComputeWorldUnitsPerPixel(actorCenter);
        if (worldUnitsPerPixel > 0.0) {
            DragLength = (std::max)(worldUnitsPerPixel * 110.0, 1e-6);
            DragRadius = (std::max)(worldUnitsPerPixel * 3.0, 5e-7);
            PickRadius = (std::max)(worldUnitsPerPixel * 12.0, DragRadius * 3.5);
            TipLength = (std::max)(worldUnitsPerPixel * 20.0, 1e-6);
            TipRadius = (std::max)(worldUnitsPerPixel * 7.0, DragRadius * 2.0);
            OriginLength = (std::max)(worldUnitsPerPixel * 24.0, 1e-6);
            OriginRadius = (std::max)(worldUnitsPerPixel * 2.0, 2.5e-7);
        } else {
            DragLength = (std::max)(effectiveDim * 0.55, 1e-6);
            DragRadius = (std::max)(effectiveDim * 0.015, 5e-7);
            PickRadius = (std::max)(effectiveDim * 0.06, DragRadius * 3.5);
            TipLength = (std::max)(effectiveDim * 0.16, 1e-6);
            TipRadius = (std::max)(effectiveDim * 0.055, DragRadius * 2.0);
            OriginLength = (std::max)(effectiveDim * 0.18, 1e-6);
            OriginRadius = (std::max)(effectiveDim * 0.008, 2.5e-7);
        }

        for (int axis = 0; axis < 3; ++axis) {
            ApplyPositiveAxisPose(DragSources[axis], DragAxes[axis], placementCenter, axis, DragLength, DragRadius);
            ApplyPositiveAxisPose(PickSources[axis], PickAxes[axis], placementCenter, axis, DragLength, PickRadius);
            ApplyArrowTipPose(TipSources[axis], DragTips[axis], placementCenter, axis, DragLength, TipLength, TipRadius);
        }

        UpdateOriginPlacement();
    }

    std::array<vtkSmartPointer<vtkActor>, 3> GetDragAxes() const { return DragAxes; }
    std::array<vtkSmartPointer<vtkActor>, 3> GetPickAxes() const { return PickAxes; }
    std::array<vtkSmartPointer<vtkActor>, 3> GetOriginAxes() const { return OriginAxes; }
    bool IsAxisActive(int axis) const { return axis >= 0 && axis < Dimension; }
    const std::array<double, 3>& GetDragCenter() const { return DragCenter; }
    double GetDragLength() const { return DragLength; }

    void HighlightAxis(int axis) {
        const std::array<std::array<double, 3>, 3> baseColors = {{
            {0.95, 0.25, 0.25},
            {0.20, 0.85, 0.30},
            {0.25, 0.45, 0.95}
        }};
        for (int i = 0; i < 3; ++i) {
            if (!DragAxes[i]) continue;
            double color[3] = {
                baseColors[i][0],
                baseColors[i][1],
                baseColors[i][2]
            };
            if (i == axis) {
                color[0] = 1.0;
                color[1] = 1.0;
                color[2] = 0.35;
            }
            DragAxes[i]->GetProperty()->SetColor(color);
            DragAxes[i]->GetProperty()->SetOpacity(i == axis ? 1.0 : 0.45);
            DragAxes[i]->GetProperty()->SetLineWidth(i == axis ? 6.0 : 1.0);
            if (DragTips[i]) {
                DragTips[i]->GetProperty()->SetColor(color);
                DragTips[i]->GetProperty()->SetOpacity(i == axis ? 1.0 : 0.85);
                DragTips[i]->GetProperty()->SetLineWidth(i == axis ? 6.0 : 1.0);
            }
        }
    }

    void ClearHighlight() {
        const std::array<std::array<double, 3>, 3> baseColors = {{
            {0.95, 0.25, 0.25},
            {0.20, 0.85, 0.30},
            {0.25, 0.45, 0.95}
        }};
        for (int i = 0; i < 3; ++i) {
            auto& actor = DragAxes[i];
            if (!actor) continue;
            actor->GetProperty()->SetColor(baseColors[i][0], baseColors[i][1], baseColors[i][2]);
            actor->GetProperty()->SetOpacity(0.45);
            actor->GetProperty()->SetLineWidth(2.0);
            if (DragTips[i]) {
                DragTips[i]->GetProperty()->SetColor(baseColors[i][0], baseColors[i][1], baseColors[i][2]);
                DragTips[i]->GetProperty()->SetOpacity(0.85);
                DragTips[i]->GetProperty()->SetLineWidth(2.0);
            }
        }
    }

private:
    void CreateActors() {
        const std::array<std::array<double, 3>, 3> dragColors = {{
            {0.95, 0.25, 0.25},
            {0.20, 0.85, 0.30},
            {0.25, 0.45, 0.95}
        }};
        const std::array<std::array<double, 3>, 3> originColors = {{
            {1.0, 0.75, 0.25},
            {0.95, 0.85, 0.35},
            {1.0, 0.95, 0.55}
        }};

        for (int i = 0; i < 3; ++i) {
            DragSources[i] = vtkSmartPointer<vtkCylinderSource>::New();
            PickSources[i] = vtkSmartPointer<vtkCylinderSource>::New();
            TipSources[i] = vtkSmartPointer<vtkArrowSource>::New();
            OriginSources[i] = vtkSmartPointer<vtkCylinderSource>::New();

            ConfigureCylinder(DragSources[i], 1.0, 0.03);
            ConfigureCylinder(PickSources[i], 1.0, 0.09);
            ConfigureArrow(TipSources[i]);
            ConfigureCylinder(OriginSources[i], 0.35, 0.015);

            DragAxes[i] = CreateActorFromSource(DragSources[i], dragColors[i].data(), 0.75, true);
            DragTips[i] = CreateActorFromArrowSource(TipSources[i], dragColors[i].data(), 0.85, true);
            PickAxes[i] = CreateActorFromSource(PickSources[i], dragColors[i].data(), 0.02, true);
            OriginAxes[i] = CreateActorFromSource(OriginSources[i], originColors[i].data(), 0.95, false);

            DragTips[i]->GetProperty()->SetRepresentationToSurface();
            PickAxes[i]->GetProperty()->SetRepresentationToSurface();
            OriginAxes[i]->GetProperty()->SetRepresentationToSurface();
        }

        Hide();
    }

    static void ConfigureCylinder(vtkCylinderSource* source, double height, double radius) {
        source->SetHeight(height);
        source->SetRadius(radius);
        source->SetResolution(24);
        source->CappingOn();
        source->Update();
    }

    static void ConfigureArrow(vtkArrowSource* source) {
        source->SetTipLength(0.32);
        source->SetTipRadius(0.22);
        source->SetShaftRadius(0.10);
        source->SetTipResolution(24);
        source->SetShaftResolution(24);
        source->Update();
    }

    static vtkSmartPointer<vtkActor> CreateActorFromSource(vtkCylinderSource* source,
                                                           const double color[3],
                                                           double opacity,
                                                           bool pickable) {
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(source->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(color[0], color[1], color[2]);
        actor->GetProperty()->SetOpacity(opacity);
        actor->SetPickable(pickable ? 1 : 0);
        actor->SetDragable(pickable ? 1 : 0);
        return actor;
    }

    static vtkSmartPointer<vtkActor> CreateActorFromArrowSource(vtkArrowSource* source,
                                                                const double color[3],
                                                                double opacity,
                                                                bool pickable) {
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(source->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(color[0], color[1], color[2]);
        actor->GetProperty()->SetOpacity(opacity);
        actor->SetPickable(pickable ? 1 : 0);
        actor->SetDragable(pickable ? 1 : 0);
        return actor;
    }

    void SetActorsVisible(const std::array<vtkSmartPointer<vtkActor>, 3>& actors, bool visible) {
        for (int i = 0; i < 3; ++i) {
            auto& actor = actors[i];
            if (actor)
                actor->SetVisibility((visible && IsAxisActive(i)) ? 1 : 0);
        }
    }

    void ApplyDimensionVisibility() {
        SetActorsVisible(DragAxes, IsVisibleFlag);
        SetActorsVisible(DragTips, IsVisibleFlag);
        SetActorsVisible(PickAxes, IsVisibleFlag);
        SetActorsVisible(OriginAxes, IsVisibleFlag);
    }

    static void ApplyPositiveAxisPose(vtkCylinderSource* source,
                                      vtkActor* actor,
                                      const double center[3],
                                      int axis,
                                      double length,
                                      double radius) {
        ConfigureCylinder(source, length, radius);

        vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
        transform->PostMultiply();
        if (axis == 0)
            transform->RotateZ(90.0);
        else if (axis == 2)
            transform->RotateX(-90.0);

        double shiftedCenter[3] = {center[0], center[1], center[2]};
        shiftedCenter[axis] += 0.5 * length;
        transform->Translate(shiftedCenter);

        actor->SetUserTransform(transform);
        actor->Modified();
    }

    static void ApplyArrowTipPose(vtkArrowSource* source,
                                  vtkActor* actor,
                                  const double center[3],
                                  int axis,
                                  double shaftLength,
                                  double tipLength,
                                  double tipRadius) {
        ConfigureArrow(source);

        vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
        transform->PostMultiply();
        if (axis == 1) {
            transform->RotateZ(90.0);
        } else if (axis == 2) {
            transform->RotateY(-90.0);
        }

        transform->Scale(tipLength, tipRadius, tipRadius);

        double tipBase[3] = {center[0], center[1], center[2]};
        tipBase[axis] += shaftLength;
        transform->Translate(tipBase);

        actor->SetUserTransform(transform);
        actor->Modified();
    }

    void UpdateOriginPlacement() {
        for (int axis = 0; axis < 3; ++axis) {
            ApplyPositiveAxisPose(OriginSources[axis], OriginAxes[axis], OriginPosition.data(),
                          axis, OriginLength, OriginRadius);
        }
    }

    double ComputeWorldUnitsPerPixel(const double center[3]) const {
        if (!ReferenceRenderer) {
            return 0.0;
        }

        vtkCamera* camera = ReferenceRenderer->GetActiveCamera();
        vtkRenderWindow* renderWindow = ReferenceRenderer->GetRenderWindow();
        if (!camera || !renderWindow) {
            return 0.0;
        }

        int* size = renderWindow->GetSize();
        if (size == nullptr || size[1] <= 0) {
            return 0.0;
        }

        if (camera->GetParallelProjection()) {
            return (2.0 * camera->GetParallelScale()) / static_cast<double>(size[1]);
        }

        double cameraPosition[3];
        camera->GetPosition(cameraPosition);
        double directionOfProjection[3];
        camera->GetDirectionOfProjection(directionOfProjection);
        vtkMath::Normalize(directionOfProjection);

        const double cameraToCenter[3] = {
            center[0] - cameraPosition[0],
            center[1] - cameraPosition[1],
            center[2] - cameraPosition[2]
        };
        const double depth = std::abs(vtkMath::Dot(cameraToCenter, directionOfProjection));
        if (depth <= 1.0e-9) {
            return 0.0;
        }

        const double viewAngleRadians = vtkMath::RadiansFromDegrees(camera->GetViewAngle());
        const double visibleWorldHeight = 2.0 * depth * std::tan(0.5 * viewAngleRadians);
        return visibleWorldHeight / static_cast<double>(size[1]);
    }

    bool ComputeViewCenteredPlacement(const double referencePoint[3], double placement[3]) const {
        if (!ReferenceRenderer) {
            return false;
        }

        vtkRenderWindow* renderWindow = ReferenceRenderer->GetRenderWindow();
        if (!renderWindow) {
            return false;
        }

        double displayReference[3] = {0.0, 0.0, 0.0};
        ReferenceRenderer->SetWorldPoint(referencePoint[0], referencePoint[1], referencePoint[2], 1.0);
        ReferenceRenderer->WorldToDisplay();
        ReferenceRenderer->GetDisplayPoint(displayReference);
        if (displayReference[2] < 0.0 || displayReference[2] > 1.0) {
            return false;
        }

        int* size = renderWindow->GetSize();
        if (size == nullptr || size[0] <= 0 || size[1] <= 0) {
            return false;
        }

        ReferenceRenderer->SetDisplayPoint(
            0.5 * static_cast<double>(size[0]),
            0.5 * static_cast<double>(size[1]),
            displayReference[2]);
        ReferenceRenderer->DisplayToWorld();

        double worldPoint[4] = {0.0, 0.0, 0.0, 0.0};
        ReferenceRenderer->GetWorldPoint(worldPoint);
        if (std::abs(worldPoint[3]) <= 1.0e-9) {
            return false;
        }

        placement[0] = worldPoint[0] / worldPoint[3];
        placement[1] = worldPoint[1] / worldPoint[3];
        placement[2] = worldPoint[2] / worldPoint[3];
        return true;
    }

private:
    std::array<vtkSmartPointer<vtkCylinderSource>, 3> DragSources;
    std::array<vtkSmartPointer<vtkCylinderSource>, 3> PickSources;
    std::array<vtkSmartPointer<vtkArrowSource>, 3> TipSources;
    std::array<vtkSmartPointer<vtkCylinderSource>, 3> OriginSources;
    std::array<vtkSmartPointer<vtkActor>, 3> DragAxes;
    std::array<vtkSmartPointer<vtkActor>, 3> DragTips;
    std::array<vtkSmartPointer<vtkActor>, 3> PickAxes;
    std::array<vtkSmartPointer<vtkActor>, 3> OriginAxes;
    vtkSmartPointer<vtkActor> TargetActor;
    vtkRenderer* ReferenceRenderer = nullptr;
    std::array<double, 3> OriginPosition = {0.0, 0.0, 0.0};
    std::array<double, 3> DragCenter = {0.0, 0.0, 0.0};
    int Dimension = 3;
    bool IsVisibleFlag = false;
    bool UseViewCenteredPlacement = true;
    bool HasCustomDragCenter = false;
    double DragLength = 1e-3;
    double DragRadius = 2.5e-5;
    double PickRadius = 8e-5;
    double TipLength = 2.2e-4;
    double TipRadius = 7.5e-5;
    double OriginLength = 3e-4;
    double OriginRadius = 1.2e-5;
    std::array<double, 3> CustomDragCenter = {0.0, 0.0, 0.0};
};

inline TransformGizmo* TransformGizmo::New() {
    auto newGizmo = new TransformGizmo;
    return newGizmo;
}

namespace {
double DistancePointToSegment2D(double px, double py,
                                double ax, double ay,
                                double bx, double by)
{
    const double abx = bx - ax;
    const double aby = by - ay;
    const double ab_len2 = abx * abx + aby * aby;
    if (ab_len2 <= 1e-12) {
        const double dx = px - ax;
        const double dy = py - ay;
        return std::sqrt(dx * dx + dy * dy);
    }

    double t = ((px - ax) * abx + (py - ay) * aby) / ab_len2;
    t = (std::max)(0.0, (std::min)(1.0, t));
    const double cx = ax + t * abx;
    const double cy = ay + t * aby;
    const double dx = px - cx;
    const double dy = py - cy;
    return std::sqrt(dx * dx + dy * dy);
}

void WorldToDisplayPoint(vtkRenderer* renderer, const double world[3], double display[3])
{
    renderer->SetWorldPoint(world[0], world[1], world[2], 1.0);
    renderer->WorldToDisplay();
    renderer->GetDisplayPoint(display);
}
}

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

    void SetPart(Part* pt) {
        this->m_part = pt;
    }

    void SetGraphicMesh(GraphicMesh* gmesh) {
        this->m_graphicMesh = gmesh;
    }

    void SetGizmoAxes(std::array<vtkSmartPointer<vtkActor>, 3> axes) {
        this->Axes = axes;
    }

    void SetPickAxes(std::array<vtkSmartPointer<vtkActor>, 3> axes) {
        this->PickAxes = axes;
    }

    void SetTransformGizmo(vtkSmartPointer<TransformGizmo> gizmo) {
        this->Gizmo = gizmo;
    }

    void OnLeftButtonDown() override {
        int* clickPos = this->GetInteractor()->GetEventPosition();
        this->ClickPos[0] = clickPos[0];
        this->ClickPos[1] = clickPos[1];

        this->SelectedAxis = PickAxisAt(clickPos[0], clickPos[1]);
        if (this->SelectedAxis >= 0) {
            if (this->Gizmo)
                this->Gizmo->HighlightAxis(this->SelectedAxis);
            this->GetInteractor()->GetRenderWindow()->Render();
            return;
        }

        vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
    }

    void OnMouseMove() override {
        if (this->SelectedAxis == -1) {
            int* pos = this->GetInteractor()->GetEventPosition();
            int hoveredAxis = PickAxisAt(pos[0], pos[1]);
            if (hoveredAxis != this->HoveredAxis) {
                this->HoveredAxis = hoveredAxis;
                if (this->Gizmo) {
                    if (hoveredAxis >= 0)
                        this->Gizmo->HighlightAxis(hoveredAxis);
                    else
                        this->Gizmo->ClearHighlight();
                }
                this->GetInteractor()->GetRenderWindow()->Render();
            }
            vtkInteractorStyleTrackballCamera::OnMouseMove();
            return;
        }

        if (this->SelectedAxis >= 0 && this->TargetActor) {
            int* currPos = this->GetInteractor()->GetEventPosition();

            const std::array<std::array<double, 3>, 3> directions = {{
                {{1.0, 0.0, 0.0}},
                {{0.0, 1.0, 0.0}},
                {{0.0, 0.0, 1.0}}
            }};

            vtkRenderer* renderer = this->GetDefaultRenderer();
            renderer->SetDisplayPoint(currPos[0], currPos[1], 0);
            renderer->DisplayToWorld();
            double worldPos1[4];
            renderer->GetWorldPoint(worldPos1);

            renderer->SetDisplayPoint(ClickPos[0], ClickPos[1], 0);
            renderer->DisplayToWorld();
            double worldPos0[4];
            renderer->GetWorldPoint(worldPos0);

            double moveVec[3] = {
                worldPos1[0] - worldPos0[0],
                worldPos1[1] - worldPos0[1],
                worldPos1[2] - worldPos0[2]
            };

            double dot = vtkMath::Dot(moveVec, directions[SelectedAxis].data());
            double translate[3] = {
                directions[SelectedAxis][0] * dot,
                directions[SelectedAxis][1] * dot,
                directions[SelectedAxis][2] * dot
            };

            if (m_polydata) {
                vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
                transform->Translate(translate);

                vtkNew<vtkTransformFilter> tf;
                tf->SetInputData(m_polydata);
                tf->SetTransform(transform);
                tf->Update();
                m_polydata->ShallowCopy(tf->GetOutput());
            }

            if (m_part && m_part->getGeom()) {
                m_part->getGeom()->Move(translate[0], translate[1], translate[2]);
            }

            if (m_graphicMesh) {
                m_graphicMesh->Translate(translate[0], translate[1], translate[2]);
            }

            if (Gizmo) {
                Gizmo->SetReferenceRenderer(renderer);
                Gizmo->UpdatePlacementFromTargetActor();
                Gizmo->SetOriginPosition(0.0, 0.0, 0.0);
            }

            this->ClickPos[0] = currPos[0];
            this->ClickPos[1] = currPos[1];
            this->GetInteractor()->GetRenderWindow()->Render();
            return;
        }

        vtkInteractorStyleTrackballCamera::OnMouseMove();
    }

    void OnLeftButtonUp() override {
        this->SelectedAxis = -1;
        this->HoveredAxis = -1;
        if (this->Gizmo)
            this->Gizmo->ClearHighlight();
        this->GetInteractor()->GetRenderWindow()->Render();
        vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
    }

private:
    int PickAxisAt(int x, int y) {
        if (!Gizmo || !this->GetDefaultRenderer())
            return -1;

        Gizmo->SetReferenceRenderer(this->GetDefaultRenderer());

        const std::array<std::array<double, 3>, 3> directions = {{
            {{1.0, 0.0, 0.0}},
            {{0.0, 1.0, 0.0}},
            {{0.0, 0.0, 1.0}}
        }};

        const std::array<double, 3>& center = Gizmo->GetDragCenter();
        const double length = Gizmo->GetDragLength();
        vtkRenderer* renderer = this->GetDefaultRenderer();

        int bestAxis = -1;
        double bestDistance = 1e100;
        const double pixelThreshold = 20.0;

        for (int i = 0; i < 3; ++i) {
            if (!Gizmo->IsAxisActive(i))
                continue;

            double start[3] = {
                center[0],
                center[1],
                center[2]
            };
            double end[3] = {
                center[0] + directions[i][0] * length,
                center[1] + directions[i][1] * length,
                center[2] + directions[i][2] * length
            };

            double startDisplay[3];
            double endDisplay[3];
            WorldToDisplayPoint(renderer, start, startDisplay);
            WorldToDisplayPoint(renderer, end, endDisplay);

            const double distance = DistancePointToSegment2D(
                static_cast<double>(x),
                static_cast<double>(y),
                startDisplay[0], startDisplay[1],
                endDisplay[0], endDisplay[1]);

            if (distance < bestDistance) {
                bestDistance = distance;
                bestAxis = i;
            }
        }

        if (bestDistance > pixelThreshold)
            return -1;

        return bestAxis;
    }

private:
    vtkSmartPointer<vtkPolyData> m_polydata;
    Part* m_part = nullptr;
    GraphicMesh* m_graphicMesh = nullptr;
    vtkSmartPointer<vtkActor> TargetActor;
    std::array<vtkSmartPointer<vtkActor>, 3> Axes;
    std::array<vtkSmartPointer<vtkActor>, 3> PickAxes;
    vtkSmartPointer<TransformGizmo> Gizmo;
    int SelectedAxis = -1;
    int HoveredAxis = -1;
    int ClickPos[2] = {0, 0};
};

inline GizmoInteractorStyle* GizmoInteractorStyle::New() {
    return new GizmoInteractorStyle;
}

#endif
