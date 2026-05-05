# Code README

Quick map of the codebase to locate the main GUI pieces.

## Application entry point

- `src/main.cpp`
  Main application entry point.
- This is where the app initializes:
  - GLFW
  - Dear ImGui
  - theme and fonts
  - the main VTK viewers
  - the top menu bar `File / Edit / View`

## Viewport overlay

- `src/main.cpp`
  The overlay shown on top of the viewport lives here.
- Main functions:
  - `drawToolbarButton(...)`
  - `drawAxisButton(...)`
  - `drawProjectionButton(...)`
  - `drawFitIconButton()`
  - `drawScreenshotIconButton()`
  - `drawViewportOverlay(...)`
- Main call sites:
  - model viewport
  - results viewport

Important:
- The active overlay is not in `src/editor.cpp`.
- It is also not in `src/ViewportWindow.cpp`.
- The current VTK viewport overlay used by the app lives in `src/main.cpp`.

## Left panel and model/results tree

- `src/editor.cpp`
  This file contains the large left-side editor panel logic.
- Main function:
  - `Editor::drawGui()`
- This is where you will find:
  - `Model` and `Results` tabs
  - the `Models`, `Sets`, `Materials`, `Boundary Conditions`, `Steps`, etc. tree
  - the `Selection` block
  - material, part, model, step, and job dialogs

## VTK viewer

- `src/VtkViewer.h`
- `src/VtkViewer.cpp`

This class wraps the VTK viewer embedded into ImGui:
- render window
- renderer
- interactor
- OpenGL viewport texture
- orthographic/perspective projection
- axes
- camera reset
- screenshot capture

Useful functions:
- `render(...)`
- `resetCamera()`
- `setProjectionMode(...)`
- `setAxesVisible(...)`
- `orientCameraToAxis(...)`
- `saveScreenshot()`

## Legacy viewport window

- `src/ViewportWindow.cpp`
- `src/ViewportWindow.h`

This module exists, but it is not where the active VTK viewer overlay currently lives.
It is better treated as old or auxiliary viewport code until we decide to clean it up or reuse it.

## Materials

- `src/material_dialog.cpp`
  Material dialog and curve plot.
- `src/model/Material.h`
  Material definition and helpers such as Hollomon / Johnson-Cook.

## Results

- `src/editor.cpp`
  Result loading and result-side editor state.
- `src/main.cpp`
  Result viewport rendering and visual overlay application.

## Good next sections to document

The next useful areas to document would be:
- model loading flow
- results loading flow
- where `Surface / Edges / Wire` are applied
- where node and set selection logic lives
