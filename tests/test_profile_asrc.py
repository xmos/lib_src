# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XCORE VocalFusion Licence.

"""
Test to profile the execution time of ASRC
"""

import pytest
from pathlib import Path
import subprocess
import re
import matplotlib.pyplot as plt
import numpy as np
import contextlib, os
from src_test_utils import gen_golden, build_firmware, src_mrh_file_name_builder


# For calculating cycle count
num_threads = 5
f_clock_mhz = 600
instr_per_mm = f_clock_mhz * 1e6 / num_threads / 1e6 
instr_per_m = f_clock_mhz * 1e6 / num_threads / 1e3

num_iterations = 32

def extract_function_timing(function, gprof_terminated):

    regex_called = fr"\s+([0-9\.]+)\s+([0-9\.]+)\s+([0-9\.]+)\s+([0-9\.]+)\s+([0-9\.]+)\s+([0-9\.]+).+{function}EOL"
    regex_not_called = fr"\s+([0-9\.]+)\s+([0-9\.]+)\s+([0-9\.]+)\s+{function}EOL"

    for line in gprof_terminated:
        m = re.match(regex_called, line)
        if m:
            vals = [float(val) for val in m.groups(0)]
            return(vals)
        m = re.match(regex_not_called, line)
        if m:
            vals = [float(val) for val in m.groups(0)]
            vals.extend([0.0, 0.0, 0.0])
            return(vals)


    return None, None, None, None, None, None

@contextlib.contextmanager
def tmp_dir(new_dir):
    curdir = os.getcwd()
    os.chdir(new_dir)
    try:
        yield
    finally:
        os.chdir(curdir)


