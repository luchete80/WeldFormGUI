#include "VtkViewer.h"

// dear imgui: Renderer for VTK(OpenGL back end)
// - Desktop GL: 2.x 3.x 4.x
// - Embedded GL: ES 2.0 (WebGL 1.0), ES 3.0 (WebGL 2.0)
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..) and a renderer binding (OpenGL)

// Implemented features:

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
// 

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>     // intptr_t
#else
#include <stdint.h>     // intptr_t
#endif
#include <cmath>

// OpenGL Loader
// This can be replaced with another loader, e.g. glad, but
// remember to change the corresponding initialize call below!
#include <GL/gl3w.h>            // GL3w, initialized with gl3wInit()

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>
#include <vtkCamera.h>
#include <vtkPNGWriter.h>
#include <vtkWindowToImageFilter.h>

//#include <vtkArrowSource.h>

namespace fs = std::filesystem;

void VtkViewer::isCurrentCallbackFn(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData){
	bool* isCurrent = static_cast<bool*>(callData);
	*isCurrent = true;
}

void VtkViewer::processEvents(){
	if (!inputEnabled)
		return;

	const bool viewportHovered = viewportItemHovered;
	if (!ImGui::IsWindowFocused() && !viewportHovered){
		return;
	}

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigWindowsMoveFromTitleBarOnly = true; // don't drag window when clicking on image.
	double xpos = static_cast<double>(io.MousePos.x - viewportScreenMin.x);
	double ypos = static_cast<double>(io.MousePos.y - viewportScreenMin.y);
	int ctrl = static_cast<int>(io.KeyCtrl);
	int shift = static_cast<int>(io.KeyShift);
	bool dclick = io.MouseDoubleClicked[0] || io.MouseDoubleClicked[1] || io.MouseDoubleClicked[2];

	interactor->SetEventInformationFlipY(xpos, ypos, ctrl, shift, dclick);

	if (viewportHovered){
		if (io.MouseClicked[ImGuiMouseButton_Left]){
			interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent, nullptr);
		}
		else if (io.MouseClicked[ImGuiMouseButton_Right]){
			interactor->InvokeEvent(vtkCommand::RightButtonPressEvent, nullptr);
			ImGui::SetWindowFocus(); // make right-clicks bring window into focus
		}
		else if (io.MouseWheel > 0){
			interactor->InvokeEvent(vtkCommand::MouseWheelForwardEvent, nullptr);
		}
		else if (io.MouseWheel < 0){
			interactor->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, nullptr);
		}
	}

	if (io.MouseReleased[ImGuiMouseButton_Left]){
		interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, nullptr);
	}
	else if (io.MouseReleased[ImGuiMouseButton_Right]){
		interactor->InvokeEvent(vtkCommand::RightButtonReleaseEvent, nullptr);
	}

	interactor->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr);
}

VtkViewer::VtkViewer() 
	: viewportWidth(0), viewportHeight(0), renderWindow(nullptr), interactor(nullptr), interactorStyle(nullptr),
	renderer(nullptr), tex(0), firstRender(true){
	init();
}

VtkViewer::VtkViewer(const VtkViewer& vtkViewer) 
	: viewportWidth(0), viewportHeight(0), renderWindow(vtkViewer.renderWindow), interactor(vtkViewer.interactor),
	interactorStyle(vtkViewer.interactorStyle), renderer(vtkViewer.renderer), tex(vtkViewer.tex),
	firstRender(vtkViewer.firstRender){
  axesActor = vtkViewer.axesActor;
  projectionMode = vtkViewer.projectionMode;
  axesVisible = vtkViewer.axesVisible;
}

VtkViewer::VtkViewer(VtkViewer&& vtkViewer) noexcept 
	: viewportWidth(0), viewportHeight(0), renderWindow(std::move(vtkViewer.renderWindow)),
	interactor(std::move(vtkViewer.interactor)), interactorStyle(std::move(vtkViewer.interactorStyle)),
	renderer(std::move(vtkViewer.renderer)), tex(vtkViewer.tex), firstRender(vtkViewer.firstRender){
  axesActor = std::move(vtkViewer.axesActor);
  projectionMode = vtkViewer.projectionMode;
  axesVisible = vtkViewer.axesVisible;
}

VtkViewer::~VtkViewer(){
	renderer = nullptr;
	interactorStyle = nullptr;
	interactor = nullptr;
	renderWindow = nullptr;

	glDeleteTextures(1, &tex);
}

