set(LIB_NAME lib_src)
set(LIB_VERSION 2.5.0)

set(LIB_DEPENDENT_MODULES "lib_logging(3.2.0)")

set(LIB_COMPILER_FLAGS -Wno-missing-braces -O3)

set(LIB_OPTIONAL_HEADERS src_conf.h)

set(LIB_INCLUDES   api
                   src/fixed_factor_of_3
                   src/fixed_factor_of_3/ds3
                   src/fixed_factor_of_3/os3
                   src/fixed_factor_of_3_voice
                   src/multirate_hifi
                   src/multirate_hifi/asrc
                   src/multirate_hifi/ssrc
                   src/fixed_factor_vpu_voice
                   src/asrc_task)

XMOS_REGISTER_MODULE()

