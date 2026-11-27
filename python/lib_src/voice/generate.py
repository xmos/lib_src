# Copyright 2023-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""
Generate the voice SRC coefficient files.

Examples::

    # VPU fixed factor of 3, 96 taps (lib_src/src/fixed_factor_vpu_voice)
    python -m lib_src.voice.generate ff3 -o <output_dir>

    # VPU fixed factor of 3, 72 taps, custom name
    python -m lib_src.voice.generate ff3 -o <output_dir> -ntp 24 -n test_src_72t

    # VPU rational factor 3/2, 96 taps (lib_src/src/fixed_factor_vpu_voice)
    python -m lib_src.voice.generate rat -o <output_dir>

    # Deprecated ff3v filter (lib_src/src/fixed_factor_of_3_voice)
    python -m lib_src.voice.generate ff3v -o <output_dir>
"""
import argparse
from pathlib import Path

import numpy as np

from . import codegen, design


def plot_response(taps, fs, name, output_dir):
    """Save full-band and passband magnitude response plots as PNG files."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from scipy import signal

    w, h = signal.freqz(taps, worN=8192, fs=fs)
    mag_db = 20 * np.log10(np.abs(h) + 1e-20)
    for suffix, ylim, xmax in (("", None, 0.5 * fs), ("_pb", (-0.2, 0.2), 0.25 * fs)):
        fig = plt.figure()
        plt.plot(w, mag_db)
        plt.xlim(0, xmax)
        if ylim:
            plt.ylim(*ylim)
        plt.grid(True)
        plt.xlabel("Frequency (Hz)")
        plt.ylabel("Gain (dB)")
        plt.title(name + suffix)
        fig.savefig(Path(output_dir) / f"{name}{suffix}.png", dpi=200)
        plt.close(fig)


def gen_ff3(args):
    num_taps = args.num_phases * args.num_taps_per_phase
    taps = design.ff3_kaiser(num_taps)
    codegen.write_ff3_files(
        args.output_dir, args.name,
        design.to_fixed(taps, 31),
        design.ff3_coefs_q30(taps, args.num_phases),
        cmd=f"python -m lib_src.voice.generate ff3 -o <output_dir> -n {args.name} "
            f"-ntp {args.num_taps_per_phase} -np {args.num_phases}")
    return taps, design.FF3_FS, args.name


def gen_rat(args):
    taps = design.rat_kaiser(args.num_taps)
    codegen.write_rat_files(
        args.output_dir,
        design.rat_coefs_q30(taps, design.RAT_NUM_PHASES_DS),
        design.rat_coefs_q30(taps, design.RAT_NUM_PHASES_US),
        cmd=f"python -m lib_src.voice.generate rat -o <output_dir> -nt {args.num_taps}")
    return taps, design.RAT_FS, "src_rat_fir"


def gen_ff3v(args):
    taps = design.ff3_legacy_remez()
    codegen.write_legacy_ff3v_files(
        args.output_dir, design.legacy_ff3v_coefs(taps),
        cmd="python -m lib_src.voice.generate ff3v -o <output_dir>")
    return taps, design.FF3_FS, "src_ff3v_fir"


def main(argv=None):
    parser = argparse.ArgumentParser(description="Generate voice SRC coefficient files")
    sub = parser.add_subparsers(dest="filter", required=True)

    p = sub.add_parser("ff3", help="VPU fixed factor of 3 (48 <-> 16 kHz)")
    p.add_argument("--name", "-n", default="src_ff3_fir", help="prefix of the generated files and symbols")
    p.add_argument("--num_taps_per_phase", "-ntp", type=int, default=design.FF3_TAPS_PER_PHASE)
    p.add_argument("--num_phases", "-np", type=int, default=design.FF3_NUM_PHASES)
    p.set_defaults(func=gen_ff3)

    p = sub.add_parser("rat", help="VPU rational factor 3/2 (48 <-> 32 kHz)")
    p.add_argument("--num_taps", "-nt", type=int, default=design.RAT_NUM_TAPS)
    p.set_defaults(func=gen_rat)

    p = sub.add_parser("ff3v", help="deprecated src_ds3_voice/src_us3_voice filter")
    p.set_defaults(func=gen_ff3v)

    for p in sub.choices.values():
        p.add_argument("--output_dir", "-o", required=True, help="output directory")
        p.add_argument("--gen_plots", "-gp", action="store_true", help="also save response plots")

    args = parser.parse_args(argv)
    Path(args.output_dir).mkdir(parents=True, exist_ok=True)
    taps, fs, name = args.func(args)
    if args.gen_plots:
        plot_response(taps, fs, name, args.output_dir)


if __name__ == "__main__":
    main()
