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

Important distinction:
- toolbar / HUD overlay belongs to `src/main.cpp`
- model-highlight and boundary-condition VTK actor overlays belong to `src/editor.cpp`
- relevant editor-side functions:
  - `Editor::updateBoundaryConditionOverlay()`
  - `Editor::clearBoundaryConditionOverlay()`
  - `Editor::updatePartOverlay()`
  - `Editor::clearPartOverlay()`

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

- Important synchronization detail:
  - the editor tree uses `Editor::m_model`
  - the application singleton uses `App::_activeModel`
  - when Python changes the active model through the workflow layer, the editor must eventually observe that change
  - the current synchronization point is at the beginning of `Editor::drawGui()`, where the editor compares `getApp().getActiveModel()` against `m_model` and adopts the active model when needed

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

## Element Types

- `src/model/Element.h`
  Runtime element classes still represent the concrete mesh items used by the current solver and viewer.
- `src/model/ElementType.h`
  Element-type inference helpers live here.
- Current behavior:
  - `ElementType` is currently inferred, not persisted
  - the initial supported inferred types are:
    - `Line2`
    - `Tria3`
    - `Quad4`
    - `Tetra4`
    - `Hexa8`
  - `ElementUsage` is also inferred:
    - `Bulk`
    - `Boundary`
    - `Unknown`

- Important:
  - this layer is intended to support model checks and exporters first
  - it should not change the current WeldForm solver input contract by itself
  - future `Section` work should build on these inferred/export-facing semantics instead of duplicating them

## Results

- `src/editor.cpp`
  Result loading and result-side editor state.
- `src/main.cpp`
  Result viewport rendering and visual overlay application.

## Model Check

- `src/modelcheck/ModelCheck.h`
- `src/modelcheck/ModelCheck.cpp`
  The current model validation layer lives here.
- Current behavior:
  - `ModelChecker` runs solver-agnostic checks plus profile-specific checks
  - the first integrated profile is `WeldForm`
  - current report shape includes:
    - severity
    - category
    - code
    - message
    - optional entity id
  - current WeldForm-oriented topology checks include:
    - deformable parts without bulk elements
    - rigid parts containing bulk elements
    - rigid 2D parts not using `Line2`
    - rigid 3D parts not using `Tria3`
    - implicit deformable parts not using `Quad4`

- `src/editor.cpp`
  Current integration point for launch-time validation.
- Relevant pieces:
  - `Editor::createJobFromActiveModel(bool runJob)`
    Runs the check before creating/running a job from the active model
  - `Editor::runModelCheckBeforeJobRun(Model&)`
    Logs issues to the app console and blocks job creation when errors exist
  - `Editor::drawModelCheckPopup()`
    Shows the blocking popup summary when the check fails

- Important:
  - the current check runs before `Run` when launching from the active model
  - it does not yet re-validate an already-exported `.wfinput` when rerunning an old job entry
  - this is intentionally model-level validation, not solver-binary validation

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

## Pending architecture priorities

- Keep planned work separate from currently-implemented behavior.
- Near-term priorities that are not yet complete:
  - `Section` model for solver-facing property assignment
  - smarter selection workflows, including face-oriented and geometry-assisted selection
  - pre-run `model check` validation before launching the solver

- Recommended order:
  - first make solver-critical assignment explicit:
    - material assignment must become robust before multi-solver export can be trusted
    - a future `Section` should likely own:
      - material reference
      - element / formulation type
      - optional solver-facing properties such as thickness or integration options
  - then continue improving selection:
    - selection should stay mesh-backed internally even when the UI becomes geometry-driven
    - `FaceSet` remains the correct base for contact and surface export workflows
  - add a `model check` gate before solver launch:
    - validate missing material / section assignment
    - validate empty or cross-mesh sets
    - validate unsupported combinations for the chosen solver/exporter
    - validate required step / boundary-condition / mesh prerequisites

- Reasoning:
  - multi-solver export needs a stable semantic layer for "what properties apply to which region"
  - that semantic layer is closer to `Section` than to raw selection
  - selection quality still matters, but it should feed stable model objects instead of becoming the semantic model itself
  - `model check` is important early because it protects solver runs while the data model is still evolving

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

## Python workflow model synchronization

- `src/python/WorkflowAPI.h`
  High-level Python helpers such as `create_active_model(...)`, STEP import, BDF import, translation, and meshing live here.

- `src/python/WorkflowBridge.cpp`
  This file is intentionally small.
