#!/usr/bin/env python3

import math

# -----------------------------
# Input
# -----------------------------
H0 = 7.5      # mm
Hf = 3.58     # mm
strain_rate = 0.1  # 1/s

# -----------------------------
# Calculations
# -----------------------------
tf = math.log(H0 / Hf) / strain_rate

v0 = strain_rate * H0
vf = strain_rate * Hf

# -----------------------------
# Results
# -----------------------------
print(f"Initial height      : {H0:.4f} mm")
print(f"Final height        : {Hf:.4f} mm")
print(f"Strain rate         : {strain_rate:.4f} 1/s")
print()

print(f"Initial velocity    : {v0:.4f} mm/s")
print(f"Final velocity      : {vf:.4f} mm/s")
print(f"Test duration       : {tf:.4f} s")
print(f"Test duration       : {tf/60:.4f} min")

print("\nTime history:")
print(f"{'t [s]':>10} {'H [mm]':>12} {'v [mm/s]':>12}")

npts = 10

for i in range(npts + 1):
    t = tf * i / npts

    H = H0 * math.exp(-strain_rate * t)
    v = strain_rate * H

    print(f"{t:10.4f} {H:12.4f} {v:12.4f}")
