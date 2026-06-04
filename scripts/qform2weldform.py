#!/usr/bin/env python3

import argparse
import csv
import json
import re
import sys
from pathlib import Path


FILE_RE = re.compile(
    r"^(?P<material>.+)-(?P<table>\d+)-(?P<temperature>[-+]?\d+(?:\.\d+)?)\.csv$",
    re.IGNORECASE,
)


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Convert QForm-like material CSV tables into WeldForm tabulated "
            "flow-stress JSON files."
        )
    )
    parser.add_argument(
        "input_dir",
        type=Path,
        help="Directory containing files such as c45-001-800.csv",
    )
    parser.add_argument(
        "-o",
        "--output-dir",
        type=Path,
        default=None,
        help="Output directory. Default: <input_dir>/weldform_out",
    )
    parser.add_argument(
        "--factor",
        type=float,
        default=1.0,
        help="Scale factor applied to all stress values. Example: --factor 1e6 for MPa -> Pa.",
    )
    parser.add_argument(
        "--indent",
        type=int,
        default=2,
        help="JSON indentation level. Default: 2",
    )
    return parser.parse_args()


def parse_float(token, file_path):
    token = token.strip().replace(",", ".")
    if not token:
      raise ValueError(f"Empty numeric token in {file_path}")
    return float(token)


def read_qform_table(file_path):
    rows = []
    with file_path.open("r", encoding="utf-8-sig", newline="") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line:
                continue
            parts = [part.strip() for part in line.split(";")]
            if parts and parts[-1] == "":
                parts = parts[:-1]
            rows.append(parts)

    if len(rows) < 2:
        raise ValueError(f"{file_path} must contain one header row and at least one data row.")

    header = rows[0]
    if len(header) < 2:
        raise ValueError(f"{file_path} header must contain temperature/rate label plus strain-rate values.")

    strain_rates = [parse_float(token, file_path) for token in header[1:]]
    strains = []
    stress_matrix = []

    expected_columns = len(strain_rates) + 1
    for row_index, row in enumerate(rows[1:], start=2):
        if len(row) != expected_columns:
            raise ValueError(
                f"{file_path} line {row_index} has {len(row)} columns, expected {expected_columns}."
            )
        strains.append(parse_float(row[0], file_path))
        stress_matrix.append([parse_float(token, file_path) for token in row[1:]])

    return strains, strain_rates, stress_matrix


def collect_material_tables(input_dir):
    grouped = {}

    for file_path in sorted(input_dir.glob("*.csv")):
        match = FILE_RE.match(file_path.name)
        if not match:
            continue

        material = match.group("material")
        table_number = int(match.group("table"))
        temperature = float(match.group("temperature"))
        strains, strain_rates, stress_matrix = read_qform_table(file_path)

        grouped.setdefault(material, []).append(
            {
                "path": file_path,
                "table_number": table_number,
                "temperature": temperature,
                "strains": strains,
                "strain_rates": strain_rates,
                "stress_matrix": stress_matrix,
            }
        )

    return grouped


def validate_and_merge_tables(material, table_entries, factor):
    if not table_entries:
        raise ValueError(f"No tables found for material '{material}'.")

    sorted_entries = sorted(table_entries, key=lambda entry: (entry["temperature"], entry["table_number"]))
    ref_strains = sorted_entries[0]["strains"]
    ref_rates = sorted_entries[0]["strain_rates"]

    for entry in sorted_entries[1:]:
        if entry["strains"] != ref_strains:
            raise ValueError(
                f"Material '{material}' has inconsistent strain rows between tables. "
                f"First mismatch: {entry['path']}"
            )
        if entry["strain_rates"] != ref_rates:
            raise ValueError(
                f"Material '{material}' has inconsistent strain-rate columns between tables. "
                f"First mismatch: {entry['path']}"
            )

    temperatures = []
    stress_values = []
    long_rows = []

    seen_temperatures = set()
    for entry in sorted_entries:
        temperature = entry["temperature"]
        if temperature in seen_temperatures:
            raise ValueError(
                f"Material '{material}' contains duplicate temperature table {temperature}."
            )
        seen_temperatures.add(temperature)
        temperatures.append(temperature)

        for rate_index, strain_rate in enumerate(ref_rates):
            for strain_index, strain in enumerate(ref_strains):
                stress = entry["stress_matrix"][strain_index][rate_index] * factor
                stress_values.append(stress)
                long_rows.append(
                    {
                        "strain": strain,
                        "strain_rate": strain_rate,
                        "temperature": temperature,
                        "stress": stress,
                    }
                )

    return {
        "strain_grid": ref_strains,
        "strain_rate_grid": ref_rates,
        "temperature_grid": temperatures,
        "stress_values": stress_values,
        "long_rows": long_rows,
    }


def build_weldform_json(material, merged):
    return {
        "Materials": [
            {
                "type": "Tabulated",
                "flowStressTable": {
                    "strainGrid": merged["strain_grid"],
                    "strainRateGrid": merged["strain_rate_grid"],
                    "temperatureGrid": merged["temperature_grid"],
                    "stressValues": merged["stress_values"],
                },
            }
        ]
    }


def write_long_csv(file_path, long_rows):
    with file_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["strain", "strain_rate", "temperature", "stress"])
        for row in long_rows:
            writer.writerow(
                [row["strain"], row["strain_rate"], row["temperature"], row["stress"]]
            )


def main():
    args = parse_args()
    input_dir = args.input_dir.resolve()
    if not input_dir.is_dir():
        print(f"ERROR: input directory does not exist: {input_dir}", file=sys.stderr)
        return 1

    output_dir = args.output_dir.resolve() if args.output_dir else (input_dir / "weldform_out")
    output_dir.mkdir(parents=True, exist_ok=True)

    grouped = collect_material_tables(input_dir)
    if not grouped:
        print(
            "ERROR: no matching CSV files were found. Expected names like material-001-800.csv",
            file=sys.stderr,
        )
        return 1

    for material, table_entries in grouped.items():
        merged = validate_and_merge_tables(material, table_entries, args.factor)
        output_json = build_weldform_json(material, merged)

        json_path = output_dir / f"{material}.json"
        with json_path.open("w", encoding="utf-8") as handle:
            json.dump(output_json, handle, indent=args.indent)
            handle.write("\n")

        long_csv_path = output_dir / f"{material}_weldform.csv"
        write_long_csv(long_csv_path, merged["long_rows"])

        print(
            f"Wrote {json_path} "
            f"(strain={len(merged['strain_grid'])}, "
            f"strain_rate={len(merged['strain_rate_grid'])}, "
            f"temperature={len(merged['temperature_grid'])})"
        )
        print(f"Wrote {long_csv_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
