# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""
Check that the committed voice SRC coefficient files match what
``python -m lib_src.voice.generate`` produces.
"""
import re
from pathlib import Path

import numpy as np
import pytest

from lib_src.voice import generate

SRC_DIR = Path(__file__).resolve().parents[2] / "lib_src" / "src"


def parse_arrays(path):
    """Return {symbol: np.array of ints} for every initialised array in a C/XC file."""
    text = Path(path).read_text()
    arrays = {}
    for m in re.finditer(r"(\w+)\s*(?:\[[^\[\]=;]*\]\s*)+=\s*\{(.*?)\};", text, re.S):
        arrays[m.group(1)] = np.array([int(v) for v in re.findall(r"-?\d+", m.group(2))])
    for m in re.finditer(r"(\w+)\s*=\s*(-?\d+);", text):
        arrays[m.group(1)] = np.array([int(m.group(2))])
    return arrays


# (generate arguments, committed directory, generated files)
CASES = {
    "ff3": (["ff3"], "fixed_factor_vpu_voice", ["src_ff3_fir_coefs.c", "src_ff3_fir_coefs.h"]),
    "rat": (["rat"], "fixed_factor_vpu_voice", ["src_rat_fir_coefs.c", "src_rat_fir_coefs.h"]),
    "ff3v": (["ff3v"], "fixed_factor_of_3_voice", ["src_ff3v_fir.xc", "src_ff3v_fir.h"]),
}


@pytest.mark.parametrize("case", CASES.keys())
def test_committed_coefs_match_generator(case, tmp_path):
    args, committed_dir, files = CASES[case]
    generate.main(args + ["-o", str(tmp_path)])

    for f in files:
        generated = tmp_path / f
        committed = SRC_DIR / committed_dir / f
        assert generated.is_file(), f"{f} was not generated"

        gen_arrays = parse_arrays(generated)
        com_arrays = parse_arrays(committed)
        assert gen_arrays.keys() == com_arrays.keys(), f"{f}: symbols differ"
        for sym in gen_arrays:
            # allow 1 LSB for differences between numpy/scipy versions
            np.testing.assert_allclose(gen_arrays[sym], com_arrays[sym], rtol=0, atol=1,
                                       err_msg=f"{f}: {sym} differs from the generator")

        # the macros (sizes) must match exactly
        def defines(p):
            return re.findall(r"#define\s+\w+\s+\(\d+\)", Path(p).read_text())
        assert defines(generated) == defines(committed), f"{f}: #defines differ"
