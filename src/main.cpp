// Standard Library
#include <algorithm>
#include <iostream>
#include <array>
#include <filesystem>
#include <functional>
#include <limits>
#include <map>


/////https://github.com/trlsmax/imgui-vtk/tree/master

// OpenGL Loader
// This can be replaced with another loader, e.g. glad, but
// remember to also change the corresponding initialize call!
#include <GL/gl3w.h>            // GL3w, initialized with gl3wInit() below

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// ImGui + imgui-vtk
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "VtkViewer.h"
#include "git_commit.h"

// VTK
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include "editor.h"



// File-Specific Includes
#include "imgui_vtk_demo.h" // Actor generator for this demo


#include <vtkActor.h>
//#include <vtkArrowSource.h>
#include <vtkNamedColors.h>
#include <vtkScalarBarActor.h>
#include <vtkCamera.h>
#include <vtkCell.h>
#include <vtkCellPicker.h>
#include <vtkContourTriangulator.h>
#include <vtkCutter.h>
#include <vtkDataSet.h>
#include <vtkExtractCells.h>
#include <vtkMapper.h>
#include <vtkPlane.h>
#include <vtkProperty.h>
#include <vtkSphereSource.h>


#include "graphics/axis.h" //test



#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataMapper.h>

#include <vtkArrowSource.h>

///// FOR GEOMETRIA
#include "vtkOCCTReader.h"
#include <vtkCompositePolyDataMapper.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkRegressionTestImage.h>

#include "geom/vtkOCCTGeom.h"

#include <gmsh.h>

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef BUILD_PYTHON
#include <Python.h>
#endif

#include "App/App.h"
#include "GraphicMesh.h"
#include "graphics/TransformGizmo.h"
#include "model/Node.h"

#include "results.h"
#include "load_plot_dialog.h"
#include "energy_plot_dialog.h"
//using App;
#include "geom/vtkOCCTGeom.h"
#include "geom/ShapeToPolyData.h"

#include <vtkSmartPointer.h>
#include <vtkFileOutputWindow.h>
#include <vtkOutputWindow.h>

//THIS IS TO AVOIDERROR WITH MINMAX
#ifdef _WIN32 
  #undef min
  #undef max
#endif

#include "demo_dialog.h"

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromMemory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

namespace {
enum class UIFontChoice {
    ImGuiDefault,
    Ubuntu
};

enum class ModelDisplayMode {
    Surface,
    Wireframe
};

struct ModelViewportOverlayState {
    ModelDisplayMode displayMode = ModelDisplayMode::Surface;
    bool showEdges = true;
    bool axesVisible = false;
    bool orthographic = false;
};

struct ResultsViewportToolState {
    bool is3DFrame = false;
    bool clipEnabled = false;
    bool transparencyControlsVisible = false;
    float surfaceOpacity = 1.0f;
    int clipAxis = 2;
    bool clipOriginInitialized = false;
    std::array<double, 3> clipOrigin = {0.0, 0.0, 0.0};
    vtkSmartPointer<vtkPlane> clipPlane;
    vtkSmartPointer<TransformGizmo> gizmo;
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> clipInteractorStyle;
    std::map<vtkActor*, vtkSmartPointer<vtkActor>> clipCapActors;
    bool gizmoAddedToRenderer = false;
    bool customInteractorInstalled = false;
};

struct ResultsPlaybackOverlayState {
    bool visible = true;
    bool minimized = false;
    float scalarBarHeightPx = 150.0f;
    float scalarBarWidthPx = 80.0f;
    ImVec2 lastMin = ImVec2(0.0f, 0.0f);
    ImVec2 lastMax = ImVec2(0.0f, 0.0f);
};

double distancePointToSegment2D(double px, double py,
                                double ax, double ay,
                                double bx, double by)
{
    const double abx = bx - ax;
    const double aby = by - ay;
    const double abLen2 = abx * abx + aby * aby;
    if (abLen2 <= 1.0e-12) {
        const double dx = px - ax;
        const double dy = py - ay;
        return std::sqrt(dx * dx + dy * dy);
    }

    double t = ((px - ax) * abx + (py - ay) * aby) / abLen2;
    t = std::max(0.0, std::min(1.0, t));
    const double cx = ax + t * abx;
    const double cy = ay + t * aby;
    const double dx = px - cx;
    const double dy = py - cy;
    return std::sqrt(dx * dx + dy * dy);
}

void worldToDisplayPoint(vtkRenderer* renderer, const double world[3], double display[3])
{
    renderer->SetWorldPoint(world[0], world[1], world[2], 1.0);
    renderer->WorldToDisplay();
    renderer->GetDisplayPoint(display);
}

class ResultClipInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
    static ResultClipInteractorStyle* New();
    vtkTypeMacro(ResultClipInteractorStyle, vtkInteractorStyleTrackballCamera);

    void SetTransformGizmo(vtkSmartPointer<TransformGizmo> gizmo) {
        Gizmo = gizmo;
    }

    void SetAxisChangedCallback(std::function<void(int)> callback) {
        AxisChangedCallback = std::move(callback);
    }

    void SetAxisDraggedCallback(std::function<void(int, double)> callback) {
        AxisDraggedCallback = std::move(callback);
    }

    void OnLeftButtonDown() override {
        int* clickPos = this->GetInteractor()->GetEventPosition();
        ClickPos[0] = clickPos[0];
        ClickPos[1] = clickPos[1];

        SelectedAxis = pickAxisAt(clickPos[0], clickPos[1]);
        if (SelectedAxis >= 0) {
            if (AxisChangedCallback)
                AxisChangedCallback(SelectedAxis);
            if (Gizmo)
                Gizmo->HighlightAxis(SelectedAxis);
            this->GetInteractor()->GetRenderWindow()->Render();
            return;
        }

        vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
    }

    void OnMouseMove() override {
        if (SelectedAxis == -1) {
            int* pos = this->GetInteractor()->GetEventPosition();
            int hoveredAxis = pickAxisAt(pos[0], pos[1]);
            if (hoveredAxis != HoveredAxis) {
                HoveredAxis = hoveredAxis;
                if (Gizmo) {
                    if (hoveredAxis >= 0)
                        Gizmo->HighlightAxis(hoveredAxis);
                    else
                        Gizmo->ClearHighlight();
                }
                this->GetInteractor()->GetRenderWindow()->Render();
            }
            vtkInteractorStyleTrackballCamera::OnMouseMove();
            return;
        }

        vtkRenderer* renderer = this->GetDefaultRenderer();
        if (renderer == nullptr) {
            vtkInteractorStyleTrackballCamera::OnMouseMove();
            return;
        }

        int* currPos = this->GetInteractor()->GetEventPosition();
        renderer->SetDisplayPoint(currPos[0], currPos[1], 0.0);
        renderer->DisplayToWorld();
        double worldPos1[4] = {0.0, 0.0, 0.0, 0.0};
        renderer->GetWorldPoint(worldPos1);

        renderer->SetDisplayPoint(ClickPos[0], ClickPos[1], 0.0);
        renderer->DisplayToWorld();
        double worldPos0[4] = {0.0, 0.0, 0.0, 0.0};
        renderer->GetWorldPoint(worldPos0);

        const std::array<std::array<double, 3>, 3> directions = {{
            {{1.0, 0.0, 0.0}},
            {{0.0, 1.0, 0.0}},
            {{0.0, 0.0, 1.0}}
        }};

        double moveVec[3] = {
            worldPos1[0] - worldPos0[0],
            worldPos1[1] - worldPos0[1],
            worldPos1[2] - worldPos0[2]
        };
        const double delta = vtkMath::Dot(moveVec, directions[SelectedAxis].data());

        if (std::abs(delta) > 1.0e-12 && AxisDraggedCallback) {
            AxisDraggedCallback(SelectedAxis, delta);
        }

        ClickPos[0] = currPos[0];
        ClickPos[1] = currPos[1];
        this->GetInteractor()->GetRenderWindow()->Render();
    }

    void OnLeftButtonUp() override {
        SelectedAxis = -1;
        HoveredAxis = -1;
        if (Gizmo)
            Gizmo->ClearHighlight();
        this->GetInteractor()->GetRenderWindow()->Render();
        vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
    }

private:
    int pickAxisAt(int x, int y) {
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
        double bestDistance = 1.0e100;
        const double pixelThreshold = 20.0;

        for (int axis = 0; axis < 3; ++axis) {
            if (!Gizmo->IsAxisActive(axis))
                continue;

            double start[3] = {center[0], center[1], center[2]};
            double end[3] = {
                center[0] + directions[axis][0] * length,
                center[1] + directions[axis][1] * length,
                center[2] + directions[axis][2] * length
            };

            double startDisplay[3];
            double endDisplay[3];
            worldToDisplayPoint(renderer, start, startDisplay);
            worldToDisplayPoint(renderer, end, endDisplay);

            const double distance = distancePointToSegment2D(
                static_cast<double>(x),
                static_cast<double>(y),
                startDisplay[0], startDisplay[1],
                endDisplay[0], endDisplay[1]);

            if (distance < bestDistance) {
                bestDistance = distance;
                bestAxis = axis;
            }
        }

        return bestDistance <= pixelThreshold ? bestAxis : -1;
    }

    vtkSmartPointer<TransformGizmo> Gizmo;
    std::function<void(int)> AxisChangedCallback;
    std::function<void(int, double)> AxisDraggedCallback;
    int SelectedAxis = -1;
    int HoveredAxis = -1;
    int ClickPos[2] = {0, 0};
};

vtkStandardNewMacro(ResultClipInteractorStyle);

bool resultFrameIs3D(const ResultFrame& frame)
{
    if (!frame.mesh) {
        return false;
    }

    for (vtkIdType cellId = 0; cellId < frame.mesh->GetNumberOfCells(); ++cellId) {
        vtkCell* cell = frame.mesh->GetCell(cellId);
        if (cell != nullptr && cell->GetCellDimension() == 3) {
            return true;
        }
    }
    return false;
}

void computeResultFrameCentroid(const ResultFrame& frame, double centroid[3])
{
    centroid[0] = 0.0;
    centroid[1] = 0.0;
    centroid[2] = 0.0;
    if (!frame.mesh || frame.mesh->GetNumberOfPoints() == 0) {
        return;
    }

    const vtkIdType pointCount = frame.mesh->GetNumberOfPoints();
    double point[3];
    for (vtkIdType pointId = 0; pointId < pointCount; ++pointId) {
        frame.mesh->GetPoint(pointId, point);
        centroid[0] += point[0];
        centroid[1] += point[1];
        centroid[2] += point[2];
    }

    centroid[0] /= static_cast<double>(pointCount);
    centroid[1] /= static_cast<double>(pointCount);
    centroid[2] /= static_cast<double>(pointCount);
}

void updateClipPlaneDefinition(ResultsViewportToolState& state)
{
    if (!state.clipPlane) {
        state.clipPlane = vtkSmartPointer<vtkPlane>::New();
    }

    double normal[3] = {0.0, 0.0, 0.0};
    normal[std::max(0, std::min(2, state.clipAxis))] = 1.0;
    state.clipPlane->SetOrigin(state.clipOrigin.data());
    state.clipPlane->SetNormal(normal);
}

void removeClipCapActors(VtkViewer& viewer, ResultsViewportToolState& state)
{
    vtkRenderer* renderer = viewer.getRenderer();
    if (renderer != nullptr) {
        for (auto& entry : state.clipCapActors) {
            if (entry.second != nullptr) {
                renderer->RemoveActor(entry.second);
            }
        }
    }
    state.clipCapActors.clear();
}

vtkSmartPointer<vtkActor> buildClipCapActor(vtkActor* sourceActor,
                                            const ResultsViewportToolState& state,
                                            bool preserveScalarColors)
{
    if (sourceActor == nullptr || state.clipPlane == nullptr) {
        return nullptr;
    }

    vtkMapper* sourceMapper = sourceActor->GetMapper();
    if (sourceMapper == nullptr) {
        return nullptr;
    }

    sourceMapper->Update();
    vtkDataObject* input = sourceMapper->GetInputDataObject(0, 0);
    if (input == nullptr) {
        return nullptr;
    }

    vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
    cutter->SetCutFunction(state.clipPlane);
    cutter->SetInputData(input);
    cutter->GenerateCutScalarsOff();

    vtkAlgorithmOutput* capOutput = cutter->GetOutputPort();
    vtkSmartPointer<vtkContourTriangulator> triangulator;
    if (vtkPolyData::SafeDownCast(input) != nullptr) {
        triangulator = vtkSmartPointer<vtkContourTriangulator>::New();
        triangulator->SetInputConnection(cutter->GetOutputPort());
        capOutput = triangulator->GetOutputPort();
    }

    vtkSmartPointer<vtkPolyDataMapper> capMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    capMapper->SetInputConnection(capOutput);
    if (preserveScalarColors && sourceMapper->GetScalarVisibility()) {
        capMapper->ScalarVisibilityOn();
        capMapper->SetScalarMode(sourceMapper->GetScalarMode());
        capMapper->SetColorMode(sourceMapper->GetColorMode());
        capMapper->SetArrayAccessMode(sourceMapper->GetArrayAccessMode());
        capMapper->SetArrayComponent(sourceMapper->GetArrayComponent());
        capMapper->SetLookupTable(sourceMapper->GetLookupTable());
        capMapper->SetScalarRange(sourceMapper->GetScalarRange());
        if (sourceMapper->GetArrayName() != nullptr && sourceMapper->GetArrayName()[0] != '\0') {
            capMapper->SelectColorArray(sourceMapper->GetArrayName());
        } else {
            capMapper->SelectColorArray(sourceMapper->GetArrayId());
        }
    } else {
        capMapper->ScalarVisibilityOff();
    }

    vtkSmartPointer<vtkActor> capActor = vtkSmartPointer<vtkActor>::New();
    capActor->SetMapper(capMapper);
    capActor->PickableOff();
    if (sourceActor->GetProperty() != nullptr && capActor->GetProperty() != nullptr) {
        capActor->GetProperty()->DeepCopy(sourceActor->GetProperty());
        capActor->GetProperty()->SetRepresentationToSurface();
    }
    return capActor;
}

void syncClipCapActor(VtkViewer& viewer,
                      ResultsViewportToolState& state,
                      vtkActor* sourceActor,
                      bool preserveScalarColors)
{
    if (!state.clipEnabled || !state.is3DFrame || sourceActor == nullptr || state.clipPlane == nullptr) {
        return;
    }

    vtkRenderer* renderer = viewer.getRenderer();
    if (renderer == nullptr) {
        return;
    }
    if (!renderer->HasViewProp(sourceActor)) {
        return;
    }

    vtkSmartPointer<vtkActor> capActor = buildClipCapActor(sourceActor, state, preserveScalarColors);
    if (capActor == nullptr) {
        return;
    }

    auto existing = state.clipCapActors.find(sourceActor);
    if (existing != state.clipCapActors.end() && existing->second != nullptr) {
        renderer->RemoveActor(existing->second);
    }

    state.clipCapActors[sourceActor] = capActor;
    renderer->AddActor(capActor);
}

