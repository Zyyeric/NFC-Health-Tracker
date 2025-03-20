PROJECT_NAME = NFC-Health-Tracker

# Configurations
NRF_IC = nrf52833
SDK_VERSION = 16
SOFTDEVICE_MODEL = blank

# Source and header files
APP_HEADER_PATHS += ./include
APP_SOURCE_PATHS += ./src
APP_SOURCES = $(notdir $(wildcard src/*.c))

# Path to base of nRF52x-base repo
NRF_BASE_DIR = external/nrf52x-base/

# Include board Makefile
include external/microbit_v2/Board.mk

# Include base Makefile
include $(NRF_BASE_DIR)/make/AppMakefile.mk
