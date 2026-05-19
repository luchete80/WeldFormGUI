from pathlib import Path

from model import *


PLANE_STRESS_2D = 0
PLANE_STRAIN_2D = 1
AXISYMMETRIC_2D = 2
SOLID_3D = 3
DEFORMABLE = 0
RIGID = 1

ANALYSIS_TYPE = AXISYMMETRIC_2D
PART_0_MESH_SIZE = 4.0e-4
PART_2_MESH_SIZE = 1.0e-3
MODEL_NAME = "extrusion_workflow"
STEP_TIME = 50.0
CONTACT_PENALTY_FACTOR = 500.0
CONTACT_GAP_PENALTY = 5.0


def ensure_active_model():
    return create_active_model(MODEL_NAME)


def configure_active_model():
    set_active_model_analysis_type(ANALYSIS_TYPE)
    set_active_model_contact_properties(CONTACT_PENALTY_FACTOR, CONTACT_GAP_PENALTY)


def add_demo_step():
    return create_implicit_step(
        "Step-1",
        STEP_TIME,
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


def scripts_root():
    if "__file__" in globals():
        root = Path(__file__).resolve().parents[1]
        print(f"[extrusion_workflow] Using script file location: {root}")
        return root

    fallback = Path.cwd() / "scripts"
    if fallback.exists():
        print(f"[extrusion_workflow] __file__ is not available, using scripts root from cwd: {fallback}")
        return fallback

    print(f"[extrusion_workflow] __file__ is not available, fallback scripts folder not found. Using cwd: {Path.cwd()}")
    return Path.cwd()


def asset_path(filename):
    path = scripts_root() / "extrusion_script" / filename
    print(f"[extrusion_workflow] Resolving asset '{filename}' to: {path}")
    return path


def import_part_step(filename):
    part = import_step_part(str(asset_path(filename)))
    if part is None:
        raise RuntimeError(f"No se pudo importar el STEP: {filename}")
    add_part_to_active_model(part)
    return part


def import_part_bdf(filename):
    part = import_bdf_part(str(asset_path(filename)))
    if part is None:
        raise RuntimeError(f"No se pudo importar el BDF: {filename}")
    add_part_to_active_model(part)
    return part


def move_part(part, dx, dy, dz):
    translate_part(part, dx, dy, dz)


if __name__ == "__main__":
    ensure_active_model()
    configure_active_model()
    add_hollomon_demo_material()
    add_demo_step()

    part_0 = import_part_step("demo_part_0.step")
    part_1 = import_part_bdf("demo_part_1.bdf")
    part_2 = import_part_step("demo_part_2.step")
    part_3 = import_part_bdf("matriz_sup.bdf")

    part_1.setType(RIGID)
    part_2.setType(RIGID)
    part_3.setType(RIGID)

    add_velocity_bc_to_part(part_1, 0.0, 5.0e-5, 0.0, False, True, False)

    #move_part(part_0, 5.0, 0.0, 0.0)
    #move_part(part_1, 65.0, 0.0, 0.0)
    #move_part(part_2, 125.0, 0.0, 0.0)
    #move_part(part_3, 185.0, 0.0, 0.0)

    if not mesh_part_geometry(part_0, PART_0_MESH_SIZE):
        raise RuntimeError("No se pudo mallar la parte 0 con tamaño de elemento 0.4")

    if not mesh_part_geometry(part_2, PART_2_MESH_SIZE):
        raise RuntimeError("No se pudo mallar la parte 0 con tamaño de elemento 0.4")
        
    request_view_update()
