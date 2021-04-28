# Copyright 2016-2021 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import xmostest, os

supported_sr = (44100, 48000, 88200, 96000, 176400, 192000)
num_in_samps = 256
FsRatio_deviations=["1.000000", "0.990099", "1.009999"]

def runtest():
    if not os.path.exists("src_output"):
        os.makedirs("src_output")

    curr_dir = os.path.dirname(os.path.realpath(__file__))
    print ("Current directory = %s" % curr_dir)

    """Smoke test single loop"""
    do_asrc_test_iteration(FsRatio_deviations[0], 192000, 44100, "smoke")

    """Nightly test nested for loop 3 x 6 x 6 = 108 tests"""
    for frequency_deviation in FsRatio_deviations:
        for input_sr in supported_sr:
            for output_sr in supported_sr:
                do_asrc_test_iteration(frequency_deviation, input_sr, output_sr, "nightly")


def do_asrc_test_iteration(frequency_deviation, input_sr, output_sr, testlevel):
    file_name = file_name_builder()
    resources = xmostest.request_resource("xsim")
    simargs_asrc = ""

    #print ('Running ASRC test iteration Deviation = %f SR input = %d, output = %d' % (float(frequency_deviation), input_sr, output_sr))
    test_files = (os.path.join("src_output", file_name.output_signal(input_sr, output_sr, "pure_sine", frequency_deviation)), os.path.join("src_output", file_name.output_signal(input_sr, output_sr, "inter_modulation", frequency_deviation)))

    golden_files = (os.path.join("asrc_test", "expected", file_name.golden_signal(input_sr, output_sr, "pure_sine", frequency_deviation)), os.path.join("asrc_test", "expected", file_name.golden_signal(input_sr, output_sr, "inter_modulation", frequency_deviation)))

    tester = FileComparisonTester(test_files, golden_files, "lib_src", "asrc_test", str(input_sr) + "->" + str(output_sr) + "->" + frequency_deviation, {}, regexp = False, ignore=[])
    tester.set_min_testlevel(testlevel)

    args = ["-i", os.path.join("src_input",file_name.test_signal(input_sr, "pure_sine")), os.path.join("src_input", file_name.test_signal(input_sr, "inter_modulation")), "-o", test_files[0], test_files[1]]
    args += ["-f", str(input_sr), "-g", str(output_sr), "-n", str(num_in_samps)]
    args += ["-e", frequency_deviation]

    appargs_asrc = args
    #print("xsim cmd line = %s" % " ".join(appargs_asrc))
    xmostest.run_on_simulator(resources["xsim"],
                                              os.path.join("asrc_test", "bin", "asrc_test.xe"),
                                              appargs=appargs_asrc,
                                              simargs=simargs_asrc,
                                              tester=tester)
    xmostest.complete_all_jobs()
    """Uncomment this line to help debug - ensures that previous test run before next iteration loop. Makes reading stdout easier"""


def check_file_count(test_files, golden_files):
    len_test_files = len(test_files)
    len_golden_files = len(golden_files)

    if isinstance(test_files, str):
        len_test_files = 1
    if isinstance(golden_files, str):
        len_golden_files = 1
    if len_test_files != len_golden_files:
        print("ERROR: %d test file(s) specified and %d golden file(s)" % (len_test_files, len_golden_files))
        xmostest.set_test_result(product, group, test, config, result=False, output = "Number of inout and output files does not match", env = env)
    print("Found %d audio channels to process" % len(test_files))


class file_name_builder:
    """Helper to build the input/output/golden filenames from various input output sample rares"""

    signal_types = {"pure_sine": "s1k_0dB", "inter_modulation": "im10k11k_m6dB"}
    file_name_helper = {44100: "44", 48000: "48", 88200: "88", 96000: "96", 176400: "176", 192000: "192"}

    def test_signal(self, input_sr, signal_type):
        file_name = file_name_builder.signal_types[signal_type] + "_" + file_name_builder.file_name_helper[input_sr] + ".dat"
        return file_name

    def golden_signal(self, input_sr, output_sr, signal_type, frequency_deviation):
        file_name = file_name_builder.signal_types[signal_type] + "_" + file_name_builder.file_name_helper[input_sr] + "_" + file_name_builder.file_name_helper[output_sr] + "_" + frequency_deviation + ".expect"
        return file_name

    def output_signal(self, input_sr, output_sr, signal_type, frequency_deviation):
        file_name = file_name_builder.signal_types[signal_type] + "_" + file_name_builder.file_name_helper[input_sr] + "_" + file_name_builder.file_name_helper[output_sr] + "_" + frequency_deviation + ".result"
        return file_name


class FileComparisonTester(xmostest.Tester):
    """
        This tester will compare two files and pass a test if
        the output matches
        """

    def __init__(self, input, golden, product, group, test, config = {}, env = {},
                 regexp = False, ignore=[]):
        super(FileComparisonTester, self).__init__()
        self.register_test(product, group, test, config)
        self._golden = golden
        self._input = input
        self._test = (product, group, test, config, env)
        self._regexp = regexp
        self._ignore = ignore

    def record_failure(self, failure_reason):
        # Append a newline if there isn't one already
        if not failure_reason.endswith('\n'):
            failure_reason += '\n'
        self.failures.append(failure_reason)
        print ("ERROR: %s" % failure_reason), # Print without newline
        self.result = False

    def run(self, output):
        input_files = self._input
        index = 0
        for input_file_name in input_files:
            print("Tester opening input file %s" % input_file_name)
            input_file = open(input_file_name, "r")
            test_result = [x.strip() for x in input_file.readlines()]

            (product, group, test, config, env) = self._test
            regexp = self._regexp

            golden_file_name = self._golden[index]
            print("Tester opening golden file %s" % golden_file_name)
            golden = open(golden_file_name, "r")
            expected = [x.strip() for x in golden.readlines()]

            if expected[0].strip()=='':
                expected = expected[1:]
            if expected[-1].strip()=='':
                expected = expected[:-1]

            self.result = True
            self.failures = []
            line_num = -1
            for i in range(len(test_result)):
                ignore = False
                for p in self._ignore:
                    if re.match(p,test_result[i].strip()):
                        ignore = True
                        break
                if ignore:
                    continue
                line_num += 1

                #print("line_num=%d, result=%s, line=%s" % (line_num, self.result, test_result[i].strip()))
                if line_num >= len(expected):
                    self.record_failure("Length of expected test result less than expected file")
                    break

                if regexp:
                    match = re.match(expected[line_num]+"$", test_result[i].strip())
                else:
                    match = expected[line_num] == test_result[i].strip()

                if not match:
                    self.record_failure(("Line %d of test result does not match expected file\n"+
                                         "  Expected     : %s\n"+
                                         "  Actual input : %s")
                                        % (line_num,
                                           expected[line_num].strip(),
                                           test_result[i].strip()))

            if (len(expected) > line_num + 1):
                self.record_failure("Length of expected file greater than test result")
                test_result = {'test_result':''.join(test_result)}
            if not self.result:
                test_result = ''.join(self.failures)
            index += 1
        xmostest.set_test_result(product, group, test, config, self.result,
                             output = test_result, env = env)
