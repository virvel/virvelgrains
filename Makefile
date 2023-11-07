# Project Name
TARGET = daisysamples

USE_FATFS=1

# Sources
#CPP_SOURCES = daisysamples.cpp pooper.cpp
CPP_SOURCES = daisygrains.cpp granulator.cpp

# Library Locations
LIBDAISY_DIR = libDaisy
DAISYSP_DIR = DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
