APP_NAME = legacy_xcommon_test
TARGET = XK-EVK-XU316
USED_MODULES = lib_src
XCC_FLAGS = -Os -g -report

# Legacy xcommon does not search this far above a nested application.
XMOS_MODULE_PATH := $(abspath ../../../..)$(if $(XMOS_MODULE_PATH),;$(XMOS_MODULE_PATH))

# Report missing local modules instead of offering to download them.
XDE_BUILD ?= 1

include $(XMOS_MAKE_PATH)/xcommon/module_xcommon/build/Makefile.common
