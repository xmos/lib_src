@rem
@rem Runs ASRC golden reference file gen
@rem ===================================
@rem
@rem This script drives the ASRC package deliverable .exe and input test waveforms from Digimath
@rem It generates the golden (reference) output values used to test the optimised XMOS implementation
@rem

@rem Dither is inactive (-d0 option). If dither is required, replace by something like -d1 -r1234 -s5678 (use seeds of your choice)
SET dither=-d0
@rem Number of input samples to process in total
SET n_samps_in_tot=-l256
@rem Number of input samples to process in one chunk
SET n_samps_proc=-n4
@rem List of deviation of actual sample rate ratio from nominal
set FsRatio_deviations=1.000000, 0.990099, 1.009999

@rem iterate over this list
for %%f in (%FsRatio_deviations%) do (
	echo %%f



@echo ASRC 44.1 - 44.1
@echo ----------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_44.dat -jIn\im10k11k_m6dB_44.dat -k0 -oOut\s1k_0dB_44_44_%%f.expect -pOut\im10k11k_m6dB_44_44_%%f.expect -q0 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 44.1 - 48
@echo --------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_44.dat -jIn\im10k11k_m6dB_44.dat -k0 -oOut\s1k_0dB_44_48_%%f.expect -pOut\im10k11k_m6dB_44_48_%%f.expect -q1 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 44.1 - 88.2
@echo ----------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_44.dat -jIn\im10k11k_m6dB_44.dat -k0 -oOut\s1k_0dB_44_88_%%f.expect -pOut\im10k11k_m6dB_44_88_%%f.expect -q2 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 44.1 - 96
@echo --------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_44.dat -jIn\im10k11k_m6dB_44.dat -k0 -oOut\s1k_0dB_44_96_%%f.expect -pOut\im10k11k_m6dB_44_96_%%f.expect -q3 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 44.1 - 176.4
@echo -----------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_44.dat -jIn\im10k11k_m6dB_44.dat -k0 -oOut\s1k_0dB_44_176_%%f.expect -pOut\im10k11k_m6dB_44_176_%%f.expect -q4 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 44.1 - 192
@echo ---------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_44.dat -jIn\im10k11k_m6dB_44.dat -k0 -oOut\s1k_0dB_44_192_%%f.expect -pOut\im10k11k_m6dB_44_192_%%f.expect -q5 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f



@echo ASRC 48 - 44.1
@echo --------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_48.dat -jIn\im10k11k_m6dB_48.dat -k1 -oOut\s1k_0dB_48_44_%%f.expect -pOut\im10k11k_m6dB_48_44_%%f.expect -q0 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 48 - 48
@echo ------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_48.dat -jIn\im10k11k_m6dB_48.dat -k1 -oOut\s1k_0dB_48_48_%%f.expect -pOut\im10k11k_m6dB_48_48_%%f.expect -q1 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 48 - 88.2
@echo --------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_48.dat -jIn\im10k11k_m6dB_48.dat -k1 -oOut\s1k_0dB_48_88_%%f.expect -pOut\im10k11k_m6dB_48_88_%%f.expect -q2 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 48 - 96
@echo ------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_48.dat -jIn\im10k11k_m6dB_48.dat -k1 -oOut\s1k_0dB_48_96_%%f.expect -pOut\im10k11k_m6dB_48_96_%%f.expect -q3 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 48 - 176.4
@echo ---------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_48.dat -jIn\im10k11k_m6dB_48.dat -k1 -oOut\s1k_0dB_48_176_%%f.expect -pOut\im10k11k_m6dB_48_176_%%f.expect -q4 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 48 - 192
@echo -------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_48.dat -jIn\im10k11k_m6dB_48.dat -k1 -oOut\s1k_0dB_48_192_%%f.expect -pOut\im10k11k_m6dB_48_192_%%f.expect -q5 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f



@echo ASRC 88.2 - 44.1
@echo ----------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_88.dat -jIn\im10k11k_m6dB_88.dat -k2 -oOut\s1k_0dB_88_44_%%f.expect -pOut\im10k11k_m6dB_88_44_%%f.expect -q0 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 88.2 - 48
@echo --------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_88.dat -jIn\im10k11k_m6dB_88.dat -k2 -oOut\s1k_0dB_88_48_%%f.expect -pOut\im10k11k_m6dB_88_48_%%f.expect -q1 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 88.2 - 88.2
@echo ----------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_88.dat -jIn\im10k11k_m6dB_88.dat -k2 -oOut\s1k_0dB_88_88_%%f.expect -pOut\im10k11k_m6dB_88_88_%%f.expect -q2 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 88.2 - 96
@echo --------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_88.dat -jIn\im10k11k_m6dB_88.dat -k2 -oOut\s1k_0dB_88_96_%%f.expect -pOut\im10k11k_m6dB_88_96_%%f.expect -q3 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 88.2 - 176.4
@echo -----------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_88.dat -jIn\im10k11k_m6dB_88.dat -k2 -oOut\s1k_0dB_88_176_%%f.expect -pOut\im10k11k_m6dB_88_176_%%f.expect -q4 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 88.2 - 192
@echo ---------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_88.dat -jIn\im10k11k_m6dB_88.dat -k2 -oOut\s1k_0dB_88_192_%%f.expect -pOut\im10k11k_m6dB_88_192_%%f.expect -q5 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f



@echo ASRC 96 - 44.1
@echo --------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_96.dat -jIn\im10k11k_m6dB_96.dat -k3 -oOut\s1k_0dB_96_44_%%f.expect -pOut\im10k11k_m6dB_96_44_%%f.expect -q0 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 96 - 48
@echo ------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_96.dat -jIn\im10k11k_m6dB_96.dat -k3 -oOut\s1k_0dB_96_48_%%f.expect -pOut\im10k11k_m6dB_96_48_%%f.expect -q1 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 96 - 88.2
@echo --------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_96.dat -jIn\im10k11k_m6dB_96.dat -k3 -oOut\s1k_0dB_96_88_%%f.expect -pOut\im10k11k_m6dB_96_88_%%f.expect -q2 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 96 - 96
@echo ------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_96.dat -jIn\im10k11k_m6dB_96.dat -k3 -oOut\s1k_0dB_96_96_%%f.expect -pOut\im10k11k_m6dB_96_96_%%f.expect -q3 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 96 - 176.4
@echo ---------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_96.dat -jIn\im10k11k_m6dB_96.dat -k3 -oOut\s1k_0dB_96_176_%%f.expect -pOut\im10k11k_m6dB_96_176_%%f.expect -q4 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 96 - 192
@echo -------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_96.dat -jIn\im10k11k_m6dB_96.dat -k3 -oOut\s1k_0dB_96_192_%%f.expect -pOut\im10k11k_m6dB_96_192_%%f.expect -q5 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f



@echo ASRC 176.4 - 44.1
@echo -----------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_176.dat -jIn\im10k11k_m6dB_176.dat -k4 -oOut\s1k_0dB_176_44_%%f.expect -pOut\im10k11k_m6dB_176_44_%%f.expect -q0 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 176.4 - 48
@echo ---------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_176.dat -jIn\im10k11k_m6dB_176.dat -k4 -oOut\s1k_0dB_176_48_%%f.expect -pOut\im10k11k_m6dB_176_48_%%f.expect -q1 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 176.4 - 88.2
@echo -----------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_176.dat -jIn\im10k11k_m6dB_176.dat -k4 -oOut\s1k_0dB_176_88_%%f.expect -pOut\im10k11k_m6dB_176_88_%%f.expect -q2 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 176.4 - 96
@echo ---------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_176.dat -jIn\im10k11k_m6dB_176.dat -k4 -oOut\s1k_0dB_176_96_%%f.expect -pOut\im10k11k_m6dB_176_96_%%f.expect -q3 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 176.4 - 176.4
@echo ------------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_176.dat -jIn\im10k11k_m6dB_176.dat -k4 -oOut\s1k_0dB_176_176_%%f.expect -pOut\im10k11k_m6dB_176_176_%%f.expect -q4 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 176.4 - 192
@echo ----------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_176.dat -jIn\im10k11k_m6dB_176.dat -k4 -oOut\s1k_0dB_176_192_%%f.expect -pOut\im10k11k_m6dB_176_192_%%f.expect -q5 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f



@echo ASRC 192 - 44.1
@echo ---------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_192.dat -jIn\im10k11k_m6dB_192.dat -k5 -oOut\s1k_0dB_192_44_%%f.expect -pOut\im10k11k_m6dB_192_44_%%f.expect -q0 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 192 - 48
@echo -------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_192.dat -jIn\im10k11k_m6dB_192.dat -k5 -oOut\s1k_0dB_192_48_%%f.expect -pOut\im10k11k_m6dB_192_48_%%f.expect -q1 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 192 - 88.2
@echo ---------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_192.dat -jIn\im10k11k_m6dB_192.dat -k5 -oOut\s1k_0dB_192_88_%%f.expect -pOut\im10k11k_m6dB_192_88_%%f.expect -q2 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 192 - 96
@echo -------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_192.dat -jIn\im10k11k_m6dB_192.dat -k5 -oOut\s1k_0dB_192_96_%%f.expect -pOut\im10k11k_m6dB_192_96_%%f.expect -q3 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 192 - 176.4
@echo ----------------
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_192.dat -jIn\im10k11k_m6dB_192.dat -k5 -oOut\s1k_0dB_192_176_%%f.expect -pOut\im10k11k_m6dB_192_176_%%f.expect -q4 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

@echo ASRC 192 - 192
@echo --------------
@echo HERE WE GO!
@..\Project\Debug\ASRC_demo.exe -iIn\s1k_0dB_192.dat -jIn\im10k11k_m6dB_192.dat -k5 -oOut\s1k_0dB_192_192_%%f.expect -pOut\im10k11k_m6dB_192_192_%%f.expect -q5 %dither% %n_samps_in_tot% %n_samps_proc% -e%%f

)
