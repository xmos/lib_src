@rem
@rem Runs SSRC golden reference file gen
@rem ===================================
@rem
@rem This script drives the SSRC package deliverable .exe and input test waveforms from Digimath
@rem It generates the golden (reference) output values used to test the optimised XMOS implementation
@rem

@rem Dither is inactive (-d0 option). If dither is required, replace by -d1 -r1234 -s5678 (or seeds of choice) option
SET dither=-d0
@rem Number of input samples to process in total
SET n_samps_in_tot=-l256
@rem Number of input samples to process in one chunk
SET n_samps_proc=-n4

@echo SSRC 44.1 - 44.1
@echo ----------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_44.dat -jIn\im10k11k_m6dB_44.dat -k0 -oOut\s1k_0dB_44_44.expect -pOut\im10k11k_m6dB_44_44.expect -q0 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 44.1 - 48
@echo --------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_44.dat -jIn\im10k11k_m6dB_44.dat -k0 -oOut\s1k_0dB_44_48.expect -pOut\im10k11k_m6dB_44_48.expect -q1 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 44.1 - 88.2
@echo ----------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_44.dat -jIn\im10k11k_m6dB_44.dat -k0 -oOut\s1k_0dB_44_88.expect -pOut\im10k11k_m6dB_44_88.expect -q2 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 44.1 - 96
@echo --------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_44.dat -jIn\im10k11k_m6dB_44.dat -k0 -oOut\s1k_0dB_44_96.expect -pOut\im10k11k_m6dB_44_96.expect -q3 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 44.1 - 176.4
@echo -----------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_44.dat -jIn\im10k11k_m6dB_44.dat -k0 -oOut\s1k_0dB_44_176.expect -pOut\im10k11k_m6dB_44_176.expect -q4 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 44.1 - 192
@echo ---------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_44.dat -jIn\im10k11k_m6dB_44.dat -k0 -oOut\s1k_0dB_44_192.expect -pOut\im10k11k_m6dB_44_192.expect -q5 %dither% %n_samps_in_tot% %n_samps_proc%



@echo SSRC 48 - 44.1
@echo --------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_48.dat -jIn\im10k11k_m6dB_48.dat -k1 -oOut\s1k_0dB_48_44.expect -pOut\im10k11k_m6dB_48_44.expect -q0 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 48 - 48
@echo ------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_48.dat -jIn\im10k11k_m6dB_48.dat -k1 -oOut\s1k_0dB_48_48.expect -pOut\im10k11k_m6dB_48_48.expect -q1 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 48 - 88.2
@echo --------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_48.dat -jIn\im10k11k_m6dB_48.dat -k1 -oOut\s1k_0dB_48_88.expect -pOut\im10k11k_m6dB_48_88.expect -q2 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 48 - 96
@echo ------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_48.dat -jIn\im10k11k_m6dB_48.dat -k1 -oOut\s1k_0dB_48_96.expect -pOut\im10k11k_m6dB_48_96.expect -q3 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 48 - 176.4
@echo ---------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_48.dat -jIn\im10k11k_m6dB_48.dat -k1 -oOut\s1k_0dB_48_176.expect -pOut\im10k11k_m6dB_48_176.expect -q4 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 48 - 192
@echo -------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_48.dat -jIn\im10k11k_m6dB_48.dat -k1 -oOut\s1k_0dB_48_192.expect -pOut\im10k11k_m6dB_48_192.expect -q5 %dither% %n_samps_in_tot% %n_samps_proc%



@echo SSRC 88.2 - 44.1
@echo ----------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_88.dat -jIn\im10k11k_m6dB_88.dat -k2 -oOut\s1k_0dB_88_44.expect -pOut\im10k11k_m6dB_88_44.expect -q0 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 88.2 - 48
@echo --------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_88.dat -jIn\im10k11k_m6dB_88.dat -k2 -oOut\s1k_0dB_88_48.expect -pOut\im10k11k_m6dB_88_48.expect -q1 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 88.2 - 88.2
@echo ----------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_88.dat -jIn\im10k11k_m6dB_88.dat -k2 -oOut\s1k_0dB_88_88.expect -pOut\im10k11k_m6dB_88_88.expect -q2 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 88.2 - 96
@echo --------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_88.dat -jIn\im10k11k_m6dB_88.dat -k2 -oOut\s1k_0dB_88_96.expect -pOut\im10k11k_m6dB_88_96.expect -q3 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 88.2 - 176.4
@echo -----------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_88.dat -jIn\im10k11k_m6dB_88.dat -k2 -oOut\s1k_0dB_88_176.expect -pOut\im10k11k_m6dB_88_176.expect -q4 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 88.2 - 192
@echo ---------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_88.dat -jIn\im10k11k_m6dB_88.dat -k2 -oOut\s1k_0dB_88_192.expect -pOut\im10k11k_m6dB_88_192.expect -q5 %dither% %n_samps_in_tot% %n_samps_proc%



