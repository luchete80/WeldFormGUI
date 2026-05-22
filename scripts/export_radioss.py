from model import *

import os


DEFAULT_OUTPUT_ROOT = None


PLANE_STRESS_2D = 0
PLANE_STRAIN_2D = 1
AXISYMMETRIC_2D = 2
SOLID_3D = 3
APPLY_TO_PART = 0
APPLY_TO_NODE_SET = 1

VELOCITY_BC = 0
DISPLACEMENT_BC = 1
SYMMETRY_BC = 2

NONE = 0
BILINEAR = 1
HOLLOMON = 2
JOHNSON_COOK = 3


def _active_model():
    try:
        return get_active_model()
    except NameError:
        return getApp().getActiveModel()


def _validate_model_for_export(profile_name="General"):
    try:
        report = run_active_model_check_report(profile_name)
        if active_model_check_has_errors(profile_name):
            raise RuntimeError(report.strip())
    except NameError:
        pass


def _safe_name(name, fallback):
    if name:
        cleaned = "".join(ch if ch.isalnum() or ch == "_" else "_" for ch in name.strip())
        if cleaned:
            return cleaned
    return fallback


def _safe_part_name(part, fallback_index):
    return _safe_name(part.getName(), f"Part_{fallback_index}")


def _safe_set_name(node_set, fallback_index):
    return _safe_name(node_set.getLabel(), f"NodeSet_{fallback_index}")


def _default_output_root(model):
    model_path = ""
    try:
        model_path = model.getFilePath()
    except Exception:
        model_path = ""

    if model_path:
        stem, _ = os.path.splitext(model_path)
        return os.path.abspath(stem)

    model_name = model.getName() or "model"
    stem, _ = os.path.splitext(model_name)
    if not stem:
        stem = "model"
    return os.path.abspath(stem)


def _starter_path(root_path):
    return root_path + "_0000.rad"


def _engine_path(root_path):
    return root_path + "_0001.rad"


def _write_int_field(value, width=10):
    return f"{int(value):>{width}d}"


def _write_float_field(value, width=20, decimals=10):
    return f"{float(value):>{width}.{decimals}e}"


def _write_10col_line(values):
    line = ""
    for value in values:
        if isinstance(value, int):
            line += _write_int_field(value, 10)
        elif isinstance(value, float):
            line += _write_float_field(value, 20, 10)
        else:
            line += f"{str(value):>10}"
    return line.rstrip() + "\n"


def _write_comment(out, text):
    out.write(f"# {text}\n")


def _node_label_map(mesh):
    mapping = {}
    for i in range(mesh.getNodeCount()):
        node = mesh.getNode(i)
        mapping[node.getId()] = node.getId()
    return mapping


def _infer_mesh_dimension(mesh):
    declared_dim = mesh.getDim()
    if declared_dim == 2:
        return 2

    if mesh.getNodeCount() <= 0 or mesh.getElemCount() <= 0:
        return declared_dim

    planar_z = True
    for node_index in range(mesh.getNodeCount()):
        node = mesh.getNode(node_index)
        if abs(node.getPos(2)) > 1.0e-12:
            planar_z = False
            break

    if not planar_z:
        return declared_dim

    supported_2d_topology = True
    for elem_index in range(mesh.getElemCount()):
        elem = mesh.getElem(elem_index)
        if elem.getNodeCount() not in (2, 3, 4):
            supported_2d_topology = False
            break

    if supported_2d_topology:
        return 2

    return declared_dim


def _analysis_type(model):
    try:
        return model.getAnalysisType()
    except Exception:
        return SOLID_3D


def _use_yz_projection(model):
    return _analysis_type(model) in (PLANE_STRAIN_2D, AXISYMMETRIC_2D)


def _xyz_to_yz_components(model, components):
    if not _use_yz_projection(model):
        return [float(component) for component in components]
    x, y, z = [float(component) for component in components]
    return [z, x, y]


def _xyz_to_yz_mask(model, mask):
    if not _use_yz_projection(model):
        return list(mask)
    x, y, z = [bool(component) for component in mask]
    return [z, x, y]


def _projected_node_coords(model, node):
    return tuple(_xyz_to_yz_components(
        model,
        [node.getPos(0), node.getPos(1), node.getPos(2)]
    ))


