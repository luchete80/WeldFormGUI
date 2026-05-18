from model import *


def active_model():
    return get_active_model()


def add_empty_mesh_part():
    part = create_empty_mesh_part()
    add_part_to_active_model(part)
    return part


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


if __name__ == "__main__":
    mesh_part = add_empty_mesh_part()
    mesh = mesh_part.getMesh()
    nx = 6
    ny = 4
    width = 20.0
    height = 10.0

    for j in range(ny + 1):
        y = height * float(j) / float(ny)
        for i in range(nx + 1):
            x = width * float(i) / float(nx)
            mesh.addNode(x, y, 0.0)

    for j in range(ny):
        for i in range(nx):
            n0 = j * (nx + 1) + i
            n1 = n0 + 1
            n2 = n1 + (nx + 1)
            n3 = n0 + (nx + 1)
            mesh.addQuad(n0, n1, n2, n3)

    print(f"Empty mesh part added and populated from Python with {mesh.getNodeCount()} nodes and {mesh.getElemCount()} elements.")

    create_demo_mesh_sets(mesh_part)

    rectangle_part = add_demo_rectangle()
    print("Rectangle geometry part added. Mesh generation can be wrapped next.")

    request_view_update()