@echo SSRC 96 - 44.1
@echo --------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_96.dat -jIn\im10k11k_m6dB_96.dat -k3 -oOut\s1k_0dB_96_44.expect -pOut\im10k11k_m6dB_96_44.expect -q0 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 96 - 48
@echo ------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_96.dat -jIn\im10k11k_m6dB_96.dat -k3 -oOut\s1k_0dB_96_48.expect -pOut\im10k11k_m6dB_96_48.expect -q1 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 96 - 88.2
@echo --------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_96.dat -jIn\im10k11k_m6dB_96.dat -k3 -oOut\s1k_0dB_96_88.expect -pOut\im10k11k_m6dB_96_88.expect -q2 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 96 - 96
@echo ------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_96.dat -jIn\im10k11k_m6dB_96.dat -k3 -oOut\s1k_0dB_96_96.expect -pOut\im10k11k_m6dB_96_96.expect -q3 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 96 - 176.4
@echo ---------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_96.dat -jIn\im10k11k_m6dB_96.dat -k3 -oOut\s1k_0dB_96_176.expect -pOut\im10k11k_m6dB_96_176.expect -q4 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 96 - 192
@echo -------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_96.dat -jIn\im10k11k_m6dB_96.dat -k3 -oOut\s1k_0dB_96_192.expect -pOut\im10k11k_m6dB_96_192.expect -q5 %dither% %n_samps_in_tot% %n_samps_proc%



@echo SSRC 176.4 - 44.1
@echo -----------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_176.dat -jIn\im10k11k_m6dB_176.dat -k4 -oOut\s1k_0dB_176_44.expect -pOut\im10k11k_m6dB_176_44.expect -q0 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 176.4 - 48
@echo ---------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_176.dat -jIn\im10k11k_m6dB_176.dat -k4 -oOut\s1k_0dB_176_48.expect -pOut\im10k11k_m6dB_176_48.expect -q1 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 176.4 - 88.2
@echo -----------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_176.dat -jIn\im10k11k_m6dB_176.dat -k4 -oOut\s1k_0dB_176_88.expect -pOut\im10k11k_m6dB_176_88.expect -q2 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 176.4 - 96
@echo ---------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_176.dat -jIn\im10k11k_m6dB_176.dat -k4 -oOut\s1k_0dB_176_96.expect -pOut\im10k11k_m6dB_176_96.expect -q3 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 176.4 - 176.4
@echo ------------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_176.dat -jIn\im10k11k_m6dB_176.dat -k4 -oOut\s1k_0dB_176_176.expect -pOut\im10k11k_m6dB_176_176.expect -q4 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 176.4 - 192
@echo ----------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_176.dat -jIn\im10k11k_m6dB_176.dat -k4 -oOut\s1k_0dB_176_192.expect -pOut\im10k11k_m6dB_176_192.expect -q5 %dither% %n_samps_in_tot% %n_samps_proc%



@echo SSRC 192 - 44.1
@echo ---------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_192.dat -jIn\im10k11k_m6dB_192.dat -k5 -oOut\s1k_0dB_192_44.expect -pOut\im10k11k_m6dB_192_44.expect -q0 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 192 - 48
@echo -------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_192.dat -jIn\im10k11k_m6dB_192.dat -k5 -oOut\s1k_0dB_192_48.expect -pOut\im10k11k_m6dB_192_48.expect -q1 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 192 - 88.2
@echo ---------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_192.dat -jIn\im10k11k_m6dB_192.dat -k5 -oOut\s1k_0dB_192_88.expect -pOut\im10k11k_m6dB_192_88.expect -q2 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 192 - 96
@echo -------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_192.dat -jIn\im10k11k_m6dB_192.dat -k5 -oOut\s1k_0dB_192_96.expect -pOut\im10k11k_m6dB_192_96.expect -q3 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 192 - 176.4
@echo ----------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_192.dat -jIn\im10k11k_m6dB_192.dat -k5 -oOut\s1k_0dB_192_176.expect -pOut\im10k11k_m6dB_192_176.expect -q4 %dither% %n_samps_in_tot% %n_samps_proc%

@echo SSRC 192 - 192
@echo --------------
@..\Project\Debug\SSRC_demo.exe -iIn\s1k_0dB_192.dat -jIn\im10k11k_m6dB_192.dat -k5 -oOut\s1k_0dB_192_192.expect -pOut\im10k11k_m6dB_192_192.expect -q5 %dither% %n_samps_in_tot% %n_samps_proc%