VtkViewer& VtkViewer::operator=(const VtkViewer& vtkViewer){
	viewportWidth = vtkViewer.viewportWidth;
	viewportHeight = vtkViewer.viewportHeight;
	renderWindow = vtkViewer.renderWindow;
	interactor = vtkViewer.interactor;
	interactorStyle = vtkViewer.interactorStyle;
	renderer = vtkViewer.renderer;
	tex = vtkViewer.tex;
	firstRender = vtkViewer.firstRender;
  axesActor = vtkViewer.axesActor;
  projectionMode = vtkViewer.projectionMode;
  axesVisible = vtkViewer.axesVisible;
	return *this;
}

void VtkViewer::init(){

	renderer = vtkSmartPointer<vtkRenderer>::New();
	renderer->ResetCamera();
	renderer->SetBackground(0.2,0.2,0.8);
  renderer->SetBackground2(0.8,0.8,0.8);
  renderer->GradientBackgroundOn();

	renderer->SetBackgroundAlpha(DEFAULT_ALPHA);

	interactorStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
	interactorStyle->SetDefaultRenderer(renderer);

	interactor = vtkSmartPointer<vtkGenericRenderWindowInteractor>::New();
	interactor->SetInteractorStyle(interactorStyle);
	interactor->EnableRenderOff();

	int viewportSize[2] = {static_cast<int>(viewportWidth), static_cast<int>(viewportHeight)};

	renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
	renderWindow->SetSize(viewportSize);

	vtkSmartPointer<vtkCallbackCommand> isCurrentCallback = vtkSmartPointer<vtkCallbackCommand>::New();
	isCurrentCallback->SetCallback(&isCurrentCallbackFn);
	renderWindow->AddObserver(vtkCommand::WindowIsCurrentEvent, isCurrentCallback);

	renderWindow->SwapBuffersOn();

	renderWindow->SetOffScreenRendering(true);
	renderWindow->SetFrameBlitModeToNoBlit();

	renderWindow->AddRenderer(renderer);
	renderWindow->SetInteractor(interactor);

  axesActor = vtkSmartPointer<vtkAxesActor>::New();
  axesActor->SetTotalLength(0.25, 0.25, 0.25);
  axesActor->SetPickable(0);
  axesActor->SetVisibility(0);
  renderer->AddActor(axesActor);

	if (!renderer || !interactorStyle || !renderWindow || !interactor){
		throw VtkViewerError("Couldn't initialize VtkViewer");
	}

}

void VtkViewer::render(){
	render(ImGui::GetContentRegionAvail());
}
void VtkViewer::render(const ImVec2 size){
	setViewportSize(size);

	renderWindow->Render();
	renderWindow->WaitForCompletion();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
	ImGui::BeginChild("##Viewport", size, true, VtkViewer::NoScrollFlags());
	ImGui::Image(reinterpret_cast<void*>(tex), ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
	viewportScreenMin = ImGui::GetItemRectMin();
	viewportScreenMax = ImGui::GetItemRectMax();
  viewportItemHovered = ImGui::IsItemHovered();
	processEvents();
	ImGui::EndChild();
	ImGui::PopStyleVar();
}

void VtkViewer::addActor(const vtkSmartPointer<vtkProp>& actor){
  cout << "Adding to renderer"<<endl;
	renderer->AddActor(actor);
	cout <<"Done"<<endl;
  if (renderer !=nullptr){
    resetCamera();
  }else
    cout << "ERROR: Null renderer."<<endl;
  cout << "Camera reset"<<endl;
}

void VtkViewer::addActors(const vtkSmartPointer<vtkPropCollection>& actors){
	actors->InitTraversal();
	vtkProp* actor;
	vtkCollectionSimpleIterator sit;
	for (actors->InitTraversal(sit); (actor = actors->GetNextProp(sit));){
		renderer->AddActor(actor);
		resetCamera();
	}
}

void VtkViewer::removeActor(const vtkSmartPointer<vtkProp>& actor){
	if (!renderer || !actor) {
		cout << "[VtkViewer::removeActor] renderer or actor is null" << endl;
		return;
	}

	cout << "[VtkViewer::removeActor] removing actor " << actor.GetPointer()
	     << " class=" << actor->GetClassName()
	     << " hasBefore=" << renderer->HasViewProp(actor) << endl;

	while (renderer->HasViewProp(actor)) {
		renderer->RemoveViewProp(actor);
	}

	cout << "[VtkViewer::removeActor] removed actor " << actor.GetPointer()
	     << " hasAfter=" << renderer->HasViewProp(actor) << endl;
}

void VtkViewer::setViewportSize(const ImVec2 newSize){
	if (((viewportWidth == newSize.x && viewportHeight == newSize.y) || viewportWidth <= 0 || viewportHeight <= 0) && !firstRender){
		return;
	}

	viewportWidth = static_cast<unsigned int>(newSize.x);
	viewportHeight = static_cast<unsigned int>(newSize.y);

	int viewportSize[] = {static_cast<int>(newSize.x), static_cast<int>(newSize.y)};

	// Free old buffers
	glDeleteTextures(1, &tex);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, viewportWidth, viewportHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D, 0);

	renderWindow->InitializeFromCurrentContext();
	renderWindow->SetSize(viewportSize);
	interactor->SetSize(viewportSize);

	auto vtkfbo = renderWindow->GetDisplayFramebuffer();
	vtkfbo->Bind();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	vtkfbo->UnBind();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	firstRender = false;
}

