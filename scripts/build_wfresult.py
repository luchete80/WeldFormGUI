#!/usr/bin/env python3

import argparse
import json
import os
import re
import sys


FRAME_RE = re.compile(r"^(?P<prefix>.+)_(?P<index>\d+)\.vtk$")


def _parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Build a .wfresult manifest from a VTK frame series like "
            "'demo_run_00000.vtk', 'demo_run_00001.vtk', ..."
        )
    )
    parser.add_argument(
        "prefix",
        help="Frame prefix before the numeric suffix, for example 'demo_run'",
    )
    parser.add_argument(
        "--directory",
        default=".",
        help="Directory containing the VTK files. Default: current directory",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="Output .wfresult path. Default: <directory>/<prefix>.wfresult",
    )

    timing = parser.add_mutually_exclusive_group(required=True)
    timing.add_argument(
        "--dt",
        type=float,
        help="Uniform time step between frames",
    )
    timing.add_argument(
        "--times-file",
        help="Text file with one time value per line, in frame order",
    )

    parser.add_argument(
        "--start-time",
        type=float,
        default=0.0,
        help="Start time used with --dt. Default: 0.0",
    )
    return parser.parse_args()


def _collect_frames(directory, prefix):
    frames = []
    for entry in os.listdir(directory):
        match = FRAME_RE.match(entry)
        if not match:
            continue
        if match.group("prefix") != prefix:
            continue
        frames.append((int(match.group("index")), entry))

    frames.sort(key=lambda item: item[0])
    return frames


def _load_times(times_file):
    times = []
    with open(times_file, "r", encoding="utf-8") as handle:
        for line_number, raw_line in enumerate(handle, start=1):
            line = raw_line.strip()
            if not line:
                continue
            try:
                times.append(float(line))
            except ValueError as exc:
                raise ValueError(
                    f"Invalid time value at line {line_number} in {times_file}: {line}"
                ) from exc
    return times


def _build_manifest(frames, dt, start_time, times):
    vtk_files = []
    for position, (_, filename) in enumerate(frames):
        if times is not None:
            time_value = times[position]
        else:
            time_value = start_time + dt * position
        vtk_files.append({"file": filename, "time": time_value})
    return {"vtk_files": vtk_files}


def main():
    args = _parse_args()

    directory = os.path.abspath(args.directory)
    if not os.path.isdir(directory):
        print(f"Directory not found: {directory}", file=sys.stderr)
        return 1

    frames = _collect_frames(directory, args.prefix)
    if not frames:
        print(
            f"No VTK files found for prefix '{args.prefix}' in {directory}",
            file=sys.stderr,
        )
        return 1

    times = None
    if args.times_file is not None:
        times = _load_times(args.times_file)
        if len(times) != len(frames):
            print(
                f"Times count ({len(times)}) does not match frame count ({len(frames)})",
                file=sys.stderr,
            )
            return 1

    output_path = args.output
    if output_path is None:
        output_path = os.path.join(directory, f"{args.prefix}.wfresult")
    else:
        output_path = os.path.abspath(output_path)

    manifest = _build_manifest(frames, args.dt, args.start_time, times)

    with open(output_path, "w", encoding="utf-8") as handle:
        json.dump(manifest, handle, indent=2)
        handle.write("\n")

    print(f"Wrote {len(frames)} frames to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
