# Copyright 2023-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""Update the generated tables in the resource usage RST files.

The xmosdoc build runs this script using the four XS3 and VX4 ASRC and SSRC
reports collected by Jenkins in tests/sim_tests. To run it manually, use:
    python doc/python/generate_resource_usage.py <report-directory> doc/rst
"""

import argparse
import csv
from decimal import Decimal, InvalidOperation, ROUND_CEILING
from pathlib import Path


RATES = (44100, 48000, 88200, 96000, 176400, 192000)
DEVIATIONS = ("0.990099", "1.000000", "1.009999")
PROFILES = ("scalar", "vpu")
BEGIN = ".. BEGIN GENERATED TABLES"
END = ".. END GENERATED TABLES"
TABLES = (
    ("xs3a", "scalar",
     "Scalar (xcore-200, XS3 and VX4 without VPU; XS3 measurements)"),
    ("xs3a", "vpu", "XS3 VPU"),
    ("vx4b", "vpu", "VX4 VPU"),
)


def read_report(path, src_type):
    """Read and validate the matrix for one converter and architecture."""
    header = ["input_sr", "output_sr"]
    if src_type == "asrc":
        header.append("fs_deviation")
    header += ["profile", "MIPS"]

    deviations = DEVIATIONS if src_type == "asrc" else ("",)
    expected = {
        (input_rate, output_rate, deviation, profile)
        for input_rate in RATES
        for output_rate in RATES
        for deviation in deviations
        for profile in PROFILES
    }
    results = {}
    with path.open(newline="", encoding="utf-8") as report:
        reader = csv.DictReader(report)
        if reader.fieldnames != header:
            raise ValueError(
                f"{path}: expected columns {header}, got {reader.fieldnames}"
            )
        for row in reader:
            try:
                key = (
                    int(row["input_sr"]),
                    int(row["output_sr"]),
                    row["fs_deviation"] if src_type == "asrc" else "",
                    row["profile"],
                )
                value = Decimal(row["MIPS"])
            except (TypeError, ValueError, InvalidOperation) as error:
                raise ValueError(
                    f"{path}:{reader.line_num}: invalid report row"
                ) from error
            if key not in expected:
                raise ValueError(
                    f"{path}:{reader.line_num}: unexpected configuration {key}"
                )
            if key in results:
                raise ValueError(
                    f"{path}:{reader.line_num}: duplicate configuration {key}"
                )
            if not value.is_finite() or value <= 0:
                raise ValueError(
                    f"{path}:{reader.line_num}: invalid MIPS value {value}"
                )
            results[key] = value

    missing = expected - results.keys()
    if missing:
        raise ValueError(
            f"{path}: missing {len(missing)} configurations, "
            f"e.g. {min(missing)}"
        )
    return results


def render_table(src_type, architecture, profile, title, results, index):
    label = (f"fig_{src_type}_mhz" if index == 0
             else f"fig_{src_type}_{architecture}_vpu_mhz")
    lines = [
        f".. _{label}:",
        "",
        f".. list-table:: {src_type.upper()} usage per channel (MHz): {title}",
        "   :header-rows: 1",
        "   :width: 100%",
        "   :widths: 22 13 13 13 13 13 13",
        "",
        "   * - Input / output rate",
    ]
    lines.extend(f"     - {rate / 1000:g} kHz" for rate in RATES)
    deviations = DEVIATIONS if src_type == "asrc" else ("",)
    for input_rate in RATES:
        lines.append(f"   * - {input_rate / 1000:g} kHz")
        for output_rate in RATES:
            peak = max(
                results[input_rate, output_rate, deviation, profile]
                for deviation in deviations
            )
            rounded = peak.to_integral_value(rounding=ROUND_CEILING)
            lines.append(f"     - {rounded}")
    return "\n".join(lines)


def update_document(path, src_type, reports):
    document = path.read_text(encoding="utf-8")
    if (document.count(BEGIN) != 1 or document.count(END) != 1
            or document.index(BEGIN) > document.index(END)):
        raise ValueError(f"{path}: expected one ordered pair of table markers")
    before, rest = document.split(BEGIN)
    _, after = rest.split(END)
    tables = [
        render_table(
            src_type, architecture, profile, title, reports[architecture],
            index,
        )
        for index, (architecture, profile, title) in enumerate(TABLES)
    ]
    return (
        f"{before}{BEGIN}\n\n"
        + "\n\n".join(tables) + f"\n\n{END}{after}"
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "reports", type=Path, help="directory containing the four MIPS reports"
    )
    parser.add_argument(
        "output", type=Path, help="directory containing resource usage RST"
    )
    parser.add_argument(
        "--source",
        help="accepted for compatibility; no longer included in the output",
    )
    args = parser.parse_args()

    try:
        documents = {}
        for src_type in ("asrc", "ssrc"):
            reports = {
                architecture: read_report(
                    args.reports
                    / f"mips_report_{src_type}_{architecture}.csv",
                    src_type,
                )
                for architecture in ("xs3a", "vx4b")
            }
            output = args.output / f"resource_usage_{src_type}.rst"
            documents[output] = update_document(output, src_type, reports)
        for output, document in documents.items():
            output.write_text(document, encoding="utf-8")
    except (OSError, ValueError) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
