#!/usr/bin/env python3
"""
Validation script for microstructure-weighted SIFT2.

Runs the original and modified SIFT2 on the same data and compares:
1. That lambda=0 produces identical weights to the original
2. That low-MicroAF streamlines get lower weights with the prior enabled
3. Summary statistics of weight distributions

Usage:
    python validate_microstructure_sift2.py <tracks.tck> <fod.mif> <microaf.txt> [tcksift2_binary]

Arguments:
    tracks.tck   - input tractogram
    fod.mif      - FOD image
    microaf.txt  - per-streamline MicroAF values (one per line)
    tcksift2     - path to tcksift2 binary (default: tcksift2)
"""

import sys
import os
import subprocess
import tempfile
import numpy as np


def run_tcksift2(binary, tracks, fod, output, extra_args=None):
    cmd = [binary, tracks, fod, output]
    if extra_args:
        cmd.extend(extra_args)
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"ERROR running: {' '.join(cmd)}")
        print(result.stderr)
        sys.exit(1)
    return result


def load_weights(path):
    return np.loadtxt(path)


def weight_stats(weights, label):
    print(f"\n  {label}:")
    print(f"    N          = {len(weights)}")
    print(f"    Mean       = {np.mean(weights):.6f}")
    print(f"    Std        = {np.std(weights):.6f}")
    print(f"    Min        = {np.min(weights):.6f}")
    print(f"    Max        = {np.max(weights):.6f}")
    print(f"    Median     = {np.median(weights):.6f}")
    nonzero = np.sum(weights > 0)
    print(f"    Non-zero   = {nonzero} ({100*nonzero/len(weights):.1f}%)")


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        sys.exit(1)

    tracks = sys.argv[1]
    fod = sys.argv[2]
    microaf_file = sys.argv[3]
    tcksift2 = sys.argv[4] if len(sys.argv) > 4 else "tcksift2"

    for f in [tracks, fod, microaf_file]:
        if not os.path.exists(f):
            print(f"ERROR: File not found: {f}")
            sys.exit(1)

    microaf = load_weights(microaf_file)
    print(f"Loaded {len(microaf)} MicroAF values "
          f"(range: {np.min(microaf):.4f} - {np.max(microaf):.4f}, "
          f"mean: {np.mean(microaf):.4f})")

    tmpdir = tempfile.mkdtemp(prefix="sift2_validate_")
    w_original = os.path.join(tmpdir, "weights_original.txt")
    w_lambda0 = os.path.join(tmpdir, "weights_lambda0.txt")
    w_modified = os.path.join(tmpdir, "weights_modified.txt")

    # --- Test 1: Original SIFT2 (no microstructure flags) ---
    print("\n=== Running original SIFT2 (no microstructure flags) ===")
    run_tcksift2(tcksift2, tracks, fod, w_original)

    # --- Test 2: Modified SIFT2 with lambda=0 ---
    print("\n=== Running SIFT2 with -microstructure_weighting and -microstructure_lambda 0 ===")
    run_tcksift2(tcksift2, tracks, fod, w_lambda0,
                 ["-microstructure_weighting", microaf_file,
                  "-microstructure_lambda", "0"])

    # --- Test 3: Modified SIFT2 with default lambda ---
    print("\n=== Running SIFT2 with -microstructure_weighting (default lambda) ===")
    run_tcksift2(tcksift2, tracks, fod, w_modified,
                 ["-microstructure_weighting", microaf_file])

    # Load all weights
    weights_orig = load_weights(w_original)
    weights_l0 = load_weights(w_lambda0)
    weights_mod = load_weights(w_modified)

    # --- Report weight distributions ---
    print("\n" + "=" * 60)
    print("WEIGHT DISTRIBUTION COMPARISON")
    print("=" * 60)
    weight_stats(weights_orig, "Original SIFT2")
    weight_stats(weights_l0, "Lambda=0 (should match original)")
    weight_stats(weights_mod, "Microstructure-weighted")

    # --- Check 1: lambda=0 should produce identical weights ---
    print("\n" + "=" * 60)
    print("CHECK 1: Lambda=0 vs Original (identity check)")
    print("=" * 60)
    max_diff = np.max(np.abs(weights_orig - weights_l0))
    mean_diff = np.mean(np.abs(weights_orig - weights_l0))
    print(f"  Max absolute difference:  {max_diff:.2e}")
    print(f"  Mean absolute difference: {mean_diff:.2e}")
    if max_diff < 1e-10:
        print("  PASS: Weights are numerically identical")
    elif max_diff < 1e-6:
        print("  PASS: Weights match within floating-point tolerance")
    else:
        print("  FAIL: Weights differ significantly!")

    # --- Check 2: Low-MicroAF streamlines should get lower weights ---
    print("\n" + "=" * 60)
    print("CHECK 2: Microstructure prior effect")
    print("=" * 60)

    # Split streamlines into low and high MicroAF groups
    median_microaf = np.median(microaf)
    low_mask = microaf < median_microaf
    high_mask = microaf >= median_microaf

    # Weight ratios: modified / original
    valid = (weights_orig > 0) & (weights_mod > 0)
    ratio = np.where(valid, weights_mod / weights_orig, np.nan)

    low_ratio = np.nanmean(ratio[low_mask & valid])
    high_ratio = np.nanmean(ratio[high_mask & valid])

    print(f"  Median MicroAF threshold: {median_microaf:.4f}")
    print(f"  Low-MicroAF group  (n={np.sum(low_mask)}):  mean weight ratio (mod/orig) = {low_ratio:.4f}")
    print(f"  High-MicroAF group (n={np.sum(high_mask)}): mean weight ratio (mod/orig) = {high_ratio:.4f}")

    if low_ratio < high_ratio:
        print("  PASS: Low-MicroAF streamlines receive systematically lower weights")
    else:
        print("  FAIL: Expected low-MicroAF streamlines to get lower relative weights")

    # Correlation between MicroAF and weight change
    valid_both = valid & np.isfinite(ratio)
    if np.sum(valid_both) > 10:
        corr = np.corrcoef(microaf[valid_both], ratio[valid_both])[0, 1]
        print(f"  Correlation(MicroAF, weight_ratio): {corr:.4f}")
        if corr > 0:
            print("  PASS: Positive correlation confirms prior is working as intended")
        else:
            print("  WARNING: Expected positive correlation between MicroAF and weight ratio")

    # --- Histogram comparison ---
    print("\n" + "=" * 60)
    print("WEIGHT HISTOGRAM (log-scale bins)")
    print("=" * 60)
    bins = np.logspace(-3, 2, 21)
    hist_orig, _ = np.histogram(weights_orig[weights_orig > 0], bins=bins)
    hist_mod, _ = np.histogram(weights_mod[weights_mod > 0], bins=bins)
    print(f"  {'Bin range':>20s}  {'Original':>10s}  {'Modified':>10s}  {'Diff':>10s}")
    for i in range(len(bins) - 1):
        print(f"  {bins[i]:9.4f}-{bins[i+1]:9.4f}  {hist_orig[i]:10d}  {hist_mod[i]:10d}  {hist_mod[i]-hist_orig[i]:+10d}")

    # --- Summary ---
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    print(f"  Output directory: {tmpdir}")
    print(f"  Files: weights_original.txt, weights_lambda0.txt, weights_modified.txt")

    print("\nValidation complete.")


if __name__ == "__main__":
    main()