def _element_keyword(mesh_dim, elem):
    node_count = elem.getNodeCount()
    if mesh_dim == 2:
        if node_count == 4:
            return "/QUAD"
        if node_count == 3:
            return "/TRIA"
    else:
        if node_count == 8:
            return "/BRICK"
        if node_count == 4:
            return "/TETRA4"
    raise ValueError(
        f"Unsupported Radioss element topology: meshDim={mesh_dim}, "
        f"nodesPerElement={node_count}"
    )


def _node_by_id(mesh, node_id):
    for node_index in range(mesh.getNodeCount()):
        node = mesh.getNode(node_index)
        if node is not None and int(node.getId()) == int(node_id):
            return node
    return None


def _signed_area_2d(model, mesh, node_ids):
    points = []
    for node_id in node_ids:
        node = _node_by_id(mesh, node_id)
        if node is None:
            return 0.0
        px, py, _ = _projected_node_coords(model, node)
        points.append((px, py))

    area2 = 0.0
    for index in range(len(points)):
        x1, y1 = points[index]
        x2, y2 = points[(index + 1) % len(points)]
        area2 += x1 * y2 - x2 * y1
    return 0.5 * area2


def _radioss_connectivity(model, mesh, mesh_dim, elem):
    node_ids = [elem.getNodeId(local_node) for local_node in range(elem.getNodeCount())]
    if mesh_dim != 2:
        return node_ids

    area = _signed_area_2d(model, mesh, node_ids)
    if area < 0.0:
        if len(node_ids) == 3:
            return [node_ids[0], node_ids[2], node_ids[1]]
        if len(node_ids) == 4:
            return [node_ids[0], node_ids[3], node_ids[2], node_ids[1]]
    return node_ids


def _element_type_name(mesh_dim, elem):
    node_count = elem.getNodeCount()
    if mesh_dim == 2:
        if node_count == 2:
            return "Line2"
        if node_count == 3:
            return "Tria3"
        if node_count == 4:
            return "Quad4"
    else:
        if node_count == 3:
            return "Tria3"
        if node_count == 4:
            return "Tetra4"
        if node_count == 8:
            return "Hexa8"
    return f"Unknown({node_count})"


def _is_bulk_element(mesh_dim, elem):
    node_count = elem.getNodeCount()
    if mesh_dim == 2:
        return node_count in (3, 4)
    return node_count in (4, 8)


def _is_boundary_element(mesh_dim, elem):
    node_count = elem.getNodeCount()
    if mesh_dim == 2:
        return node_count == 2
    return node_count == 3


def _part_type_name(part):
    try:
        return "Rigid" if part.getType() == 1 else "Deformable"
    except Exception:
        return "Unknown"


def _collect_exportable_elements(part, mesh, mesh_dim):
    deformable = True
    try:
        deformable = part.getType() == 0
    except Exception:
        deformable = True

    exportable = []
    skipped_boundary = []
    skipped_unsupported = []

    for elem_index in range(mesh.getElemCount()):
        elem = mesh.getElem(elem_index)
        if elem is None:
            continue

        if deformable and _is_bulk_element(mesh_dim, elem):
            exportable.append((elem_index, elem))
            continue

        if (not deformable) and _is_boundary_element(mesh_dim, elem):
            skipped_boundary.append((elem_index, elem))
            continue

        skipped_unsupported.append((elem_index, elem))

    return exportable, skipped_boundary, skipped_unsupported


def _material_id(index):
    return index + 1


def _property_id(index):
    return index + 1


def _part_virtual_thickness(mesh_dim):
    return 0.0


def _write_header_and_begin(out, run_name, title):
    out.write("#RADIOSS STARTER\n")
    _write_comment(out, "WeldFormGUI export to OpenRadioss starter")
    out.write("/BEGIN\n")
    out.write(f"{run_name}\n")
    out.write(f"{2023:>10}{0:>10}\n")
    out.write(f"{'kg':>20}{'mm':>20}{'ms':>20}\n")
    out.write(f"{'kg':>20}{'mm':>20}{'ms':>20}\n")
    out.write("/TITLE\n")
    out.write(f"{title}\n")


def _radioss_analysis_flag(model):
    analysis_type = _analysis_type(model)

    if analysis_type == AXISYMMETRIC_2D:
        return 1
    if analysis_type in (PLANE_STRESS_2D, PLANE_STRAIN_2D):
        return 2
    return 0