void applyResultsToolState(Editor* editor, VtkViewer& viewer, ResultsViewportToolState& state)
{
    if (editor == nullptr || editor->getResults() == nullptr) {
        return;
    }

    updateClipPlaneDefinition(state);
    removeClipCapActors(viewer, state);

    for (auto& frame : editor->getResults()->frames) {
        if (!frame) {
            continue;
        }

        if (frame->mapper) {
            frame->mapper->RemoveAllClippingPlanes();
            if (state.clipEnabled && state.is3DFrame && state.clipPlane) {
                frame->mapper->AddClippingPlane(state.clipPlane);
            }
            frame->mapper->Modified();
        }

        if (frame->vectorMapper) {
            frame->vectorMapper->RemoveAllClippingPlanes();
            if (state.clipEnabled && state.is3DFrame && state.clipPlane) {
                frame->vectorMapper->AddClippingPlane(state.clipPlane);
            }
            frame->vectorMapper->Modified();
        }

        if (frame->actor && frame->actor->GetProperty()) {
            const double opacity = state.is3DFrame ? static_cast<double>(state.surfaceOpacity) : 1.0;
            frame->actor->GetProperty()->SetOpacity(opacity);
            frame->actor->Modified();
        }

        if (state.clipEnabled && state.is3DFrame) {
            syncClipCapActor(viewer, state, frame->actor, true);
        }
    }
}

vtkSmartPointer<vtkActor> getFirstActiveModelActor()
{
    Model& model = getApp().getActiveModel();
    for (int partIndex = 0; partIndex < model.getPartCount(); ++partIndex) {
        Part* part = model.getPart(partIndex);
        if (part == nullptr) {
            continue;
        }

        if (GraphicMesh* graphicMesh = getApp().getGraphicMeshFromPart(part)) {
            if (vtkSmartPointer<vtkActor> actor = graphicMesh->getActor()) {
                return actor;
            }
        }

        if (vtkOCCTGeom* visual = getApp().getVisualForPart(part)) {
            if (visual->actor != nullptr) {
                return visual->actor;
            }
        }
    }

    return nullptr;
}

bool activeModelIs3D()
{
    return getApp().getActiveModel().getDimension() == 3;
}

bool computeActiveModelCentroid(double centroid[3])
{
    centroid[0] = 0.0;
    centroid[1] = 0.0;
    centroid[2] = 0.0;

    bool hasBounds = false;
    double bounds[6] = {
        std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()
    };

    Model& model = getApp().getActiveModel();
    for (int partIndex = 0; partIndex < model.getPartCount(); ++partIndex) {
        Part* part = model.getPart(partIndex);
        if (part == nullptr) {
            continue;
        }

        vtkActor* actor = nullptr;
        if (GraphicMesh* graphicMesh = getApp().getGraphicMeshFromPart(part)) {
            actor = graphicMesh->getActor();
        }
        if (actor == nullptr) {
            if (vtkOCCTGeom* visual = getApp().getVisualForPart(part)) {
                actor = visual->actor;
            }
        }
        if (actor == nullptr) {
            continue;
        }

        double actorBounds[6];
        actor->GetBounds(actorBounds);
        if (!std::isfinite(actorBounds[0]) || !std::isfinite(actorBounds[1])) {
            continue;
        }

        if (!hasBounds) {
            for (int i = 0; i < 6; ++i)
                bounds[i] = actorBounds[i];
            hasBounds = true;
        } else {
            bounds[0] = (std::min)(bounds[0], actorBounds[0]);
            bounds[1] = (std::max)(bounds[1], actorBounds[1]);
            bounds[2] = (std::min)(bounds[2], actorBounds[2]);
            bounds[3] = (std::max)(bounds[3], actorBounds[3]);
            bounds[4] = (std::min)(bounds[4], actorBounds[4]);
            bounds[5] = (std::max)(bounds[5], actorBounds[5]);
        }
    }

    if (!hasBounds) {
        return false;
    }

    centroid[0] = 0.5 * (bounds[0] + bounds[1]);
    centroid[1] = 0.5 * (bounds[2] + bounds[3]);
    centroid[2] = 0.5 * (bounds[4] + bounds[5]);
    return true;
}

void applyClipAndOpacityToActor(vtkActor* actor, const ResultsViewportToolState& state)
{
    if (actor == nullptr) {
        return;
    }

    if (vtkMapper* mapper = actor->GetMapper()) {
        mapper->RemoveAllClippingPlanes();
        if (state.clipEnabled && state.is3DFrame && state.clipPlane) {
            mapper->AddClippingPlane(state.clipPlane);
        }
        mapper->Modified();
    }

    if (actor->GetProperty() != nullptr) {
        const double opacity = state.is3DFrame ? static_cast<double>(state.surfaceOpacity) : 1.0;
        actor->GetProperty()->SetOpacity(opacity);
        actor->Modified();
    }
}

void applyActiveModelToolState(VtkViewer& viewer, ResultsViewportToolState& state)
{
    updateClipPlaneDefinition(state);
    removeClipCapActors(viewer, state);

    Model& model = getApp().getActiveModel();
    for (int partIndex = 0; partIndex < model.getPartCount(); ++partIndex) {
        Part* part = model.getPart(partIndex);
        if (part == nullptr) {
            continue;
        }

        if (GraphicMesh* graphicMesh = getApp().getGraphicMeshFromPart(part)) {
            applyClipAndOpacityToActor(graphicMesh->getActor(), state);
            syncClipCapActor(viewer, state, graphicMesh->getActor(), false);
        }

        if (vtkOCCTGeom* visual = getApp().getVisualForPart(part)) {
            applyClipAndOpacityToActor(visual->actor, state);
            syncClipCapActor(viewer, state, visual->actor, false);
        }
    }
}

void detachResultsClipTools(VtkViewer& viewer, ResultsViewportToolState& state)
{
    removeClipCapActors(viewer, state);
    if (state.gizmo && state.gizmoAddedToRenderer && viewer.getRenderer() != nullptr) {
        state.gizmo->Hide();
        state.gizmo->RemoveFromRenderer(viewer.getRenderer());
        state.gizmoAddedToRenderer = false;
    }
    if (state.customInteractorInstalled) {
        viewer.restoreDefaultInteractorStyle();
        state.customInteractorInstalled = false;
    }
}

void syncResultsClipTools(VtkViewer& viewer,
                          ResultsViewportToolState& state,
                          ResultFrame& frame)
{
    if (!state.clipEnabled || !state.is3DFrame || frame.actor == nullptr) {
        detachResultsClipTools(viewer, state);
        return;
    }

    if (!state.gizmo) {
        state.gizmo = vtkSmartPointer<TransformGizmo>::New();
        state.gizmo->SetDimension(3);
        state.gizmo->SetViewCenteredPlacementEnabled(false);
    }

    if (!state.clipInteractorStyle) {
        vtkSmartPointer<ResultClipInteractorStyle> style =
            vtkSmartPointer<ResultClipInteractorStyle>::New();
        style->SetTransformGizmo(state.gizmo);
        style->SetAxisChangedCallback([&state](int axis) {
            state.clipAxis = axis;
            updateClipPlaneDefinition(state);
        });
        style->SetAxisDraggedCallback([&viewer, &state](int axis, double delta) {
            state.clipAxis = axis;
            state.clipOrigin[axis] += delta;
            updateClipPlaneDefinition(state);
            if (state.gizmo) {
                state.gizmo->SetDragCenterPosition(state.clipOrigin.data());
                state.gizmo->SetOriginPosition(state.clipOrigin.data());
            }
            if (viewer.getRenderer() != nullptr) {
                viewer.getRenderer()->ResetCameraClippingRange();
            }
        });
        state.clipInteractorStyle = style;
    }

    state.gizmo->SetReferenceRenderer(viewer.getRenderer());
    state.gizmo->SetTargetActor(frame.actor);
    state.gizmo->SetDragCenterPosition(state.clipOrigin.data());
    state.gizmo->SetOriginPosition(state.clipOrigin.data());
    state.gizmo->Show();

    if (!state.gizmoAddedToRenderer && viewer.getRenderer() != nullptr) {
        state.gizmo->AddToRenderer(viewer.getRenderer());
        state.gizmoAddedToRenderer = true;
    }

    if (!state.customInteractorInstalled && viewer.getInteractor() != nullptr) {
        state.clipInteractorStyle->SetDefaultRenderer(viewer.getRenderer());
        viewer.getInteractor()->SetInteractorStyle(state.clipInteractorStyle);
        viewer.getInteractor()->EnableRenderOff();
        state.customInteractorInstalled = true;
    }
}

void syncModelClipTools(VtkViewer& viewer, ResultsViewportToolState& state)
{
    if (!state.clipEnabled || !state.is3DFrame) {
        detachResultsClipTools(viewer, state);
        return;
    }

    vtkSmartPointer<vtkActor> targetActor = getFirstActiveModelActor();
    if (targetActor == nullptr) {
        detachResultsClipTools(viewer, state);
        return;
    }

    if (!state.gizmo) {
        state.gizmo = vtkSmartPointer<TransformGizmo>::New();
        state.gizmo->SetDimension(3);
        state.gizmo->SetViewCenteredPlacementEnabled(false);
    }

    if (!state.clipInteractorStyle) {
        vtkSmartPointer<ResultClipInteractorStyle> style =
            vtkSmartPointer<ResultClipInteractorStyle>::New();
        style->SetTransformGizmo(state.gizmo);
        style->SetAxisChangedCallback([&state](int axis) {
            state.clipAxis = axis;
            updateClipPlaneDefinition(state);
        });
        style->SetAxisDraggedCallback([&viewer, &state](int axis, double delta) {
            state.clipAxis = axis;
            state.clipOrigin[axis] += delta;
            updateClipPlaneDefinition(state);
            if (state.gizmo) {
                state.gizmo->SetDragCenterPosition(state.clipOrigin.data());
                state.gizmo->SetOriginPosition(state.clipOrigin.data());
            }
            if (viewer.getRenderer() != nullptr) {
                viewer.getRenderer()->ResetCameraClippingRange();
            }
        });
        state.clipInteractorStyle = style;
    }

    state.gizmo->SetReferenceRenderer(viewer.getRenderer());
    state.gizmo->SetTargetActor(targetActor);
    state.gizmo->SetDragCenterPosition(state.clipOrigin.data());
    state.gizmo->SetOriginPosition(state.clipOrigin.data());
    state.gizmo->Show();

    if (!state.gizmoAddedToRenderer && viewer.getRenderer() != nullptr) {
        state.gizmo->AddToRenderer(viewer.getRenderer());
        state.gizmoAddedToRenderer = true;
    }

    if (!state.customInteractorInstalled && viewer.getInteractor() != nullptr) {
        state.clipInteractorStyle->SetDefaultRenderer(viewer.getRenderer());
        viewer.getInteractor()->SetInteractorStyle(state.clipInteractorStyle);
        viewer.getInteractor()->EnableRenderOff();
        state.customInteractorInstalled = true;
    }
}

void applyDisplayModeToActor(vtkActor* actor, ModelDisplayMode mode, bool showEdges, bool preserveScalarColors)
{
    if (actor == nullptr || actor->GetProperty() == nullptr) {
        return;
    }

    vtkMapper* mapper = actor->GetMapper();
    if (mapper != nullptr) {
        if (preserveScalarColors) {
            mapper->ScalarVisibilityOn();
        } else {
            mapper->ScalarVisibilityOff();
        }
    }

    const bool hasScalarColors =
        preserveScalarColors && mapper != nullptr && mapper->GetScalarVisibility();
    vtkProperty* property = actor->GetProperty();
    switch (mode) {
    case ModelDisplayMode::Surface:
        property->SetRepresentationToSurface();
        if (showEdges) {
            property->EdgeVisibilityOn();
            property->SetEdgeColor(0.0, 0.0, 0.0);
        } else {
            property->EdgeVisibilityOff();
        }
        if (!hasScalarColors) {
            property->SetColor(0.84, 0.84, 0.84);
        }
        break;
    case ModelDisplayMode::Wireframe:
        property->SetRepresentationToWireframe();
        property->EdgeVisibilityOff();
        property->SetColor(0.0, 0.0, 0.0);
        break;
    }
}

void applyDisplayModeToGeometryActor(vtkOCCTGeom* visual, ModelDisplayMode mode)
{
    if (visual == nullptr || visual->actor == nullptr || visual->actor->GetProperty() == nullptr) {
        return;
    }

    vtkMapper* mapper = visual->actor->GetMapper();
    if (mapper != nullptr) {
        mapper->ScalarVisibilityOff();
    }

    vtkProperty* property = visual->actor->GetProperty();
    if (visual->hasOnlyLineCells()) {
        property->SetRepresentationToWireframe();
        property->EdgeVisibilityOff();
        property->SetOpacity(1.0);
        property->SetLineWidth(4.0);
        property->SetRenderLinesAsTubes(true);
        property->SetColor(0.45, 0.45, 0.45);
        return;
    }

    property->SetRenderLinesAsTubes(false);
    property->SetLineWidth(1.0);
    property->EdgeVisibilityOff();
    property->SetColor(0.84, 0.84, 0.84);
    if (mode == ModelDisplayMode::Wireframe) {
        property->SetRepresentationToWireframe();
    } else {
        property->SetRepresentationToSurface();
    }
}

void applyDisplayModeToActiveModel(ModelDisplayMode mode, bool showEdges)
{
    Model& model = getApp().getActiveModel();
    for (int p = 0; p < model.getPartCount(); ++p) {
        Part* part = model.getPart(p);
        if (part == nullptr) {
            continue;
        }

        if (GraphicMesh* graphicMesh = getApp().getGraphicMeshFromPart(part)) {
            applyDisplayModeToActor(graphicMesh->getActor(), mode, showEdges, false);
        }

        if (vtkOCCTGeom* visual = getApp().getVisualForPart(part)) {
            applyDisplayModeToGeometryActor(visual, mode);
        }
    }
}

void applyDisplayModeToResults(ModelDisplayMode mode, bool showEdges, Editor* editor)
{
    if (editor == nullptr || editor->getResults() == nullptr) {
        return;
    }

    for (auto& frame : editor->getResults()->frames) {
        if (!frame || !frame->actor) {
            continue;
        }
        applyDisplayModeToActor(frame->actor, mode, showEdges, true);
    }
}

bool drawToolbarButton(const char* label, bool active = false, const char* tooltip = nullptr)
{
    const ImVec2 buttonSize(0.0f, 24.0f);
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.48f, 0.86f, 0.92f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.55f, 0.92f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.40f, 0.78f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.11f, 0.14f, 0.18f, 0.42f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.16f, 0.20f, 0.26f, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.25f, 0.32f, 0.82f));
    }

    const bool pressed = ImGui::Button(label, buttonSize);
    if (tooltip != nullptr && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }

    ImGui::PopStyleColor(3);

    return pressed;
}

bool drawAxisButton(const char* label, const ImVec4& baseColor)
{
    const ImVec2 buttonSize(24.0f, 24.0f);
    const ImVec4 hoveredColor(
        (std::min)(baseColor.x + 0.10f, 1.0f),
        (std::min)(baseColor.y + 0.10f, 1.0f),
        (std::min)(baseColor.z + 0.10f, 1.0f),
        1.0f);
    const ImVec4 activeColor(
        (std::max)(baseColor.x - 0.08f, 0.0f),
        (std::max)(baseColor.y - 0.08f, 0.0f),
        (std::max)(baseColor.z - 0.08f, 0.0f),
        1.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.949f, 0.957f, 0.965f, 1.0f));

    const bool pressed = ImGui::Button(label, buttonSize);

    ImGui::PopStyleColor(4);
    return pressed;
}

