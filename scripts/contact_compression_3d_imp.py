from pathlib import Path

from model import *


PLANE_STRESS_2D = 0
PLANE_STRAIN_2D = 1
AXISYMMETRIC_2D = 2
SOLID_3D = 3
RIGID = 1

MODEL_NAME = "contact_compression_3d_imp"
ANALYSIS_TYPE = SOLID_3D
CYLINDER_DIAMETER = 0.0254
CYLINDER_RADIUS = 0.5 * CYLINDER_DIAMETER
CYLINDER_HEIGHT = 0.03
ELEMENT_SIZE = 0.003
PLANE_SIZE = 0.04
PLANE_MIN_X = -0.02
PLANE_MIN_Y = -0.02
BOTTOM_PLANE_Z = 0.0
TOP_PLANE_Z = CYLINDER_HEIGHT
STEP_TIME = 1.0
STEP_OUTPUT_TIME = 0.05
SAVE_MODEL = True
WRITE_SOLVER_INPUT = True
RUN_SOLVER = False
KEEP_ALIVE = []


def keep_alive(obj):
    KEEP_ALIVE.append(obj)
    return obj


def output_root():
    script_dir = Path(__file__).resolve().parent
    runtime_root = script_dir.parent
    return runtime_root / MODEL_NAME


def ensure_active_model():
    return create_active_model(MODEL_NAME)


def configure_active_model():
    set_active_model_analysis_type(ANALYSIS_TYPE)
    print("Active model analysis type:", get_active_model_analysis_type())


def add_implicit_step():
    step = keep_alive(create_implicit_step(
        "Step-1",
        STEP_TIME,
        STEP_OUTPUT_TIME,
        "Picard",
        200,
        5.0e-2,
        10.0,
        10.0,
        1.0,
        0.4,
        0.1,
        1.2,
        1,
    ))
    print("Configured implicit step:", step.getName())
    return step


def add_hollomon_material():
    material = keep_alive(create_hollomon_material(
        210.0e9,
        0.30,
        7850.0,
        250.0e6,
        650.0e6,
        0.22,
        0.0,
        0.65,
    ))
    add_material_to_active_model(material)
    print("Added Hollomon material.")
    return material


def create_meshed_cylinder_part():
    part = keep_alive(create_cylinder_part(CYLINDER_RADIUS, CYLINDER_HEIGHT))
    if part is None:
        raise RuntimeError("Could not create cylinder geometry")
    part.setName("Cylinder")
    add_part_to_active_model(part)
    if not mesh_part_geometry(part, ELEMENT_SIZE):
        raise RuntimeError("Could not mesh cylinder geometry")
    return part


def assign_section(part, material_index=0):
    model = get_active_model()
    if model is None:
        raise RuntimeError("Active model is null")

    section = keep_alive(Section())
    section.setId(0)
    section.setName("Section_0")
    section.setMaterialIndex(material_index)
    section.setIntendedElementType("Tetra4")
    model.addSection(section)
    part.setSectionId(section.getId())
    print("Assigned section", section.getName(), "to part.")


def mesh_part(part):
    mesh = part.getMesh()
    if mesh is None:
        raise RuntimeError("Imported cylinder part does not have a mesh")
    print("Loaded cylinder mesh with", mesh.getNodeCount(), "nodes and", mesh.getElemCount(), "elements.")
    return mesh


def create_plane_part(name, z):
    part = keep_alive(create_rectangle_part(
        PLANE_SIZE,
        PLANE_SIZE,
        PLANE_MIN_X,
        PLANE_MIN_Y,
        z,
    ))
    if part is None:
        raise RuntimeError(f"Could not create plane part: {name}")
    part.setName(name)
    part.setType(RIGID)
    add_part_to_active_model(part)
    print(f"Added {name}: x/y from -0.02 to 0.02 at z={z}")
    return part


def save_workflow_model():
    model_path = str(output_root().with_suffix(".wfmodel"))
    save_active_model(model_path)
    print("Saved model to:", model_path)
    return model_path


def write_solver_input_file():
    input_path = str(output_root().with_suffix(".json"))
    write_active_model_input(input_path)
    print("Wrote solver input to:", input_path)
    return input_path


def launch_solver(input_path):
    pid = run_active_model_solver(input_path)
    if pid < 0:
        raise RuntimeError("Could not start solver from workflow API")
    print("Solver PID:", pid)
    return pid


if __name__ == "__main__":
    ensure_active_model()
    configure_active_model()
    add_hollomon_material()
    add_implicit_step()

    cylinder_part = create_meshed_cylinder_part()
    assign_section(cylinder_part, 0)
    mesh_part(cylinder_part)

    create_plane_part("BottomPlane", BOTTOM_PLANE_Z)
    create_plane_part("TopPlane", TOP_PLANE_Z)

    solver_input_path = None
    if SAVE_MODEL:
        save_workflow_model()
    if WRITE_SOLVER_INPUT:
        solver_input_path = write_solver_input_file()
    if RUN_SOLVER:
        if solver_input_path is None:
            solver_input_path = write_solver_input_file()
        launch_solver(solver_input_path)

    request_view_update()