def _write_analysis_block(model, out):
    out.write("/ANALY\n")
    out.write(f"{_radioss_analysis_flag(model):>10}\n")


def _write_nodes(model, out):
    out.write("/NODE\n")
    for part_index in range(model.getPartCount()):
        part = model.getPart(part_index)
        if part is None or part.getMesh() is None:
            continue
        mesh = part.getMesh()
        for node_index in range(mesh.getNodeCount()):
            node = mesh.getNode(node_index)
            px, py, pz = _projected_node_coords(model, node)
            out.write(
                _write_int_field(node.getId(), 10) +
                _write_float_field(px, 20, 10) +
                _write_float_field(py, 20, 10) +
                _write_float_field(pz, 20, 10) + "\n"
            )


def _max_node_id(model):
    max_id = 0
    for part_index in range(model.getPartCount()):
        part = model.getPart(part_index)
        if part is None or part.getMesh() is None:
            continue
        mesh = part.getMesh()
        for node_index in range(mesh.getNodeCount()):
            node = mesh.getNode(node_index)
            if node is not None:
                max_id = max(max_id, int(node.getId()))
    return max_id


def _boundary_part_centroid(model, mesh):
    count = 0
    sx = 0.0
    sy = 0.0
    sz = 0.0

    for node_index in range(mesh.getNodeCount()):
        node = mesh.getNode(node_index)
        if node is None:
            continue
        px, py, pz = _projected_node_coords(model, node)
        sx += px
        sy += py
        sz += pz
        count += 1

    if count <= 0:
        return 0.0, 0.0, 0.0
    return sx / count, sy / count, sz / count


def _build_rigid_part_records(model):
    records = []
    next_ref_node_id = _max_node_id(model) + 1

    for part_index in range(model.getPartCount()):
        part = model.getPart(part_index)
        if part is None or part.getMesh() is None:
            continue

        if part.getType() != 1:
            continue

        mesh = part.getMesh()
        mesh_dim = _infer_mesh_dimension(mesh)
        exportable_elements, skipped_boundary_elements, skipped_unsupported_elements = (
            _collect_exportable_elements(part, mesh, mesh_dim)
        )

        if exportable_elements:
            continue
        if not skipped_boundary_elements:
            continue

        cx, cy, cz = _boundary_part_centroid(model, mesh)
        records.append(
            {
                "part": part,
                "part_index": part_index,
                "part_name": _safe_part_name(part, part_index),
                "mesh": mesh,
                "mesh_dim": mesh_dim,
                "boundary_elements": skipped_boundary_elements,
                "unsupported_elements": skipped_unsupported_elements,
                "reference_node_id": next_ref_node_id,
                "reference_point": (cx, cy, cz),
            }
        )
        next_ref_node_id += 1

    return records


def _write_rigid_reference_nodes(rigid_part_records, out):
    for record in rigid_part_records:
        x, y, z = record["reference_point"]
        out.write(
            _write_int_field(record["reference_node_id"], 10) +
            _write_float_field(x, 20, 10) +
            _write_float_field(y, 20, 10) +
            _write_float_field(z, 20, 10) + "\n"
        )


