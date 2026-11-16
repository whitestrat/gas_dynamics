#!/usr/bin/env python3
"""Пять обязательных тестов Торо через Python → JSON → C++ → CSV."""

import argparse
import csv
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "python"))
from calculation import Calculation
from riemann_problem import RiemannProblem

FIELDS = ("p_star", "u_star", "rho_star_left", "rho_star_right")
GUESSES = ("linear", "two_rarefactions", "two_shocks")


def run_case(case, output, bin_dir, layers):
    states = {side: {"density": float(case[f"rho_{letter}"]),
                     "velocity": float(case[f"u_{letter}"]),
                     "pressure": float(case[f"p_{letter}"])}
              for side, letter in (("left", "L"), ("right", "R"))}
    task = RiemannProblem(mesh=ROOT / "examples/riemann/mesh.csv", states=states,
                          time={"t_end": float(case["t_out"]), "N": layers})
    calc = Calculation(output / f"test_{case['test']}", task).run(bin_dir=bin_dir)
    constants = {name: float(calc.load(name)[0]) for name, info in calc.quantities.items()
                 if info["type"] == "c"}
    row = {"test": case["test"]}
    passed = True
    for name in FIELDS:
        value, reference = constants[name], float(case[f"{name}_ref"])
        error = abs(value - reference)
        tolerance = float(case[f"{name}_tol"])
        row.update({name: value, f"{name}_reference": reference,
                    f"{name}_absolute_error": error,
                    f"{name}_relative_error": error / abs(reference) if reference else "",
                    f"{name}_absolute_tolerance": tolerance})
        passed = passed and error <= tolerance
    for side in ("left", "right"):
        row[f"{side}_wave"] = "shock" if constants[f"{side}_is_shock"] else "rarefaction"
        for edge in ("shock", "head", "tail"):
            key = f"{side}_{edge}_speed"
            row[key] = constants.get(key, "")
    for name in ("contact_speed", "p_lin", "p_initial", "newton_iterations",
                 "damped_steps", "relative_change", "residual"):
        row[name] = constants[name]
    row["GUESSP"] = GUESSES[int(constants["guess_type"])]
    row["passed"] = passed
    print(f"Торо {case['test']}: {'ПРОЙДЕН' if passed else 'НЕ ПРОЙДЕН'}; "
          f"GUESSP={row['GUESSP']}; Ньютон: {int(row['newton_iterations'])}", flush=True)
    return row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bin-dir", type=Path)
    parser.add_argument("--output", type=Path, default=ROOT / "results/toro")
    parser.add_argument("--layers", type=int, default=101)
    args = parser.parse_args()
    if args.layers < 2:
        parser.error("--layers должен быть не меньше 2")
    try:
        with (ROOT / "examples/riemann/toro_cases.csv").open(encoding="utf-8") as stream:
            cases = list(csv.DictReader(stream))
        output = args.output.expanduser().resolve()
        output.mkdir(parents=True, exist_ok=True)
        rows = [run_case(case, output, args.bin_dir, args.layers) for case in cases]
        with (output / "comparison.csv").open("w", encoding="utf-8", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
            writer.writeheader()
            writer.writerows(rows)
        print(f"Пройдено {sum(row['passed'] for row in rows)} / 5. "
              f"Таблица: {output / 'comparison.csv'}")
        return 0 if all(row["passed"] for row in rows) else 1
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        print(f"Ошибка тестов Торо: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
