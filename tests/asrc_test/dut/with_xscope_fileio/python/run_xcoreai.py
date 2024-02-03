# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import xscope_fileio
import argparse
import shutil
import subprocess
import scipy.io.wavfile
import pathlib
import numpy as np
from pathlib import Path

def get_adapter_id():
    try:
        xrun_out = subprocess.check_output(['xrun', '-l'], text=True, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        print('Error: %s' % e.output)
        assert False

    xrun_out = xrun_out.split('\n')
    # Check that the first 4 lines of xrun_out match the expected lines
    expected_header = ["", "Available XMOS Devices", "----------------------", ""]
    if len(xrun_out) < len(expected_header):
        raise RuntimeError(
            f"Error: xrun output:\n{xrun_out}\n"
            f"does not contain expected header:\n{expected_header}"
        )

    header_match = True
    for i, expected_line in enumerate(expected_header):
        if xrun_out[i] != expected_line:
            header_match = False

    if not header_match:
        raise RuntimeError(
            f"Error: xrun output header:\n{xrun_out[:4]}\n"
            f"does not match expected header:\n{expected_header}"
        )

    try:
        if "No Available Devices Found" in xrun_out[4]:
            raise RuntimeError(f"Error: No available devices found\n")
            return
    except IndexError:
        raise RuntimeError(f"Error: xrun output is too short:\n{xrun_out}\n")

    for line in xrun_out[6:]:
        if line.strip():
            adapterID = line[26:34].strip()
            status = line[34:].strip()
        else:
            continue
    print("adapter_id = ",adapterID)
    return adapterID


def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("xe", nargs='?',
                        help=".xe file to run")
    parser.add_argument('-i', '--input', type=pathlib.Path, required=True, help="input .dat files", nargs='+')
    parser.add_argument('-o', '--output', type=pathlib.Path, required=True, help="output .dat files", nargs='+')
    parser.add_argument('-f', '--input-sample-rate', type=str, help="input sample rate", default=None)
    parser.add_argument('-g', '--output-sample-rate', type=str, help="output sample rate", default=None)
    parser.add_argument('-e', '--freq-deviation', type=str, help="Frequency deviation", default=None)
    parser.add_argument('-n', '--num-input-samples', type=str, help="Number of input samples (all channels) to process", default=None)

    args = parser.parse_args()
    return args

def run(xe, input_files, output_files, input_sample_rate, output_sample_rate, freq_deviation, num_input_samples, return_stdout=False):
    # Convert from input data files to binary files
    input_bin_files = []
    for f in input_files:
        f_bin = str(f) + ".bin"
        a = np.loadtxt(f, dtype=np.int32)
        a = np.array(a, dtype=np.int32)
        a.tofile(f_bin)
        input_bin_files.append(f_bin)

    # Create the cmd line string
    args = "-i "
    args = args + " ".join(str(i) for i in input_bin_files)

    args = args + " -o "
    args = args + " ".join(str(i) for i in output_files)

    if input_sample_rate != None:
        args = args + f" -f {input_sample_rate}"

    if output_sample_rate != None:
        args = args + f" -g {output_sample_rate}"

    if freq_deviation != None:
        args = args + f" -e {freq_deviation}"

    if num_input_samples != None:
        args = args + f" -n {num_input_samples}"

    print(args)

    with open("args.txt", "w") as fp:
        fp.write(args)

    adapter_id = get_adapter_id()
    print("Running on adapter_id ",adapter_id)

    stdout = ''
    if return_stdout == False:
        xscope_fileio.run_on_target(adapter_id, xe)
    else:
        with open("stdout.txt", "w+") as ff:
            xscope_fileio.run_on_target(adapter_id, xe, stdout=ff)
            ff.seek(0)
            stdout = ff.readlines()

    # Convert from binary to .dat and save in the same output file
    for f in output_files:
        data = np.fromfile(f, dtype=np.int32)
        np.savetxt(f, data, fmt='%i')

    # Remove the input binary files
    for f in input_bin_files:
        Path.unlink(Path(f))

    return stdout

if __name__ == "__main__":
    args = parse_arguments()
    assert args.xe is not None, "Specify vaild .xe file"
    print(f"args.input = {args.input}")

    run(args.xe, args.input, args.output, args.input_sample_rate, args.output_sample_rate, args.freq_deviation, args.num_input_samples)

