from pathlib import Path

from model import *


PLANE_STRESS_2D = 0
PLANE_STRAIN_2D = 1
AXISYMMETRIC_2D = 2
SOLID_3D = 3

ANALYSIS_TYPE = PLANE_STRAIN_2D
PART_0_MESH_SIZE = 0.4


def configure_active_model():
    set_active_model_analysis_type(ANALYSIS_TYPE)


def add_demo_step():
    return create_implicit_step(
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


def script_dir():
    return Path(__file__).resolve().parent


def import_part_step(filename):
    part = import_step_part(str(script_dir() / filename))
    if part is None:
        raise RuntimeError(f"No se pudo importar el STEP: {filename}")
    add_part_to_active_model(part)
    return part


def import_part_bdf(filename):
    part = import_bdf_part(str(script_dir() / filename))
    if part is None:
        raise RuntimeError(f"No se pudo importar el BDF: {filename}")
    add_part_to_active_model(part)
    return part


def move_part(part, dx, dy, dz):
    translate_part(part, dx, dy, dz)


if __name__ == "__main__":
    configure_active_model()
    add_hollomon_demo_material()
    add_demo_step()

    part_0 = import_part_step("demo_part_0.step")
    part_1 = import_part_bdf("demo_part_1.bdf")
    part_2 = import_part_step("demo_part_2.step")
    part_3 = import_part_step("demo_part_3.step")

    move_part(part_0, 5.0, 0.0, 0.0)
    move_part(part_1, 65.0, 0.0, 0.0)
    move_part(part_2, 125.0, 0.0, 0.0)
    move_part(part_3, 185.0, 0.0, 0.0)

    if not mesh_part_geometry(part_0, PART_0_MESH_SIZE):
        raise RuntimeError("No se pudo mallar la parte 0 con tamaño de elemento 0.4")

    request_view_update()
