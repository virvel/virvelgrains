# Project Name
TARGET = daisysamples

USE_FATFS=1

# Sources
CPP_SOURCES = daisygrains.cpp dsp/granulator.cpp dsp/frequencyshifter.cpp

# Library Locations
LIBDAISY_DIR = libDaisy
DAISYSP_DIR = DaisySP

C_SOURCES ?= \
$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Source/FilteringFunctions/arm_biquad_cascade_df1_f32.c\
$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Source/FilteringFunctions/arm_biquad_cascade_df1_init_f32.c\
$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Source/BasicMathFunctions/arm_mult_f32.c\
$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Source/BasicMathFunctions/arm_scale_f32.c\
$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Source/BasicMathFunctions/arm_sub_f32.c

C_INCLUDES += \
-I$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Include 

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

CPPFLAGS += -I./dsp