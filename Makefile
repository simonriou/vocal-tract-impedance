CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -O2
CPPFLAGS ?= -Iexternal/kiss_fft -I/opt/homebrew/include
LDFLAGS ?= -lm -L/opt/homebrew/lib -lportaudio

# Library and object files
LIB_NAME := libprocessing.a
KISS_FFT_OBJ := external/kiss_fft/kiss_fft.o
PROCESSING_OBJ := processing.o
AUDIO_IO_OBJ := audio_io.o

# Test executable
TEST_AUDIO_EXEC := test_audio_io
TEST_AUDIO_OBJ := test_audio_io.o

# Default target
.PHONY: all clean

all: $(LIB_NAME)

$(LIB_NAME): $(PROCESSING_OBJ) $(AUDIO_IO_OBJ) $(KISS_FFT_OBJ)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

test_audio: $(TEST_AUDIO_OBJ) $(AUDIO_IO_OBJ) $(KISS_FFT_OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(TEST_AUDIO_EXEC) $^ $(LDFLAGS)

clean:
	rm -f $(PROCESSING_OBJ) $(AUDIO_IO_OBJ) $(KISS_FFT_OBJ) $(LIB_NAME) $(TEST_AUDIO_OBJ) $(TEST_AUDIO_EXEC)

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all        - Build the processing library (default)"
	@echo "  test_audio - Build the audio I/O test suite executable"
	@echo "  clean      - Remove built objects and executables"
	@echo "  help       - Show this message"
