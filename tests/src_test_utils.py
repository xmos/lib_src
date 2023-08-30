# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import subprocess
import re
import shutil
import soundfile


def gen_tone(name, rate_hz, length_s, frequencies_hz, amplitude_db):
    """ Generate one or more tones and save as 1.31 intereger text 
        as used by the SRC models and test progroams. Relies on sox being installed. """

    assert amplitude_db < 0.0
    if isinstance(frequencies_hz, tuple) or isinstance(frequencies_hz, list):
        freqs = " ".join([f"sin {f}" for f in frequencies_hz])
    else:
        freqs = f"sin {frequencies_hz}"

    # gen using sox
    cmd = f"sox -n -r {rate_hz} -c 1 -b 32 temp.wav synth {length_s} {freqs} gain {amplitude_db}"
    subprocess.run(cmd, shell=True)

    # read as np int 1d array
    data, sr = soundfile.read("temp.wav", dtype=np.int32)
    assert sr == rate_hz

    # save as integers in text
    np.savetxt(name, data, fmt='%i')


class src_mrh_file_name_builder:
    """ Helper to build the input/output/golden filenames from various input output sample rates """

    signal_types = {"pure_sine": "s1k_0dB", "inter_modulation": "im10k11k_m6dB", "inter_modulation_low": "im5k6k_m6dB"}
    file_name_helper = {16000: "16", 44100: "44", 48000: "48", 88200: "88", 96000: "96", 176400: "176", 192000: "192"}

    def test_signal(self, input_sr, signal_type):
        file_name = self.signal_types[signal_type] + "_" + self.file_name_helper[input_sr] + ".dat"
        return file_name

    def output_signal(self, input_sr, output_sr, signal_type):
        file_name = self.signal_types[signal_type] + "_" + self.file_name_helper[input_sr] + "_" + self.file_name_helper[output_sr]
        return file_name

    def get_in_signal_pair(self, input_sr, src_type):
        ch0 = self.test_signal(input_sr, "pure_sine")
        im_type = "inter_modulation" if src_type == "asrc" or src_type == "ssrc" else "inter_modulation_low"
        ch1 = self.test_signal(input_sr, im_type)
        return (ch0, ch1)

    def get_out_signal_pair(self, input_sr, output_sr, src_type, extension, fs_deviation=None):
        if src_type == "asrc":
            suffix = f"_{fs_deviation}"
        else:
            suffix = ""
        im_type = "inter_modulation" if src_type == "asrc" or src_type == "ssrc" else "inter_modulation_low"
        ch0 = self.output_signal(input_sr, output_sr, "pure_sine") + suffix + f".{extension}"
        ch1 = self.output_signal(input_sr, output_sr, im_type) + suffix + f".{extension}"
        return (ch0, ch1)

def gen_golden(host_app, src_type, num_samples_to_process, input_freqs, output_freqs, fs_deviations):
    """ generate golden reference by running the test vector through the compiled 
        model on the host. """

    fnb = src_mrh_file_name_builder()

    file_dir = Path(__file__).resolve().parent
    bin_path = file_dir / f"../build/tests/{src_type}_test/{src_type}_golden" 
    test_in_path = file_dir / "src_input"
    test_out_path = file_dir / "src_output" / src_type
    test_out_path.mkdir(exist_ok=True, parents=True)

    def run_cmd(cmd):
        output = subprocess.run(cmd, input=" ", text=True, shell=True, capture_output=True) # pass an input as the binary expects to press any key at end 
        if output.returncode != 0 or "ERROR" in output.stdout:
            print(f"Error, stdout: {output.stdout}, stderr {output.stderr}")
            assert 0

    for q, out_sr in zip(range(len(output_freqs)), output_freqs):
        for k, in_sr in zip(range(len(input_freqs)), input_freqs):
            for fs_deviation in fs_deviations if src_type == "asrc" else [1.0]:
                print(f"Generating golden {in_sr}->{out_sr} ({fs_deviation})")
                test_signal_0, test_signal_1 = fnb.get_in_signal_pair(in_sr, src_type)
                golden_signal_0, golden_signal_1 = fnb.get_out_signal_pair(in_sr, out_sr, src_type, "golden", fs_deviation=fs_deviation)

                if src_type == "asrc" or src_type == "ssrc":
                    cmd = f"{bin_path} -i{test_in_path/test_signal_0} -j{test_in_path/test_signal_1} -k{k}"
                    if src_type == "ssrc":
                        cmd += f" -o{test_out_path/golden_signal_0} -p{test_out_path/golden_signal_1}"
                    else:
                        op_path = test_out_path / f"{fs_deviation}"
                        op_path.mkdir(exist_ok=True, parents=True)
                        cmd += f" -o{op_path/golden_signal_0} -p{op_path/golden_signal_1}"
                    cmd += f" -q{q} -d0 -l{num_samples_to_process} -n4"
                    if src_type == "asrc":
                        cmd += f" -e{fs_deviation}"
                    run_cmd(cmd)
                else:
                    cmd = f"{bin_path} -i{test_in_path/test_signal_0} -o{test_out_path/golden_signal_0}"
                    run_cmd(cmd)
                    cmd = f"{bin_path} -i{test_in_path/test_signal_1} -o{test_out_path/golden_signal_1}"
                    run_cmd(cmd)
                    