- Current purpose:
  - provide a native bridge entry point callable from `WorkflowAPI.h`
  - keep SWIG-facing workflow code independent from GUI-heavy headers
  - avoid including `src/editor.h` in the Python wrapper build

- Current implementation rule:
  - `WorkflowBridge.cpp` should only depend on lightweight application-side APIs when possible
  - right now `syncScriptModelWithEditor(Model*)` only sets `App::setActiveModel(model)`
  - it does not directly access the global `editor` symbol anymore
  - this avoids unresolved-symbol issues when loading `_dnlPython.so`

- Current UI sync behavior:
  - Python changes the active model through `App`
  - `Editor::drawGui()` detects when `App::getActiveModel()` differs from `Editor::m_model`
  - the editor then updates:
    - `m_model`
    - `selected_mod`
    - `is_model`
    - `m_creating_model`
    - `m_expand_model_tree_once`
  - this makes the tree follow Python-created models without requiring the Python module to link against editor-only globals

## Python wrapper constraints

- `src/python/WorkflowAPI.h`
  This header is parsed by SWIG and must stay lightweight.
- Important constraint:
  - do not include `src/editor.h` from `WorkflowAPI.h`
  - `editor.h` pulls GUI and rendering types such as `GLFWwindow` and `GLuint`
  - those dependencies break the Python wrapper build because the SWIG module is not a GUI target

- `src/python/WorkflowBridge.cpp`
  This file exists as a bridge between the Python workflow layer and the live editor instance.
- Purpose:
  - keep `WorkflowAPI.h` independent from editor UI headers
  - provide a small native C++ entry point for active-model synchronization
  - allow Python workflows to create or switch the active model without exposing full editor internals to SWIG

- Current design intent:
  - Python code should call high-level workflow helpers such as `create_active_model(...)`
  - workflow helpers may call a bridge function such as `syncScriptModelWithEditor(Model*)`
  - `WorkflowAPI.h` should only declare the bridge function, not include editor-heavy implementation details
  - prefer indirect synchronization through `App` over direct references to `Editor` from the Python module

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

## Console notes

- `src/console.h`
  The embedded console now prioritizes output visibility over demo controls.
- Current behavior:
  - multiline Python input
  - compact input area
  - `Run`, `Clear Input`, `Copy Log`, and `Clear Log` controls grouped near the input
  - output area should consume most of the console height
- Important limitation:
  - this console is useful for short interactive snippets and small script blocks
  - larger workflows are still better executed as files through the Script Browser

## Build output notes

- `src/CMakeLists.txt`
  The build root is expected to contain the main executable and selected runtime libraries/modules used during development.
- Practical note:
  - copying runtime artifacts from subdirectory targets must not use `add_custom_command(TARGET ...)` from a different CMake directory
  - if a copy step needs to aggregate targets created in subdirectories, prefer a dedicated custom target that depends on them

## Good prompt/context practice

Yes, it is good practice to maintain a compact "working context" for coding tasks in this repository.

Useful context usually includes:
- target directories
- affected subsystems
- known ownership boundaries
- runtime assumptions
- build constraints
- current limitations

A good compact prompt/context block for this codebase would mention:
- where the GUI lives
- where Python workflow code lives
- where model-side classes live
- whether the task is UI-side, model-side, or SWIG-side
- whether the change must be safe for the Python module build
- whether the result must appear in the editor tree, the viewer, or only in exported data

Recommended rule:
- keep this context short and operational
- prefer directory and responsibility notes over long prose
- update it when architecture decisions change
- do not duplicate large code explanations when a short map is enough

## Suggested Task Context Template

Use a compact block like this before starting a change:

For a reusable standalone version, see:
- `TASK_CONTEXT_TEMPLATE.md`

```text
Task:
- Short description of the requested change

Scope:
- Main directories/files involved
- Example: src/editor.cpp, src/python/WorkflowAPI.h, src/model/*

Layer:
- UI / Editor
- App state
- Model / Mesh
- Geometry / VTK
- Python / SWIG
- Build / CMake

Expected visible result:
- Tree update
- Viewer update
- Exported file change
- Console behavior
- Python API change

Constraints:
- Must not include editor-heavy headers in SWIG-facing code
- Must preserve runtime loading from build root
- Must not break model-only code paths

Validation:
- Reconfigure CMake if sources/CMake changed
- Rebuild affected targets
- Test in UI / Script Browser / Python console as needed
```

Example:

