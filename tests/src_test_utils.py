# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import subprocess
import re

def graph_it(filename, freqs, y_axis_name, horiz_line=None, title_suffix=None):
    plt.clf()

    # Plot the data
    x_axis = [x / sof_freq for x in range(len(freqs))]
    y_axis = freqs

    plt.plot(x_axis, y_axis)
    if horiz_line is not None:
        plt.axhline(y=horiz_line, color='r', linestyle='-')
    title = 'PI controller'
    title +=  "" if title_suffix is None else f"{title_suffix}"
    plt.title(title)
    plt.xlabel('Seconds')
    plt.ylabel(y_axis_name)
    filename = filename if title_suffix is None else filename.replace(".", f"{title_suffix}.").replace(" ", "_")
    plt.savefig(filename, dpi=300)
    # plt.show()

def plot_histogram(filename, data):
    plt.clf()
    plt.hist(data)
    plt.title("Final frequency PPM at end of simulation")
    plt.xlabel("PLL Frequency PPM")
    plt.ylabel("Occurance")
    plt.savefig(filename, dpi=300)

class src_mrh_file_name_builder:
    """Helper to build the input/output/golden filenames from various input output sample rares"""

    signal_types = {"pure_sine": "s1k_0dB", "inter_modulation": "im10k11k_m6dB"}
    file_name_helper = {44100: "44", 48000: "48", 88200: "88", 96000: "96", 176400: "176", 192000: "192"}

    def test_signal(self, input_sr, signal_type):
        file_name = self.signal_types[signal_type] + "_" + self.file_name_helper[input_sr] + ".dat"
        return file_name

    def output_signal(self, input_sr, output_sr, signal_type):
        file_name = self.signal_types[signal_type] + "_" + self.file_name_helper[input_sr] + "_" + self.file_name_helper[output_sr]
        return file_name

    def get_in_signal_pair(self, input_sr):
        ch0 = self.test_signal(input_sr, "pure_sine")
        ch1 = self.test_signal(input_sr, "inter_modulation")
        return (ch0, ch1)

    def get_out_signal_pair(self, input_sr, output_sr, extension):
        ch0 = self.output_signal(input_sr, output_sr, "pure_sine") + f".{extension}"
        ch1 = self.output_signal(input_sr, output_sr, "inter_modulation") + f".{extension}"
        return (ch0, ch1)

def compare_results(golden_file, dut_file, isclose=False):
    golden = np.loadtxt(golden_file, dtype=np.int32)
    dut = np.loadtxt(golden_file, dtype=np.int32)

    same = True
    if(isclose):
        same = np.all_close(golden, dut, rtol=2)
    else:
        same = np.array_equal(golden, dut)
    return same

def build_firmware(targets):
    file_dir = Path(__file__).resolve().parent
    build_path = file_dir / "../build" 
    build_path.mkdir(exist_ok=True)
    subprocess.run("rm -rf CMakeCache.txt CMakeFiles/", shell=True, cwd=str(build_path))
    subprocess.run("cmake --toolchain ../xmos_cmake_toolchain/xs3a.cmake  ..", shell=True, cwd=str(build_path))
    
    for target in targets:
        subprocess.run(f"make -j {target}_test", shell=True, cwd=str(build_path))
    return [target + ".xe" for target in targets]

def build_host_app(targets):
    file_dir = Path(__file__).resolve().parent
    build_path = file_dir / "../build" 
    build_path.mkdir(exist_ok=True)
    subprocess.run("rm -rf CMakeCache.txt CMakeFiles/", shell=True, cwd=str(build_path))
    subprocess.run("cmake  ..", shell=True, cwd=str(build_path))

    for target in targets:
        bin_path = file_dir / f"{target}_/model" 
        subprocess.run(f"make {target}_golden",  shell=True, cwd=str(build_path))


def generate_mips_report(src_type):
    mips_report_filename = f"mips_report_{src_type}.csv"
    with open(mips_report_filename, "w") as mips_report:
        mips_report.write("input_sr, output_sr, MIPS\n")
        file_list = subprocess.run(f'find tmp/{src_type} -iname "max_mips.txt"', shell=True, capture_output=True, text=True).stdout
        for mips_file in file_list.split():
            with open(mips_file) as mf:
                vals = re.search(r'(\d+)->(\d+):([\d.]+)', mf.readlines()[0])
                mips_report.write(f"{vals.group(1)}, {vals.group(2)}, {vals.group(3)}\n")

if __name__ == "__main__":
    print("test")