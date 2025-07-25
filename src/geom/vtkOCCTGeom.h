// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef _VTKOCCGEOM_
#define _VTKOCCGEOM_

#include <iostream>

#include <vtkActor.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkMultiBlockDataSet.h>

#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>

#include <vtkNew.h>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <TopoDS_Shape.hxx>

#include "vtkOCCTReader.h"

class Geom;

class Mesh;
//TODO: ASSOCIATE WITH A MESH! 
class vtkOCCTGeom{
protected:
  //vtkSmartPointer<vtkOCCTReader> occtreader;
  Mesh *m_mesh;
  Geom *geom = nullptr;   // Non-owning pointer (In geom is the real thing)

public:
  vtkSmartPointer <vtkActor> actor;
  vtkOCCTGeom(){}
  int readFile(int argc, char* argv[]);

  int TestReader(const std::string& path, unsigned int format);
  void LoadCylinder(double radius, double height);

  void SetGeometry(Geom* g);

  void BuildVTKData();


  bool isRendered = false;

  ~vtkOCCTGeom(){}
};

// int argc, char* argv are required for vtkRegressionTestImage
//int TestReader(int argc, char* argv[], const std::string& path, unsigned int format);

  /*
  vtkNew<vtkOCCTReader> reader;
  reader->RelativeDeflectionOn();
  reader->SetLinearDeflection(0.1);
  reader->SetAngularDeflection(0.5);
  reader->ReadWireOn();
  reader->SetFileName(path.c_str());
  reader->SetFileFormat(format);
  reader->Update();

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputDataObject(reader->GetOutput());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->RotateY(90);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderWindow->SetSize(400, 400);
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return retVal;
  */
/*

int TestOCCTReader(int argc, char* argv[])
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
}*/

#endif
