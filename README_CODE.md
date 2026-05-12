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

## Dialog blocking and set creation

- `src/main.cpp`
  Controls when the VTK viewers are globally frozen through `setInputEnabled(...)`.
- Current behavior:
  - most blocking dialogs disable viewer interaction
  - `Create Set` is explicitly excluded so the user can still orbit, pan, zoom, and pick nodes while the dialog is open

- `src/editor.cpp`
  Contains the node selection flow and the final node set creation flow.
- Relevant pieces:
  - `Editor::handleSelectionInteraction()`
    Handles node picking and box selection in the model viewport.
  - `Editor::isSelectorInteractionEnabled()`
    Keeps node selection enabled specifically while the set dialog is open.
  - the `m_show_set_dlg` block inside `Editor::drawGui()`
    Finalizes node set creation after the dialog is closed.
  - `findCommonMeshForNodes(...)`
    Ensures all selected nodes belong to the same mesh before creating the set.

- `src/set_dialog.cpp`
  Owns the `Create Set` dialog UI.
- Current behavior:
  - the dialog shows set name, set type, and selected node count
  - `Create` is enabled only when there is at least one selected item and the name is not empty
  - clicking inside the dialog should not clear the node selection used to build the set

Summary:
- viewer camera interaction and node picking are intentionally separated for `Create Set`
- the viewer stays interactive during set creation
- node selection is preserved while typing in the set name field
- the actual `NodeSet` object is built in `src/editor.cpp`, not in `src/set_dialog.cpp`

## Current set architecture

- `src/model/Set.h`
  Defines the generic template `Set<T>` and the current concrete `NodeSet`, `ElementSet`, and `FaceSet`.
- Current facts:
  - `NodeSet`, `ElementSet`, and `FaceSet` exist as concrete mesh-backed set types
  - `NodeSet` and `ElementSet` both have interactive selection workflows in the GUI
  - `Set<T>` already gives the common storage pattern:
    - owning mesh pointer
    - item pointer list
    - label
    - entity id
  - `FaceSet` is value-based and stores explicit boundary sides as `Face` records with:
    - node ids
    - owner element id
    - local face index

- `src/model/Mesh.h`
  Stores node sets in `m_node_sets`, element sets in `m_element_sets`, and face sets in `m_face_sets`.
- Current facts:
  - `Mesh` exposes `getNodeSetCount()`, `getNodeSet(...)`, `addNodeSet(...)`, `removeNodeSetById(...)`, and `findNodeSetById(...)`
  - `Mesh` also exposes `getElementSetCount()`, `getElementSet(...)`, `addElementSet(...)`, `removeElementSetById(...)`, and `findElementSetById(...)`
  - `Mesh` also exposes `getFaceSetCount()`, `getFaceSet(...)`, `addFaceSet(...)`, `removeFaceSetById(...)`, and `findFaceSetById(...)`

- `src/editor.cpp`
  Owns the real set workflow.
- Current facts:
  - the `Sets` tree currently lists `NodeSet`
  - context menu supports creating a new set
  - per-set context menu supports editing
  - rename has a dedicated dialog path
  - `Element Sets` has its own tree and selection state
  - `Face Sets` has its own tree
  - `ElementSet` can derive a boundary `FaceSet`
  - contact-oriented surface work should build on `FaceSet`, not directly on `NodeSet`

- `src/set_dialog.cpp`
  The dialog can display a type label such as `Node Set` or `Element Set`, but the editor-side commit path currently only executes real logic for `NODE_SET`

- `src/action.h`
- `src/action.cpp`
  Current undo/redo support for sets is minimal.
- Current facts:
  - `CreateNodeSetAction`, `CreateElementSetAction`, and `CreateFaceSetAction` exist
  - there is no equivalent action yet for:
    - rename set
    - edit set membership
    - delete set

## Selection limitations relevant to sets

- `src/selector.h`
  The selector currently stores:
  - `std::vector<Node*>`
  - `std::vector<Element*>`
- Consequences:
  - the set dialog consumes the active selection target and can now build `NodeSet` or `ElementSet`
  - there is still no dedicated face-selection target in the interactive selector
  - `sets from geometry` is still a later layer because the current internal selection abstraction is mesh-centric, not CAD-topology-centric

