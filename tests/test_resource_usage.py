# Copyright 2023-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import csv
import subprocess
import sys
from pathlib import Path

import pytest

from sim_tests import test_mrhf


SCRIPT = (
    Path(__file__).resolve().parents[1]
    / "doc/python/generate_resource_usage.py"
)
RATES = (44100, 48000, 88200, 96000, 176400, 192000)
DEVIATIONS = ("0.990099", "1.000000", "1.009999")


@pytest.mark.parametrize("src_type,architecture,report_exists", [
    ("ssrc", "xs3a", True),
    ("asrc", "vx4b", True),
    ("asrc", "xs3a", False),
])
def test_prepare_removes_only_its_mips_report(
    tmp_path, monkeypatch, src_type, architecture, report_exists
):
    # Isolate report files and temporarily select the architecture.
    monkeypatch.chdir(tmp_path)
    monkeypatch.setenv(f"{src_type.upper()}_TEST_PROFILE", architecture)
    target = Path(f"mips_report_{src_type}_{architecture}.csv")
    other = Path("mips_report_ssrc_vx4b.csv")
    other.touch()
    if report_exists:
        target.touch()

    # Skip builds and golden generation while checking cleanup happens first.
    def check_cleanup(*_args):
        assert not target.exists()

    for name in ("build_host_app_xcommon_cmake", "build_firmware_xcommon_cmake", "gen_golden"):
        monkeypatch.setattr(test_mrhf, name, check_cleanup)

    test_mrhf.test_prepare(src_type)
    assert not target.exists()
    assert other.exists()


def write_reports(directory):
    for src_type in ("asrc", "ssrc"):
        for architecture in ("xs3a", "vx4b"):
            path = directory / f"mips_report_{src_type}_{architecture}.csv"
            with path.open("w", newline="", encoding="utf-8") as report:
                writer = csv.writer(report)
                header = ["input_sr", "output_sr"]
                if src_type == "asrc":
                    header.append("fs_deviation")
                writer.writerow(header + ["profile", "MIPS"])
                for input_index, input_rate in enumerate(RATES):
                    for output_index, output_rate in enumerate(RATES):
                        for profile in ("scalar", "vpu"):
                            base = (10 if profile == "scalar" else 20)
                            if architecture == "vx4b":
                                base += 10
                            base += input_index * 7 + output_index
                            deviations = (
                                DEVIATIONS if src_type == "asrc" else ("",)
                            )
                            for deviation_index, deviation in enumerate(
                                deviations
                            ):
                                row = [input_rate, output_rate]
                                if src_type == "asrc":
                                    row.append(deviation)
                                offset = ("0.1", "0.2", "1.3")[deviation_index]
                                value = base + float(offset)
                                writer.writerow(row + [profile, value])


def run_generator(reports, output, source=None):
    command = [sys.executable, str(SCRIPT), str(reports), str(output)]
    if source is not None:
        command.extend(["--source", source])
    return subprocess.run(
        command,
        capture_output=True, text=True,
    )


def copy_documents(directory):
    for src_type in ("asrc", "ssrc"):
        name = f"resource_usage_{src_type}.rst"
        source = SCRIPT.parents[1] / "rst" / name
        (directory / name).write_text(source.read_text())


def test_generates_tables_from_correct_reports_and_deviation_maximum(tmp_path):
    write_reports(tmp_path)
    copy_documents(tmp_path)
    path = tmp_path / "resource_usage_asrc.rst"
    text = path.read_text().replace(
        "The ASRC uses cascaded",
        "A custom introduction. The ASRC uses cascaded",
    )
    path.write_text(text + "\nA custom conclusion.\n")
    result = run_generator(tmp_path, tmp_path, source="test reports")
    assert result.returncode == 0, result.stderr

    asrc = path.read_text()
    ssrc = (tmp_path / "resource_usage_ssrc.rst").read_text()
    for document in (asrc, ssrc):
        assert document.count(".. list-table::") == 3
        for table in document.split(".. list-table::")[1:]:
            assert "   :width: 100%\n" in table
            assert "   :widths: 22 13 13 13 13 13 13\n" in table
        assert "two-channel instance" in document.replace("\n", " ")
        assert "XS3 measurements" in document
        assert "Report source:" not in document

    assert asrc.startswith("A custom introduction.")
    assert asrc.endswith("\nA custom conclusion.\n")
    assert "archived simulator reports" not in asrc
    scalar, xs3_vpu, vx4_vpu = asrc.split(".. list-table::")[1:]
    assert "   * - 44.1 kHz\n     - 12\n     - 13" in scalar
    assert "   * - 48 kHz\n     - 19\n     - 20" in scalar
    assert "   * - 44.1 kHz\n     - 22\n     - 23" in xs3_vpu
    assert "   * - 44.1 kHz\n     - 32\n     - 33" in vx4_vpu
    assert "0.990099, 1.000000 and 1.009999" in asrc
    assert "first channel" in asrc
    result = run_generator(tmp_path, tmp_path)
    assert result.returncode == 0, result.stderr
    assert path.read_text() == asrc


@pytest.mark.parametrize("corruption, message", [
    ("missing", "missing 1 configurations"),
    ("duplicate", "duplicate configuration"),
    ("nonfinite", "invalid MIPS value"),
])
def test_rejects_incomplete_or_invalid_reports(tmp_path, corruption, message):
    write_reports(tmp_path)
    copy_documents(tmp_path)
    originals = {
        src_type: (tmp_path / f"resource_usage_{src_type}.rst").read_text()
        for src_type in ("asrc", "ssrc")
    }
    report = tmp_path / "mips_report_asrc_xs3a.csv"
    lines = report.read_text().splitlines()
    if corruption == "missing":
        lines.pop()
    elif corruption == "duplicate":
        lines.append(lines[-1])
    else:
        lines[-1] = lines[-1].rsplit(",", 1)[0] + ",NaN"
    report.write_text("\n".join(lines) + "\n")

    result = run_generator(tmp_path, tmp_path)
    assert result.returncode != 0
    assert message in result.stderr
    for src_type, original in originals.items():
        path = tmp_path / f"resource_usage_{src_type}.rst"
        assert path.read_text() == original


@pytest.mark.parametrize("markers", [
    ".. BEGIN GENERATED TABLES\n",
    ".. END GENERATED TABLES\n.. BEGIN GENERATED TABLES\n",
    ".. BEGIN GENERATED TABLES\n.. BEGIN GENERATED TABLES\n"
    ".. END GENERATED TABLES\n",
])
def test_rejects_invalid_table_markers_without_modifying_documents(
    tmp_path, markers
):
    write_reports(tmp_path)
    copy_documents(tmp_path)
    ssrc = tmp_path / "resource_usage_ssrc.rst"
    asrc = tmp_path / "resource_usage_asrc.rst"
    original_asrc = asrc.read_text()
    ssrc.write_text(markers)

    result = run_generator(tmp_path, tmp_path)
    assert result.returncode != 0
    assert "expected one ordered pair of table markers" in result.stderr
    assert asrc.read_text() == original_asrc
    assert ssrc.read_text() == markers
