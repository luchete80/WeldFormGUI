from model import *
from pathlib import Path


PLANE_STRESS_2D = 0
PLANE_STRAIN_2D = 1
AXISYMMETRIC_2D = 2
SOLID_3D = 3

ANALYSIS_TYPE = PLANE_STRAIN_2D
MESH_SIZE = 1.0
IMPORT_ORIGIN = (0.0, 0.0, 0.0)
SAVE_MODEL = True
WRITE_SOLVER_INPUT = False
RUN_SOLVER = False
OUTPUT_STEM = "easy_import_n_mesh"


def runtime_root():
    return Path(__file__).resolve().parent.parent


def repo_root():
    return runtime_root().parent / "WeldFormGUI"


STEP_FILE = repo_root() / "examples" / "demo_part_0.step.step"


def active_model():
    return get_active_model()


def configure_active_model():
    set_active_model_analysis_type(ANALYSIS_TYPE)
    active_model().setElementSize(MESH_SIZE)
    print("Active model analysis type:", get_active_model_analysis_type())
    print("Target element size:", active_model().getElementSize())


def import_and_mesh_part():
    step_path = Path(STEP_FILE).expanduser().resolve()
    if not step_path.exists():
        raise RuntimeError(f"STEP file not found: {step_path}")

    part = import_and_mesh_step_part(
        str(step_path),
        MESH_SIZE,
        IMPORT_ORIGIN[0],
        IMPORT_ORIGIN[1],
        IMPORT_ORIGIN[2],
    )
    if part is None or part.getMesh() is None:
        raise RuntimeError(
            "Could not import and mesh STEP part. "
            "For 2D this uses mesh-adapt; for 3D it uses the internal gmsh path."
        )

    mesh = part.getMesh()
    print("Imported STEP:", step_path)
    print(
        "Meshed part with",
        mesh.getNodeCount(),
        "nodes and",
        mesh.getElemCount(),
        "elements.",
    )
    return part


def output_root():
    return runtime_root() / OUTPUT_STEM


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
    import_and_mesh_part()

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