def _write_materials(model, out):
    material_count = model.getMaterialCount()
    if material_count <= 0:
        return {}

    material_ids = {}
    for material_index in range(material_count):
        material = model.getMaterial(material_index)
        if material is None:
            continue
        mat_id = _material_id(material_index)
        material_ids[material_index] = mat_id
        mat_name = _safe_name(material.getName() if hasattr(material, 'getName') else '', f'Material_{mat_id}')

        plastic_type = NONE
        try:
            if material.isPlastic() and material.getPlastic() is not None:
                plastic_type = material.getPlastic().getType()
        except Exception:
            plastic_type = getattr(material, "Material_model", NONE)

        if plastic_type in (HOLLOMON, JOHNSON_COOK):
            density = float(material.getDensityConstant())
            young = float(material.Elastic().E())
            poisson = float(material.Elastic().nu())
            yield_stress = float(getattr(material, "yieldStress0", 0.0))

            if plastic_type == HOLLOMON:
                jc_a = yield_stress
                jc_b = float(getattr(material, "K", 0.0))
                jc_n = float(getattr(material, "m", 0.0))
                jc_c = 0.0
                jc_eps0 = max(float(getattr(material, "epsdot0", 1.0)), 1.0e-12)
                jc_m = 0.0
                jc_tm = float(getattr(material, "T_m", 0.0))
                jc_tr = float(getattr(material, "T_t", 273.0))
            else:
                jc_a = yield_stress
                jc_b = float(getattr(material, "B", 0.0))
                jc_n = float(getattr(material, "n", 0.0))
                jc_c = float(getattr(material, "C", 0.0))
                jc_eps0 = max(float(getattr(material, "eps_0", 1.0)), 1.0e-12)
                jc_m = float(getattr(material, "m", 0.0))
                jc_tm = float(getattr(material, "T_m", 0.0))
                jc_tr = float(getattr(material, "T_t", 273.0))

            if jc_tm <= 0.0:
                jc_tm = max(float(getattr(material, "T_max", 0.0)), jc_tr)
            if jc_tr <= 0.0:
                jc_tr = 273.0

            rho_cp = density * max(float(getattr(material, "cp_T", 0.0)), 0.0)

            out.write(f"/MAT/PLAS_JOHNS/{mat_id}\n")
            out.write(f"{mat_name}\n")
            out.write(_write_float_field(density, 20, 10) + "\n")
            out.write(
                _write_float_field(young, 20, 10) +
                _write_float_field(poisson, 20, 10) +
                _write_int_field(0, 10) +
                _write_float_field(0.0, 20, 10) +
                _write_float_field(0.0, 20, 10) + "\n"
            )
            out.write(
                _write_float_field(jc_a, 20, 10) +
                _write_float_field(jc_b, 20, 10) +
                _write_float_field(jc_n, 20, 10) +
                _write_float_field(1.0e30, 20, 10) +
                _write_float_field(1.0e30, 20, 10) + "\n"
            )
            out.write(
                _write_float_field(jc_c, 20, 10) +
                _write_float_field(jc_eps0, 20, 10) +
                _write_int_field(1, 10) +
                _write_int_field(1, 10) +
                _write_float_field(1.0e30, 20, 10) +
                _write_int_field(0, 10) + "\n"
            )
            out.write(
                _write_float_field(jc_m, 20, 10) +
                _write_float_field(jc_tm, 20, 10) +
                _write_float_field(rho_cp, 20, 10) +
                _write_float_field(jc_tr, 20, 10) + "\n"
            )
            continue

        out.write(f"/MAT/LAW1/{mat_id}\n")
        out.write(f"{mat_name}\n")
        out.write(_write_float_field(material.getDensityConstant(), 20, 10) + "\n")
        out.write(
            _write_float_field(material.Elastic().E(), 20, 10) +
            _write_float_field(material.Elastic().nu(), 20, 10) + "\n"
        )
    return material_ids


