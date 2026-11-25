# The following patterns are to be excluded from the documentation build
CHANGELOG.rst
LICENSE.rst
tests/sim_tests/riscv32-xmos_test/README.rst
AGENTS.md
tests/sim_tests/legacy_xcommon_test/README.rst
**/.pytest_cache/**

# We need to ensure these are not processed twice which breaks figure numbering
# They are included by other documentation pages
**/resource_usage_asrc.rst
**/resource_usage_ssrc.rst
**/latency_asrc.rst
**/ssrc_example.rst
**/ff3_example.rst
