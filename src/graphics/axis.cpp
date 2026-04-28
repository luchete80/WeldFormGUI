#include "axis.h"

#include <array>
#include <cmath>

vtkStandardNewMacro(HoverableOrientationMarkerWidget);

namespace {
double distancePointToSegment2D(double px, double py,
                                double ax, double ay,
                                double bx, double by)
{
  const double abx = bx - ax;
  const double aby = by - ay;
  const double abLen2 = abx * abx + aby * aby;
  if (abLen2 <= 1e-12) {
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
}

Axis::Axis()
  : colors(nullptr)
{
  init();

  actor = vtkSmartPointer<vtkAxesActor>::New();
  actor->SetTotalLength(0.9, 0.9, 0.9);

  widget = vtkSmartPointer<HoverableOrientationMarkerWidget>::New();

  double rgba[4]{0.0, 0.0, 0.0, 0.0};
  widget->SetOutlineColor(rgba[0], rgba[1], rgba[2]);
  widget->SetOrientationMarker(actor);
  widget->InteractiveOff();

  setAxisHighlight(-1);
}

Axis::~Axis(){
  colors = nullptr;
}

IMGUI_IMPL_API void Axis::init(){
  colors = vtkSmartPointer<vtkNamedColors>::New();
}

void Axis::setInteractor(vtkSmartPointer<vtkRenderWindowInteractor> rwi,
                         vtkSmartPointer<vtkRenderer> mainRenderer){
  interactor = rwi;
  sceneRenderer = mainRenderer;

  widget->SetInteractor(rwi);
  widget->SetViewport(0.0, 0.0, 0.4, 0.4);
  widget->SetEnabled(1);
  widget->InteractiveOff();
  actor->PickableOff();
  setAxisHighlight(-1);

  mouseMoveCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  mouseMoveCallback->SetClientData(this);
  mouseMoveCallback->SetCallback(&Axis::HandleMouseMove);
  interactor->AddObserver(vtkCommand::MouseMoveEvent, mouseMoveCallback, 1.0);

  leftButtonPressCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  leftButtonPressCallback->SetClientData(this);
  leftButtonPressCallback->SetCallback(&Axis::HandleLeftButtonPress);
  interactor->AddObserver(vtkCommand::LeftButtonPressEvent, leftButtonPressCallback, 1.0);
}

void Axis::HandleMouseMove(vtkObject*, unsigned long, void* clientdata, void*)
{
  Axis* self = static_cast<Axis*>(clientdata);
  if (self != nullptr) {
    self->onMouseMove();
  }
}

void Axis::HandleLeftButtonPress(vtkObject*, unsigned long, void* clientdata, void*)
{
  Axis* self = static_cast<Axis*>(clientdata);
  if (self != nullptr) {
    self->onLeftButtonPress();
  }
}

bool Axis::eventInsideWidgetViewport() const
{
  if (!interactor || !widget) {
    return false;
  }

  int* eventPos = interactor->GetEventPosition();
  vtkRenderWindow* renderWindow = interactor->GetRenderWindow();
  if (!eventPos || !renderWindow) {
    return false;
  }

  int* size = renderWindow->GetSize();
  if (!size || size[0] <= 0 || size[1] <= 0) {
    return false;
  }

  double viewport[4];
  widget->GetViewport(viewport);

  const double x = static_cast<double>(eventPos[0]) / static_cast<double>(size[0]);
  const double y = static_cast<double>(eventPos[1]) / static_cast<double>(size[1]);
  return x >= viewport[0] && x <= viewport[2] && y >= viewport[1] && y <= viewport[3];
}

int Axis::pickAxisAtEventPosition() const
{
  if (!eventInsideWidgetViewport() || !widget || widget->GetOverlayRenderer() == nullptr || !interactor) {
    return -1;
  }

  vtkRenderer* widgetRenderer = widget->GetOverlayRenderer();
  int* eventPos = interactor->GetEventPosition();
  if (!eventPos) {
    return -1;
  }

  const std::array<std::array<double, 3>, 3> endpoints = {{
    {{1.0, 0.0, 0.0}},
    {{0.0, 1.0, 0.0}},
    {{0.0, 0.0, 1.0}}
  }};
  const double origin[3] = {0.0, 0.0, 0.0};
  double originDisplay[3];
  worldToDisplayPoint(widgetRenderer, origin, originDisplay);

  int bestAxis = -1;
  double bestDistance = 1e100;
  const double pixelThreshold = 18.0;

  for (int axis = 0; axis < 3; ++axis) {
    double endDisplay[3];
    worldToDisplayPoint(widgetRenderer, endpoints[axis].data(), endDisplay);
    const double distance = distancePointToSegment2D(
      static_cast<double>(eventPos[0]),
      static_cast<double>(eventPos[1]),
      originDisplay[0], originDisplay[1],
      endDisplay[0], endDisplay[1]);
    if (distance < bestDistance) {
      bestDistance = distance;
      bestAxis = axis;
    }
  }

  if (bestDistance > pixelThreshold) {
    return -1;
  }

  return bestAxis;
}

void Axis::setAxisHighlight(int axis)
{
  const std::array<std::array<double, 3>, 3> baseColors = {{
    {{0.95, 0.25, 0.25}},
    {{0.20, 0.75, 0.30}},
    {{0.25, 0.45, 0.95}}
  }};

  vtkProperty* shaftProps[3] = {
    actor->GetXAxisShaftProperty(),
    actor->GetYAxisShaftProperty(),
    actor->GetZAxisShaftProperty()
  };
  vtkProperty* tipProps[3] = {
    actor->GetXAxisTipProperty(),
    actor->GetYAxisTipProperty(),
    actor->GetZAxisTipProperty()
  };
  vtkCaptionActor2D* captions[3] = {
    actor->GetXAxisCaptionActor2D(),
    actor->GetYAxisCaptionActor2D(),
    actor->GetZAxisCaptionActor2D()
  };

  for (int i = 0; i < 3; ++i) {
    double color[3] = {
      baseColors[i][0],
      baseColors[i][1],
      baseColors[i][2]
    };
    if (i == axis) {
      color[0] = 1.0;
      color[1] = 0.95;
      color[2] = 0.35;
    }

    if (shaftProps[i]) {
      shaftProps[i]->SetColor(color);
      shaftProps[i]->SetOpacity(1.0);
    }
    if (tipProps[i]) {
      tipProps[i]->SetColor(color);
      tipProps[i]->SetOpacity(1.0);
    }
    if (captions[i] && captions[i]->GetCaptionTextProperty()) {
      captions[i]->GetCaptionTextProperty()->SetColor(color);
      captions[i]->GetCaptionTextProperty()->SetBold(i == axis ? 1 : 0);
    }
  }
}

void Axis::orientCameraToAxis(int axis)
{
  if (!sceneRenderer) {
    return;
  }

  vtkCamera* camera = sceneRenderer->GetActiveCamera();
  if (!camera) {
    return;
  }

  double focalPoint[3];
  double position[3];
  camera->GetFocalPoint(focalPoint);
  camera->GetPosition(position);

  double viewDirection[3] = {
    position[0] - focalPoint[0],
    position[1] - focalPoint[1],
    position[2] - focalPoint[2]
  };
  double distance = vtkMath::Norm(viewDirection);
  if (distance <= 1e-9) {
    distance = 1.0;
  }

  double axisDirection[3] = {0.0, 0.0, 0.0};
  double viewUp[3] = {0.0, 1.0, 0.0};
  axisDirection[axis] = 1.0;

  if (axis == 0) {
    viewUp[0] = 0.0; viewUp[1] = 0.0; viewUp[2] = 1.0;
  } else if (axis == 1) {
    viewUp[0] = 0.0; viewUp[1] = 0.0; viewUp[2] = 1.0;
  } else {
    viewUp[0] = 0.0; viewUp[1] = 1.0; viewUp[2] = 0.0;
  }

  camera->SetPosition(focalPoint[0] + axisDirection[0] * distance,
                      focalPoint[1] + axisDirection[1] * distance,
                      focalPoint[2] + axisDirection[2] * distance);
  camera->SetViewUp(viewUp);
  sceneRenderer->ResetCameraClippingRange();

  if (interactor && interactor->GetRenderWindow()) {
    interactor->GetRenderWindow()->Render();
  }
}

void Axis::onMouseMove()
{
  const int axis = pickAxisAtEventPosition();
  if (axis == hoveredAxis) {
    return;
  }

  hoveredAxis = axis;
  setAxisHighlight(hoveredAxis);
  if (interactor && interactor->GetRenderWindow()) {
    interactor->GetRenderWindow()->Render();
  }
}

void Axis::onLeftButtonPress()
{
  const int axis = pickAxisAtEventPosition();
  if (axis < 0) {
    return;
  }

  orientCameraToAxis(axis);
}