def _write_properties_and_parts(model, out, material_ids):
    part_records = []
    for part_index in range(model.getPartCount()):
        part = model.getPart(part_index)
        if part is None or part.getMesh() is None:
            continue

        mesh = part.getMesh()
        mesh_dim = _infer_mesh_dimension(mesh)
        exportable_elements, skipped_boundary_elements, skipped_unsupported_elements = (
            _collect_exportable_elements(part, mesh, mesh_dim)
        )
        part_id = part_index + 1
        prop_id = _property_id(part_index)
        mat_id = material_ids.get(0, 1)
        part_name = _safe_part_name(part, part_index)
        part_type_name = _part_type_name(part)

        if not exportable_elements:
            if skipped_boundary_elements:
                out.write(
                    f"# Skipping part {part_name}: {part_type_name} part contains only boundary elements "
                    f"({', '.join(_element_type_name(mesh_dim, elem) for _, elem in skipped_boundary_elements[:3])}) "
                    "and is handled separately as a rigid body candidate.\n"
                )
            else:
                out.write(
                    f"# Skipping part {part_name}: no exportable bulk elements for Radioss.\n"
                )
            continue

        out.write(f"/PROP/TYPE14/{prop_id}\n")
        out.write(f"{part_name}_solid_prop\n")
        out.write(
            _write_int_field(1, 10) +
            _write_int_field(-1, 10) +
            _write_int_field(-1, 10) +
            _write_int_field(-1, 10) +
            _write_int_field(0, 10) +
            _write_int_field(0, 10) +
            _write_int_field(0, 10) +
            _write_int_field(0, 10) +
            _write_float_field(0.0, 20, 10) + "\n"
        )
        out.write(
            _write_float_field(1.1, 20, 10) +
            _write_float_field(0.05, 20, 10) +
            _write_float_field(0.1, 20, 10) +
            _write_float_field(0.0, 20, 10) +
            _write_float_field(0.0, 20, 10) + "\n"
        )
        out.write(
            _write_float_field(0.0, 20, 10) +
            _write_float_field(0.0, 20, 10) +
            _write_float_field(0.0, 20, 10) +
            _write_float_field(0.0, 20, 10) +
            _write_float_field(0.0, 20, 10) + "\n"
        )
        out.write(
            _write_int_field(0, 10) +
            _write_int_field(0, 10) +
            _write_int_field(0, 10) + "\n"
        )

        out.write(f"/PART/{part_id}\n")
        out.write(f"{part_name}\n")
        out.write(
            _write_int_field(prop_id, 10) +
            _write_int_field(mat_id, 10) +
            _write_int_field(0, 10) +
            _write_float_field(_part_virtual_thickness(mesh_dim), 20, 10) + "\n"
        )

        part_records.append(
            {
                "part": part,
                "target_part_id": part.getId(),
                "part_id": part_id,
                "part_name": part_name,
                "mesh": mesh,
                "mesh_dim": mesh_dim,
                "exportable_elements": exportable_elements,
                "skipped_boundary_elements": skipped_boundary_elements,
                "skipped_unsupported_elements": skipped_unsupported_elements,
            }
        )

    return part_records


def _write_elements(model, part_records, out):
    for record in part_records:
        mesh = record["mesh"]
        part_id = record["part_id"]
        mesh_dim = record["mesh_dim"]
        exportable_elements = record.get("exportable_elements", [])
        skipped_boundary_elements = record.get("skipped_boundary_elements", [])
        skipped_unsupported_elements = record.get("skipped_unsupported_elements", [])

        grouped = {}
        for elem_index, elem in exportable_elements:
            keyword = _element_keyword(mesh_dim, elem)
            grouped.setdefault(keyword, []).append((elem_index, elem))

        if skipped_boundary_elements:
            out.write(
                f"# Part {record['part_name']}: skipped {len(skipped_boundary_elements)} boundary element(s) "
                "for current Radioss bulk export.\n"
            )

        if skipped_unsupported_elements:
            out.write(
                f"# Part {record['part_name']}: skipped {len(skipped_unsupported_elements)} unsupported element(s): "
                f"{', '.join(sorted(set(_element_type_name(mesh_dim, elem) for _, elem in skipped_unsupported_elements)))}\n"
            )

        for keyword, elems in grouped.items():
            out.write(f"{keyword}/{part_id}\n")
            for elem_index, elem in elems:
                fields = [elem_index + 1]
                for node_id in _radioss_connectivity(model, mesh, mesh_dim, elem):
                    fields.append(node_id)
                if keyword == "/SH3N":
                    fields += [0.0, 1.0]
                elif keyword == "/SHELL":
                    fields += [1.0]
                out.write(_write_10col_line(fields))


def _write_node_groups(part_records, out):
    group_records = {
        "part_groups": {},
        "set_groups": {},
        "rigid_ref_groups": {},
    }

    next_group_id = 1

    for record in part_records:
        mesh = record["mesh"]
        part_group_id = next_group_id
        next_group_id += 1
        group_records["part_groups"][record["target_part_id"]] = part_group_id

        out.write(f"/GRNOD/NODE/{part_group_id}\n")
        out.write(f"{record['part_name']}_all_nodes\n")
        node_ids = [mesh.getNode(i).getId() for i in range(mesh.getNodeCount())]
        for start in range(0, len(node_ids), 10):
            out.write("".join(_write_int_field(node_id, 10) for node_id in node_ids[start:start + 10]) + "\n")

        for set_index in range(mesh.getNodeSetCount()):
            node_set = mesh.getNodeSet(set_index)
            set_group_id = next_group_id
            next_group_id += 1
            group_records["set_groups"][node_set.getId()] = {
                "group_id": set_group_id,
                "part_id": record["part_id"],
                "name": _safe_set_name(node_set, set_index),
            }

            out.write(f"/GRNOD/NODE/{set_group_id}\n")
            out.write(f"{_safe_set_name(node_set, set_index)}\n")
            node_ids = [node_set.getItem(i).getId() for i in range(node_set.getItemCount())]
            for start in range(0, len(node_ids), 10):
                out.write("".join(_write_int_field(node_id, 10) for node_id in node_ids[start:start + 10]) + "\n")

    return group_records


