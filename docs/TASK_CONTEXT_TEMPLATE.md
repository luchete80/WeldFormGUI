# Task Context Template

Use this as a short working context before starting a code change.

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
- Do not include editor-heavy headers in SWIG-facing code
- Do not wrap OCC / VTK / ImGui / GLFW / OpenGL types through SWIG
- Keep Python bindings focused on model/workflow APIs
- Preserve runtime loading from the build root when required
- Do not break model-only code paths

Validation:
- Reconfigure CMake if sources or CMake files changed
- Rebuild affected targets
- Test in UI / Script Browser / Python console as needed
```

## Example

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
- Do not wrap GUI/render/CAD internals through SWIG

Validation:
- Reconfigure CMake
- Rebuild _dnlPython
- Open app and run the helper from Python
```
