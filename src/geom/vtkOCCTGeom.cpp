// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkProperty.h>
#include "vtkOCCTGeom.h"


//#include <vtkOCCTShapeToPolyData.h> // Ensure this utility is available!
#include <vtkCompositePolyDataMapper.h>

#include "ShapeToPolyData.h"

#include <BRepPrimAPI_MakeCylinder.hxx>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <BRepTools.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <STEPControl_Writer.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <algorithm>
#include <cmath>
#include "Geom.h"

namespace {
constexpr double kPi = 3.14159265358979323846;
}

bool vtkOCCTGeom::hasSurfaceCells() const
{
    return m_polydata != nullptr && m_polydata->GetNumberOfPolys() > 0;
}

bool vtkOCCTGeom::hasOnlyLineCells() const
{
    return m_polydata != nullptr &&
           m_polydata->GetNumberOfLines() > 0 &&
           m_polydata->GetNumberOfPolys() == 0;
}

// int argc, char* argv are required for vtkRegressionTestImage
int vtkOCCTGeom::TestReader(const std::string& path, unsigned int format)
{
    vtkNew<vtkOCCTReader> reader;
    reader->ReadWireOn();
    reader->SetFileName(path.c_str());
    reader->SetFileFormat(format);
    reader->Update();

    // Reader output is a multiblock
    vtkMultiBlockDataSet* mb = reader->GetOutput();

    // Extract first PolyData block
    vtkSmartPointer<vtkPolyData> polyData = nullptr;
    for (unsigned int i = 0; i < mb->GetNumberOfBlocks(); ++i)
    {
        vtkSmartPointer<vtkPolyData> pd = vtkPolyData::SafeDownCast(mb->GetBlock(i));
        if (pd)
        {
            polyData = pd;
            break;
        }
    }

    if (!polyData)
    {
        std::cerr << "Error: No PolyData found in the imported model." << std::endl;
        return 0;
    }

    // Save it in the member variable (so moving works)
    m_polydata = polyData;
    m_polydata->Modified();

    // Create mapper and actor
    m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    m_mapper->SetInputData(m_polydata);
    m_mapper->SetScalarRange(m_polydata->GetScalarRange());

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(m_mapper);
    actor->GetProperty()->SetLineWidth(1.0);
    actor->GetProperty()->SetOpacity(0.5);

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


void WriteSTEP(const TopoDS_Shape& shape, const std::string& filename)
{
    STEPControl_Writer writer;
    IFSelect_ReturnStatus status = writer.Transfer(shape, STEPControl_AsIs);
    if (status != IFSelect_RetDone) {
        std::cerr << "Error: STEP transfer failed." << std::endl;
        return;
    }
    status = writer.Write(filename.c_str());
    if (status != IFSelect_RetDone) {
        std::cerr << "Error: STEP write failed." << std::endl;
        return;
    }
}


void vtkOCCTGeom::LoadCylinder(double radius, double height, double angleDeg)
{
    try {
        const double clampedAngleDeg = std::max(0.0, std::min(angleDeg, 360.0));
        const bool isFullCylinder = std::abs(clampedAngleDeg - 360.0) <= 1.0e-9;

        TopoDS_Shape shape;
        if (isFullCylinder) {
            shape = BRepPrimAPI_MakeCylinder(radius, height).Shape();
        } else {
            const double angleRad = clampedAngleDeg * kPi / 180.0;
            shape = BRepPrimAPI_MakeCylinder(radius, height, angleRad).Shape();
        }
        
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
        
        //actor->GetProperty()->SetOpacity(0.5);
        //actor->GetProperty()->SetLineWidth(1.0);
        actor->GetProperty()->SetRepresentationToWireframe();

        //BRepTools::Write(shape, "cylinder.step");
        
        WriteSTEP(shape, "cylinder.step");

        
    }
    catch (const Standard_Failure& e) {
        std::cerr << "OpenCASCADE error: " << e.GetMessageString() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error in LoadCylinder" << std::endl;
    }
}

void vtkOCCTGeom::SetGeometry(Geom* g) {
  geom = g;
}

void vtkOCCTGeom::BuildVTKData(/*double deflection = 0.01*/) {
  double deflection = 0.01;
  if (!geom) return;

  LoadFromShape(geom->getShape(), deflection);
}

void vtkOCCTGeom::ReloadFromGeometry(double deflection)
{
  if (!geom) return;
  LoadFromShape(geom->getShape(), deflection);
}

void vtkOCCTGeom::LoadFromShape(const TopoDS_Shape& shape, double deflection)
{
    try {
        vtkSmartPointer<vtkPolyData> updatedPolyData = ShapeToPolyData(shape, deflection);
        if (!updatedPolyData || updatedPolyData->GetNumberOfPoints() == 0) {
            std::cerr << "Error: Failed to create polyData from shape" << std::endl;
            return;
        }

        m_polydata = updatedPolyData;
        m_polydata->Modified();

        if (!m_mapper) {
            m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        }
        m_mapper->SetInputData(m_polydata);
        m_mapper->SetScalarRange(m_polydata->GetScalarRange());

        if (!actor) {
            actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(m_mapper);
        } else if (actor->GetMapper() != m_mapper) {
            actor->SetMapper(m_mapper);
        }

        // Keep the actor transform clean: OCC geometry already contains the real transform.
        actor->SetUserTransform(nullptr);
        actor->SetPosition(0.0, 0.0, 0.0);
        actor->SetOrientation(0.0, 0.0, 0.0);
        actor->SetScale(1.0, 1.0, 1.0);

        actor->GetProperty()->SetRenderLinesAsTubes(false);
        actor->GetProperty()->SetOpacity(0.5);
        actor->GetProperty()->SetLineWidth(1.0);
        actor->GetProperty()->SetColor(1.0, 1.0, 1.0);

        const bool isLineShape =
            shape.ShapeType() == TopAbs_EDGE || shape.ShapeType() == TopAbs_WIRE;
        const bool hasOnlyLines = hasOnlyLineCells();

        if (isLineShape || hasOnlyLines) {
            actor->GetProperty()->SetOpacity(1.0);
            actor->GetProperty()->SetLineWidth(4.0);
            actor->GetProperty()->SetRenderLinesAsTubes(true);
            actor->GetProperty()->SetColor(0.45, 0.45, 0.45);
        }

        m_mapper->Update();
        actor->Modified();

    } catch (const Standard_Failure& e) {
        std::cerr << "OpenCASCADE error: " << e.GetMessageString() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error in LoadFromShape" << std::endl;
    }
}