def _write_rigid_body_groups(rigid_part_records, out, group_records, start_group_id):
    next_group_id = start_group_id

    for record in rigid_part_records:
        part_group_id = next_group_id
        next_group_id += 1
        group_records["part_groups"][record["part"].getId()] = part_group_id

        out.write(f"/GRNOD/NODE/{part_group_id}\n")
        out.write(f"{record['part_name']}_rigid_nodes\n")
        node_ids = [record["mesh"].getNode(i).getId() for i in range(record["mesh"].getNodeCount())]
        for start in range(0, len(node_ids), 10):
            out.write("".join(_write_int_field(node_id, 10) for node_id in node_ids[start:start + 10]) + "\n")

        ref_group_id = next_group_id
        next_group_id += 1
        group_records["rigid_ref_groups"][record["part"].getId()] = ref_group_id

        out.write(f"/GRNOD/NODE/{ref_group_id}\n")
        out.write(f"{record['part_name']}_ref_node\n")
        out.write(_write_int_field(record["reference_node_id"], 10) + "\n")

    return next_group_id


def _write_rigid_bodies(rigid_part_records, group_records, out):
    if not rigid_part_records:
        return

        out.write("# Rigid body definitions generated for boundary-only rigid parts.\n")
    out.write("# Boundary line element export card is still pending; RBODY uses the rigid part node group and reference node.\n")

    unit_id = 1
    for rigid_index, record in enumerate(rigid_part_records, start=1):
        part_id = record["part"].getId()
        sens_group_id = group_records["part_groups"].get(part_id)
        if sens_group_id is None:
            out.write(f"# Skipping RBODY for {record['part_name']}: missing node group.\n")
            continue

        out.write(f"/RBODY/{rigid_index}/{unit_id}\n")
        out.write(f"{record['part_name']}_rbody\n")
        out.write(
            _write_int_field(record["reference_node_id"], 10) +
            _write_int_field(sens_group_id, 10) +
            _write_int_field(0, 10) +
            _write_int_field(0, 10) +
            _write_float_field(0.0, 20, 10) +
            _write_int_field(0, 10) +
            _write_int_field(0, 10) +
            _write_int_field(0, 10) +
            _write_int_field(0, 10) + "\n"
        )
        out.write(
            _write_float_field(0.0, 20, 10) +
            _write_float_field(0.0, 20, 10) +
            _write_float_field(0.0, 20, 10) + "\n"
        )
        out.write(
            _write_float_field(0.0, 20, 10) +
            _write_float_field(0.0, 20, 10) +
            _write_float_field(0.0, 20, 10) + "\n"
        )
        out.write(
            _write_int_field(0, 10) +
            _write_int_field(0, 10) + "\n"
        )


def _bc_target_group_id(bc, group_records):
    if bc.getApplyTo() == APPLY_TO_PART:
        if bc.getTargetId() in group_records.get("rigid_ref_groups", {}):
            return group_records["rigid_ref_groups"].get(bc.getTargetId())
        return group_records["part_groups"].get(bc.getTargetId())
    if bc.getApplyTo() == APPLY_TO_NODE_SET:
        record = group_records["set_groups"].get(bc.getTargetId())
        if record is not None:
            return record["group_id"]
    return None


def _bc_dof_mask(bc):
    return [bc.getDofMaskX(), bc.getDofMaskY(), bc.getDofMaskZ()]


def _bcs_code_from_mask(mask):
    return f"{int(mask[0])}{int(mask[1])}{int(mask[2])}111"


def _direction_labels_from_mask(mask):
    labels = []
    if mask[0]:
        labels.append("X")
    if mask[1]:
        labels.append("Y")
    if mask[2]:
        labels.append("Z")
    return labels