def max_mips_fron_std_out(output, in_sr, timer_hz = 100e6, cpu_hz = 600e6, num_threads = 5, get_max_instr_per_samp=False):
    max_mips = 0.0
    max_instr_per_samp = 0
    for line in output:
        found = re.search(r".*Process time per chan ticks=(\d+).*", line)
        if found:
            process_time_ticks =int(found.group(1))

            instr_per_sample = process_time_ticks * cpu_hz / timer_hz / num_threads
            max_instr_per_samp = max_instr_per_samp if max_instr_per_samp > instr_per_sample else instr_per_sample


            mips = instr_per_sample * in_sr / 1e6
            max_mips = mips if mips > max_mips else max_mips

    return max_instr_per_samp if get_max_instr_per_samp else max_mips

def run_dut(in_sr, out_sr, src_type, num_samples_to_process, fs_deviation=None):
    """ Run the test vector through the compiled fimrware application and compare with 
        the output generated by the model on the host. The MIPS output from the firmware
        is also scraped and reported in a log file for later collation. """

    file_dir = Path(__file__).resolve().parent
    tmp_dir = file_dir / "tmp" / src_type / f"{in_sr}_{out_sr}"
    if src_type == "asrc":
        tmp_dir = tmp_dir / f"{fs_deviation}"
    tmp_dir.mkdir(exist_ok=True, parents=True)

    # When running parallel, make copies of files so we don't encounter issues using xdist
    bin_name = f"test_{src_type}.xe"
    bin_path = file_dir / f"../build/tests/{src_type}_test" / bin_name
    shutil.copy(bin_path, tmp_dir / bin_name)

    test_in_path = file_dir / "src_input"
    test_out_path = file_dir / "src_output" / src_type
    if src_type == "asrc":
        test_out_path = test_out_path / fs_deviation
    test_out_path.mkdir(exist_ok=True)

    fnb = src_mrh_file_name_builder()

    if src_type == "ds3" or src_type == "os3":
        input_signal_0 = fnb.test_signal(in_sr, "pure_sine")
        input_signal_1 = fnb.test_signal(in_sr, "inter_modulation_low")
        shutil.copy(test_in_path / input_signal_0, tmp_dir / input_signal_0)
        shutil.copy(test_in_path / input_signal_1, tmp_dir / input_signal_1)
        cmd = f"xsim {bin_name}"
        output = subprocess.run(cmd, shell=True, capture_output=True, text=True, cwd=tmp_dir)
        if output.returncode != 0:
            assert 0, f"Error, stdout: {output.stdout}, sterr: {output.stderr}, running: {cmd}"

        golden_signal_0, golden_signal_1 = fnb.get_out_signal_pair(in_sr, out_sr, src_type, "golden", "1.0")
        assert(compare_results_ff3(output.stdout, test_out_path / golden_signal_0, test_out_path / golden_signal_1))
        
        return

    input_signal_0, input_signal_1 = fnb.get_in_signal_pair(in_sr, src_type)
    shutil.copy(test_in_path / input_signal_0, tmp_dir / input_signal_0)
    shutil.copy(test_in_path / input_signal_1, tmp_dir / input_signal_1)

    dut_signal_0, dut_signal_1 = fnb.get_out_signal_pair(in_sr, out_sr, src_type, "dut", fs_deviation=fs_deviation)
    
    cmd = f"xsim --args {str(tmp_dir / bin_name)}"
    cmd += f" -i {tmp_dir / input_signal_0} {tmp_dir / input_signal_1}"
    cmd += f" -o {tmp_dir / dut_signal_0} {tmp_dir / dut_signal_1}"
    cmd += f" -f {in_sr} -g {out_sr} -n {num_samples_to_process}"
    if src_type == "asrc":
        cmd += f" -e {fs_deviation}"
    print(f"Generating dut {in_sr}->{out_sr}")
    output = subprocess.run(cmd, shell=True, capture_output=True, text=True)

    if output.returncode != 0:
        assert 0, f"Error, stdout: {output.stdout}, sterr: {output.stderr}, running: {cmd}"

    golden_signal_0, golden_signal_1 = fnb.get_out_signal_pair(in_sr, out_sr, src_type, "golden", fs_deviation)

    shutil.copy(test_out_path / golden_signal_0, tmp_dir / golden_signal_0)
    shutil.copy(test_out_path / golden_signal_1, tmp_dir / golden_signal_1)

    print(f"comparing {golden_signal_0} and {dut_signal_0}")

    assert(compare_results(tmp_dir / golden_signal_0, tmp_dir / dut_signal_0))
    assert(compare_results(tmp_dir / golden_signal_1, tmp_dir / dut_signal_1))

    # shutil.copy(tmp_dir / test_signal_0, test_out_path / test_signal_0)
    # shutil.copy(tmp_dir / test_signal_1, test_out_path / test_signal_1)

    max_mips = max_mips_fron_std_out(output.stdout.split("\n"), in_sr)

    with open(tmp_dir / f"max_mips.txt", "wt") as mf:
        if src_type == "ssrc":
            line = f"{in_sr}->{out_sr}:{max_mips}"
        else:
            line = f"{in_sr}->{out_sr},{fs_deviation}:{max_mips}"
        mf.write(line)
        print(f"max_mips {line}")

