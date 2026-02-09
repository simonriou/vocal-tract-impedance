CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -O2
CPPFLAGS ?= -Iexternal/kiss_fft -I/opt/homebrew/include
LDFLAGS ?= -lm -L/opt/homebrew/lib -lportaudio

# Library and object files
LIB_NAME := libprocessing.a
KISS_FFT_OBJ := external/kiss_fft/kiss_fft.o
PROCESSING_OBJ := processing.o
AUDIO_IO_OBJ := audio_io.o

# Default target
.PHONY: all clean

all: $(LIB_NAME)

$(LIB_NAME): $(PROCESSING_OBJ) $(AUDIO_IO_OBJ) $(KISS_FFT_OBJ)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(PROCESSING_OBJ) $(AUDIO_IO_OBJ) $(KISS_FFT_OBJ) $(LIB_NAME)

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all     - Build the processing library (default)"
	@echo "  clean   - Remove built objects and library"
	@echo "  help    - Show this message"