def _write_function(out, func_id, title, xy_points):
    out.write(f"/FUNCT/{func_id}\n")
    out.write(f"{title}\n")
    for x_value, y_value in xy_points:
        out.write(
            _write_float_field(x_value, 20, 10) +
            _write_float_field(y_value, 20, 10) + "\n"
        )


def _scaled_function_points(bc, component_value, stop_time, ramp_from_zero):
    try:
        uses_amplitude = bc.usesAmplitude()
    except Exception:
        uses_amplitude = False

    if uses_amplitude:
        try:
            time_values = list(bc.getAmplitudeTime())
            amplitude_values = list(bc.getAmplitudeValue())
            amplitude_factor = float(bc.getAmplitudeFactor())
        except Exception:
            time_values = []
            amplitude_values = []
            amplitude_factor = 1.0

        point_count = min(len(time_values), len(amplitude_values))
        if point_count >= 2:
            points = []
            for i in range(point_count):
                points.append((float(time_values[i]),
                               float(component_value) * amplitude_factor * float(amplitude_values[i])))
            points.sort(key=lambda point: point[0])
            return points

    if ramp_from_zero:
        return [(0.0, 0.0), (float(stop_time), float(component_value))]
    return [(0.0, float(component_value)), (float(stop_time), float(component_value))]


def _write_boundary_conditions(model, out, group_records, stop_time):
    function_id = 1
    imposed_id = 1
    bcs_id = 1
    directions = ["X", "Y", "Z"]

    for bc_index in range(model.getBCCount()):
        bc = model.getBC(bc_index)
        if bc is None:
            continue

        group_id = _bc_target_group_id(bc, group_records)
        if group_id is None:
            out.write(f"# Skipping BC {bc_index}: missing node group target\n")
            continue

        mask = _xyz_to_yz_mask(model, _bc_dof_mask(bc))
        values = _xyz_to_yz_components(model, [bc.getValueX(), bc.getValueY(), bc.getValueZ()])
        bc_type = bc.getType()

        if bc_type == SYMMETRY_BC:
            normal = [
                abs(component)
                for component in _xyz_to_yz_components(
                    model,
                    [bc.getNormalX(), bc.getNormalY(), bc.getNormalZ()]
                )
            ]
            if normal[0] > 0.5:
                mask = [True, False, False]
            elif normal[1] > 0.5:
                mask = [False, True, False]
            elif normal[2] > 0.5:
                mask = [False, False, True]
            else:
                out.write(f"# Skipping BC {bc_index}: unsupported symmetry normal\n")
                continue
            out.write(f"/BCS/{bcs_id}\n")
            out.write(f"symmetry_{bc_index}\n")
            out.write(
                f"{_bcs_code_from_mask(mask):>10}" +
                _write_int_field(0, 10) +
                _write_int_field(group_id, 10) + "\n"
            )
            bcs_id += 1
            continue

        if bc_type == DISPLACEMENT_BC:
            for axis, active in enumerate(mask):
                if not active:
                    continue
                value = values[axis]
                direction = directions[axis]
                function_points = _scaled_function_points(bc, value, stop_time, ramp_from_zero=True)
                if not function_points:
                    continue
                _write_function(out, function_id, f"disp_bc_{bc_index}_{direction}", function_points)
                out.write(f"/IMPDISP/{imposed_id}\n")
                out.write(f"disp_bc_{bc_index}_{axis}\n")
                out.write(
                    _write_int_field(function_id, 10) +
                    f"{direction:>10}" +
                    _write_int_field(0, 10) +
                    _write_int_field(0, 10) +
                    _write_int_field(group_id, 10) +
                    f"{'':>10}" +
                    _write_int_field(0, 10) + "\n"
                )
                out.write(
                    _write_float_field(1.0, 20, 10) +
                    _write_float_field(1.0, 20, 10) +
                    _write_float_field(0.0, 20, 10) +
                    _write_float_field(stop_time, 20, 10) + "\n"
                )
                function_id += 1
                imposed_id += 1
            continue

        if bc_type == VELOCITY_BC:
            for axis, active in enumerate(mask):
                if not active:
                    continue
                value = values[axis]
                direction = directions[axis]
                function_points = _scaled_function_points(bc, value, stop_time, ramp_from_zero=False)
                if not function_points:
                    continue
                _write_function(out, function_id, f"vel_bc_{bc_index}_{direction}", function_points)
                out.write(f"/IMPVEL/{imposed_id}\n")
                out.write(f"vel_bc_{bc_index}_{axis}\n")
                out.write(
                    _write_int_field(function_id, 10) +
                    f"{direction:>10}" +
                    _write_int_field(0, 10) +
                    _write_int_field(0, 10) +
                    _write_int_field(group_id, 10) +
                    _write_int_field(0, 10) +
                    _write_int_field(0, 10) + "\n"
                )
                out.write(
                    _write_float_field(1.0, 20, 10) +
                    _write_float_field(1.0, 20, 10) +
                    _write_float_field(0.0, 20, 10) +
                    _write_float_field(stop_time, 20, 10) + "\n"
                )
                function_id += 1
                imposed_id += 1
            continue

        out.write(f"# Skipping BC {bc_index}: unsupported BC type {bc_type}\n")


