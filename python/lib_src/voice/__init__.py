# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""
Voice-quality synchronous SRC: fixed factor of 3 (48 <-> 16 kHz) and rational
factor 3/2 (48 <-> 32 kHz).

Modules:
    design   -- filter design, polyphase decomposition and quantisation
    ff3      -- float models of the fixed factor of 3 down/upsamplers
    rat      -- float models of the rational factor 3/2 down/upsamplers
    codegen  -- writers for the generated C/XC coefficient files
    generate -- command line entry point (python -m lib_src.voice.generate)
"""