```text
Task:
- Add a Python helper that creates a new active model

Scope:
- src/python/WorkflowAPI.h
- src/python/WorkflowBridge.cpp
- src/editor.cpp

Layer:
- Python / SWIG
- App state
- UI / Editor

Expected visible result:
- New model becomes active
- Model tree updates on next frame

Constraints:
- Do not include editor.h in WorkflowAPI.h
- Do not depend on the global editor symbol from the Python module

Validation:
- Reconfigure CMake
- Rebuild _dnlPython
- Open app and run the helper from Python
```

- Warning:
  - `src/python/pywfGUI.i` should stay lean and model-only
  - avoid reintroducing `geom/vtkOCCTGeom.h`, `App.h`, or other viewer / OpenCascade dependencies into the SWIG surface

## Python workflow wrapping plan

- Goal:
  support end-to-end scripting for common preprocessing tasks without exposing viewer internals.

- The boundary for the SWIG layer should be:
  - allowed:
    - `Model`
    - `Part`
    - `Mesh`
    - `Node`
    - `Element`
    - `NodeSet`
    - `ElementSet`
    - exporter helpers
    - small workflow helper functions in `src/python`
  - not allowed:
    - `vtkOCCTGeom`
    - ImGui dialog classes
    - VTK picker / renderer types
    - raw OCC topology types such as `TopoDS_Shape`

- Practical consequence:
  - if Python needs `import STEP`, do not wrap OCC directly
  - instead, expose a narrow helper such as `import_step_part(...)` that returns a regular `Part*`
  - if Python needs `mesh imported geometry`, expose a narrow helper such as `mesh_part_geometry(...)` or `import_and_mesh_step_part(...)`
  - if Python needs an empty mesh-backed part, expose a helper such as `create_empty_mesh_part(...)`
  - if Python needs `create rectangle`, expose a helper such as `create_rectangle_part(...)`
  - if Python needs set creation, expose mesh-side helpers that populate `NodeSet` and `ElementSet`
  - if Python needs access to the active GUI model, prefer tiny workflow helpers such as `get_active_model()`, `add_part_to_active_model(...)`, and `request_view_update()` instead of wrapping the full `App` API

- Current first implementation:
  - `src/python/WorkflowAPI.h`
  - exported through `src/python/pywfGUI.i`
  - intended to keep the scripting API at the model/workflow level instead of the viewer level

- Why this is the right cut:
  - geometry creation/import can still use existing C++ `Geom` logic internally
  - geometry meshing can still use the existing C++ meshing path internally:
    - 2D deformable parts via `mesh-adapt`
    - 3D or rigid parts via the existing Gmsh path
  - Python does not need to know about OCC handles or VTK actors
  - the GUI remains responsible for visualization and picking
  - exporters can work from the same mesh/set data used by the GUI

## Set scripting direction

- `src/set_dialog.cpp` is only the UI layer for naming, mode, and commit/cancel behavior.
- The real data model is still mesh-backed:
  - `NodeSet`
  - `ElementSet`
  - later `FaceSet`

- For scripting, the useful contract is not dialog interaction itself.
- The useful contract is:
  - create a node set from node indices or node IDs
  - create an element set from element indices or element IDs
  - later add edit/replace helpers with the same semantics as the GUI commit path in `src/editor.cpp`

- Existing reference:
  - `scripts/selection.py` already contains a lightweight Python-side implementation for node and element sets by ID.
  - this is a good behavioral reference, but the longer-term API should live in the wrapped C++ workflow layer so scripts use the same supported entry points.

## Scriptable node selection roadmap

- First scriptable selection layer:
  - mesh-space box selection by coordinates
  - example API:
    - `find_node_indices_in_box(...)`
    - `find_node_ids_in_box(...)`
    - `add_node_set_from_box(...)`

- Why start here:
  - it is deterministic
  - it depends only on `Mesh` and `Node` coordinates
  - it does not require wrapping the GUI selector, VTK picker, or OCC topology
  - it is immediately useful for preprocessing scripts and exporters

- Later extensions that make sense:
  - selection from one or more points with tolerance
  - selection from line / polyline distance bands
  - selection from meshed boundary faces
  - selection from geometry-derived surfaces once a stable CAD-to-mesh query layer exists

- Important design rule:
  - Python should express selection criteria
  - C++ should materialize the resulting `NodeSet` / `ElementSet`
  - the interactive selector in the GUI remains a separate concern

## Recommended next wrapper steps

- Add `replace_node_set_*` and `replace_element_set_*` helpers that mirror the edit path in `src/editor.cpp`.
- Add a safe helper to list set contents back to Python for exporter and QA scripts.
- Keep all of that in `src/python` instead of wrapping dialog or rendering code.

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
