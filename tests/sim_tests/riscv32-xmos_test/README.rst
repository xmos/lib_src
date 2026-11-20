:orphan:

VX4 RISC-V build smoke test
===========================

This standalone C application builds and links ``lib_src`` with the VX4
RISC-V compiler. It exercises the public SSRC and asynchronous FIFO APIs
on the default VX4 simulator target supplied by XCommon CMake.

Source ``SetEnv.sh`` from a VX4 toolchain with RISC-V XCommon CMake
support. This test was verified with VX4 ``0.6.1-experimental-develop-21``.
The older VX4 ``0.3.1`` (main build 11) includes Clang, but its XCommon
CMake setup still selects ``xcc`` and cannot build this test.

With ``lib_basix`` and ``lib_logging`` checked out next to ``lib_src``,
build and run from this directory. Use a fresh build directory after
switching toolchains::

    cmake -S . -B build-vx4 -G "Unix Makefiles"
    cmake --build build-vx4
    xsim bin/riscv32-xmos_test.xe

The simulation returns a nonzero exit code if SSRC does not double the
sample count and preserve silence, or if the FIFO does not return a zero
sample from its prefill. This test is separate from the XS simulator suite.

The Jenkins ``vx4 tests`` stage builds and runs this application with
``TOOLS_VERSION_VX`` and fails if ``xsim`` exits with a nonzero status.
