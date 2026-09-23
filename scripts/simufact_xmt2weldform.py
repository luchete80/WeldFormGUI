#!/usr/bin/env python3
"""Convert a Simufact .xmt material with a GMT equation to WeldForm JSON."""

import argparse
import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


# Simufact XMT order differs from WeldForm's internal JSON order.
XMT_TO_WELDFORM_ORDER = (2, 3, 0, 1, 6, 7, 4, 5)


def child_text(element, name, *, required=True, default=None):
    child = element.find(name)
    if child is None or child.text is None:
        if required:
            raise ValueError(f"Missing required .xmt element <{name}>")
        return default
    try:
        return float(child.text.strip())
    except ValueError as exc:
        raise ValueError(f"Invalid numeric value in <{name}>: {child.text!r}") from exc


def convert(input_path):
    root = ET.parse(input_path).getroot()
    approach = root.find("./flow_curves_plasticity_approach")
    equations = approach.find("./equations") if approach is not None else None
    equation = equations.find("./equation") if equations is not None else None
    if equation is None:
        raise ValueError("No active equation at flow_curves_plasticity_approach/equations/equation")

    xmt_params = [child_text(equation, f"flowcurve_parameter_{i:02d}") for i in range(1, 9)]
    # XMT uses [C1, C2, n1, n2, I1, I2, m1, m2]; WeldForm stores
    # [n1, n2, C1, C2, m1, m2, I1, I2]. XMT stress is in MPa, WeldForm in Pa.
    xmt_params[0] *= 1.0e6
    params = [xmt_params[index] for index in XMT_TO_WELDFORM_ORDER]

    material = {
        "type": "GMT",
        "const": params,
        "density0": child_text(root, "density"),
        "youngsModulus": child_text(root, "youngs_modulus") * 1.0e6,
        "poissonsRatio": child_text(root, "transverse_contraction"),
        "yieldStress0": child_text(root, "yield_strength") * 1.0e6,
        "strRange": [child_text(equation, "min_effective_plastic_strain"),
                     child_text(equation, "max_effective_plastic_strain")],
        "strdotRange": [child_text(equation, "min_strain_rate"),
                        child_text(equation, "max_strain_rate")],
        "tempRange": [child_text(equation, "min_temperature"),
                      child_text(equation, "max_temperature")],
    }
    # These scalar XMT values use kJ/(kg K), W/(m K), and 1/K respectively.
    thermal = (
        ("thermalHeatCap", "specific_heat_capacity", 1000.0),
        ("thermalCond", "thermal_conductivity", 1.0),
        ("thermalExp", "thermal_expansion", 1.0),
    )
    for out_key, in_key, scale in thermal:
        value = child_text(root, in_key, required=False)
        if value is not None:
            material[out_key] = value * scale

    return {"Materials": [material]}


def main(argv=None):
    parser = argparse.ArgumentParser(description="Convert a Simufact GMT .xmt material to WeldForm JSON.")
    parser.add_argument("input", type=Path, help="Input Simufact .xmt file")
    parser.add_argument("-o", "--output", type=Path, help="Output JSON path (default: <input>.json)")
    parser.add_argument("--indent", type=int, default=2, help="JSON indentation (default: 2)")
    args = parser.parse_args(argv)
    output = args.output if args.output is not None else args.input.with_suffix(".json")
    try:
        converted = convert(args.input)
        with output.open("w", encoding="utf-8") as handle:
            json.dump(converted, handle, indent=args.indent)
            handle.write("\n")
    except (OSError, ET.ParseError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    print(f"Wrote {output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
