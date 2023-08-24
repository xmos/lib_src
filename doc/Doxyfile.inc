# This file provides overrides to the Doxyfile configuration

PROJECT_NAME           = lib_ssrc
PROJECT_BRIEF          = "Synchronous and Asynchronous rate conversion documentation"

INPUT                  = ../lib_src/api ../lib_src/src ../examples/AN00231_ASRC_SPDIF_TO_DAC/src

PREDEFINED             = C_API= EXTERN_C= 

USE_MATHJAX            = YES
MATHJAX_FORMAT         = HTML-CSS
MATHJAX_RELPATH        = https://cdnjs.cloudflare.com/ajax/libs/mathjax/2.7.5/

# Make short MATHJAX sequences prettier
ALIASES += math{1}="@f$\1@f$"
ALIASES += vector{1}="@f$\bar \1@f$"
ALIASES += operation{1}="@par Operation Performed:^^@f{align*}{ \1 @f}"