void drawOverlaySeparator()
{
    ImGui::SameLine(0.0f, 8.0f);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddLine(ImVec2(pos.x, pos.y + 3.0f), ImVec2(pos.x, pos.y + 21.0f),
                       IM_COL32(255, 255, 255, 28), 1.0f);
    ImGui::Dummy(ImVec2(1.0f, 24.0f));
    ImGui::SameLine(0.0f, 8.0f);
}

template <typename DrawFn>
bool drawOverlayIconButton(const char* id,
                           bool active,
                           const char* tooltip,
                           DrawFn&& drawFn)
{
    const ImVec2 size(24.0f, 24.0f);
    ImGui::PushID(id);
    const bool pressed = ImGui::InvisibleButton("##icon", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 bgColor = active ? IM_COL32(61, 122, 219, 235) : IM_COL32(28, 34, 44, 110);
    if (held) {
        bgColor = active ? IM_COL32(53, 106, 191, 255) : IM_COL32(46, 74, 122, 220);
    } else if (hovered) {
        bgColor = active ? IM_COL32(70, 132, 232, 245) : IM_COL32(54, 64, 80, 180);
    }

    const ImU32 strokeColor = hovered || held
        ? IM_COL32(255, 255, 255, 255)
        : IM_COL32(220, 224, 230, 255);

    drawList->AddRectFilled(min, max, bgColor, 6.0f);
    drawList->AddRect(min, max, IM_COL32(255, 255, 255, 18), 6.0f, 0, 1.0f);
    drawFn(drawList, min, max, strokeColor);

    if (tooltip != nullptr && hovered) {
        ImGui::SetTooltip("%s", tooltip);
    }

    ImGui::PopID();
    return pressed;
}

bool drawClipIconButton(bool active, const char* tooltip = nullptr)
{
    return drawOverlayIconButton("clip_icon_button", active, tooltip,
        [](ImDrawList* drawList, const ImVec2& min, const ImVec2& max, ImU32 strokeColor) {
            const float left = min.x + 6.0f;
            const float right = max.x - 6.0f;
            const float top = min.y + 6.0f;
            const float bottom = max.y - 6.0f;
            const float thickness = 1.5f;
            drawList->AddRect(ImVec2(left, top), ImVec2(right, bottom), strokeColor, 2.0f, 0, thickness);
            const float lineX0 = left + 3.0f;
            const float lineY0 = bottom + 1.5f;
            const float lineX1 = right - 1.5f;
            const float lineY1 = top - 2.5f;
            drawList->AddLine(ImVec2(lineX0, lineY0),
                              ImVec2(lineX1, lineY1),
                              strokeColor,
                              1.7f);
        });
}

bool drawMeshDisplayButton(const char* id, bool active, bool filled, const char* tooltip = nullptr)
{
    return drawOverlayIconButton(id, active, tooltip,
        [filled](ImDrawList* drawList, const ImVec2& min, const ImVec2& max, ImU32 strokeColor) {
            const float left = min.x + 5.5f;
            const float right = max.x - 5.5f;
            const float top = min.y + 5.5f;
            const float bottom = max.y - 5.5f;
            const float midX = (left + right) * 0.5f;
            const float midY = (top + bottom) * 0.5f;
            const float thickness = 1.35f;

            if (filled) {
                drawList->AddRectFilled(ImVec2(left, top),
                                        ImVec2(right, bottom),
                                        IM_COL32(170, 176, 186, 130),
                                        2.0f);
            }

            drawList->AddRect(ImVec2(left, top), ImVec2(right, bottom), strokeColor, 2.0f, 0, thickness);
            drawList->AddLine(ImVec2(midX, top), ImVec2(midX, bottom), strokeColor, thickness);
            drawList->AddLine(ImVec2(left, midY), ImVec2(right, midY), strokeColor, thickness);
        });
}

bool drawOpacityIconButton(bool active, const char* tooltip = nullptr)
{
    return drawOverlayIconButton("opacity_icon_button", active, tooltip,
        [](ImDrawList* drawList, const ImVec2& min, const ImVec2& max, ImU32 strokeColor) {
            const ImVec2 center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
            const float eyeHalfWidth = 7.0f;
            const float eyeHalfHeight = 4.2f;
            const float pupilRadius = 2.2f;
            const float thickness = 1.45f;

            drawList->AddBezierCubic(ImVec2(center.x - eyeHalfWidth, center.y),
                                     ImVec2(center.x - 3.8f, center.y - eyeHalfHeight),
                                     ImVec2(center.x + 3.8f, center.y - eyeHalfHeight),
                                     ImVec2(center.x + eyeHalfWidth, center.y),
                                     strokeColor,
                                     thickness);
            drawList->AddBezierCubic(ImVec2(center.x - eyeHalfWidth, center.y),
                                     ImVec2(center.x - 3.8f, center.y + eyeHalfHeight),
                                     ImVec2(center.x + 3.8f, center.y + eyeHalfHeight),
                                     ImVec2(center.x + eyeHalfWidth, center.y),
                                     strokeColor,
                                     thickness);
            drawList->AddCircleFilled(center, pupilRadius, strokeColor);
        });
}

bool drawProjectionButton(const char* id, bool orthographic, bool active, const char* tooltip)
{
    const ImVec2 size(24.0f, 24.0f);
    ImGui::PushID(id);
    const bool pressed = ImGui::InvisibleButton("##projection", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 bgColor = active ? IM_COL32(61, 122, 219, 235) : IM_COL32(28, 34, 44, 110);
    if (held) {
        bgColor = active ? IM_COL32(53, 106, 191, 255) : IM_COL32(46, 74, 122, 220);
    } else if (hovered) {
        bgColor = active ? IM_COL32(70, 132, 232, 245) : IM_COL32(54, 64, 80, 180);
    }

    const ImU32 strokeColor = IM_COL32(236, 240, 245, 255);
    drawList->AddRectFilled(min, max, bgColor, 6.0f);
    drawList->AddRect(min, max, IM_COL32(255, 255, 255, 18), 6.0f, 0, 1.0f);

    const float left = min.x + 5.0f;
    const float right = max.x - 5.0f;
    const float top = min.y + 6.0f;
    const float bottom = max.y - 5.0f;
    const float thickness = 1.4f;

    if (orthographic) {
        const float x1 = left + 1.0f;
        const float x2 = (left + right) * 0.5f;
        const float x3 = right - 1.0f;
        drawList->AddLine(ImVec2(x1, top), ImVec2(x1, bottom), strokeColor, thickness);
        drawList->AddLine(ImVec2(x2, top), ImVec2(x2, bottom), strokeColor, thickness);
        drawList->AddLine(ImVec2(x3, top), ImVec2(x3, bottom), strokeColor, thickness);
        drawList->AddLine(ImVec2(x1 - 1.0f, top), ImVec2(x3 + 1.0f, top), strokeColor, thickness);
        drawList->AddLine(ImVec2(x1 - 1.0f, bottom), ImVec2(x3 + 1.0f, bottom), strokeColor, thickness);
    } else {
        const float offset = 3.0f;
        drawList->AddLine(ImVec2(left + offset, top), ImVec2(right, top + 1.0f), strokeColor, thickness);
        drawList->AddLine(ImVec2(left, bottom - 1.0f), ImVec2(right - offset, bottom), strokeColor, thickness);
        drawList->AddLine(ImVec2(left + offset, top), ImVec2(left, bottom - 1.0f), strokeColor, thickness);
        drawList->AddLine(ImVec2((left + right) * 0.5f + 1.0f, top + 1.0f),
                          ImVec2((left + right) * 0.5f - 2.0f, bottom), strokeColor, thickness);
        drawList->AddLine(ImVec2(right, top + 1.0f), ImVec2(right - offset, bottom), strokeColor, thickness);
    }

    if (tooltip != nullptr && hovered) {
        ImGui::SetTooltip("%s", tooltip);
    }

    ImGui::PopID();
    return pressed;
}

bool drawFitIconButton()
{
    const ImVec2 size(24.0f, 24.0f);
    ImGui::PushID("fit_icon_button");
    const bool pressed = ImGui::InvisibleButton("##fit", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 bgColor = IM_COL32(28, 34, 44, 110);
    if (held) {
        bgColor = IM_COL32(46, 74, 122, 220);
    } else if (hovered) {
        bgColor = IM_COL32(54, 64, 80, 180);
    }

    const ImU32 strokeColor = hovered || held
        ? IM_COL32(255, 255, 255, 255)
        : IM_COL32(220, 224, 230, 255);

    drawList->AddRectFilled(min, max, bgColor, 6.0f);
    drawList->AddRect(min, max, IM_COL32(255, 255, 255, 18), 6.0f, 0, 1.0f);

    const float pad = 5.5f;
    const float arm = 4.0f;
    const float thickness = 1.6f;

    const float left = min.x + pad;
    const float right = max.x - pad;
    const float top = min.y + pad;
    const float bottom = max.y - pad;

    drawList->AddLine(ImVec2(left, top + arm), ImVec2(left, top), strokeColor, thickness);
    drawList->AddLine(ImVec2(left, top), ImVec2(left + arm, top), strokeColor, thickness);

    drawList->AddLine(ImVec2(right - arm, top), ImVec2(right, top), strokeColor, thickness);
    drawList->AddLine(ImVec2(right, top), ImVec2(right, top + arm), strokeColor, thickness);

    drawList->AddLine(ImVec2(left, bottom - arm), ImVec2(left, bottom), strokeColor, thickness);
    drawList->AddLine(ImVec2(left, bottom), ImVec2(left + arm, bottom), strokeColor, thickness);

    drawList->AddLine(ImVec2(right - arm, bottom), ImVec2(right, bottom), strokeColor, thickness);
    drawList->AddLine(ImVec2(right, bottom - arm), ImVec2(right, bottom), strokeColor, thickness);

    if (hovered) {
        ImGui::SetTooltip("Fit");
    }

    ImGui::PopID();
    return pressed;
}

bool drawScreenshotIconButton()
{
    const ImVec2 size(24.0f, 24.0f);
    ImGui::PushID("screenshot_icon_button");
    const bool pressed = ImGui::InvisibleButton("##screenshot", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 bgColor = IM_COL32(28, 34, 44, 110);
    if (held) {
        bgColor = IM_COL32(46, 74, 122, 220);
    } else if (hovered) {
        bgColor = IM_COL32(54, 64, 80, 180);
    }

    const ImU32 strokeColor = hovered || held
        ? IM_COL32(255, 255, 255, 255)
        : IM_COL32(220, 224, 230, 255);

    drawList->AddRectFilled(min, max, bgColor, 6.0f);
    drawList->AddRect(min, max, IM_COL32(255, 255, 255, 18), 6.0f, 0, 1.0f);

    const float left = min.x + 5.0f;
    const float right = max.x - 5.0f;
    const float top = min.y + 6.0f;
    const float bottom = max.y - 5.0f;
    const float thickness = 1.5f;

    drawList->AddLine(ImVec2(left, top + 4.0f), ImVec2(left, top), strokeColor, thickness);
    drawList->AddLine(ImVec2(left, top), ImVec2(left + 4.0f, top), strokeColor, thickness);
    drawList->AddLine(ImVec2(right - 4.0f, top), ImVec2(right, top), strokeColor, thickness);
    drawList->AddLine(ImVec2(right, top), ImVec2(right, top + 4.0f), strokeColor, thickness);
    drawList->AddLine(ImVec2(left, bottom - 4.0f), ImVec2(left, bottom), strokeColor, thickness);
    drawList->AddLine(ImVec2(left, bottom), ImVec2(left + 4.0f, bottom), strokeColor, thickness);
    drawList->AddLine(ImVec2(right - 4.0f, bottom), ImVec2(right, bottom), strokeColor, thickness);
    drawList->AddLine(ImVec2(right, bottom - 4.0f), ImVec2(right, bottom), strokeColor, thickness);

    const ImVec2 center((left + right) * 0.5f, (top + bottom) * 0.5f);
    drawList->AddCircle(center, 4.0f, strokeColor, 0, thickness);

    if (hovered) {
        ImGui::SetTooltip("Screenshot");
    }

    ImGui::PopID();
    return pressed;
}

bool drawMeasureIconButton(bool active, const char* tooltip = nullptr)
{
    const ImVec2 size(24.0f, 24.0f);
    ImGui::PushID("measure_icon_button");
    const bool pressed = ImGui::InvisibleButton("##measure", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 bgColor = active ? IM_COL32(168, 118, 26, 210) : IM_COL32(28, 34, 44, 110);
    if (held) {
        bgColor = IM_COL32(46, 74, 122, 220);
    } else if (hovered) {
        bgColor = active ? IM_COL32(192, 138, 32, 225) : IM_COL32(54, 64, 80, 180);
    }

    const ImU32 strokeColor = active || hovered || held
        ? IM_COL32(255, 245, 210, 255)
        : IM_COL32(220, 224, 230, 255);

    drawList->AddRectFilled(min, max, bgColor, 6.0f);
    drawList->AddRect(min, max, IM_COL32(255, 255, 255, 18), 6.0f, 0, 1.0f);

    const float left = min.x + 5.0f;
    const float right = max.x - 5.0f;
    const float centerY = (min.y + max.y) * 0.5f;
    const float thickness = 1.6f;
    drawList->AddLine(ImVec2(left, centerY + 4.0f), ImVec2(right - 2.0f, centerY - 4.0f), strokeColor, thickness);

    for (int i = 0; i < 4; ++i) {
        const float t = static_cast<float>(i) / 4.0f;
        const float x = left + (right - left - 3.0f) * t;
        const float y = centerY + 4.0f - 8.0f * t;
        const float tick = (i % 2 == 0) ? 3.0f : 2.0f;
        drawList->AddLine(ImVec2(x, y), ImVec2(x + 1.5f, y - tick), strokeColor, thickness * 0.85f);
    }

    if (hovered) {
        ImGui::SetTooltip("%s", tooltip != nullptr ? tooltip : "Measure distance");
    }

    ImGui::PopID();
    return pressed;
}

void drawSelectionCountBadge(int selectedNodeCount, int selectedElementCount)
{
    if (selectedNodeCount <= 0 && selectedElementCount <= 0) {
        return;
    }

    std::string label;
    if (selectedNodeCount > 0) {
        label += "N: " + std::to_string(selectedNodeCount);
    }
    if (selectedElementCount > 0) {
        if (!label.empty()) {
            label += "  ";
        }
        label += "E: " + std::to_string(selectedElementCount);
    }
    const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
    const ImVec2 size(textSize.x + 18.0f, 24.0f);

    ImGui::PushID("selection_count_badge");
    ImGui::InvisibleButton("##selection_count", size);

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(min, max, IM_COL32(212, 176, 64, 120), 10.0f);
    drawList->AddRect(min, max, IM_COL32(255, 244, 196, 70), 10.0f, 0, 1.0f);
    drawList->AddText(ImVec2(min.x + 9.0f, min.y + 4.0f),
                      IM_COL32(255, 248, 220, 255),
                      label.c_str());

    if (ImGui::IsItemHovered()) {
        if (selectedNodeCount > 0 && selectedElementCount > 0) {
            ImGui::SetTooltip("%d selected nodes, %d selected elements",
                              selectedNodeCount,
                              selectedElementCount);
        } else if (selectedNodeCount > 0) {
            ImGui::SetTooltip("%d selected nodes", selectedNodeCount);
        } else {
            ImGui::SetTooltip("%d selected elements", selectedElementCount);
        }
    }

    ImGui::PopID();
}

void drawViewportOverlay(VtkViewer& viewer,
                         ModelViewportOverlayState& state,
                         const char* windowId,
                         bool allowAxes,
                         Editor* editor = nullptr,
                         ResultsViewportToolState* resultsTools = nullptr)
{
    const ImVec2 viewportMin = viewer.getViewportScreenMin();
    const ImVec2 viewportMax = viewer.getViewportScreenMax();
    if (viewportMax.x <= viewportMin.x || viewportMax.y <= viewportMin.y) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(viewportMin.x + 12.0f, viewportMin.y + 12.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f),
                                        ImVec2((viewportMax.x - viewportMin.x) - 24.0f,
                                               (std::numeric_limits<float>::max)()));
    ImGui::SetNextWindowBgAlpha(0.18f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.05f, 0.07f, 0.18f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.06f));

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoMove;

    if (ImGui::Begin(windowId, nullptr, flags)) {
        if (drawFitIconButton()) {
            viewer.resetCamera();
        }
        ImGui::SameLine();
        if (drawAxisButton("X", ImVec4(0.70f, 0.24f, 0.24f, 1.0f))) {
            viewer.orientCameraToAxis(0);
        }
        ImGui::SameLine();
        if (drawAxisButton("Y", ImVec4(0.22f, 0.58f, 0.28f, 1.0f))) {
            viewer.orientCameraToAxis(1);
        }
        ImGui::SameLine();
        if (drawAxisButton("Z", ImVec4(0.24f, 0.40f, 0.76f, 1.0f))) {
            viewer.orientCameraToAxis(2);
        }

        drawOverlaySeparator();
        if (drawProjectionButton("ortho", true, state.orthographic, "Orthographic")) {
            state.orthographic = true;
            viewer.setProjectionMode(VtkViewer::ProjectionMode::Orthographic);
        }
        ImGui::SameLine();
        if (drawProjectionButton("persp", false, !state.orthographic, "Perspective")) {
            state.orthographic = false;
            viewer.setProjectionMode(VtkViewer::ProjectionMode::Perspective);
        }
        if (allowAxes) {
            ImGui::SameLine();
            if (drawToolbarButton("Ax", state.axesVisible, "Axes")) {
                state.axesVisible = !state.axesVisible;
                viewer.setAxesVisible(state.axesVisible);
            }
        }

        drawOverlaySeparator();
        if (resultsTools != nullptr && resultsTools->is3DFrame) {
            if (drawOpacityIconButton(resultsTools->transparencyControlsVisible, "Transparency")) {
                resultsTools->transparencyControlsVisible = !resultsTools->transparencyControlsVisible;
            }

            if (resultsTools->transparencyControlsVisible) {
                ImGui::SameLine();
                float transparencyPercent = resultsTools->surfaceOpacity * 100.0f;
                ImGui::SetNextItemWidth(96.0f);
                if (ImGui::SliderFloat("##surface_opacity",
                                       &transparencyPercent,
                                       8.0f,
                                       100.0f,
                                       "%.0f%%",
                                       ImGuiSliderFlags_AlwaysClamp)) {
                    resultsTools->surfaceOpacity = transparencyPercent / 100.0f;
                }
            }
            ImGui::SameLine();
        }

        bool wireframeEnabled = state.displayMode == ModelDisplayMode::Wireframe;
        if (drawMeshDisplayButton("wireframe_icon_button",
                                  wireframeEnabled,
                                  false,
                                  "Wireframe")) {
            state.displayMode = wireframeEnabled ? ModelDisplayMode::Surface : ModelDisplayMode::Wireframe;
        }
        ImGui::SameLine();
        if (drawMeshDisplayButton("edges_icon_button", state.showEdges, true, "Edges")) {
            state.showEdges = !state.showEdges;
        }

        if (resultsTools != nullptr && resultsTools->is3DFrame) {
            ImGui::SameLine();
            if (drawClipIconButton(resultsTools->clipEnabled, "Clip plane")) {
                resultsTools->clipEnabled = !resultsTools->clipEnabled;
                if (!resultsTools->clipEnabled) {
                    resultsTools->clipOriginInitialized = false;
                }
            }
            if (resultsTools->clipEnabled) {
                ImGui::SameLine();
                if (drawAxisButton("X", ImVec4(0.70f, 0.24f, 0.24f, 1.0f))) {
                    resultsTools->clipAxis = 0;
                }
                ImGui::SameLine();
                if (drawAxisButton("Y", ImVec4(0.22f, 0.58f, 0.28f, 1.0f))) {
                    resultsTools->clipAxis = 1;
                }
                ImGui::SameLine();
                if (drawAxisButton("Z", ImVec4(0.24f, 0.40f, 0.76f, 1.0f))) {
                    resultsTools->clipAxis = 2;
                }
            }
        }

        drawOverlaySeparator();
        if (drawScreenshotIconButton()) {
            viewer.saveScreenshot();
        }
        if (editor != nullptr) {
            ImGui::SameLine();
            if (drawToolbarButton("Ni", editor->getShowAllNodeLabelsForViewer(&viewer), "Show all node ids")) {
                editor->setShowAllNodeLabelsForViewer(&viewer, !editor->getShowAllNodeLabelsForViewer(&viewer));
            }
            ImGui::SameLine();
            if (drawToolbarButton("Ei", editor->getShowAllElementLabelsForViewer(&viewer), "Show all element ids")) {
                editor->setShowAllElementLabelsForViewer(&viewer, !editor->getShowAllElementLabelsForViewer(&viewer));
            }
            ImGui::SameLine();
            if (drawMeasureIconButton(editor->isMeasurementEnabled(), "Measure distance")) {
                editor->setMeasurementEnabled(!editor->isMeasurementEnabled());
            }
        }
        if (editor != nullptr &&
            (editor->getSelectedNodeCount() > 0 || editor->getSelectedElementCount() > 0)) {
            ImGui::SameLine();
            drawSelectionCountBadge(editor->getSelectedNodeCount(),
                                    editor->getSelectedElementCount());
            ImGui::SameLine();
            if (drawToolbarButton("Id", editor->getShowSelectedNodeLabels(), "Show selected node/element ids")) {
                editor->setShowSelectedNodeLabels(!editor->getShowSelectedNodeLabels());
            }
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);

    viewer.setProjectionMode(state.orthographic
        ? VtkViewer::ProjectionMode::Orthographic
        : VtkViewer::ProjectionMode::Perspective);
    if (allowAxes) {
        viewer.setAxesVisible(state.axesVisible);
    }
}

void drawResultsPlaybackOverlay(VtkViewer& viewer,
                                ResultsPlaybackOverlayState& overlayState,
                                int currentFrame,
                                int totalFrames,
                                double currentTime,
                                bool& isPlaying,
                                const std::vector<std::string>& fieldNames,
                                const std::string& activeFieldName,
                                int& selectedField,
                                bool& fieldSelectionChanged,
                                bool& manualColorScale,
                                bool& colorScaleModeChanged,
                                int activeFieldComponents,
                                int& selectedFieldComponent,
                                bool& componentChanged,
                                bool vectorFieldAvailable,
                                bool& showVectorGlyphs,
                                bool& vectorGlyphVisibilityChanged,
                                double& manualMin,
                                double& manualMax,
                                bool& manualRangeChanged)
{
    const ImVec2 viewportMin = viewer.getViewportScreenMin();
    const ImVec2 viewportMax = viewer.getViewportScreenMax();
    if (viewportMax.x <= viewportMin.x || viewportMax.y <= viewportMin.y) {
        return;
    }

    const ImVec2 defaultPos(viewportMin.x + 12.0f, viewportMin.y + 56.0f);
    if (!overlayState.visible) {
        ImGui::SetNextWindowPos(defaultPos, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.24f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.05f, 0.07f, 0.24f));
        const ImGuiWindowFlags restoreFlags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoMove;
        if (ImGui::Begin("##ResultsPlaybackOverlayRestore", nullptr, restoreFlags)) {
            overlayState.lastMin = ImGui::GetWindowPos();
            overlayState.lastMax = ImVec2(overlayState.lastMin.x + ImGui::GetWindowSize().x,
                                          overlayState.lastMin.y + ImGui::GetWindowSize().y);
            if (ImGui::Button("Results")) {
                overlayState.visible = true;
                overlayState.minimized = false;
            }
        }
        ImGui::End();
        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(2);
        return;
    }

    ImGui::SetNextWindowPos(defaultPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(255.0f, overlayState.minimized ? 48.0f : 305.0f),
                             overlayState.minimized ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(210.0f, 42.0f),
                                        ImVec2(std::max(230.0f, viewportMax.x - viewportMin.x - 24.0f),
                                               std::max(120.0f, viewportMax.y - viewportMin.y - 70.0f)));
    ImGui::SetNextWindowBgAlpha(0.24f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.05f, 0.07f, 0.22f));

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoNavFocus;

    if (ImGui::Begin("Results", nullptr, flags)) {
        overlayState.lastMin = ImGui::GetWindowPos();
        overlayState.lastMax = ImVec2(overlayState.lastMin.x + ImGui::GetWindowSize().x,
                                      overlayState.lastMin.y + ImGui::GetWindowSize().y);
        const float headerButtonWidth = 24.0f;
        const float headerButtonStart = ImGui::GetWindowContentRegionMax().x - 2.0f * headerButtonWidth - 4.0f;
        ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), headerButtonStart));
        if (ImGui::SmallButton(overlayState.minimized ? "+" : "-")) {
            overlayState.minimized = !overlayState.minimized;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("x")) {
            overlayState.visible = false;
        }

        if (overlayState.minimized) {
            ImGui::End();
            ImGui::PopStyleColor(1);
            ImGui::PopStyleVar(3);
            return;
        }

        if (ImGui::Button(isPlaying ? "Pause" : "Play")) {
            isPlaying = !isPlaying;
        }
        ImGui::SameLine();
        ImGui::Text("Frame %d / %d", currentFrame + 1, totalFrames);
        ImGui::Text("Time: %.6g", currentTime);
        if (!fieldNames.empty()) {
            std::vector<const char*> fieldCStrs;
            fieldCStrs.reserve(fieldNames.size());
            for (const auto& fieldName : fieldNames) {
                fieldCStrs.push_back(fieldName.c_str());
            }

            if (selectedField < 0 || selectedField >= static_cast<int>(fieldCStrs.size())) {
                selectedField = 0;
            }

            ImGui::SetNextItemWidth(168.0f);
            fieldSelectionChanged = ImGui::Combo("Active Field", &selectedField,
                                                 fieldCStrs.data(),
                                                 static_cast<int>(fieldCStrs.size()));

            if (activeFieldComponents > 1) {
                ImGui::Text("Component");
                int buttonCount = activeFieldComponents;
                const bool vectorMagnitudeButton = (activeFieldComponents == 3);
                if (vectorMagnitudeButton)
                    buttonCount = 4;

                for (int comp = 0; comp < buttonCount; ++comp) {
                    std::string label;
                    if (comp == 0) label = "X";
                    else if (comp == 1) label = "Y";
                    else if (comp == 2) label = "Z";
                    else if (comp == 3) label = "|V|";
                    else label = std::to_string(comp);
                    const bool isSelected = (selectedFieldComponent == comp);
                    if (isSelected)
                        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

                    if (ImGui::Button(label.c_str())) {
                        selectedFieldComponent = comp;
                        componentChanged = true;
                    }

                    if (isSelected)
                        ImGui::PopStyleColor();

                    if (comp + 1 < buttonCount)
                        ImGui::SameLine();
                }

                const char* selectedComponentLabel = "All";
                if (selectedFieldComponent == 0) selectedComponentLabel = "X";
                else if (selectedFieldComponent == 1) selectedComponentLabel = "Y";
                else if (selectedFieldComponent == 2) selectedComponentLabel = "Z";
                else if (selectedFieldComponent == 3) selectedComponentLabel = "|V|";
                ImGui::Text("Viewing: %s [%s]", activeFieldName.c_str(), selectedComponentLabel);
            } else if (!activeFieldName.empty()) {
                ImGui::Text("Viewing: %s", activeFieldName.c_str());
            }

            if (vectorFieldAvailable) {
                bool toggled = ImGui::Checkbox("Show Vector Shape", &showVectorGlyphs);
                if (toggled)
                    vectorGlyphVisibilityChanged = true;
            }

            const bool autoScale = !manualColorScale;
            if (ImGui::RadioButton("Auto", autoScale)) {
                manualColorScale = false;
                colorScaleModeChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Manual", manualColorScale)) {
                manualColorScale = true;
                colorScaleModeChanged = true;
            }

            if (manualColorScale) {
                ImGui::SetNextItemWidth(168.0f);
                manualRangeChanged |= ImGui::InputDouble("Min", &manualMin, 0.0, 0.0, "%.6g");
                ImGui::SetNextItemWidth(168.0f);
                manualRangeChanged |= ImGui::InputDouble("Max", &manualMax, 0.0, 0.0, "%.6g");
                if (manualMax < manualMin)
                    manualMax = manualMin;
            }

            if (ImGui::CollapsingHeader("Colorbar")) {
                ImGui::SetNextItemWidth(168.0f);
                ImGui::SliderFloat("Height", &overlayState.scalarBarHeightPx, 60.0f, 260.0f, "%.0f px");
                ImGui::SetNextItemWidth(168.0f);
                ImGui::SliderFloat("Width", &overlayState.scalarBarWidthPx, 45.0f, 140.0f, "%.0f px");
            }
        } else {
            ImGui::TextDisabled("No fields available");
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(3);
}

void positionResultsScalarBar(VtkViewer& viewer,
                              vtkScalarBarActor* scalarBar,
                              const ResultsPlaybackOverlayState& overlayState)
{
    if (scalarBar == nullptr) {
        return;
    }

    const ImVec2 viewportMin = viewer.getViewportScreenMin();
    const ImVec2 viewportMax = viewer.getViewportScreenMax();
    const float viewportWidth = viewportMax.x - viewportMin.x;
    const float viewportHeight = viewportMax.y - viewportMin.y;
    if (viewportWidth <= 1.0f || viewportHeight <= 1.0f) {
        return;
    }

    const float barHeightPx = std::min(overlayState.scalarBarHeightPx,
                                       std::max(45.0f, viewportHeight - 24.0f));
    const float barWidthPx = std::min(overlayState.scalarBarWidthPx,
                                      std::max(30.0f, viewportWidth * 0.28f));
    const bool hasOverlayRect =
        overlayState.lastMax.x > overlayState.lastMin.x &&
        overlayState.lastMax.y > overlayState.lastMin.y;
    const float topPx = hasOverlayRect ? overlayState.lastMax.y + 12.0f : viewportMin.y + 115.0f;
    const float clampedTopPx = std::min(topPx, viewportMax.y - barHeightPx - 12.0f);
    const float normalizedY = (viewportMax.y - (clampedTopPx + barHeightPx)) / viewportHeight;

    scalarBar->SetMaximumWidthInPixels(static_cast<int>(barWidthPx));
    scalarBar->SetMaximumHeightInPixels(static_cast<int>(barHeightPx));
    scalarBar->SetPosition(0.02, std::max(0.02f, normalizedY));
    scalarBar->SetWidth(barWidthPx / viewportWidth);
    scalarBar->SetHeight(barHeightPx / viewportHeight);
}

struct ResultProbeInfo {
    bool valid = false;
    bool isCellField = false;
    vtkIdType pointId = -1;
    int displayNodeId = -1;
    vtkIdType cellId = -1;
    std::string valueText;
    ImVec2 screenPos = ImVec2(0.0f, 0.0f);
};

std::string formatProbeValue(vtkDataArray* array, vtkIdType tupleId, int selectedComponent) {
    if (array == nullptr || tupleId < 0 || tupleId >= array->GetNumberOfTuples()) {
        return "";
    }

    char buffer[128];
    if (selectedComponent >= 0 && selectedComponent < array->GetNumberOfComponents()) {
        std::snprintf(buffer, sizeof(buffer), "%.6g", array->GetComponent(tupleId, selectedComponent));
        return std::string(buffer);
    }

    if (array->GetNumberOfComponents() == 3 && selectedComponent == 3) {
        const double x = array->GetComponent(tupleId, 0);
        const double y = array->GetComponent(tupleId, 1);
        const double z = array->GetComponent(tupleId, 2);
        std::snprintf(buffer, sizeof(buffer), "%.6g", std::sqrt(x * x + y * y + z * z));
        return std::string(buffer);
    }

    if (array->GetNumberOfComponents() == 1) {
        std::snprintf(buffer, sizeof(buffer), "%.6g", array->GetComponent(tupleId, 0));
        return std::string(buffer);
    }

    std::string text = "(";
    for (int comp = 0; comp < array->GetNumberOfComponents(); ++comp) {
        if (comp > 0) {
            text += ", ";
        }
        std::snprintf(buffer, sizeof(buffer), "%.6g", array->GetComponent(tupleId, comp));
        text += buffer;
    }
    text += ")";
    return text;
}

ResultProbeInfo buildResultProbeInfo(VtkViewer& viewer,
                                     ResultFrame& frame,
                                     Mesh* referenceMesh,
                                     const std::string& activeFieldName,
                                     bool isCellField,
                                     int selectedFieldComponent) {
    ResultProbeInfo probe;
    if (!viewer.isViewportHovered() || activeFieldName.empty() || frame.mesh == nullptr || frame.actor == nullptr) {
        return probe;
    }

    const ImVec2 viewportMin = viewer.getViewportScreenMin();
    const ImVec2 viewportMax = viewer.getViewportScreenMax();
    const ImVec2 mousePos = ImGui::GetMousePos();
    if (mousePos.x < viewportMin.x || mousePos.x > viewportMax.x ||
        mousePos.y < viewportMin.y || mousePos.y > viewportMax.y) {
        return probe;
    }

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.0005);

    const double localX = mousePos.x - viewportMin.x;
    const double localY = mousePos.y - viewportMin.y;
    const double pickY = static_cast<double>(viewer.getViewportHeight()) - localY;
    if (!picker->Pick(localX, pickY, 0.0, viewer.getRenderer())) {
        return probe;
    }

    if (picker->GetActor() != frame.actor) {
        return probe;
    }

    const vtkIdType cellId = picker->GetCellId();
    if (cellId < 0 || cellId >= frame.mesh->GetNumberOfCells()) {
        return probe;
    }

    probe.valid = true;
    probe.isCellField = isCellField;
    probe.cellId = cellId;
    probe.screenPos = mousePos;

    vtkDataArray* field = isCellField
        ? frame.mesh->GetCellData()->GetArray(activeFieldName.c_str())
        : frame.mesh->GetPointData()->GetArray(activeFieldName.c_str());
    if (field == nullptr) {
        probe.valid = false;
        return probe;
    }

    if (isCellField) {
        probe.valueText = formatProbeValue(field, cellId, selectedFieldComponent);
        return probe;
    }

    vtkCell* cell = frame.mesh->GetCell(cellId);
    if (cell == nullptr || cell->GetNumberOfPoints() == 0) {
        probe.valid = false;
        return probe;
    }

    double pickPos[3];
    picker->GetPickPosition(pickPos);

    vtkIdType closestPointId = -1;
    double closestDistance2 = std::numeric_limits<double>::max();
    for (vtkIdType i = 0; i < cell->GetNumberOfPoints(); ++i) {
        const vtkIdType pointId = cell->GetPointId(i);
        double point[3];
        frame.mesh->GetPoint(pointId, point);
        const double dx = point[0] - pickPos[0];
        const double dy = point[1] - pickPos[1];
        const double dz = point[2] - pickPos[2];
        const double distance2 = dx * dx + dy * dy + dz * dz;
        if (distance2 < closestDistance2) {
            closestDistance2 = distance2;
            closestPointId = pointId;
        }
    }

    if (closestPointId < 0) {
        probe.valid = false;
        return probe;
    }

    probe.pointId = closestPointId;
    if (referenceMesh != nullptr &&
        closestPointId >= 0 &&
        closestPointId < referenceMesh->getNodeCount() &&
        referenceMesh->getNode(static_cast<int>(closestPointId)) != nullptr) {
        probe.displayNodeId = referenceMesh->getNode(static_cast<int>(closestPointId))->getId();
    } else {
        probe.displayNodeId = static_cast<int>(closestPointId) + 1;
    }
    probe.valueText = formatProbeValue(field, closestPointId, selectedFieldComponent);
    return probe;
}

