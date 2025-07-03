// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkProperty.h>
#include "vtkOCCTGeom.h"

#include <BRepPrimAPI_MakeCylinder.hxx>
#include <TopoDS_Shape.hxx>

//#include <vtkOCCTShapeToPolyData.h> // Ensure this utility is available!
#include <vtkCompositePolyDataMapper.h>

#include "ShapeToPolyData.h"

#include <BRepPrimAPI_MakeCylinder.hxx>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

// int argc, char* argv are required for vtkRegressionTestImage
int vtkOCCTGeom::TestReader(const std::string& path, unsigned int format)
{
 
  vtkNew<vtkOCCTReader> reader;
  //reader->RelativeDeflectionOn();
  //reader->SetLinearDeflection(0.1);
  //reader->SetAngularDeflection(0.5);
  reader->ReadWireOn();
  reader->SetFileName(path.c_str());
  reader->SetFileFormat(format);
  reader->Update();
  
  
  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputDataObject(reader->GetOutput());
  actor = vtkSmartPointer <vtkActor>::New();
  actor->SetMapper(mapper);
  //actor->GetProperty()->SetRepresentationToWireframe();
  actor->GetProperty()->SetLineWidth(1.0);
  actor->GetProperty()->SetOpacity(0.5); // 50% transparente 

  /*
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  renderWindowInteractor->SetRenderWindow(renderWindow);
  
  */
  
  /*
  renderWindow->SetSize(400, 400);
  renderer->ResetCamera();
  renderWindow->Render();


  /* SHOULD ADD TESTING MODULE!
  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
  */
   // renderWindowInteractor->Start();
  /*
  }

  return retVal;
  */
  return 1;
}



  int vtkOCCTGeom::readFile(int argc, char* argv[])
  {
    /*
    if (argc < 3)
    {
      //return EXIT_FAILURE;
    }

    if (!TestReader(
          argc, argv, std::string{ argv[2] } += "/Data/wall.stp", vtkOCCTReader::Format::STEP))
    {
      return EXIT_FAILURE;
    }

    if (!TestReader(
          argc, argv, std::string{ argv[2] } += "/Data/wall.iges", vtkOCCTReader::Format::IGES))
    {
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
    */
    return EXIT_SUCCESS;
  }


/*
int vtkOCCTGeom::TestOCCTReader(int argc, char* argv[])
{
  if (argc < 3)
  {
    return EXIT_FAILURE;
  }

  if (!TestReader(
        argc, argv, std::string{ argv[2] } += "/Data/wall.stp", vtkOCCTReader::Format::STEP))
  {
    return EXIT_FAILURE;
  }

  if (!TestReader(
        argc, argv, std::string{ argv[2] } += "/Data/wall.iges", vtkOCCTReader::Format::IGES))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
*/


void vtkOCCTGeom::LoadCylinder(double radius, double height)
{
    try {
        // Create OCC cylinder
        TopoDS_Shape shape = BRepPrimAPI_MakeCylinder(radius, height).Shape();
        
        // Convert to VTK polydata with reasonable deflection
        const double deflection = radius / 20.0;
        vtkSmartPointer<vtkPolyData> polyData = ShapeToPolyData(shape, deflection);
        
        if (!polyData || polyData->GetNumberOfPoints() == 0) {
            std::cerr << "Error: Failed to create polyData from shape" << std::endl;
            return;
        }
        
        // Create mapper and actor
        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputData(polyData);
        
        actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetOpacity(0.5);
        actor->GetProperty()->SetLineWidth(1.0);
    }
    catch (const Standard_Failure& e) {
        std::cerr << "OpenCASCADE error: " << e.GetMessageString() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error in LoadCylinder" << std::endl;
    }
}