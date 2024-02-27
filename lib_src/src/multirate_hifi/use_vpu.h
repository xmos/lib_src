// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef SRC_USE_VPU
    #if defined(__XS3A__)
        #define  SRC_USE_VPU    1
    #else
        #define  SRC_USE_VPU    0
    #endif
#else
    #if(defined(__XS2A__) && (SRC_USE_VPU != 0))
        #error VPU optimisation in lib_src is only available on XS3 (xcore.ai) architecture
    #endif
#endif 
