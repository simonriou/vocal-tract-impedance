CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -O2
CPPFLAGS ?= -Iexternal/kiss_fft -Isrc/core -Isrc/config -Isrc/interface -Isrc/orchestration

# Platform detection for PortAudio
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  CPPFLAGS += -I/opt/homebrew/include
  LDFLAGS_AUDIO ?= -L/opt/homebrew/lib -lportaudio
else
  LDFLAGS_AUDIO ?= -lportaudio
endif

LDFLAGS ?= -lm

# Build and source directories
SRCDIR := src
CORE_DIR := $(SRCDIR)/core
CONFIG_DIR := $(SRCDIR)/config
INTERFACE_DIR := $(SRCDIR)/interface
ORCHESTRATION_DIR := $(SRCDIR)/orchestration
TESTS_DIR := tests
BUILD_DIR := build

# Library and object files
LIB_NAME := libprocessing.a
KISS_FFT_OBJ := external/kiss_fft/kiss_fft.o
PROCESSING_OBJ := $(BUILD_DIR)/processing.o
AUDIO_IO_OBJ := $(BUILD_DIR)/audio_io.o
USER_INTERFACE_OBJ := $(BUILD_DIR)/user_interface.o
PIPELINE_OBJ := $(BUILD_DIR)/pipeline.o

# Main executable
MAIN_EXEC := main
MAIN_OBJ := $(BUILD_DIR)/main.o

# Test executables
TEST_INVERSE_EXEC := test_inverse
TEST_INVERSE_OBJ := $(BUILD_DIR)/test_inverse.o
TEST_WINDOW_EXEC := test_window
TEST_WINDOW_OBJ := $(BUILD_DIR)/test_window.o

# Header dependencies
PROCESSING_DEPS := $(CORE_DIR)/processing.h $(CORE_DIR)/complex_utils.h
AUDIO_IO_DEPS := $(CORE_DIR)/audio_io.h
USER_INTERFACE_DEPS := $(INTERFACE_DIR)/user_interface.h $(CONFIG_DIR)/config.h $(CORE_DIR)/audio_io.h
PIPELINE_DEPS := $(ORCHESTRATION_DIR)/pipeline.h $(CONFIG_DIR)/config.h $(CORE_DIR)/audio_io.h $(CORE_DIR)/processing.h $(INTERFACE_DIR)/user_interface.h
MAIN_DEPS := $(SRCDIR)/main.c $(CORE_DIR)/audio_io.h $(CONFIG_DIR)/config.h $(INTERFACE_DIR)/user_interface.h $(ORCHESTRATION_DIR)/pipeline.h

# Declare phony targets
.PHONY: all clean test_inverse test_window help

# Default target
all: $(BUILD_DIR) $(MAIN_EXEC)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $@

$(LIB_NAME): $(PROCESSING_OBJ) $(AUDIO_IO_OBJ) $(KISS_FFT_OBJ)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(PROCESSING_OBJ): $(CORE_DIR)/processing.c $(PROCESSING_DEPS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(AUDIO_IO_OBJ): $(CORE_DIR)/audio_io.c $(AUDIO_IO_DEPS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(USER_INTERFACE_OBJ): $(INTERFACE_DIR)/user_interface.c $(USER_INTERFACE_DEPS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(PIPELINE_OBJ): $(ORCHESTRATION_DIR)/pipeline.c $(PIPELINE_DEPS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(MAIN_OBJ): $(MAIN_DEPS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(SRCDIR)/main.c -o $@

$(MAIN_EXEC): $(MAIN_OBJ) $(PROCESSING_OBJ) $(AUDIO_IO_OBJ) $(USER_INTERFACE_OBJ) $(PIPELINE_OBJ) $(KISS_FFT_OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDFLAGS_AUDIO)

test_inverse: $(BUILD_DIR) $(TEST_INVERSE_OBJ) $(PROCESSING_OBJ) $(AUDIO_IO_OBJ) $(KISS_FFT_OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(TEST_INVERSE_EXEC) $(TEST_INVERSE_OBJ) $(PROCESSING_OBJ) $(AUDIO_IO_OBJ) $(KISS_FFT_OBJ) $(LDFLAGS) $(LDFLAGS_AUDIO)

$(TEST_INVERSE_OBJ): $(TESTS_DIR)/test_inverse.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

test_window: $(BUILD_DIR) $(TEST_WINDOW_OBJ) $(PROCESSING_OBJ) $(KISS_FFT_OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(TEST_WINDOW_EXEC) $(TEST_WINDOW_OBJ) $(PROCESSING_OBJ) $(KISS_FFT_OBJ) $(LDFLAGS)

$(TEST_WINDOW_OBJ): $(TESTS_DIR)/test_window.c $(PROCESSING_DEPS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(MAIN_EXEC) $(TEST_INVERSE_EXEC) $(TEST_WINDOW_EXEC) external/kiss_fft/kiss_fft.o

help:
	@echo "Available targets:"
	@echo "  all          - Build the main executable (default)"
	@echo "  test_inverse - Build the inverse filter test executable"
	@echo "  test_window  - Build the Tukey window test executable"
	@echo "  clean        - Remove built objects and executables"
	@echo "  help         - Show this message"