void VtkViewer::resetCamera(){
  if (!renderer) {
    return;
  }

  renderer->ResetCamera();
  renderer->ResetCameraClippingRange();
}

void VtkViewer::setProjectionMode(ProjectionMode mode){
  projectionMode = mode;
  if (!renderer) {
    return;
  }

  vtkCamera* camera = renderer->GetActiveCamera();
  if (!camera) {
    return;
  }

  camera->SetParallelProjection(mode == ProjectionMode::Orthographic ? 1 : 0);
  renderer->ResetCameraClippingRange();
}

void VtkViewer::setAxesVisible(bool visible){
  axesVisible = visible;
  if (axesActor) {
    axesActor->SetVisibility(visible ? 1 : 0);
  }
}

void VtkViewer::orientCameraToAxis(int axis){
  if (!renderer || axis < 0 || axis > 2) {
    return;
  }

  vtkCamera* camera = renderer->GetActiveCamera();
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
  double distance = std::sqrt(viewDirection[0] * viewDirection[0] +
                              viewDirection[1] * viewDirection[1] +
                              viewDirection[2] * viewDirection[2]);
  if (distance <= 1e-9) {
    distance = 1.0;
  }

  const double invDistance = 1.0 / distance;
  double currentDirection[3] = {
    viewDirection[0] * invDistance,
    viewDirection[1] * invDistance,
    viewDirection[2] * invDistance
  };

  double axisDirection[3] = {0.0, 0.0, 0.0};
  double viewUp[3] = {0.0, 1.0, 0.0};
  double sign = 1.0;
  const double alignment = currentDirection[axis];
  if (std::abs(alignment) >= 0.98) {
    sign = (alignment > 0.0) ? -1.0 : 1.0;
  }
  axisDirection[axis] = sign;

  if (axis == 0 || axis == 1) {
    viewUp[0] = 0.0;
    viewUp[1] = 0.0;
    viewUp[2] = 1.0;
  }

  camera->SetPosition(focalPoint[0] + axisDirection[0] * distance,
                      focalPoint[1] + axisDirection[1] * distance,
                      focalPoint[2] + axisDirection[2] * distance);
  camera->SetViewUp(viewUp);
  camera->OrthogonalizeViewUp();
  renderer->ResetCameraClippingRange();
}

bool VtkViewer::saveScreenshot() const{
	if (!renderWindow) {
		return false;
	}

	std::time_t now = std::time(nullptr);
	std::tm local_tm = {};
#ifdef _WIN32
	localtime_s(&local_tm, &now);
#else
	localtime_r(&now, &local_tm);
#endif

	std::ostringstream base_name;
	base_name << "screenshot_" << std::put_time(&local_tm, "%y%m%d");

	fs::path output_path = fs::current_path() / (base_name.str() + ".png");
	for (int suffix = 1; fs::exists(output_path); ++suffix) {
		output_path = fs::current_path() /
			(base_name.str() + "_" + (suffix < 10 ? "0" : "") + std::to_string(suffix) + ".png");
	}

	vtkSmartPointer<vtkWindowToImageFilter> window_to_image =
		vtkSmartPointer<vtkWindowToImageFilter>::New();
	window_to_image->SetInput(renderWindow);
	window_to_image->SetInputBufferTypeToRGBA();
	window_to_image->ReadFrontBufferOff();
	window_to_image->Update();

	vtkSmartPointer<vtkPNGWriter> writer = vtkSmartPointer<vtkPNGWriter>::New();
	writer->SetFileName(output_path.string().c_str());
	writer->SetInputConnection(window_to_image->GetOutputPort());
	writer->Write();

	const bool written = fs::exists(output_path);
	if (written) {
		std::cout << "Saved screenshot to " << output_path.string() << std::endl;
	} else {
		std::cout << "Failed to save screenshot to " << output_path.string() << std::endl;
	}
	return written;
}




void VtkViewer::arrowtest(){
  /*
    vtkNew<vtkNamedColors> colors;

  // Create an arrow.
  vtkNew<vtkArrowSource> arrowSource;
  // arrowSource->SetShaftRadius(1.0);
  // arrowSource->SetTipLength(1.0);
  arrowSource->Update();

  // Create a mapper and actor
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(arrowSource->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Visualize
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetWindowName("Arrow");
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  */
}
#include <vtkImageData.h>

namespace fs = std::filesystem;
