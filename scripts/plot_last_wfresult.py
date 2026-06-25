#!/usr/bin/env python3
import argparse
import json
import math
from pathlib import Path

import vtk

PLASTIC_FIELD_CANDIDATES = (
    "EPSP",
    "PEEQ",
    "plastic_strain",
    "plastic strain",
    "plasticStrain",
    "effective_plastic_strain",
    "equivalent_plastic_strain",
    "equivalent plastic strain",
    "eps_p",
    "epsp",
    "epbar",
    "plastic_deformation",
    "plastic deformation",
)


def load_last_frame(wfresult_path):
    wfresult_path = Path(wfresult_path)
    with wfresult_path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)

    frames = data.get("vtk_files", [])
    if not frames:
        raise RuntimeError(f"No vtk_files entries found in {wfresult_path}")

    entry = frames[-1]
    vtk_path = Path(entry["file"])
    if not vtk_path.is_absolute():
        vtk_path = wfresult_path.parent / vtk_path

    return vtk_path, entry.get("time")


def read_dataset(vtk_path):
    reader = vtk.vtkDataSetReader()
    reader.SetFileName(str(vtk_path))
    reader.Update()

    dataset = reader.GetOutput()
    if dataset is None or dataset.GetNumberOfPoints() == 0:
        raise RuntimeError(f"Could not read a valid VTK dataset from {vtk_path}")
    return dataset


def find_array(dataset, name):
    if not name:
        return None, None

    point_data = dataset.GetPointData()
    if point_data and point_data.GetArray(name):
        return point_data.GetArray(name), "point"

    cell_data = dataset.GetCellData()
    if cell_data and cell_data.GetArray(name):
        return cell_data.GetArray(name), "cell"

    available = []
    if point_data:
        available.extend(
            f"point:{point_data.GetArray(i).GetName()}"
            for i in range(point_data.GetNumberOfArrays())
            if point_data.GetArray(i) and point_data.GetArray(i).GetName()
        )
    if cell_data:
        available.extend(
            f"cell:{cell_data.GetArray(i).GetName()}"
            for i in range(cell_data.GetNumberOfArrays())
            if cell_data.GetArray(i) and cell_data.GetArray(i).GetName()
        )
    raise RuntimeError(f"Field '{name}' not found. Available fields: {', '.join(available)}")


def iter_named_arrays(dataset):
    for data, association in ((dataset.GetPointData(), "point"), (dataset.GetCellData(), "cell")):
        if not data:
            continue
        for i in range(data.GetNumberOfArrays()):
            array = data.GetArray(i)
            if array and array.GetName():
                yield array, association


def normalize_field_name(name):
    return "".join(ch for ch in name.lower() if ch.isalnum())


def find_plastic_array(dataset):
    arrays = list(iter_named_arrays(dataset))
    normalized_candidates = [normalize_field_name(name) for name in PLASTIC_FIELD_CANDIDATES]

    for candidate in normalized_candidates:
        for array, association in arrays:
            if normalize_field_name(array.GetName()) == candidate:
                return array, association

    for array, association in arrays:
        normalized = normalize_field_name(array.GetName())
        if "plastic" in normalized or "epsp" in normalized or "peeq" in normalized:
            return array, association

    available = ", ".join(f"{association}:{array.GetName()}" for array, association in arrays)
    raise RuntimeError(
        "Could not find a plastic deformation field. "
        f"Tried: {', '.join(PLASTIC_FIELD_CANDIDATES)}. "
        f"Available fields: {available}"
    )


def choose_default_array(dataset):
    for array, association in iter_named_arrays(dataset):
        if array.GetName().lower() not in {"part_id", "point_part_id"}:
            return array, association
    return None, None


def select_array(dataset, field_name=None, plastic=False):
    if plastic:
        return find_plastic_array(dataset)
    if field_name:
        return find_array(dataset, field_name)
    return choose_default_array(dataset)


def configure_mapper(dataset, field_name, plastic=False):
    mapper = vtk.vtkDataSetMapper()
    mapper.SetInputData(dataset)

    array, association = select_array(dataset, field_name, plastic)
    if array is None:
        mapper.ScalarVisibilityOff()
        return mapper, None

    if association == "point":
        mapper.SetScalarModeToUsePointFieldData()
    else:
        mapper.SetScalarModeToUseCellFieldData()

    mapper.SelectColorArray(array.GetName())
    if array.GetNumberOfComponents() == 3:
        mapper.SetArrayComponent(-1)
    mapper.SetScalarRange(array.GetRange(-1 if array.GetNumberOfComponents() == 3 else 0))
    mapper.ScalarVisibilityOn()
    return mapper, array.GetName()


