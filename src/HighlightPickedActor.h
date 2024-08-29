//https://examples.vtk.org/site/Cxx/Picking/HighlightPickedActor/

#include <vtkActor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkPropPicker.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

namespace {
// Handle mouse events
class MouseInteractorHighLightActor : public vtkInteractorStyleTrackballCamera
{
public:
  static MouseInteractorHighLightActor* New();
  vtkTypeMacro(MouseInteractorHighLightActor,
               vtkInteractorStyleTrackballCamera);

  MouseInteractorHighLightActor()
  {
    LastPickedActor = NULL;
    LastPickedProperty = vtkProperty::New();
  }
  virtual ~MouseInteractorHighLightActor()
  {
    LastPickedProperty->Delete();
  }
  virtual void OnLeftButtonDown() override
  {
    vtkNew<vtkNamedColors> colors;

    int* clickPos = this->GetInteractor()->GetEventPosition();

    // Pick from this location.
    vtkNew<vtkPropPicker> picker;
    picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());

    // If we picked something before, reset its property.
    if (this->LastPickedActor)
    {
      this->LastPickedActor->GetProperty()->DeepCopy(this->LastPickedProperty);
    }
    this->LastPickedActor = picker->GetActor();
    if (this->LastPickedActor)
    {
      // Save the property of the picked actor so that we can
      // restore it next time.
      this->LastPickedProperty->DeepCopy(this->LastPickedActor->GetProperty());
      // Highlight the picked actor by changing its properties.
      this->LastPickedActor->GetProperty()->SetColor(
          colors->GetColor3d("Red").GetData());
      this->LastPickedActor->GetProperty()->SetDiffuse(1.0);
      this->LastPickedActor->GetProperty()->SetSpecular(0.0);
      this->LastPickedActor->GetProperty()->EdgeVisibilityOn();
    }

    // Forward events.
    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
  }

private:
  vtkActor* LastPickedActor;
  vtkProperty* LastPickedProperty;
};

vtkStandardNewMacro(MouseInteractorHighLightActor);
} // namespace