## Recommended set roadmap

- First stabilize mesh-based sets before geometry-driven sets:
  - `NodeSet from selected nodes`
  - `ElementSet from selected elements`
  - `NodeSet from selected elements`
  - `FaceSet from selected element faces`
  - `ElementSet -> boundary FaceSet`
  - rename / delete / show-hide / color for sets
  - undo/redo for create, edit, rename, and delete

- Reasoning:
  - geometry-driven sets should end by materializing robust mesh-backed sets
  - contact should prefer `FaceSet` / surface-style data rather than raw node groups
  - exporters and boundary-condition assignment become much easier once set identity and membership rules are stable
  - implementing geometry-driven sets before the mesh set model is generalized would force duplicate logic

## Script Browser and Python scripts

- `src/editor.cpp`
  The `Script Browser` already exists.
- Current behavior:
  - scans `./scripts`
  - lists Python files recursively
  - runs the selected script
  - captures script output into the app console

- `scripts/Radioss.py`
  There is already a Radioss-oriented Python script, but it is still basic.
- Current facts:
  - it writes nodes
  - it writes shell connectivity / part blocks
  - it is not yet a complete exporter architecture for robust set-driven workflows

## Python wrapper constraints

- `src/python/pywfGUI.i`
  Current SWIG interface mixes core model headers with GUI / geometry-heavy headers.
- Important constraint for future work:
  - the Python wrapper should stay focused on model and I/O APIs
  - do not include `VTK` types in the SWIG wrapper
  - do not include `OpenCascade` types in the SWIG wrapper
  - if needed, expose plain model classes such as:
    - `Model`
    - `Mesh`
    - `Node`
    - `Element`
    - `NodeSet`
    - `ElementSet`
    - exporters / readers that depend only on model-side data

- Practical guideline:
  - keep the scripting layer split from the interactive viewer layer
  - if a feature needs VTK or OpenCascade for picking or display, keep that in C++ GUI code and expose only the resulting model-side data structures to Python
  - for example, `Set` creation results should be scriptable, but VTK pick actors and OCC shapes should not be part of the SWIG surface

- Warning:
  - `src/python/pywfGUI.i` should stay lean and model-only
  - avoid reintroducing `geom/vtkOCCTGeom.h`, `App.h`, or other viewer / OpenCascade dependencies into the SWIG surface

## Good next sections to document

The next useful areas to document would be:
- model loading flow
- results loading flow
- where `Surface / Edges / Wire` are applied
- where node and set selection logic lives

## Solver edition runtime selection

- `src/Job.cpp`
  Runtime solver selection for jobs lives here.

Current solver selection layers:
- `Step` still decides `explicit` vs `implicit` through the exported input file.
- `Job` decides solver edition `Auto / Std / Full`.
- if the job is set to `Auto`, environment variables and local solver binaries decide which executable is launched.

Environment variables:
- `WELDFORM_SOLVER_EDITION`
  Accepted values: `auto`, `std`, `student`, `full`
  Used only when the job override is `Auto`.
- `WELDFORM_STD_NODE_LIMIT`
  Default: `500`
  Maximum allowed node count when the resolved solver edition is student/std.
- `WELDFORM_STD_ALLOW_THERMAL`
  Default: `false`
  Accepted truthy values include `1`, `true`, `yes`, `on`.
  Controls whether thermal coupling is allowed when the resolved solver edition is student/std.

Resolution order:
1. Job dialog override `Auto / Student / Full`
2. `WELDFORM_SOLVER_EDITION` when the job is `Auto`
3. automatic binary detection in `solvers/`
   If `weldform_exp` or `weldform_imp` exists, full is used.
   Otherwise the code falls back to `weldform_exp_std` or `weldform_imp_std`.

Practical examples:
- Development machine with full solvers installed:
  `export WELDFORM_SOLVER_EDITION=full`
- Student/release install:
  `export WELDFORM_SOLVER_EDITION=std`
  `export WELDFORM_STD_NODE_LIMIT=500`
  `export WELDFORM_STD_ALLOW_THERMAL=0`