@pytest.mark.parametrize("in_sr", [44100])
@pytest.mark.parametrize("out_sr", [192000])
@pytest.mark.parametrize("fs_deviation", ["0.990099"])
def test_profile_asrc(in_sr, out_sr, fs_deviation):
    run = lambda cmd : subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT).decode().splitlines()

    file_dir = Path(__file__).resolve().parent
    output_dir = file_dir / "gprof_results"
    Path(output_dir).mkdir(exist_ok=True)

    build_firmware("test_asrc")

    with tmp_dir(output_dir):
        xe_file = f"{file_dir}/../build/tests/asrc_test/test_asrc.xe"
        print("Dumping object...")
        run(f"xobjdump --split {xe_file}")

        print(f"Simulating {int(num_iterations)} iterations...")
        try:
            fnb = src_mrh_file_name_builder()
            input_signal_0, input_signal_1 = fnb.get_in_signal_pair(in_sr, "asrc")
            dut_signal_0, dut_signal_1 = fnb.get_out_signal_pair(in_sr, out_sr, "asrc", "dut", fs_deviation=fs_deviation)
            signal_dir = file_dir / "src_input"
            num_samples_to_process = num_iterations
    
            cmd = f"xsim --gprof --args {xe_file}"
            cmd += f" -i {signal_dir / input_signal_0} {signal_dir / input_signal_1}"
            cmd += f" -o {dut_signal_0} {dut_signal_1}"
            cmd += f" -f {in_sr} -g {out_sr} -n {num_samples_to_process}"
            cmd += f" -e {fs_deviation}"

            xsim_output = run(cmd)
            print(f"Command: {cmd}")
            with open("xsim_output.txt", "w") as gpo:
                for line in xsim_output:
                    gpo.write(f"{line}\n")
        except subprocess.CalledProcessError as e:
            # xsim returns non-zero after hitting max_cycles so this is OK
            pass

        print("Profiling with gprof...")

        # The core assignment can change on compile so scan through elf files to find right one
        core_found = None
        for core_number in range(8):
            gprof_output_file = f"tile[0]_core{core_number}.gprof"
            print(f"Found core - {gprof_output_file}")
            #                    Use tile 1 app   use core number for gprof file
            gprof = run(f"xgprof image_n0c0_2.elf {gprof_output_file}")

            known_function = "asrc_process"
            if known_function in "\n".join(gprof):
                print(f"Found correct gprof file {gprof_output_file} containing {known_function}..")
                core_found = core_number

                # Save file for archiving
                with open("gprof_results.txt", "w") as gpo:
                    for line in gprof:
                        gpo.write(f"{line}\n")
                break

        assert core_number is not None, f"ASRC functions not found in tile[1]_core_0..7 - see {gprof_file_name}"
        gprof_terminated = [line + "EOL" for line in gprof]

        # gprof can change its units depending on how long the run was. Use % if reporting in m rather than mm for better accuracy
        if "mm/call" in "\n".join(gprof):
            use_mm = True 
        else:
            use_mm = False
            # First get call to asrc_process to calc scaling factors for better accuracy as gprof uses milliseconds
            # this function has big numbers so we can get an accurate factor
            percent, time_milli_cum, time_milli, calls, mm_call_self, mm_call_tot = extract_function_timing("asrc_process", gprof_terminated)
            percent_scaling = time_milli / percent

        functions_of_interest = [
                                "asrc_process",

                                "src_mrhf_fir_os_inner_loop_asm",
                                "src_mrhf_fir_os_main_loop",
                                "src_mrhf_fir_os_done",

                                "src_mrhf_fir_os_inner_loop_asm_odd",
                                "src_mrhf_fir_os_main_loop_odd",
                                "src_mrhf_fir_os_done_odd",

                                "src_mrhf_adfir_inner_loop_asm",
                                "src_mrhf_adfir_main_loop",
                                "src_mrhf_adfir_done",

                                "src_mrhf_adfir_inner_loop_asm_odd",
                                "src_mrhf_adfir_main_loop_odd",
                                "src_mrhf_adfir_done_odd",

                                "src_mrhf_spline_coeff_gen_inner_loop_asm",
                                "src_mrhf_spline_coeff_gen_main_loop",
                                "src_mrhf_spline_coeff_gen_done",


                                "ADFIR_init_from_desc",
                                "ASRC_prepare_coefs",
                                "FIR_proc_os2",
                                "ASRC_proc_F1_F2",
                                "FIR_proc_ds2",
                                
                                ]

        """ From https://ftp.gnu.org/old-gnu/Manuals/gprof-2.9.1/html_chapter/gprof_5.html
        % time
        This is the percentage of the total execution time your program spent in this function. These should all add up to 100%.
        cumulative seconds
        This is the cumulative total number of seconds the computer spent executing this functions, plus the time spent in all the functions above this one in this table.
        self seconds
        This is the number of seconds accounted for by this function alone. The flat profile listing is sorted first by this number.
        calls
        This is the total number of times the function was called. If the function was never called, or the number of times it was called cannot be determined (probably because the function was not compiled with profiling enabled), the calls field is blank.
        self ms/call
        This represents the average number of milliseconds spent in this function per call, if this function is profiled. Otherwise, this field is blank for this function.
        total ms/call
        This represents the average number of milliseconds spent in this function and its descendants per call, if this function is profiled. Otherwise, this field is blank for this function. This is the only field in the flat profile that uses call graph analysis.
        name
        This is the name of the function. The flat profile is sorted by this field alphabetically after the self seconds and calls fields are sorted.
        """


        # Now build a dictionary with just the call count and time for all calls
        instr_cycles = []
        num_calls = []
        for function in functions_of_interest:
            percent, time_milli_cum,  time_milli, calls, mm_call_self, mm_call_tot = extract_function_timing(function, gprof_terminated)
            if percent is None:
                print(f"{function} - NOT EXECUTED IN RUN..")
                instr_cycles.append(0)
                num_calls.append(0)
                continue
            # print(f"{function} - {percent} {time_milli_cum} {time_milli} {calls} {mm_call_self} {mm_call_tot}")
            
            # gprof doesn't report loops in ASM properly so manually insert the number of runs  
            hack_list =     ["src_mrhf_spline_coeff_gen_main_loop",
                            "src_mrhf_fir_os_main_loop_odd",
                            "src_mrhf_fir_os_main_loop",
                            "src_mrhf_adfir_main_loop_odd",
                            "src_mrhf_adfir_main_loop",
                            "src_mrhf_adfir_done",
                            "src_mrhf_adfir_done_odd",
                            "src_mrhf_spline_coeff_gen_done",
                            "src_mrhf_fir_os_done",
                            "src_mrhf_fir_os_done_odd",
                            ]
            if function in hack_list:
                calls = num_iterations

            # gprof can change its units depending on how long the run was. Use % if reporting in m rather than mm for better accuracy
            if use_mm:
                instructions = int(mm_call_self * instr_per_mm * calls / num_iterations)
            else:
                instructions = 0 if calls == 0 else int(percent * percent_scaling * instr_per_m / num_iterations)

            instr_cycles.append(instructions)
            num_calls.append(int(calls))  
            print(function, int(calls), instructions)          

        # Plot the data
        plt.rcdefaults()
        plt.rc('axes', labelsize=10)    # fontsize of the x and y labels
        plt.rc('ytick', labelsize=8)    # fontsize of the tick labels
        fig, ax = plt.subplots(figsize=(10, 5), dpi=150)
        y_pos = np.arange(len(instr_cycles))
        ax.barh(y_pos, instr_cycles, align='center')
        labels = [ f"{f} ({c/num_iterations})" for f, c in zip(functions_of_interest, num_calls)]
        ax.set_yticks(y_pos, labels=labels)
        ax.invert_yaxis()  # labels read top-to-bottom
        ax.set_xlabel('Instructions')
        ax.set_title(f'ASRC instr per sample after {int(num_iterations)} loops (not incl. descendents)')
        plt.subplots_adjust(left=0.3, right=0.99, top=0.95, bottom=0.05) # margins
        plt.savefig("asrc_profile_bargraph.png")
    
    