def _simulation_time(model):
    if model.getStepCount() > 0 and model.getStep(0) is not None:
        return float(model.getStep(0).m_simTime)
    return 1.0


def _output_dt(model):
    if model.getStepCount() > 0 and model.getStep(0) is not None:
        return max(float(model.getStep(0).m_outTime), 1.0e-9)
    return 0.1


def _write_engine_file(model, engine_path, run_name):
    t_stop = _simulation_time(model)
    dt_out = _output_dt(model)

    with open(engine_path, "w", encoding="ascii") as out:
        out.write("/RUN/" + run_name + "/1\n")
        out.write(_write_float_field(t_stop, 20, 10) + "\n")
        out.write("/ANIM/DT\n")
        out.write(
            _write_float_field(0.0, 20, 10) +
            _write_float_field(dt_out, 20, 10) +
            _write_float_field(t_stop, 20, 10) + "\n"
        )
        for result_type in ("VEL", "DISP", "ACC"):
            out.write(f"/ANIM/VECT/{result_type}\n")
        for result_type in (
            "P",
            "SIGX",
            "SIGY",
            "SIGZ",
            "SIGXY",
            "SIGYZ",
            "SIGZX",
            "VONM",
        ):
            out.write(f"/ANIM/ELEM/{result_type}\n")
        out.write("/TFILE/3\n")
        out.write(_write_float_field(dt_out, 20, 10) + "\n")


def export_model_to_radioss(model, output_root):
    if model is None:
        raise RuntimeError("No active model available")

    output_root = os.path.abspath(output_root)
    output_dir = os.path.dirname(output_root)
    if output_dir and not os.path.isdir(output_dir):
        os.makedirs(output_dir, exist_ok=True)

    run_name = os.path.basename(output_root)
    starter_path = _starter_path(output_root)
    engine_path = _engine_path(output_root)
    title = model.getName() or run_name
    t_stop = _simulation_time(model)
    rigid_part_records = _build_rigid_part_records(model)

    with open(starter_path, "w", encoding="ascii") as out:
        _write_header_and_begin(out, run_name, title)
        _write_analysis_block(model, out)
        _write_nodes(model, out)
        _write_rigid_reference_nodes(rigid_part_records, out)
        material_ids = _write_materials(model, out)
        part_records = _write_properties_and_parts(model, out, material_ids)
        _write_elements(model, part_records, out)
        group_records = _write_node_groups(part_records, out)
        _write_rigid_body_groups(
            rigid_part_records,
            out,
            group_records,
            len(group_records["part_groups"]) + len(group_records["set_groups"]) + 1
        )
        _write_rigid_bodies(rigid_part_records, group_records, out)
        _write_boundary_conditions(model, out, group_records, t_stop)
        out.write("/END\n")

    _write_engine_file(model, engine_path, run_name)

    print(f"OpenRadioss starter written to: {starter_path}")
    print(f"OpenRadioss engine written to: {engine_path}")


def export_active_model_to_radioss(output_root=None):
    model = _active_model()
    _validate_model_for_export("OpenRadioss")
    if output_root is None:
        output_root = _default_output_root(model)
    export_model_to_radioss(model, output_root)


if __name__ == "__main__":
    export_active_model_to_radioss(DEFAULT_OUTPUT_ROOT)