def render_png(dataset, output_path, field_name=None, plastic=False, width=1400, height=1000):
    mapper, active_field = configure_mapper(dataset, field_name, plastic)

    actor = vtk.vtkActor()
    actor.SetMapper(mapper)
    actor.GetProperty().EdgeVisibilityOn()
    actor.GetProperty().SetEdgeColor(0.05, 0.05, 0.05)
    actor.GetProperty().SetLineWidth(0.5)

    renderer = vtk.vtkRenderer()
    renderer.SetBackground(1.0, 1.0, 1.0)
    renderer.AddActor(actor)
    renderer.ResetCamera()

    window = vtk.vtkRenderWindow()
    window.SetOffScreenRendering(1)
    window.SetSize(width, height)
    window.AddRenderer(renderer)
    window.Render()

    capture = vtk.vtkWindowToImageFilter()
    capture.SetInput(window)
    capture.Update()

    writer = vtk.vtkPNGWriter()
    writer.SetFileName(str(output_path))
    writer.SetInputConnection(capture.GetOutputPort())
    writer.Write()
    return active_field


def dataset_points(dataset):
    points = dataset.GetPoints()
    return [points.GetPoint(i) for i in range(points.GetNumberOfPoints())]


def scalar_values(array):
    if array is None:
        return None
    if array.GetNumberOfComponents() == 3:
        return [
            math.sqrt(sum(component * component for component in array.GetTuple(i)[:3]))
            for i in range(array.GetNumberOfTuples())
        ]
    return [array.GetTuple1(i) for i in range(array.GetNumberOfTuples())]


def cell_centers_and_values(dataset, array):
    centers = vtk.vtkCellCenters()
    centers.SetInputData(dataset)
    centers.Update()
    center_points = centers.GetOutput().GetPoints()
    xyz = [center_points.GetPoint(i) for i in range(center_points.GetNumberOfPoints())]
    values = scalar_values(array)
    return xyz, values


def point_values(array):
    return scalar_values(array)


def render_matplotlib_png(dataset, output_path, field_name=None, plastic=False, width=1400, height=1000):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    array, association = select_array(dataset, field_name, plastic)
    active_field = array.GetName() if array is not None else None

    if array is not None and association == "cell":
        xyz, values = cell_centers_and_values(dataset, array)
    else:
        xyz = dataset_points(dataset)
        values = point_values(array)

    if not xyz:
        raise RuntimeError("Dataset has no points to plot")

    xs = [p[0] for p in xyz]
    ys = [p[1] for p in xyz]
    zs = [p[2] for p in xyz]

    fig = plt.figure(figsize=(width / 100.0, height / 100.0), dpi=100)
    ax = fig.add_subplot(111, projection="3d")
    scatter = ax.scatter(xs, ys, zs, c=values, cmap="viridis", s=4 if len(xyz) > 5000 else 12)

    if active_field:
        fig.colorbar(scatter, ax=ax, shrink=0.75, pad=0.08, label=active_field)

    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Z")
    ax.set_title(active_field or "Last frame")

    ranges = [max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs)]
    radius = max(ranges) * 0.5
    if radius > 0.0:
        cx = (max(xs) + min(xs)) * 0.5
        cy = (max(ys) + min(ys)) * 0.5
        cz = (max(zs) + min(zs)) * 0.5
        ax.set_xlim(cx - radius, cx + radius)
        ax.set_ylim(cy - radius, cy + radius)
        ax.set_zlim(cz - radius, cz + radius)

    fig.tight_layout()
    fig.savefig(output_path)
    plt.close(fig)
    return active_field


def main(default_plastic=False):
    parser = argparse.ArgumentParser(description="Render the last VTK frame listed in a .wfresult file.")
    parser.add_argument("wfresult", help="Path to .wfresult")
    parser.add_argument("-o", "--output", help="Output PNG path")
    parser.add_argument("-f", "--field", help="Point or cell data field to color by")
    parser.add_argument("--plastic", action="store_true", default=default_plastic,
                        help="Automatically plot the plastic deformation/plastic strain field in the final frame")
    parser.add_argument("--backend", choices=("matplotlib", "vtk"), default="matplotlib",
                        help="matplotlib works headless; vtk needs a working OpenGL/X setup")
    parser.add_argument("--width", type=int, default=1400)
    parser.add_argument("--height", type=int, default=1000)
    args = parser.parse_args()

    wfresult_path = Path(args.wfresult)
    vtk_path, time_value = load_last_frame(wfresult_path)
    default_suffix = ".plastic.png" if args.plastic else ".last.png"
    output_path = Path(args.output) if args.output else wfresult_path.with_suffix(default_suffix)

    dataset = read_dataset(vtk_path)
    if args.backend == "vtk":
        active_field = render_png(dataset, output_path, args.field, args.plastic, args.width, args.height)
    else:
        active_field = render_matplotlib_png(dataset, output_path, args.field, args.plastic, args.width, args.height)

    print(f"Rendered: {vtk_path}")
    if time_value is not None:
        print(f"Time: {time_value}")
    if active_field:
        print(f"Field: {active_field}")
    print(f"Output: {output_path}")


if __name__ == "__main__":
    main()
