// ShapeToPolyData.h
#pragma once

#include <TopoDS_Shape.hxx>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>

// Add proper return type (vtkSmartPointer<vtkPolyData>) and parameter type (const TopoDS_Shape&)
vtkSmartPointer<vtkPolyData> ShapeToPolyData(const TopoDS_Shape& shape, double deflection = 0.1);