void drawResultProbeOverlay(const ResultProbeInfo& probe, const std::string& activeFieldName) {
    if (!probe.valid) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(probe.screenPos.x + 14.0f, probe.screenPos.y + 18.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.82f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("##ResultProbeOverlay", nullptr, flags)) {
        if (probe.isCellField) {
            ImGui::Text("Element %lld", static_cast<long long>(probe.cellId));
        } else {
            ImGui::Text("Node %d", probe.displayNodeId);
        }
        ImGui::Separator();
        ImGui::Text("%s = %s", activeFieldName.c_str(), probe.valueText.c_str());
    }
    ImGui::End();

    ImGui::PopStyleVar(3);
}

std::string buildActiveFieldDisplayName(const std::string& activeFieldName,
                                        int activeFieldComponents,
                                        int selectedFieldComponent) {
    if (activeFieldName.empty()) {
        return "";
    }

    if (activeFieldComponents <= 1) {
        return activeFieldName;
    }

    if (selectedFieldComponent == 0) {
        return activeFieldName + " [X]";
    }
    if (selectedFieldComponent == 1) {
        return activeFieldName + " [Y]";
    }
    if (selectedFieldComponent == 2) {
        return activeFieldName + " [Z]";
    }
    if (selectedFieldComponent == 3) {
        return activeFieldName + " [|V|]";
    }
    return activeFieldName;
}

