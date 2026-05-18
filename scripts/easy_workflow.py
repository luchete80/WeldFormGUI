from model import *
from pathlib import Path


PLANE_STRESS_2D = 0
PLANE_STRAIN_2D = 1
AXISYMMETRIC_2D = 2
SOLID_3D = 3

ANALYSIS_TYPE = PLANE_STRAIN_2D
STEP_TYPE = "implicit"
SAVE_MODEL = True
WRITE_SOLVER_INPUT = True
RUN_SOLVER = False
OUTPUT_STEM = "easy_workflow"


def active_model():
    return get_active_model()


def add_empty_mesh_part():
    part = create_empty_mesh_part()
    add_part_to_active_model(part)
    return part


def configure_active_model():
    set_active_model_analysis_type(ANALYSIS_TYPE)
    print("Active model analysis type:", get_active_model_analysis_type())


def add_demo_step():
    if STEP_TYPE.lower() == "implicit":
        step = create_implicit_step(
            "Step-1",
            1.0,
            0.05,
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
        )
        print(
            "Configured implicit step:",
            step.getName(),
            "simTime=",
            step.m_simTime,
            "outTime=",
            step.m_outTime,
        )
        return step
    raise RuntimeError(f"Unsupported STEP_TYPE: {STEP_TYPE}")


def add_hollomon_demo_material():
    material = create_hollomon_material(
        210000.0,
        0.30,
        7.85e-9,
        250.0,
        650.0,
        0.22,
        0.0,
        0.65,
    )
    add_material_to_active_model(material)
    return material


def add_demo_rectangle():
    part = create_rectangle_part(40.0, 20.0, 0.0, 0.0, 0.0)
    add_part_to_active_model(part)
    return part


def add_step(step_path, origin=(0.0, 0.0, 0.0)):
    part = import_step_part_at(step_path, origin[0], origin[1], origin[2])
    if part is None:
        raise RuntimeError(f"Could not import STEP: {step_path}")
    add_part_to_active_model(part)
    return part


def create_demo_mesh_sets(part):
    mesh = part.getMesh()
    if mesh is None:
        raise RuntimeError("Part does not have a mesh yet")

    node_set_id = add_node_set_from_indices(mesh, "seed_nodes", [0, 1, 2])
    box_node_set_id = add_node_set_from_box(mesh, "box_nodes", 0.0, 0.0, -1.0, 25.0, 5.0, 1.0)
    element_indices = list(range(min(mesh.getElemCount(), 4)))
    element_set_id = add_element_set_from_indices(mesh, "sample_elements", element_indices)
    print("Created node set id:", node_set_id)
    print("Created box node set id:", box_node_set_id)
    print("Created element set id:", element_set_id)


def create_support_and_loading_sets(part, width, height, tol=1.0e-6):
    mesh = part.getMesh()
    if mesh is None:
        raise RuntimeError("Part does not have a mesh yet")

    bottom_set_id = add_node_set_from_box(
        mesh,
        "bottom_nodes",
        -tol,
        -tol,
        -1.0,
        width + tol,
        tol,
        1.0,
    )
    top_set_id = add_node_set_from_box(
        mesh,
        "top_nodes",
        -tol,
        height - tol,
        -1.0,
        width + tol,
        height + tol,
        1.0,
    )
    return bottom_set_id, top_set_id


def add_demo_boundary_conditions(bottom_set_id, top_set_id):
    add_fixed_bc_to_node_set(bottom_set_id, True, True, False)
    add_velocity_bc_to_node_set(top_set_id, 0.0, -1.0, 0.0, False, True, False)


def output_root():
    script_dir = Path(__file__).resolve().parent
    runtime_root = script_dir.parent
    return runtime_root / OUTPUT_STEM


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
    configure_active_model()
    mesh_part = add_empty_mesh_part()
    mesh = mesh_part.getMesh()
    width = 20.0
    height = 10.0
    element_size = 1.0
    mesh.addPlane(0.0, 0.0, width, height, element_size)

    print(
        f"Empty mesh part added and populated from Python with "
        f"{mesh.getNodeCount()} nodes and {mesh.getElemCount()} elements."
    )

    add_hollomon_demo_material()
    print("Added demo Hollomon material to active model.")
    add_demo_step()

    create_demo_mesh_sets(mesh_part)
    bottom_set_id, top_set_id = create_support_and_loading_sets(mesh_part, width, height)
    add_demo_boundary_conditions(bottom_set_id, top_set_id)
    print("Added fixed BC on bottom nodes and velocity BC on top nodes.")

    rectangle_part = add_demo_rectangle()
    print("Rectangle geometry part added. Mesh generation can be wrapped next.")

    saved_model_path = None
    solver_input_path = None
    if SAVE_MODEL:
        saved_model_path = save_workflow_model()
    if WRITE_SOLVER_INPUT:
        solver_input_path = write_solver_input_file()
    if RUN_SOLVER:
        if solver_input_path is None:
            solver_input_path = write_solver_input_file()
        launch_solver(solver_input_path)

    request_view_update()
