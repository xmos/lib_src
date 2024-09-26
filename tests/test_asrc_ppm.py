# Copyright 2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""
Characterise the ASRC for latency, FIFO lock etc.
Not intended to be run by Jenkins - this is here for characterisation run manually
Requires any xcore.ai hardware to be attached
These scripts take half a day or so to run due to the large combinatorial space of SR * SR * PPM
"""

import pytest
import subprocess
from src_test_utils import vcd2wav, build_firmware_xcommon_cmake
import numpy as np
from scipy.io import wavfile
import matplotlib.pyplot as plt
import itertools
from thdncalculator import THDN_and_freq
import csv


SR_LIST = (44100, 48000, 88200, 96000, 176400, 192000)


def build_vcd_to_csv():
    path = "asrc_task_test_ppm/host"
    bin_name = "vcd_to_csv"
    cmd = f"gcc -O3 vcd_to_csv.c -o {bin_name}"
    print(f"running: {cmd}") # so we can try locally
    output = subprocess.run(cmd, shell=True, capture_output=True, check=True, text=True, cwd=path)

    return path + "/" + bin_name

def run_dut(bin_name, cmds, timeout=60, xscope=True):
    """
    Runs DUT with a set of frequency commands. See asrc_task_test_ppm.xc for expected format
    """
    print("Running DUT")
    print(cmds)
    flattend_cmds = ' '.join(str(x)for x in list(itertools.chain(*cmds)))

    if xscope:
        xscope_arg = "--xscope-file trace"
    else:
        xscope_arg = "--io"

    cmd = f"xrun --id 0 {xscope_arg} --args {bin_name} {flattend_cmds}"
    print(f"running: {cmd}") # so we can try locally
    output = subprocess.run(cmd, shell=True, capture_output=True, check=True, text=True, timeout=timeout)
    assert output.returncode == 0

    return output.stderr #xrun puts output on err for some reason


# @pytest.fixture(scope="module")
def build_xes():
    print("Building DUT")
    xes = {}
    for build in ["STEP", "SINE"]:
        xes[build] = build_firmware_xcommon_cmake("asrc_task_test_ppm", config=build)

    return xes


def analyse_thd(data, in_sr, out_sr, max_thd=None):
    start_chop_s = 2.0

    audio_vcd = data[1:, 1]
    # Skip repeats you get in VCD
    mask = np.append(True, audio_vcd[1:] != audio_vcd[:-1])
    data = audio_vcd[mask].astype(np.int32)

    data = (data[int(out_sr*start_chop_s):] / ((1<<31) - 1)).astype(np.float64)
    thd, freq = THDN_and_freq(data, out_sr)

    wavfile.write("thd.wav", out_sr, data)

    expected_freq = (44100 / 44) * in_sr / 44100
    print(F"THD: {thd:.2f}, freq: {freq:.4f}, expected_freq: {expected_freq:.4f}")

    if max_thd is not None:
        assert thd < max_thd
        assert np.isclose(freq, expected_freq, rtol = 0.0001)

    return thd

def apply_ppm(freq, ppm):
    freq += freq * (ppm / 1e6)
    return int(freq + 0.5)

def plot_trace(data, name):
    data = np.array(data)

    xpoints = np.arange(0, data.size)
    ypoints = np.array(data)

    plt.title(name)
    plt.plot(xpoints, ypoints)
    plt.savefig(name + ".png")
    # plt.show()
    plt.clf()

def analyse_lock(data, sr_out):
    data = np.array(data, dtype=np.float64)

    # Discard the junk at startup by zeroing it
    clip_value = np.max(np.sqrt(np.square(data))) / 2
    pos_clip_idxs = data > clip_value
    neg_clip_idxs = data < -clip_value
    data[pos_clip_idxs] = 0
    data[neg_clip_idxs] = 0

    # now flip if negative so we have a positive curve
    if np.max(data) < -np.min(data):
        data = -data

    plot_trace(data, "phase_plot")


    #grab part after overshoot (if present)
    peak_idx = np.argmax(data) # always positive
    peak_phase_diff = np.max(data)

    window_size = sr_out // 10 # 100ms
    phase_lock_threshold = 100
    lock_acheived_at = False
    for idx in range(window_size, data.size - window_size, window_size // 2): # step through with 50% overlap. Skip first one
        windowed = data[idx:idx+1000]
        ave_phase = np.mean(windowed)
        if abs(ave_phase) < phase_lock_threshold and np.max(windowed) < phase_lock_threshold:
            lock_acheived_at = idx
            time_lock = idx / sr_out
            print(f"Lock acheived at: {time_lock:.2f}s")
            break

    return lock_acheived_at

def analyse_latency(data, sample_rate, fifo_len, specialised=True): # specialised means do the work expected by test_asrc_range()
    data =np.array(data)
    if specialised:
        zero_initial_data_end = sample_rate // 100 # Zero first 10ms to allow init filters to empty
        data[:zero_initial_data_end] = 0
    step_seen = np.argmax(data > 0x60000000 // 2) # when step reaches half of the input step
    if not specialised:
        return step_seen

    step_delay = sample_rate * 9

    latency_total = step_seen - step_delay
    latency_fifo = fifo_len / 2
    latency_filter = latency_total - latency_fifo


    print("FIFO LATENCY", latency_fifo)
    print("FILTER LATENCY", latency_filter)

    return latency_filter

def analyse_fifo(data, sr_in, sr_out, fifo_len, ppm):

    # print(data)
    data =np.array(data)
    data -= int(fifo_len) // 2
    plot_trace(data, f"FIFO_depth_relative_to_half_sr_in:{sr_in}_sr_out:{sr_out}_PPM:{ppm}")

    # Final depth
    final_excursion = np.mean(data[-100:])

    # peak analysis
    min_fifo = np.min(data)
    max_fifo = np.max(data)

    if max_fifo > -min_fifo:
        peak_fifo = max_fifo
    else:
        peak_fifo = -min_fifo

    print(f"Peak FIFO excursion: {peak_fifo} (max: {fifo_len//2}) - min {min_fifo}, max {max_fifo}. Final: {final_excursion}")

    return int(peak_fifo)

def characterise_asrc_fn_latency():
    """
    Measures the raw filter latency across all SRs
    Needs an xcore ai target attached (any)
    Takes 10 mins or so. Needs to only be run again if the filters change
    """
    build_xe = "asrc_test/bin/asrc_test.xe"
    num_samples = 300
    step_delay = 100

    def gen_step_dat(sample_rate, num_samples, step_delay):
        step = np.zeros(num_samples, dtype=np.int32)
        step[step_delay:] = 0x60000000 # 3/4 FSD
        filename = "step_in.dat"
        step.tofile(filename, sep="\n")
        return filename

    results = []
    for sr_in in SR_LIST:
        input_file = gen_step_dat(sr_in, num_samples, step_delay)
        output_file = "step_out.dat"
        for sr_out in SR_LIST:
            sr_ratio = sr_in / sr_out
            cmd_list = [f"-i {input_file} -o {output_file} -f {sr_in} -g {sr_out} -n {num_samples} -e {sr_ratio}".split(" ")]
            output = run_dut(build_xe, cmd_list, timeout=20, xscope=False) # Time to run + enough time for XTAG to come up
            out_data = np.fromfile(output_file, sep="\n", dtype=np.int32)
            latency_at_output_sr = analyse_latency(out_data, sr_out, 0, specialised=False)
            latency_ms = ((latency_at_output_sr / sr_out) - (step_delay / sr_in)) * 1000
            print("Latency of fn:", sr_in, sr_out, float(latency_ms))
            results.append([sr_in, sr_out, float(latency_ms)])

    # print nicely
    print("\n\t", "\t".join([str(sr) for sr in SR_LIST]))
    idx = 0
    for sr_in in SR_LIST:
        line = str(sr_in)
        for sr_out in SR_LIST:
            line += f"\t{results[idx][2]:.3f}"
            idx += 1
        print(line)


def characterise_asrc_range(input_sr):
    """
    This runs a slightly longer test at a single frequency and checks THDN
    Needs an xcore ai target attached (any)
    """
    # mode = "STEP" # CHecks latency only by running a step fn through the system. This has been supersceded by characterise_asrc_fn_latency()
    mode = "SINE" # Checks THDN (ie. FIFO works without resets) and peak FIFO excursion (normally at startup)


    build_xe = build_xes()[mode]
    vcd_bin = build_vcd_to_csv()

    ppm_list = [-5000, -2000, -1000, -500, -250, 0, 250, 500, 1000, 2000, 5000]
    fifo_len_list = [20, 40, 80, 160, 320, 640, 1280]

    test_len_s = 6
    filenames = {"SINE": f"sweep_fifo_required_{input_sr}.csv",
                "STEP":"sweep_filter_latency.csv"}
    paramkeys = {"SINE":['sr_in', 'sr_out', 'ppm', 'fifo_len', 'thd', 'lock_time', 'peak_fifo'],
                "STEP":['sr_in', 'sr_out', 'ppm', 'latency_filter']}
    parameters = {}
    with open(filenames[mode], 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=paramkeys[mode])
        writer.writeheader()

        # for sr_in in SR_LIST:
        for sr_in in [input_sr]:
            for sr_out in SR_LIST:
                for ppm in ppm_list if mode == "SINE" else [0]:
                    # for fifo_len in fifo_len_list if mode == "SINE" else [fifo_len_list[-1]]:
                    for fifo_len in [fifo_len_list[-1]]:
                        cmd_list = [[sr_in, sr_out, apply_ppm(sr_out, ppm), test_len_s, fifo_len]]
                        print() # newline
                        output = run_dut(build_xe, cmd_list, timeout=test_len_s + 10) # add enough time for XTAG to come up
                        print("Analysing...")

                        subprocess.run(f"./{vcd_bin} trace.vcd trace.csv", shell=True)
                        data = np.loadtxt("trace.csv", delimiter=",", dtype=str)

                        parameters['sr_in'] = sr_in
                        parameters['sr_out'] = sr_out
                        parameters['ppm'] = ppm

                        if mode == "SINE":

                            thd = analyse_thd(data, sr_in, sr_out)

                            # if thd < -60:
                            if True:
                                # helper to sort out repeated values in
                                def vcd_to_sequence(data, idx, sr_out):
                                    times = data[1:,0]
                                    length = times.shape[0]
                                    time = int(times[4]) # skip first few to miss zero
                                    extract = []
                                    # print(f"length to search: {length}")
                                    for i in range(length):
                                        if int(times[i]) >= time:
                                            extract.append(data[i + 1, idx])
                                            time += 100000000 / sr_out

                                    return np.array(extract, dtype=np.int32)

                                phase_data = vcd_to_sequence(data, 2, sr_out)
                                lock_time = analyse_lock(phase_data, sr_out)
                                fifo_data = vcd_to_sequence(data, 3, sr_out)
                                peak_fifo = analyse_fifo(fifo_data, sr_in, sr_out, fifo_len, ppm)

                                parameters['fifo_len'] = fifo_len
                                parameters['thd'] = int(thd)
                                parameters['lock_time'] = lock_time
                                parameters['peak_fifo'] = peak_fifo
                                print(parameters)
                                writer.writerow(parameters)
                                break

                        if mode == "STEP":
                            latency_filter = analyse_latency(data[0], sr_out, fifo_len)
                            parameters['latency_filter'] = int(latency_filter)
                            print(parameters)
                            writer.writerow(parameters)

###################
# PLOTTING UTILS
###################
def plot_required_fifo_len(sr_in):

    # Load data from CSV
    sr_out = []
    ppm = []
    peak_fifo = []

    with open(f'sweep_fifo_required_{sr_in}.csv', 'r') as csvfile:
        csvreader = csv.reader(csvfile)
        header = next(csvreader)
        # print(header)
        for row in csvreader:
            if row[header.index("sr_in")] == str(sr_in):
                sr_out.append(int(row[header.index("sr_out")]))
                ppm.append(int(row[header.index("ppm")]))
                peak_fifo.append(int(row[header.index("peak_fifo")]))

    # Convert lists to numpy arrays
    sr_out = np.array(sr_out)
    ppm = np.array(ppm)
    z_base = np.zeros_like(sr_out)  # The base of the bars (z=0)
    column_size = 500
    dx = np.ones_like(sr_out) * column_size
    dy = np.ones_like(ppm) * column_size
    dz = np.array(peak_fifo)

    # Create the 3D plot
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')

    # Create 3D bar chart
    ax.bar3d(sr_out, ppm, z_base, dx, dy, dz, color='b', zsort='average')
    ax.view_init(elev=36, azim=160)

    # Set labels
    ax.set_title(f"FIFO length needed for SR in: {sr_in}")
    ax.set_xlabel('sr_out')
    ax.set_ylabel('ppm')
    ax.set_zlabel('peak fifo excursion')

    plt.savefig(f"FIFO_needed_for_SR_in_{sr_in}" + ".png")
    # plt.show()

def plot_filter_latency():

    # Load data from CSV
    sr_in = []
    sr_out = []
    latency_ms = []

    with open('sweep_filter_latency.csv', 'r') as csvfile:
        csvreader = csv.reader(csvfile)
        header = next(csvreader)
        print(header)
        for row in csvreader:
            sr_in.append(int(row[header.index("sr_in")]))
            sample_rate_out = int(row[header.index("sr_out")])
            sr_out.append(sample_rate_out)
            print(row[header.index("latency_filter")])
            latency_in_milliseconds = float(row[header.index("latency_filter")]) / sample_rate_out * 1000
            latency_ms.append(latency_in_milliseconds)

    print(latency_ms)

    # Convert lists to numpy arrays
    sr_out = np.array(sr_out)
    sr_in = np.array(sr_in)
    z_base = np.zeros_like(latency_ms)  # The base of the bars (z=0)
    column_size = 1000
    dx = np.ones_like(sr_out) * column_size
    dy = np.ones_like(sr_in) * column_size
    dz = np.array(latency_ms)

    # Create the 3D plot
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')

    # Create 3D bar chart
    ax.bar3d(sr_out, sr_in, z_base, dx, dy, dz, color='b', zsort='average')
    ax.view_init(elev=27, azim=31)

    # Set labels
    ax.set_title(f"ASRC Filter latency across SR")
    ax.set_xlabel('sr_out')
    ax.set_ylabel('sr_in')
    ax.set_zlabel('milliseconds')

    # plt.show()


# For local test only
if __name__ == "__main__":
    # characterise_asrc_fn_latency()
    for in_sr in SR_LIST:
        # characterise_asrc_range(in_sr)
        plot_required_fifo_len(in_sr)