double computeProbeMarkerRadius(vtkUnstructuredGrid* mesh) {
    if (mesh == nullptr) {
        return 1.0;
    }

    double bounds[6];
    mesh->GetBounds(bounds);
    const double dx = bounds[1] - bounds[0];
    const double dy = bounds[3] - bounds[2];
    const double dz = bounds[5] - bounds[4];
    const double diag = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (diag <= 1.0e-12) {
        return 1.0;
    }
    return diag * 0.008;
}

vtkSmartPointer<vtkActor> buildProbeHighlightActor(ResultFrame& frame, const ResultProbeInfo& probe) {
    if (!probe.valid || frame.mesh == nullptr) {
        return nullptr;
    }

    if (!probe.isCellField && probe.pointId >= 0) {
        double point[3];
        frame.mesh->GetPoint(probe.pointId, point);

        vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
        sphere->SetCenter(point);
        sphere->SetRadius(computeProbeMarkerRadius(frame.mesh));
        sphere->SetThetaResolution(18);
        sphere->SetPhiResolution(18);

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(sphere->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(1.0, 0.9, 0.1);
        actor->GetProperty()->SetAmbient(1.0);
        actor->GetProperty()->SetDiffuse(0.0);
        actor->GetProperty()->SetSpecular(0.0);
        actor->PickableOff();
        return actor;
    }

    if (probe.cellId >= 0) {
        vtkSmartPointer<vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();
        cellIds->InsertNextId(probe.cellId);

        vtkSmartPointer<vtkExtractCells> extractCells = vtkSmartPointer<vtkExtractCells>::New();
        extractCells->SetInputData(frame.mesh);
        extractCells->SetCellList(cellIds);
        extractCells->Update();

        vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInputConnection(extractCells->GetOutputPort());
        mapper->ScalarVisibilityOff();

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetRepresentationToWireframe();
        actor->GetProperty()->SetColor(1.0, 0.9, 0.1);
        actor->GetProperty()->SetOpacity(1.0);
        actor->GetProperty()->LightingOff();
        actor->GetProperty()->SetAmbient(1.0);
        actor->GetProperty()->SetDiffuse(0.0);
        actor->GetProperty()->SetSpecular(0.0);
        actor->GetProperty()->SetLineWidth(4.0);
        actor->GetProperty()->SetRenderLinesAsTubes(1);
        actor->PickableOff();
        return actor;
    }

    return nullptr;
}

std::filesystem::path ResolveExistingFontPath(const std::filesystem::path& executable_path,
                                              const std::vector<std::filesystem::path>& relative_candidates)
{
    std::vector<std::filesystem::path> search_roots;
    if (!executable_path.empty()) {
        search_roots.push_back(std::filesystem::absolute(executable_path).parent_path());
    }
    search_roots.push_back(std::filesystem::current_path());
    search_roots.push_back(std::filesystem::current_path() / "resources");
    search_roots.push_back(std::filesystem::current_path() / "data");
    search_roots.push_back(std::filesystem::current_path().parent_path() / "WeldFormGUI");
    search_roots.push_back(std::filesystem::current_path().parent_path() / "WeldFormGUI" / "resources");
    search_roots.push_back(std::filesystem::current_path().parent_path() / "WeldFormGUI" / "data");

    for (const auto& root : search_roots) {
        for (const auto& candidate : relative_candidates) {
            const std::filesystem::path full_path = root / candidate;
            if (std::filesystem::exists(full_path)) {
                return full_path;
            }
        }
    }

    return {};
}
}

// Open and read a file, then forward to LoadTextureFromMemory()
bool LoadTextureFromFile(const char* file_name, GLuint* out_texture, int* out_width, int* out_height)
{
    FILE* f = fopen(file_name, "rb");
    if (f == NULL)
        return false;
    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    if (file_size == -1)
        return false;
    fseek(f, 0, SEEK_SET);
    void* file_data = IM_ALLOC(file_size);
    const size_t bytes_read = fread(file_data, 1, file_size, f);
    if (bytes_read != file_size) {
        IM_FREE(file_data);
        fclose(f);
        return false;
    }
    bool ret = LoadTextureFromMemory(file_data, file_size, out_texture, out_width, out_height);
    IM_FREE(file_data);
    fclose(f);
    return ret;
}

static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

#ifdef BUILD_PYTHON
namespace {
void addPythonSearchPath(const std::filesystem::path& path)
{
  if (path.empty()) {
    return;
  }

  const std::string normalized = std::filesystem::absolute(path).lexically_normal().string();
  if (normalized.empty()) {
    return;
  }

  PyObject* sysPath = PySys_GetObject("path");
  if (sysPath == nullptr || !PyList_Check(sysPath)) {
    std::cerr << "Failed to access Python sys.path" << std::endl;
    return;
  }

  PyObject* pyPath = PyUnicode_FromString(normalized.c_str());
  if (pyPath == nullptr) {
    PyErr_Clear();
    std::cerr << "Failed to convert Python search path: " << normalized << std::endl;
    return;
  }

  if (PySequence_Contains(sysPath, pyPath) == 0) {
    PyList_Insert(sysPath, 0, pyPath);
  }
  Py_DECREF(pyPath);
}
}
#endif


int main(int argc, char* argv[])
{
  vtkSmartPointer<vtkFileOutputWindow> output =
  vtkSmartPointer<vtkFileOutputWindow>::New();
  output->SetFileName("vtk_warnings.log");
  vtkOutputWindow::SetInstance(output);
    
  // Setup pipeline
  //auto actor = SetupDemoPipeline();

  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()){
    return 1;
  }

  // Decide GLSL version
#ifdef __APPLE__
  // GLSL 150
  const char* glsl_version = "#version 150";
#else
  // GLSL 130
  const char* glsl_version = "#version 130";