def compare_results(golden_file, dut_file, isclose=False):
    golden = np.loadtxt(golden_file, dtype=np.int32)
    dut = np.loadtxt(dut_file, dtype=np.int32)

    return array_compare_1d(golden, dut)

def compare_results_ff3(stdout, golden_signal_0, golden_signal_1):
    #remove non-data prints
    str_data = ",".join(stdout.split('\n')[2:])
    dut_data = np.fromstring(str_data, dtype=np.int32, sep=',').reshape((-1, 2)) # two channels of test data
    golden_0 = np.loadtxt(golden_signal_0, dtype=np.int32)
    golden_1 = np.loadtxt(golden_signal_1, dtype=np.int32)

    same = array_compare_1d(golden_0, dut_data[:,0], allow_different_lengths=True) 
    same &= array_compare_1d(golden_1, dut_data[:,1], allow_different_lengths=True)

    return same  

def build_firmware(target, extra_args=""):
    file_dir = Path(__file__).resolve().parent
    build_path = file_dir / "../build" 
    build_path.mkdir(exist_ok=True)
    subprocess.run("rm -rf CMakeCache.txt CMakeFiles/", shell=True, cwd=str(build_path))
    subprocess.run(f"cmake --toolchain ../xmos_cmake_toolchain/xs3a.cmake {extra_args} ..", shell=True, cwd=str(build_path))
    subprocess.run(f"make -j {target}", shell=True, cwd=str(build_path))

    return target + ".xe"

def build_host_app(target):
    file_dir = Path(__file__).resolve().parent
    build_path = file_dir / "../build" 
    build_path.mkdir(exist_ok=True)
    subprocess.run("rm -rf CMakeCache.txt CMakeFiles/", shell=True, cwd=str(build_path))
    subprocess.run("cmake  ..", shell=True, cwd=str(build_path))
    bin_path = file_dir / f"{target}_/model" 
    subprocess.run(f"make {target}_golden",  shell=True, cwd=str(build_path))

    return f"{target}_golden"

