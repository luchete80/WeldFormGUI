#!/usr/bin/env python3
"""Run the existing MINI3D D2 combined case after the GUI project/input roundtrip."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    repo = Path(__file__).resolve().parents[3]
    parser.add_argument("--engine", type=Path, default=repo.parent / "weldform-engine")
    parser.add_argument("--harness", type=Path,
                        default=repo.parent / "WeldFormGUI_bin/src/angular_motion_project_io_tests")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    engine = args.engine.resolve()
    checker = engine / "validation/implicit/rigid_motion_3d_d2/check_runtime.py"
    spec = importlib.util.spec_from_file_location("mini3d_d2", checker)
    d2 = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(d2)
    work = args.output.resolve() if args.output else Path(tempfile.mkdtemp(prefix="wfg_g3_runtime_"))
    work.mkdir(parents=True, exist_ok=True)
    source, linear, angular = d2.prepare_input(
        engine / "validation/implicit/energy_delivery3/contact_normal")
    source_path = work / "source.wfinput"
    source_path.write_text(json.dumps(source, indent=2) + "\n")
    exported_path = work / "exported.wfinput"
    conversion = subprocess.run([str(args.harness.resolve()), "--roundtrip-input",
                                 str(source_path), str(exported_path)], text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
    (work / "roundtrip.log").write_text(conversion.stdout)
    if conversion.returncode:
        raise RuntimeError(f"GUI roundtrip failed; see {work / 'roundtrip.log'}")
    exported = json.loads(exported_path.read_text())
    # Keep every setting emitted by the GUI. Only relocate packaged meshes for
    # the runtime helper, which writes its input in a child directory.
    for block in exported["DomainBlocks"] + exported["RigidBodies"]:
        block["fileName"] = str((exported_path.parent / block["fileName"]).resolve())
    result, events, run_dir = d2.run(engine / "build/weldform_imp_3d", work,
                                   "solver", exported,
                                   {"WF_MINI3D_TEST_REJECT_RIGID_MOTION_COUNT": "2"})
    (work / "solver.log").write_text(result.stdout)
    d2.require_complete_run(result, events, linear, angular, 2)
    print(f"PASS: GUI project/input roundtrip accepted by MINI3D; two tools, knots, "
          f"geometry, shear and two rollbacks verified. Evidence: {work}")


if __name__ == "__main__":
    main()