#endif


  const GLFWvidmode* modes = glfwGetVideoMode(glfwGetPrimaryMonitor()/*, &count*/);
  cout << "Monitor width: "<<modes->width<<", height: "<<modes->height<<endl;

  // Create window with graphics context
  GLFWwindow* window = nullptr;

  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  window = glfwCreateWindow(modes->width, modes->height-80, "WeldForm GUI", NULL, NULL);

  if (window == NULL) {
    fprintf(stderr, "Retrying with OpenGL 3.0 compatibility context.\n");
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    window = glfwCreateWindow(modes->width, modes->height-80, "WeldForm GUI", NULL, NULL);
  }

  if (window == NULL) {
    fprintf(stderr, "Retrying with default GLFW context hints.\n");
    glfwDefaultWindowHints();
    window = glfwCreateWindow(modes->width, modes->height-80, "WeldForm GUI", NULL, NULL);
  }

  if (window == NULL){
    fprintf(stderr, "Failed to create GLFW window and OpenGL context.\n");
    return 1;
  }

  //glfwSetWindowAttrib(window, GLFW_MAXIMIZED, GLFW_TRUE);
  glfwSetWindowPos(window, 1, 30);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // Initialize OpenGL loader
  if (gl3wInit() != 0){
    fprintf(stderr, "Failed to initialize OpenGL loader!\n");
    return 1;
  }

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  ImFont* font1 = nullptr;
  ImFont* font_ubu = nullptr;
  bool ubuntu_font_available = false;
  float ui_font_size = 16.0f;
  float pending_ui_font_size = ui_font_size;
  bool font_rebuild_requested = false;
  const std::filesystem::path ubuntu_font_path = ResolveExistingFontPath(
      argc > 0 && argv[0] != nullptr ? std::filesystem::path(argv[0]) : std::filesystem::path(),
      {
          "Ubuntu-Regular.ttf",
          "Ubuntu-L.ttf",
          std::filesystem::path("resources") / "Ubuntu-Regular.ttf",
          std::filesystem::path("resources") / "Ubuntu-L.ttf",
          std::filesystem::path("data") / "Ubuntu-Regular.ttf",
          std::filesystem::path("data") / "Ubuntu-L.ttf"
      });
  if (!ubuntu_font_path.empty()) {
    std::fprintf(stderr, "INFO: Ubuntu font candidate found at: %s\n", ubuntu_font_path.string().c_str());
  } else {
    std::fprintf(stderr, "INFO: Ubuntu font candidate not found. cwd=%s\n",
                 std::filesystem::current_path().string().c_str());
    if (argc > 0 && argv[0] != nullptr) {
      std::fprintf(stderr, "INFO: Executable path argument: %s\n", argv[0]);
    }
  }
  UIFontChoice current_font_choice = UIFontChoice::Ubuntu;
  ImFont* current_ui_font = nullptr;

  auto rebuildUiFonts = [&]() {
    io.Fonts->Clear();
    font1 = io.Fonts->AddFontDefault();
    font_ubu = nullptr;
    if (!ubuntu_font_path.empty()) {
      font_ubu = io.Fonts->AddFontFromFileTTF(ubuntu_font_path.string().c_str(), ui_font_size, NULL, io.Fonts->GetGlyphRangesDefault());
    }
    ubuntu_font_available = font_ubu != nullptr;
    if (!ubuntu_font_available) {
      fprintf(stderr,"WARNING. Cannot load Ubuntu font, using Dear ImGui default.\n");
      current_font_choice = UIFontChoice::ImGuiDefault;
    } else {
      std::fprintf(stderr, "INFO: Ubuntu font added successfully at %.1f px.\n", ui_font_size);
    }

    current_ui_font = (current_font_choice == UIFontChoice::Ubuntu && ubuntu_font_available) ? font_ubu : font1;
    io.FontDefault = current_ui_font;
    io.Fonts->Build();
  };

  rebuildUiFonts();
    
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable keyboard navigation in Dear ImGui widgets/dialogs
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows'

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  ImGui::GetStyle().ScaleAllSizes(1.1f);
  ImGuiStyle& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_Text]               = ImVec4(0.949f, 0.957f, 0.965f, 1.0f); // #f2f4f6
  style.Colors[ImGuiCol_TextDisabled]       = ImVec4(0.620f, 0.671f, 0.733f, 1.0f);
  style.Colors[ImGuiCol_WindowBg]           = ImVec4(0.067f, 0.094f, 0.153f, 0.96f); // #111827
  style.Colors[ImGuiCol_ChildBg]            = ImVec4(0.067f, 0.094f, 0.153f, 0.92f);
  style.Colors[ImGuiCol_PopupBg]            = ImVec4(0.090f, 0.117f, 0.176f, 0.98f);
  style.Colors[ImGuiCol_FrameBg]            = ImVec4(0.118f, 0.145f, 0.204f, 0.88f);
  style.Colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.157f, 0.188f, 0.255f, 0.95f);
  style.Colors[ImGuiCol_FrameBgActive]      = ImVec4(0.184f, 0.220f, 0.294f, 1.0f);
  style.Colors[ImGuiCol_TitleBg]            = ImVec4(0.067f, 0.094f, 0.153f, 1.0f);
  style.Colors[ImGuiCol_TitleBgActive]      = ImVec4(0.090f, 0.117f, 0.176f, 1.0f);
  style.Colors[ImGuiCol_MenuBarBg]          = ImVec4(0.090f, 0.117f, 0.176f, 0.98f);
  style.Colors[ImGuiCol_Header]             = ImVec4(0.149f, 0.184f, 0.255f, 0.85f);
  style.Colors[ImGuiCol_HeaderHovered]      = ImVec4(0.188f, 0.227f, 0.310f, 0.95f);
  style.Colors[ImGuiCol_HeaderActive]       = ImVec4(0.220f, 0.259f, 0.353f, 1.0f);
  style.Colors[ImGuiCol_Button]             = ImVec4(0.149f, 0.184f, 0.255f, 0.80f);
  style.Colors[ImGuiCol_ButtonHovered]      = ImVec4(0.188f, 0.227f, 0.310f, 0.92f);
  style.Colors[ImGuiCol_ButtonActive]       = ImVec4(0.220f, 0.259f, 0.353f, 1.0f);
  style.Colors[ImGuiCol_Tab]                = ImVec4(0.118f, 0.145f, 0.204f, 0.94f);
  style.Colors[ImGuiCol_TabHovered]         = ImVec4(0.157f, 0.188f, 0.255f, 0.98f);
  style.Colors[ImGuiCol_TabActive]          = ImVec4(0.184f, 0.220f, 0.294f, 1.0f);
  style.Colors[ImGuiCol_TabUnfocused]       = ImVec4(0.090f, 0.117f, 0.176f, 0.92f);
  style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.149f, 0.184f, 0.255f, 0.96f);
  style.Colors[ImGuiCol_Separator]          = ImVec4(0.220f, 0.259f, 0.353f, 0.65f);
  style.Colors[ImGuiCol_Border]             = ImVec4(0.220f, 0.259f, 0.353f, 0.40f);
  style.Colors[ImGuiCol_CheckMark]          = ImVec4(0.949f, 0.957f, 0.965f, 1.0f);
  style.Colors[ImGuiCol_SliderGrab]         = ImVec4(0.482f, 0.580f, 0.741f, 0.90f);
  style.Colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.561f, 0.659f, 0.820f, 1.0f);

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);


  // Initialize VtkViewer objects
  //VtkViewer vtkViewer1;
  //vtkViewer1.addActor(actor);
  
  VtkViewer vtkViewer2;
  VtkViewer vtkViewer_res;
  LoadPlotDialog loadPlotDialog;
  EnergyPlotDialog energyPlotDialog;
  bool showLoadPlotDialog = false;
  bool showEnergyPlotDialog = false;

  static DemoDialog demoDialog;
  static bool showDemoDialog = false;
  static bool showDemoLoadedPopup = false;
  demoDialog.SetDemoRoot("examples/demo");

  //vtkViewer2.getRenderer()->SetBackground(0, 0, 0); // Black background

  vtkViewer2.getRenderer()->SetBackground(0.2,0.2,0.4);
  vtkViewer2.getRenderer()->SetBackground2(0.8,0.8,0.8);

  Axis axis;  
  axis.setInteractor(vtkViewer2.getInteractor(), vtkViewer2.getRenderer());  
  Axis axis_res;

  vtkViewer_res.getRenderer()->SetBackground(0.2,0.2,0.4);
  vtkViewer_res.getRenderer()->SetBackground2(0.8,0.8,0.8);
  axis_res.setInteractor(vtkViewer_res.getInteractor(), vtkViewer_res.getRenderer());

  //vtkViewer2.addActor(actor);

  // Our state
  bool show_demo_window = true;
  bool show_another_window = false;
  bool vtk_2_open = true;
  bool vtk_res_open = true;
  ImVec4 clear_color = ImVec4(0.067f, 0.094f, 0.153f, 1.00f);
  

  cout << "Done. "<<endl;
  cout << "Initialize gmsh"<<endl;
  gmsh::initialize(argc, argv);
        
	//cout << "creating app app"<<endl;
	//pApp= new EditorApp();

/*
    vtkNew<vtkNamedColors> colors;
    vtkNew<vtkArrowSource> arrowSource;
    arrowSource->Update();
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(arrowSource->GetOutputPort());
    vtkNew<vtkActor> arrow_actor;
    vtkViewer2.addActor(arrow_actor);
    
    
    actor->SetMapper(mapper);
*/    


    //vtkViewer2.addActor(axis.actor);