def generate_mips_report(src_type):
    mips_report_filename = f"mips_report_{src_type}.csv"
    with open(mips_report_filename, "w") as mips_report:
        if src_type == "ssrc":
            mips_report.write("input_sr, output_sr, MIPS\n")
        else:
            mips_report.write("input_sr, output_sr, fs_deviation, MIPS\n")
        file_list = subprocess.run(f'find tmp/{src_type} -iname "max_mips.txt"', shell=True, capture_output=True, text=True).stdout
        for mips_file in file_list.split():
            with open(mips_file) as mf:
                if src_type == "ssrc":
                    vals = re.search(r'(\d+)->(\d+):([\d.]+)', mf.readlines()[0])
                    mips_report.write(f"{vals.group(1)}, {vals.group(2)}, {vals.group(3)}\n")
                else:
                    vals = re.search(r'(\d+)->(\d+),([\d.]+):([\d.]+)', mf.readlines()[0])
                    mips_report.write(f"{vals.group(1)}, {vals.group(2)}, {vals.group(3)}, {vals.group(4)}\n")

def array_compare_1d(array_a, array_b, rtol=None, atol=None, close=False, max_print=200, save_comparison_file=False, allow_different_lengths=False):
    """ Do numpy compare except give useful debug if fails

        array_a - first comparision array
        array_b - second comparision array
        rtol - relative tolernace. If rtol and atol are None, an exact comparison is made. If either is not None then it uses the supplied value or np default if not specified.
        atol - absolute tolerance. See above.
        close - allows a closeness test rather exact test. Use this if you don't want to supply rtol and atol, in which case it uses np default
        max_print - if differing values are found, it will print the diff up to this count
        save_comparison_file - optionally save a wave file with the two comparison data with the name passed. This will succeed even if the array lengths are different
        allow_different_lengths - optionally allows largest array to be trimmed down to smallest array size (chops end off, start is unchanged)
    """
    shape_a = array_a.shape
    shape_b = array_b.shape

    num_axes = len(shape_a)
    size = array_a.size

    assert num_axes == 1, f"multidimensional arrays not yet supported"

    if save_comparison_file is not False:
        largest = array_a.size if array_a.size > array_b.size else array_b.size
        dtype = type(array_a[0])
        comparison_data = np.zeros((largest, 2), dtype=dtype)
        comparison_data[:array_a.size, 0] = array_a
        comparison_data[:array_b.size, 1] = array_b

        soundfile.write(save_comparison_file, comparison_data, 16000)

    if allow_different_lengths is True:
        if array_a.size > array_b.size:
            array_a = array_a[:array_b.size]
        else:
            array_b = array_b[:array_a.size]
    else:
        assert shape_a == shape_b, f"Compared arrays have differing shapes: {shape_a} {shape_b}"

    # array equality
    if rtol == None and atol == None and close == False:
        passed = True
        if not np.array_equal(array_a, array_b, num_axes):
            print(f"ERROR: in array_compare_1d (exact):")
            diff_count = 0
            for idx in range(size):
                if array_a[idx] != array_b[idx]:
                    print(f"arrays differ at idx {idx}: {array_a[idx]}, {array_b[idx]}")
                    passed = False
                    diff_count += 1
                    if diff_count == max_print:
                        print(f"reached {max_print} diffs, giving up....")
                        return False
            print("All other values equal.")
        return passed

    # array closeness
    else:
        if rtol is None:
            rtol=1e-05  # numpy default https://numpy.org/doc/stable/reference/generated/numpy.allclose.html
        if atol is None:
            atol=1e-08  # numpy default
        passed = True
        if not np.allclose(array_a, array_b, rtol=rtol, atol=atol):
            print(f"ERROR: in array_compare_1d (close) rtol: {rtol} atol: {atol}")
            diff_count = 0
            for idx in range(size):
                if not np.allclose(array_a[idx], array_b[idx], rtol=rtol, atol=atol):
                    ratio = abs(array_a[idx]) / abs(array_b[idx]) - 1 if abs(array_a[idx]) > abs(array_b[idx]) else abs(array_b[idx]) / abs(array_a[idx]) - 1
                    diff = abs(array_a[idx] - array_b[idx])
                    print(f"arrays differ at idx {idx}: {array_a[idx]}, {array_b[idx]} - ratio: {ratio} diff: {diff}")
                    passed = False
                    diff_count += 1
                    if diff_count == max_print:
                        print(f"reached {max_print} diffs, giving up....")
                        return False
            print("All other values close.")

        return passed

# For test only
if __name__ == "__main__":
    gen_tone("t1.dat", 48000, 10, 1000, -6)
    gen_tone("t2.dat", 48000, 10, [1000, 2000], -6)