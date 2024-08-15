# Copyright 2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""
Characterise the ASRC for latency, FIFO lock etc.
Not intended to be run by Jenkins - this is here for characterisation run manually
"""

import pytest
import subprocess
from src_test_utils import vcd2wav, build_firmware_xccm
import numpy as np
from scipy.io import wavfile
import matplotlib.pyplot as plt
import itertools
from thdncalculator import THDN_and_freq
import csv


SR_LIST = (44100, 48000, 88200, 96000, 176400, 192000)


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
        xes[build] = build_firmware_xccm("asrc_task_test_ppm", build=build)

    return xes


def analyse_thd(wav_file, in_sr, max_thd=None):
    sample_rate, data = wavfile.read(wav_file)
    start_chop_s = 4.0
    data = (data[int(sample_rate*start_chop_s):] / ((1<<31) - 1)).astype(np.float64)
    thd, freq = THDN_and_freq(data, sample_rate)

    expected_freq = (44100 / 44) * in_sr / 44100
    print(F"THD: {thd:.2f}, freq: {freq:.4f}, expected_freq: {expected_freq:.4f}")

    if max_thd is not None:
        assert thd < max_thd
        assert np.isclose(freq, expected_freq, rtol = 0.0001)

    return thd

def apply_ppm(freq, ppm):
    freq += freq * (ppm / 1e6)
    return int(freq + 0.5)

def plot_phase(data, name):
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

    plot_phase(data, "phase_plot")


    #grab part after overshoot (if present)
    peak_idx = np.argmax(data) # always positive
    peak_phase_diff = np.max(data)

    window_size = 1000
    phase_lock_threshold = 20
    lock_acheived_at = False
    for idx in range(0, data.size - window_size, window_size // 2): # step through with 50% overlap
        ave_phase = np.mean(data[idx:idx+1000])
        if abs(ave_phase) < phase_lock_threshold:
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
    plot_phase(data, f"FIFO_depth_relative_to_half_sr_in:{sr_in}_sr_out:{sr_out}_PPM:{ppm}")

    min_fifo = np.min(data)
    max_fifo = np.max(data)

    if max_fifo > -min_fifo:
        peak_fifo = max_fifo
    else:
        peak_fifo = -min_fifo

    print(f"Peak FIFO excursion: {peak_fifo} (max: {fifo_len//2}) - min {min_fifo}, max {max_fifo}")

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


def characterise_asrc_range():
    """
    This runs a slightly longer test at a single frequency and checks THDN
    Needs an xcore ai target attached (any)
    """
    # mode = "STEP" # CHecks latency only by running a step fn through the system. This has been supersceded by characterise_asrc_fn_latency()
    mode = "SINE" # Checks THDN (ie. FIFO works without resets) and peak FIFO excursion (normally at startup)


    build_xe = build_xes()[mode]

    # ppm_list = [-4000, -2000, -1000, -500, -250, 0, 250, 500, 1000, 2000, 4000]
    # ppm_list = [-2000, 2000]
    ppm_list = [-100, 100]
    fifo_len_list = [20, 40, 80, 160, 320, 640, 1280]

    test_len_s = 6
    filenames = {"SINE": "sweep_fifo_required.csv",
                "STEP":"sweep_filter_latency.csv"}
    paramkeys = {"SINE":['sr_in', 'sr_out', 'ppm', 'fifo_len', 'thd', 'lock_time', 'peak_fifo'],
                "STEP":['sr_in', 'sr_out', 'ppm', 'latency_filter']}
    parameters = {}
    with open(filenames[mode], 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=paramkeys[mode])
        writer.writeheader()

        # for sr_in in SR_LIST:
        for sr_in in [48000]:
            for sr_out in SR_LIST:
            # for sr_out in [192000]:
                for ppm in ppm_list if mode == "SINE" else [0]:
                    # for fifo_len in fifo_len_list if mode == "SINE" else [fifo_len_list[-1]]:
                    for fifo_len in [fifo_len_list[-1]]:
                    # for fifo_len in [16]:
                        cmd_list = [[sr_in, sr_out, apply_ppm(sr_out, ppm), test_len_s, fifo_len]]
                        print() # newline
                        output = run_dut(build_xe, cmd_list, timeout=test_len_s + 10) # add enough time for XTAG to come up
                        print("Analysing...")
                        chan_start = 0
                        chan_stop = 1
                        data = vcd2wav("trace.vcd", chan_start, chan_stop, sr_out)
                        parameters['sr_in'] = sr_in
                        parameters['sr_out'] = sr_out
                        parameters['ppm'] = ppm

                        if mode == "SINE":
                            thd = analyse_thd(f"ch{chan_start}-{chan_stop}-{sr_out}.wav", sr_in)

                            # if thd < -60:
                            if True:
                                lock_time = analyse_lock(data[chan_stop], sr_out)
                                peak_fifo = analyse_fifo(data[chan_stop + 1], sr_in, sr_out, fifo_len, ppm)

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
def plot_required_fifo_len():
    sr_in = 44100

    # Load data from CSV
    sr_out = []
    ppm = []
    peak_fifo = []

    with open('sweep_fifo_required.csv', 'r') as csvfile:
        csvreader = csv.reader(csvfile)
        header = next(csvreader)
        print(header)
        for row in csvreader:
            if row[header.index("sr_in")] == str(sr_in):
                sr_out.append(int(row[header.index("sr_out")]))
                ppm.append(int(row[header.index("ppm")]))
                peak_fifo.append(int(row[header.index("peak_fifo")]))
                print(row)

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

    plt.show()

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
            print(row)

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




# For local test only
if __name__ == "__main__":
    # characterise_asrc_fn_latency()
    characterise_asrc_range()
    # plot_required_fifo_len()