CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -O2
CPPFLAGS ?= -Iexternal/kiss_fft

# Platform detection for PortAudio
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  CPPFLAGS += -I/opt/homebrew/include
  LDFLAGS_AUDIO ?= -L/opt/homebrew/lib -lportaudio
else
  LDFLAGS_AUDIO ?= -lportaudio
endif

LDFLAGS ?= -lm

# Library and object files
LIB_NAME := libprocessing.a
KISS_FFT_OBJ := external/kiss_fft/kiss_fft.o
PROCESSING_OBJ := processing.o
AUDIO_IO_OBJ := audio_io.o
USER_INTERFACE_OBJ := user_interface.o
PIPELINE_OBJ := pipeline.o

# Main executable
MAIN_EXEC := main
MAIN_OBJ := main.o

# Test executables
TEST_AUDIO_EXEC := test_audio_io
TEST_AUDIO_OBJ := test_audio_io.o

TEST_INVERSE_EXEC := test_inverse
TEST_INVERSE_OBJ := test_inverse.o

# Header dependencies
PROCESSING_DEPS := processing.h complex_utils.h
AUDIO_IO_DEPS := audio_io.h
USER_INTERFACE_DEPS := user_interface.h config.h audio_io.h
PIPELINE_DEPS := pipeline.h config.h audio_io.h processing.h user_interface.h
MAIN_DEPS := main.c audio_io.h config.h user_interface.h pipeline.h

# Declare phony targets
.PHONY: all clean test_audio test_inverse help

# Default target
all: $(MAIN_EXEC)

$(LIB_NAME): $(PROCESSING_OBJ) $(AUDIO_IO_OBJ) $(KISS_FFT_OBJ)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(PROCESSING_OBJ): processing.c $(PROCESSING_DEPS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(AUDIO_IO_OBJ): audio_io.c $(AUDIO_IO_DEPS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(USER_INTERFACE_OBJ): user_interface.c $(USER_INTERFACE_DEPS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(PIPELINE_OBJ): pipeline.c $(PIPELINE_DEPS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(MAIN_OBJ): $(MAIN_DEPS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c main.c -o $@

$(MAIN_EXEC): $(MAIN_OBJ) $(PROCESSING_OBJ) $(AUDIO_IO_OBJ) $(USER_INTERFACE_OBJ) $(PIPELINE_OBJ) $(KISS_FFT_OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDFLAGS_AUDIO)

test_audio: $(TEST_AUDIO_OBJ) $(AUDIO_IO_OBJ) $(KISS_FFT_OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(TEST_AUDIO_EXEC) $^ $(LDFLAGS) $(LDFLAGS_AUDIO)

test_inverse: $(TEST_INVERSE_OBJ) $(PROCESSING_OBJ) $(AUDIO_IO_OBJ) $(KISS_FFT_OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(TEST_INVERSE_EXEC) $^ $(LDFLAGS) $(LDFLAGS_AUDIO)

clean:
	rm -f $(PROCESSING_OBJ) $(AUDIO_IO_OBJ) $(USER_INTERFACE_OBJ) $(PIPELINE_OBJ) $(KISS_FFT_OBJ) $(LIB_NAME) $(MAIN_OBJ) $(MAIN_EXEC) $(TEST_AUDIO_OBJ) $(TEST_AUDIO_EXEC) $(TEST_INVERSE_OBJ) $(TEST_INVERSE_EXEC)

help:
	@echo "Available targets:"
	@echo "  all          - Build the main executable (default)"
	@echo "  test_audio   - Build the audio I/O test suite executable"
	@echo "  test_inverse - Build the inverse filter test executable"
	@echo "  clean        - Remove built objects and executables"
	@echo "  help         - Show this message"