/*    
    vtkOCCTGeom geom;
    geom.TestReader("valoppi_z_3.step", vtkOCCTReader::Format::STEP);
    //widget->SetInteractor(renderWindowInteractor);
    vtkViewer2.addActor(geom.actor);
*/


      GLuint my_image_texture = 0;
      int my_image_width = 0;
      int my_image_height = 0;
      cout << "Opening button images.."<<endl;
      bool ret = LoadTextureFromFile("buttons/xy.png", &my_image_texture, &my_image_width, &my_image_height);
      IM_ASSERT(ret);
      cout << "Done."<<endl;
          

  // Main loop
  #ifdef BUILD_PYTHON
  Py_Initialize();
  addPythonSearchPath(std::filesystem::current_path());
  if (argc > 0 && argv[0] != nullptr) {
    addPythonSearchPath(std::filesystem::absolute(argv[0]).parent_path());
  }
  #endif
  
  App::initApp(); //singleton
  ///AFTER APP INITIALIZATIO
  cout << "WeldFormGUI version " << GIT_DESCRIBE_VERSION
       << " commit " << GIT_COMMIT_HASH
       << " build " << BUILD_TIMESTAMP << endl;
  cout << "Creating Editor"<<endl;
  Editor* editor = new Editor();//THIS RELIES ON THE App Singleton!! ALWAIS GENERATE IT FIRST AND THE CALL EDITOR, otherwise crashes
  editor->addViewer(&vtkViewer2);  
  editor->addResViewer(&vtkViewer_res);  
  cout << "Done "<<endl;
  //getApp().setActiveModel(m_model);
  
          //~ std::string filename = "out_0.000010.vtk";
      //~ ResultFrame *frame = new ResultFrame(filename);
    //~ frame->printAvailableFields();
    
    //~ // Mostrar el campo DISP (magnitud del vector)
    //~ frame->setActiveScalarField("DISP");
          
      //~ vtkViewer2.addActor(frame->actor);


      
  #ifdef BUILD_PYTHON
  //PyRun_SimpleString("from model import *");
  #endif
  while (!glfwWindowShouldClose(window))
  {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();

    if (font_rebuild_requested) {
      ui_font_size = pending_ui_font_size;
      rebuildUiFonts();
      ImGui_ImplOpenGL3_DestroyFontsTexture();
      ImGui_ImplOpenGL3_CreateFontsTexture();
      font_rebuild_requested = false;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    static bool show_about_popup = false;
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        ImGui::MenuItem("Project actions are available in the left panel", nullptr, false, false);
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Edit")) {
        ImGui::MenuItem("Editing tools are available in the left panel", nullptr, false, false);
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("View")) {
        if (ImGui::BeginMenu("Font")) {
          const bool use_ubuntu = current_font_choice == UIFontChoice::Ubuntu;
          if (ImGui::MenuItem("Ubuntu", nullptr, use_ubuntu, ubuntu_font_available)) {
            current_font_choice = UIFontChoice::Ubuntu;
            current_ui_font = font_ubu;
            io.FontDefault = current_ui_font;
          }
          if (ImGui::MenuItem("ImGui Default", nullptr, current_font_choice == UIFontChoice::ImGuiDefault)) {
            current_font_choice = UIFontChoice::ImGuiDefault;
            current_ui_font = font1;
            io.FontDefault = current_ui_font;
          }
          ImGui::Separator();
          float new_font_size = pending_ui_font_size;
          if (ImGui::SliderFloat("Size", &new_font_size, 16.0f, 20.0f, "%.1f px")) {
            pending_ui_font_size = new_font_size;
            font_rebuild_requested = true;
          }
          ImGui::EndMenu();
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("About")) {
          show_about_popup = true;
        }
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }

    if (show_about_popup) {
      ImGui::OpenPopup("About WeldFormGUI");
      show_about_popup = false;
    }
    if (ImGui::BeginPopupModal("About WeldFormGUI", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("WeldFormGUI");
      ImGui::Separator();
      ImGui::Text("Version: %s", GIT_DESCRIBE_VERSION);
      ImGui::Text("Commit: %s", GIT_COMMIT_HASH);
      ImGui::Text("Build: %s", BUILD_TIMESTAMP);
      ImGui::Spacing();
      if (ImGui::Button("Close")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
    
    editor->drawGui();
    
    editor->processInput(window); //KEYBOARD, good for maintain pressed
    
    /*
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).

    if (show_demo_window){
      ImGui::ShowDemoWindow(&show_demo_window);
    }
    */

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
      static float f = 0.0f;
      static int counter = 0;

      ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
      ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
      ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
      ImGui::Checkbox("Another Window", &show_another_window);
      ImGui::Checkbox("VTK Model  Viewer", &vtk_2_open);
      ImGui::Checkbox("VTK Result Viewer", &vtk_res_open);

      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
      ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

      if (ImGui::Button("Button")){                            // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
      }
      ImGui::SameLine();
      ImGui::Text("counter = %d", counter);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }
    ImGui::End();

    // 3. Show another simple window.
    if (show_another_window){
      ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
      ImGui::Text("Hello from another window!");
      if (ImGui::Button("Close Me")){
        show_another_window = false;
      }
      ImGui::End();
    }
    
    //vtkArrowSource *arrowSource = vtkNew<vtkArrowSource>;
    
    // 4. Show a simple VtkViewer Instance (Always Open)
    ImGui::SetNextWindowSize(ImVec2(360, 240), ImGuiCond_FirstUseEver);
    
    //ImGui::Begin("Vtk Viewer 1", nullptr, VtkViewer::NoScrollFlags());
    //vtkViewer1.render(); // default render size = ImGui::GetContentRegionAvail()
    //ImGui::End();

    // 5. Show a more complex VtkViewer Instance (Closable, Widgets in Window)
    //ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 200),ImGuiCond_FirstUseEver);
    
    ImGui::Begin("VTK Viewers"); // ventana padre para los tabs

      if (ImGui::BeginTabBar("##ViewersTabBar", ImGuiTabBarFlags_None))
      {
          bool activate_model_tab = editor->consumeModelViewerActivationRequest();
          bool activate_results_tab = editor->consumeResultsViewerActivationRequest();
          static bool was_results_viewer_active = false;
          bool results_viewer_active = false;
          static ModelViewportOverlayState resultsOverlayState;
          static ResultsViewportToolState resultsToolState;
          static ResultsPlaybackOverlayState resultsPlaybackOverlayState;
          static int currentFrame = 0;
          static int lastFrame = -1;
          static double globalMin = 0.0;
          static double globalMax = 1.0;
          static bool manualColorScale = false;
          static double manualMin = 0.0;
          static double manualMax = 1.0;
          static bool isCellField = false;
          static bool isPlaying = false;
          static double lastPlaybackAdvanceTime = 0.0;
          static std::string activeFieldName = "";
          static vtkSmartPointer<vtkScalarBarActor> currentScalarBar = nullptr;
          static vtkSmartPointer<vtkActor> currentProbeHighlightActor = nullptr;
          static vtkSmartPointer<vtkActor> currentVectorFieldActor = nullptr;
          static int activeFieldComponents = 1;
          static int selectedField = 0;
          static int selectedFieldComponent = -2;
          static bool showVectorGlyphs = false;
          auto clearResultsViewerTransientProps = [&]() {
              auto resultsRenderer = vtkViewer_res.getRenderer();
              if (resultsRenderer != nullptr) {
                  if (editor->getResults() != nullptr) {
                      for (auto& frame : editor->getResults()->frames) {
                          if (!frame) {
                              continue;
                          }
                          vtkSmartPointer<vtkScalarBarActor> frameScalarBar = frame->getScalarBarActor();
                          if (frameScalarBar != nullptr) {
                              frame->hideScalarBar();
                              resultsRenderer->RemoveActor2D(frameScalarBar);
                              resultsRenderer->RemoveViewProp(frameScalarBar);
                          }
                      }
                  }
                  if (currentScalarBar != nullptr) {
                      resultsRenderer->RemoveActor2D(currentScalarBar);
                      resultsRenderer->RemoveViewProp(currentScalarBar);
                  }
                  if (currentProbeHighlightActor != nullptr) {
                      resultsRenderer->RemoveActor(currentProbeHighlightActor);
                  }
                  if (currentVectorFieldActor != nullptr) {
                      resultsRenderer->RemoveActor(currentVectorFieldActor);
                  }
              }
              currentScalarBar = nullptr;
              currentProbeHighlightActor = nullptr;
              currentVectorFieldActor = nullptr;
              activeFieldName.clear();
              selectedField = 0;
              selectedFieldComponent = -2;
              activeFieldComponents = 1;
              showVectorGlyphs = false;
              currentFrame = 0;
              lastFrame = -1;
          };
          if (activate_results_tab) {
              vtk_res_open = true;
          }

          // ================= TAB: Modelo =================
          ImGuiTabItemFlags model_tab_flags = activate_model_tab ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
	          if (vtk_2_open && ImGui::BeginTabItem("Model Viewer", &vtk_2_open, model_tab_flags))
          {
	              ImGui::PushFont(current_ui_font);
		              auto renderer = vtkViewer2.getRenderer();
                static ModelViewportOverlayState modelOverlayState;
                static ResultsViewportToolState modelToolState;

	              if (ImGui::Button("Demo")) {
                  showDemoDialog = true;
	                // const std::filesystem::path demo_path("./demo/demo.wfmodel");
	                // if (std::filesystem::exists(demo_path))
	                  // editor->openModelFromPath(demo_path.string());
	                // else
	                  // std::cout << "Demo file not found: " << demo_path.string() << std::endl;
	              }
	              ImGui::SameLine();
	              if (ImGui::Button("Run")) {
	                editor->createJobFromActiveModel(true);
	              }
	              ImGui::SameLine();
	              if (ImGui::Button("Zoom all")) {
	                vtkViewer2.resetCamera();
	              }
                  ImGui::SameLine();
                  if (ImGui::SmallButton("X##close_active_model_viewer")) {
                      editor->requestCloseCurrentModel();
                  }
                  if (ImGui::IsItemHovered()) {
                      ImGui::SetTooltip("Close model");
                  }
                  const std::string activeModelPath = editor->getModel().getFilePath();
                  if (!activeModelPath.empty()) {
                      ImGui::SameLine();
                      ImGui::TextDisabled("%s%s",
                                          std::filesystem::path(activeModelPath).filename().string().c_str(),
                                          editor->getModel().isDirty() ? " *" : "");
                      if (ImGui::IsItemHovered()) {
                          ImGui::SetTooltip("%s", activeModelPath.c_str());
                      }
                  } else if (editor->getModel().isDirty()) {
                      ImGui::SameLine();
                      ImGui::TextDisabled("*");
                  }

	              // Botones de background específicos
	              //~ if (ImGui::Button("Black BG"))        renderer->SetBackground(0,0,0);
	              //~ ImGui::SameLine();
              //~ if (ImGui::Button("Red BG"))          renderer->SetBackground(1,0,0);
              //~ ImGui::SameLine();
              //~ if (ImGui::Button("Green BG"))        renderer->SetBackground(0,1,0);
              //~ ImGui::SameLine();
              //~ if (ImGui::Button("Blue BG"))         { renderer->SetBackground(0.2,0.2,0.4); renderer->SetBackground2(0.8,0.8,0.8); }

              // Slider de alpha del background
              static float vtk2BkgAlpha = 0.2f;
              ImGui::SliderFloat("BG Alpha", &vtk2BkgAlpha, 0.0f, 1.0f);
              renderer->SetBackgroundAlpha(vtk2BkgAlpha);

              applyDisplayModeToActiveModel(modelOverlayState.displayMode, modelOverlayState.showEdges);
              modelToolState.is3DFrame = activeModelIs3D();
              if (!modelToolState.is3DFrame) {
                  modelToolState.clipEnabled = false;
                  modelToolState.transparencyControlsVisible = false;
                  modelToolState.surfaceOpacity = 1.0f;
                  detachResultsClipTools(vtkViewer2, modelToolState);
              } else {
                  if (modelToolState.clipEnabled && !modelToolState.clipOriginInitialized) {
                      if (computeActiveModelCentroid(modelToolState.clipOrigin.data())) {
                          modelToolState.clipOriginInitialized = true;
                      }
                  }
                  applyActiveModelToolState(vtkViewer2, modelToolState);
                  syncModelClipTools(vtkViewer2, modelToolState);
              }
              // Render del viewer
              vtkViewer2.render();
              const bool previousModelClipEnabled = modelToolState.clipEnabled;
              const int previousModelClipAxis = modelToolState.clipAxis;
              const float previousModelOpacity = modelToolState.surfaceOpacity;
              drawViewportOverlay(vtkViewer2,
                                  modelOverlayState,
                                  "##ModelViewportOverlay",
                                  true,
                                  editor,
                                  &modelToolState);
              if (modelToolState.is3DFrame) {
                  if (modelToolState.clipEnabled && !previousModelClipEnabled) {
                      if (computeActiveModelCentroid(modelToolState.clipOrigin.data())) {
                          modelToolState.clipOriginInitialized = true;
                      }
                  }
                  if (modelToolState.clipEnabled != previousModelClipEnabled ||
                      modelToolState.clipAxis != previousModelClipAxis ||
                      std::abs(modelToolState.surfaceOpacity - previousModelOpacity) > 1.0e-6f) {
                      applyActiveModelToolState(vtkViewer2, modelToolState);
                      syncModelClipTools(vtkViewer2, modelToolState);
                  }
              }
              editor->setActiveViewer(&vtkViewer2);
              editor->handleMeasurementInteraction();
              editor->handleSelectionInteraction();
              editor->drawMeasurementOverlay();
              editor->drawSelectionOverlay();

              ImGui::PopFont();
              ImGui::EndTabItem();
          }

          // ================= TAB: Resultados =================
          const bool results_tab_was_open = vtk_res_open;
          ImGuiTabItemFlags results_tab_flags = activate_results_tab ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
          if (vtk_res_open && ImGui::BeginTabItem("Results Viewer", &vtk_res_open, results_tab_flags))
          {
                results_viewer_active = true;
	                if (!was_results_viewer_active) {
	                    editor->clearPartSelectionState();
	                }
	              ImGui::PushFont(current_ui_font);

	              auto renderer = vtkViewer_res.getRenderer();
	              auto getActiveArray = [&](ResultFrame& resultFrame) -> vtkDataArray* {
	                  if (activeFieldName.empty() || !resultFrame.mesh)
	                      return nullptr;
	                  return isCellField
	                      ? resultFrame.mesh->GetCellData()->GetArray(activeFieldName.c_str())
	                      : resultFrame.mesh->GetPointData()->GetArray(activeFieldName.c_str());
	              };
	              auto recomputeActiveFieldScale = [&]() {
	                  if (manualColorScale) {
	                      globalMin = manualMin;
	                      globalMax = manualMax;
	                      return;
	                  }
	                  if (!editor->getResults() || activeFieldName.empty())
	                      return;

	                  globalMin = 1.0e10;
	                  globalMax = -1.0e10;

	                  for (auto& f : editor->getResults()->frames) {
	                      if (!f || !f->mesh)
	                          continue;

	                      vtkDataArray* array = getActiveArray(*f);

	                      if (!array)
	                          continue;

	                      double range[2];
	                      if (selectedFieldComponent >= 0 &&
	                          selectedFieldComponent < array->GetNumberOfComponents())
	                          array->GetRange(range, selectedFieldComponent);
	                      else if (array->GetNumberOfComponents() == 3 && selectedFieldComponent == 3)
	                          array->GetRange(range, -1);
	                      else
	                          array->GetRange(range);

	                      globalMin = (std::min)(globalMin, range[0]);
	                      globalMax = (std::max)(globalMax, range[1]);
	                  }

	                  if (globalMin > globalMax) {
	                      globalMin = 0.0;
	                      globalMax = 1.0;
	                  }
	                  manualMin = globalMin;
	                  manualMax = globalMax;
	              };
	              auto applyActiveFieldSelection = [&](ResultFrame& resultFrame) {
	                  if (activeFieldName.empty())
	                      return;

	                  auto mapper = resultFrame.actor->GetMapper();
	                  vtkDataArray* array = getActiveArray(resultFrame);
	                  if (!mapper || !array)
	                      return;

	                  if (array->GetNumberOfComponents() == 3 && selectedFieldComponent == 3) {
	                      resultFrame.setActiveScalarField(activeFieldName);
                        mapper->SetScalarRange(globalMin, globalMax);
                        mapper->ScalarVisibilityOn();
                        mapper->Update();
                        resultFrame.updateVectorGlyphs(activeFieldName, isCellField, showVectorGlyphs);
	                      resultFrame.updateScalarBar(
                            buildActiveFieldDisplayName(activeFieldName, array->GetNumberOfComponents(), selectedFieldComponent),
                            globalMin,
                            globalMax);
	                      return;
	                  }

	                  if (isCellField)
	                      mapper->SetScalarModeToUseCellFieldData();
	                  else
	                      mapper->SetScalarModeToUsePointFieldData();

	                  mapper->SelectColorArray(activeFieldName.c_str());
	                  if (selectedFieldComponent >= 0 &&
	                      selectedFieldComponent < array->GetNumberOfComponents())
	                      mapper->SetArrayComponent(selectedFieldComponent);

	                  mapper->SetScalarRange(globalMin, globalMax);
	                  mapper->ScalarVisibilityOn();
	                  mapper->Update();
                    resultFrame.updateVectorGlyphs(activeFieldName, isCellField, showVectorGlyphs);
	                  resultFrame.updateScalarBar(
                        buildActiveFieldDisplayName(activeFieldName, array->GetNumberOfComponents(), selectedFieldComponent),
                        globalMin,
                        globalMax);
	              };
	              auto applyActiveFieldSelectionToAllFrames = [&]() {
	                  if (!editor->getResults() || activeFieldName.empty())
	                      return;
	                  for (auto& resultFrame : editor->getResults()->frames) {
	                      if (resultFrame)
	                          applyActiveFieldSelection(*resultFrame);
	                  }
                      if (resultsToolState.clipEnabled && resultsToolState.is3DFrame) {
                          applyResultsToolState(editor, vtkViewer_res, resultsToolState);
                      }
	              };
                auto syncCurrentVectorFieldActor = [&](ResultFrame& resultFrame) {
                    if (currentVectorFieldActor != nullptr) {
                        renderer->RemoveActor(currentVectorFieldActor);
                        currentVectorFieldActor = nullptr;
                    }

                    if (resultFrame.hasVisibleVectorGlyphs() && resultFrame.getVectorActor() != nullptr) {
                        currentVectorFieldActor = resultFrame.getVectorActor();
                        renderer->AddActor(currentVectorFieldActor);
                    }
                };

	              // Botones de background específicos
	              //~ if (ImGui::Button("Black BG"))        renderer->SetBackground(0,0,0);
	              //~ ImGui::SameLine();
	              //~ if (ImGui::Button("Red BG"))          renderer->SetBackground(1,0,0);
	              //~ ImGui::SameLine();
	              //~ if (ImGui::Button("Green BG"))        renderer->SetBackground(0,1,0);
	              //~ ImGui::SameLine();
	              //~ if (ImGui::Button("Blue BG"))         { renderer->SetBackground(0.2,0.2,0.4); renderer->SetBackground2(0.8,0.8,0.8); }
	              //~ ImGui::SameLine();
		              if (ImGui::Button("Refresh Results")) {
		                  int previousFrame = currentFrame;
		                  if (editor->refreshOpenResults(previousFrame)) {
		                      lastFrame = -1;
		                  }
		              }
                  ImGui::SameLine();
                  if (ImGui::SmallButton("X##close_active_results_viewer")) {
                      editor->closeCurrentResults();
                  }
                  if (ImGui::IsItemHovered()) {
                      ImGui::SetTooltip("Close results");
                  }
	              ImGui::SameLine();
	              if (ImGui::Button("load plot")) {
	                  std::filesystem::path results_dir;
	                  if (editor->getResults()) {
	                      if (!editor->getResults()->sourceDirectory.empty()) {
	                          results_dir = editor->getResults()->sourceDirectory;
	                      } else if (!editor->getResults()->frames.empty()) {
	                          results_dir = std::filesystem::path(editor->getResults()->frames.front()->name).parent_path();
	                      }
	                  }
	                  std::vector<std::string> csv_paths;
	                  if (!results_dir.empty()) {
	                      const std::filesystem::path contact_path = results_dir / "Contact_Forces.csv";
	                      const std::filesystem::path set_path = results_dir / "Set_Forces.csv";
	                      if (std::filesystem::exists(contact_path)) {
	                          csv_paths.push_back(contact_path.string());
	                      }
	                      if (std::filesystem::exists(set_path)) {
	                          csv_paths.push_back(set_path.string());
	                      }
	                  }
	                  loadPlotDialog.SetCsvPaths(csv_paths);
	                  showLoadPlotDialog = true;
	              }
                ImGui::SameLine();
                if (ImGui::Button("energy plot")) {
                    std::filesystem::path csv_path;
                    if (editor->getResults()) {
                        if (!editor->getResults()->sourceDirectory.empty()) {
                            csv_path = editor->getResults()->sourceDirectory / "energy.csv";
                        } else if (!editor->getResults()->frames.empty()) {
                            csv_path = std::filesystem::path(editor->getResults()->frames.front()->name).parent_path() / "energy.csv";
                        }
                    }
                    energyPlotDialog.SetCsvPath(csv_path.string());
                    showEnergyPlotDialog = true;
                }
		              if (editor->getResults()){
                      const int restoredFrame = editor->consumePendingResultsFrameIndex();
                      if (restoredFrame >= 0) {
                          if (!editor->getResults()->frames.empty()) {
                              currentFrame = std::min(restoredFrame, (int)editor->getResults()->frames.size() - 1);
                              recomputeActiveFieldScale();
                              applyActiveFieldSelectionToAllFrames();
                          } else {
                              currentFrame = 0;
                          }
                          lastFrame = -1;
                      }
		              if (!editor->getResults()->frames.empty()) {
                    const double now = ImGui::GetTime();
                    if (lastPlaybackAdvanceTime <= 0.0) {
                        lastPlaybackAdvanceTime = now;
                    }
                    if (isPlaying && (now - lastPlaybackAdvanceTime) >= 0.12) {
                        currentFrame = (currentFrame + 1) % (int)editor->getResults()->frames.size();
                        lastPlaybackAdvanceTime = now;
                    }
	                  if (currentFrame >= (int)editor->getResults()->frames.size())
	                      currentFrame = 0;
		                  ImGui::SliderInt("Frame", &currentFrame, 0, (int)editor->getResults()->frames.size() - 1);
		                  auto& frame = *editor->getResults()->frames[currentFrame];  // referencia al frame actual
		                  if (frame.getScalarBarActor() && currentScalarBar != frame.getScalarBarActor()) {
	                      if (currentScalarBar)
	                          renderer->RemoveActor2D(currentScalarBar);
	                      currentScalarBar = frame.getScalarBarActor();
	                      positionResultsScalarBar(vtkViewer_res, currentScalarBar, resultsPlaybackOverlayState);
	                      renderer->AddActor2D(currentScalarBar);
	                  }

	                  if (currentFrame != lastFrame) {              // Solo si cambió el frame
	                      vtkCamera* camera = renderer->GetActiveCamera();
	                      bool preserveView = (lastFrame >= 0 && camera != nullptr);
	                      double cameraPosition[3] = {0.0, 0.0, 1.0};
	                      double cameraFocalPoint[3] = {0.0, 0.0, 0.0};
	                      double cameraViewUp[3] = {0.0, 1.0, 0.0};
	                      double cameraClippingRange[2] = {0.1, 1000.0};
	                      double cameraParallelScale = 1.0;
	                      double cameraViewAngle = 30.0;
	                      int cameraParallelProjection = 0;
	                      if (preserveView) {
	                          camera->GetPosition(cameraPosition);
	                          camera->GetFocalPoint(cameraFocalPoint);
	                          camera->GetViewUp(cameraViewUp);
	                          camera->GetClippingRange(cameraClippingRange);
	                          cameraParallelScale = camera->GetParallelScale();
	                          cameraViewAngle = camera->GetViewAngle();
	                          cameraParallelProjection = camera->GetParallelProjection();
	                      }

	                      vtkViewer_res.setActor(editor->getResults()->frames[currentFrame]->actor);
	                      if (preserveView) {
	                          camera->SetPosition(cameraPosition);
	                          camera->SetFocalPoint(cameraFocalPoint);
	                          camera->SetViewUp(cameraViewUp);
	                          camera->SetClippingRange(cameraClippingRange);
	                          camera->SetParallelScale(cameraParallelScale);
	                          camera->SetViewAngle(cameraViewAngle);
	                          camera->SetParallelProjection(cameraParallelProjection);
	                          renderer->ResetCameraClippingRange();
	                      } else {
	                          renderer->ResetCamera();
	                      }
	                      if (!activeFieldName.empty()) {
	                          applyActiveFieldSelection(*editor->getResults()->frames[currentFrame]);
	                      }
                        syncCurrentVectorFieldActor(*editor->getResults()->frames[currentFrame]);
                        applyDisplayModeToActor(editor->getResults()->frames[currentFrame]->actor,
                                                resultsOverlayState.displayMode, resultsOverlayState.showEdges, true);
                        if (resultsToolState.clipEnabled && resultsToolState.is3DFrame) {
                            syncResultsClipTools(vtkViewer_res,
                                                 resultsToolState,
                                                 *editor->getResults()->frames[currentFrame]);
                        }
                        editor->refreshSelectedResultNodeSetHighlight();
	                      
	                      lastFrame = currentFrame;                // Actualizamos el frame anterior
                      //vtkViewer_res.render();
	                  }

	                  auto fieldNames = frame.getAvailableFieldNames();

                  //~ if (!fieldNames.empty()) {
                      //~ // Crear array de const char* para ImGui
                      //~ std::vector<const char*> fieldCStrs;
                      //~ for (auto& s : fieldNames) fieldCStrs.push_back(s.c_str());

                      //~ ImGui::Text("Active Field:");
                      //~ if (ImGui::Combo("##FieldSelector", &selectedField, fieldCStrs.data(), fieldCStrs.size())) {
                          //~ // cuando cambia la selección
                          //~ frame.setActiveScalarField(fieldNames[selectedField]);
                          //~ frame.actor->GetMapper()->Update();
                          //~ vtkViewer_res.render();
                      //~ }
                  //~ } else {
                      //~ ImGui::Text("No fields available");
                  //~ }

	              } else {
                    clearResultsViewerTransientProps();
                    vtkViewer_res.setActor(nullptr);
                    vtkViewer_res.render();
                }
	            }
              
              applyDisplayModeToResults(resultsOverlayState.displayMode, resultsOverlayState.showEdges, editor);
              if (editor->getResults() != nullptr && !editor->getResults()->frames.empty()) {
                  const int safeFrame = std::max(0, std::min(currentFrame, (int)editor->getResults()->frames.size() - 1));
                  auto& activeFrame = *editor->getResults()->frames[safeFrame];
                  resultsToolState.is3DFrame = resultFrameIs3D(activeFrame);
                  if (!resultsToolState.is3DFrame) {
                      resultsToolState.clipEnabled = false;
                      resultsToolState.transparencyControlsVisible = false;
                      resultsToolState.surfaceOpacity = 1.0f;
                  }

                  if (resultsToolState.clipEnabled && !resultsToolState.clipOriginInitialized) {
                      computeResultFrameCentroid(activeFrame, resultsToolState.clipOrigin.data());
                      resultsToolState.clipOriginInitialized = true;
                  }

                  applyResultsToolState(editor, vtkViewer_res, resultsToolState);
                  syncResultsClipTools(vtkViewer_res, resultsToolState, activeFrame);
              } else {
                  resultsToolState.is3DFrame = false;
                  resultsToolState.clipEnabled = false;
                  resultsToolState.transparencyControlsVisible = false;
                  resultsToolState.surfaceOpacity = 1.0f;
                  detachResultsClipTools(vtkViewer_res, resultsToolState);
              }
              // Render del viewer
              vtkViewer_res.render();
              const bool previousClipEnabled = resultsToolState.clipEnabled;
              const int previousClipAxis = resultsToolState.clipAxis;
              const float previousOpacity = resultsToolState.surfaceOpacity;
              drawViewportOverlay(vtkViewer_res,
                                  resultsOverlayState,
                                  "##ResultsViewportOverlay",
                                  false,
                                  editor,
                                  &resultsToolState);
              if (editor->getResults() != nullptr && !editor->getResults()->frames.empty()) {
                  const int safeFrame = std::max(0, std::min(currentFrame, (int)editor->getResults()->frames.size() - 1));
                  auto& activeFrame = *editor->getResults()->frames[safeFrame];
                  if (resultsToolState.clipEnabled && !previousClipEnabled) {
                      computeResultFrameCentroid(activeFrame, resultsToolState.clipOrigin.data());
                      resultsToolState.clipOriginInitialized = true;
                  }
                  if (resultsToolState.clipEnabled != previousClipEnabled ||
                      resultsToolState.clipAxis != previousClipAxis ||
                      std::abs(resultsToolState.surfaceOpacity - previousOpacity) > 1.0e-6f) {
                      applyResultsToolState(editor, vtkViewer_res, resultsToolState);
                      syncResultsClipTools(vtkViewer_res, resultsToolState, activeFrame);
                  }
              }
              editor->setActiveViewer(&vtkViewer_res);
              editor->handleMeasurementInteraction();
              editor->handleSelectionInteraction();
              editor->drawMeasurementOverlay();
              editor->drawSelectionOverlay();
              if (editor->getResults() != nullptr && !editor->getResults()->frames.empty()) {
                  const int safeFrame = std::max(0, std::min(currentFrame, (int)editor->getResults()->frames.size() - 1));
                  auto& overlayFrame = *editor->getResults()->frames[safeFrame];
                  const auto overlayFieldNames = overlayFrame.getAvailableFieldNames();
                  bool fieldSelectionChanged = false;
                  bool colorScaleModeChanged = false;
                  bool componentChanged = false;
                  bool vectorGlyphVisibilityChanged = false;
                  bool manualRangeChanged = false;
                  vtkDataArray* overlayActiveArray = getActiveArray(overlayFrame);
                  if (overlayActiveArray)
                      activeFieldComponents = overlayActiveArray->GetNumberOfComponents();
                  const bool vectorFieldAvailable = (overlayActiveArray != nullptr &&
                                                     overlayActiveArray->GetNumberOfComponents() == 3);
                  drawResultsPlaybackOverlay(vtkViewer_res,
                                             resultsPlaybackOverlayState,
                                             safeFrame,
                                             (int)editor->getResults()->frames.size(),
                                             editor->getResults()->frames[safeFrame]->time,
                                             isPlaying,
                                             overlayFieldNames,
                                             activeFieldName,
                                             selectedField,
                                             fieldSelectionChanged,
                                             manualColorScale,
                                             colorScaleModeChanged,
                                             activeFieldComponents,
                                             selectedFieldComponent,
                                             componentChanged,
                                             vectorFieldAvailable,
                                             showVectorGlyphs,
                                             vectorGlyphVisibilityChanged,
                                             manualMin,
                                             manualMax,
                                             manualRangeChanged);
                  positionResultsScalarBar(vtkViewer_res, currentScalarBar, resultsPlaybackOverlayState);

                  if (fieldSelectionChanged &&
                      selectedField >= 0 &&
                      selectedField < static_cast<int>(overlayFieldNames.size())) {
                      std::string selected = overlayFieldNames[selectedField];
                      isCellField = (selected.rfind("[C]", 0) == 0);
                      activeFieldName = selected.substr(4);
                      vtkDataArray* selectedArray = getActiveArray(overlayFrame);
                      activeFieldComponents = selectedArray ? selectedArray->GetNumberOfComponents() : 1;
                      selectedFieldComponent = (activeFieldComponents == 3) ? 3 :
                                               (activeFieldComponents > 1 ? 0 : -1);
                      showVectorGlyphs = false;

	                      recomputeActiveFieldScale();
	                      applyActiveFieldSelectionToAllFrames();
                        syncCurrentVectorFieldActor(overlayFrame);
	                      applyDisplayModeToResults(resultsOverlayState.displayMode, resultsOverlayState.showEdges, editor);
	                      vtkViewer_res.render();
	                  }

                  if (colorScaleModeChanged) {
                      if (manualColorScale) {
                          manualMin = globalMin;
                          manualMax = globalMax;
                      } else {
	                          recomputeActiveFieldScale();
	                          applyActiveFieldSelectionToAllFrames();
                            syncCurrentVectorFieldActor(overlayFrame);
	                      }
	                      vtkViewer_res.render();
	                  }

	                  if (componentChanged) {
	                      recomputeActiveFieldScale();
	                      applyActiveFieldSelectionToAllFrames();
                        syncCurrentVectorFieldActor(overlayFrame);
	                      vtkViewer_res.render();
	                  }

                    if (vectorGlyphVisibilityChanged) {
                        applyActiveFieldSelectionToAllFrames();
                        syncCurrentVectorFieldActor(overlayFrame);
                        vtkViewer_res.render();
                    }

	                  if (manualRangeChanged) {
	                      recomputeActiveFieldScale();
	                      applyActiveFieldSelectionToAllFrames();
                        syncCurrentVectorFieldActor(overlayFrame);
	                      vtkViewer_res.render();
	                  }

                  if (!activeFieldName.empty()) {
                      Mesh* probeReferenceMesh = editor->getResultViewerTargetMesh();
                      const ResultProbeInfo probe = buildResultProbeInfo(vtkViewer_res,
                                                                         overlayFrame,
                                                                         probeReferenceMesh,
                                                                         activeFieldName,
                                                                         isCellField,
                                                                         selectedFieldComponent);
                      drawResultProbeOverlay(
                          probe,
                          buildActiveFieldDisplayName(activeFieldName,
                                                      activeFieldComponents,
                                                      selectedFieldComponent));

                      if (vtkViewer_res.isViewportHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                          if (currentProbeHighlightActor != nullptr) {
                              renderer->RemoveActor(currentProbeHighlightActor);
                              currentProbeHighlightActor = nullptr;
                          }

                          if (probe.valid) {
                              currentProbeHighlightActor = buildProbeHighlightActor(overlayFrame, probe);
                              if (currentProbeHighlightActor != nullptr) {
                                  renderer->AddActor(currentProbeHighlightActor);
                                  vtkViewer_res.render();
                              }
                          }
                      }
                  }
              }

              ImGui::PopFont();
              ImGui::EndTabItem();
          }
          if (results_tab_was_open && !vtk_res_open) {
              editor->closeCurrentResults();
              clearResultsViewerTransientProps();
              detachResultsClipTools(vtkViewer_res, resultsToolState);
              vtkViewer_res.setActor(nullptr);
          }
          if (editor->getResults() == nullptr && currentScalarBar != nullptr) {
              clearResultsViewerTransientProps();
          }
          if (was_results_viewer_active && !results_viewer_active) {
              editor->setActiveViewer(nullptr);
          }
          was_results_viewer_active = results_viewer_active;
          
          ImGui::EndTabBar();
      }

        ImGui::End(); // cierre de la ventana contenedora

    loadPlotDialog.Draw("Load Plot", &showLoadPlotDialog);
    energyPlotDialog.Draw("Energy Plot", &showEnergyPlotDialog);
    demoDialog.Draw("Available Demos", &showDemoDialog);
    std::string selectedDemoFolder = demoDialog.ConsumeSelectedDemoPath();

    if (!selectedDemoFolder.empty()) {
        std::filesystem::path demoModel =
            std::filesystem::path(selectedDemoFolder) / "demo.wfmodel";

	      std::cout << "Opening model: " << demoModel.string() << std::endl;
        if (std::filesystem::exists(demoModel)) {
            editor->openModelFromPath(demoModel.string());
            showDemoLoadedPopup = true;

            vtkViewer2.getRenderer()->ResetCamera();
            vtkViewer2.getRenderer()->ResetCameraClippingRange();
        } else {
            std::cout << "Demo model not found: "
                      << demoModel.string() << std::endl;
        }
    }

    if (showDemoLoadedPopup) {
        ImGui::OpenPopup("Demo Loaded");
        showDemoLoadedPopup = false;
    }

    ImGui::SetNextWindowSize(ImVec2(420.0f, 0.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                            ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Demo Loaded", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize |
                               ImGuiWindowFlags_NoResize |
                               ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextWrapped("Demo loaded. Click on \"Run\" and then \"Load Results\".");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    const bool blockViewerInteraction =
        (editor->hasBlockingDialogOpen() && !editor->isSetDialogOpen()) ||
        editor->isSetSelectionActive() ||
        showLoadPlotDialog ||
        showDemoDialog ||
        ImGui::IsPopupOpen("Demo Loaded");
    vtkViewer2.setInputEnabled(!blockViewerInteraction);
    vtkViewer_res.setInputEnabled(!blockViewerInteraction);
    
    getApp().checkUpdate(); //To new Graphics Meshed and so on
    for (vtkSmartPointer<vtkProp>& actor : getApp().getPendingActorRemovals()) {
      if (actor != nullptr) {
        std::cout << "[main] removing pending actor " << actor.GetPointer()
                  << " class=" << actor->GetClassName() << std::endl;
        vtkViewer2.removeActor(actor);
      } else {
        std::cout << "[main] pending actor is null" << std::endl;
      }
    }
    getApp().clearPendingActorRemovals();

    for (int gm=0;gm<getApp().getGraphicMeshCount();gm++) {
      //cout << "is actor needed for mesh "<<gm<<": "<<getApp().getGraphicMesh(gm)->isActorNeeded()<<endl;
        if (getApp().getGraphicMesh(gm)->isActorNeeded()){
        vtkSmartPointer<vtkActor> act = getApp().getGraphicMesh(gm)->getActor();
        if (act != nullptr)
          std::cout << "Actor class: " << act->GetClassName() << std::endl;
        else
          std::cout << "Null actor pointer!" << std::endl;
          cout << "Adding Actor"<<endl;
          if (getApp().getGraphicMesh(gm)->getActor() != nullptr)
            vtkViewer2.addActor(getApp().getGraphicMesh(gm)->getActor());
          else 
            cout <<"ERROR:Null mesh ptr"<<endl;
          cout << "added "<<endl;
          getApp().getGraphicMesh(gm)->setActorNeeded(false); //CHANGE THIS TO SOMEHOW CONTAIN THE RENDERER
          
        }
      //cout << "graphi mesh count "<<getApp().getGraphicMeshCount()<<endl;
    }

    std::unordered_map<Geom*, vtkOCCTGeom*>& geomMap = getApp().getGeomToVisual();
    for (std::unordered_map<Geom*, vtkOCCTGeom*>::iterator it = geomMap.begin(); it != geomMap.end(); ++it) {
        Geom* geom = it->first;
        vtkOCCTGeom* visual = it->second;

        if (!visual->isRendered && visual->actor) {
            vtkViewer2.addActor(visual->actor);
            visual->isRendered = true;
        }
    }
    ImGui::Render();

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(window);
  
  
    
  }//main while

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
