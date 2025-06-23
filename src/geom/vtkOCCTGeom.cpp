// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkProperty.h>
#include "vtkOCCTGeom.h"

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

