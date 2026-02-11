# Project Structure

This document describes the filesystem organization of the project.

## Organization

### `src/core/` - Core Audio & DSP
- **audio_io.c/h**: PortAudio wrapper for device I/O and duplex operations
- **processing.c/h**: Signal processing pipeline (FFT, deconvolution, regularization)
- **complex_utils.h**: Complex number utilities for KissFFT integration

### `src/config/` - Configuration
- **config.h**: Shared data structures (`AudioConfig`, `ChirpParams`, `ProcessingMode`)
- **audio_config.txt**: Audio device configuration file

### `src/interface/` - User Interaction
- **user_interface.c/h**: Command-line prompts and parameter input
  - Device selection
  - Chirp parameter entry
  - Mode selection
  - User confirmations

### `src/orchestration/` - Workflow Coordination
- **pipeline.c/h**: Orchestrates the three processing modes
  - Calibration workflow
  - Measurement workflow
  - Processing workflow
  - File I/O operations

### `tests/` - Test Suite
- **test_inverse.c**: Validates inverse filter quality

### `scripts/` - Analysis Tools
- **plot_frf.py**: Plots frequency response function from CSV
- **plot_results.py**: Visualizes spectrograms and time-domain signals

## Build System

The Makefile compiles all source files with proper include paths:

```makefile
CPPFLAGS: -Iexternal/kiss_fft -Isrc/core -Isrc/config -Isrc/interface -Isrc/orchestration
```

This allows headers to be included by simple names (e.g., `#include "config.h"`) while maintaining clear logical separation.

### Compilation Flow

1. Source files in `src/*/` are compiled to object files in `build/`
2. External dependencies (KissFFT) are compiled once
3. All objects are linked together to create the main executable
4. Tests are compiled separately

## Include Paths

When adding new files, follow these conventions:

- **Local includes** (same module): `#include "header.h"`
- **Cross-module includes** (from different src/ subdirs): `#include "module_name.h"` 
  - CPPFLAGS adds all subdirectories, so no path prefix needed
- **External includes**: `#include "kiss_fft.h"` (CPPFLAGS adds external/kiss_fft)
- **Standard library**: `#include <stdio.h>`

## Git Ignore

Build artifacts and generated results are in `.gitignore`:
- `build/` - Compilation objects and executables
- `output/` - Generated measurement and calibration data
- `*.o` - Object files
- Test executables (`main`, `test_inverse`)

## Adding New Modules

To add a new module (e.g., `analysis`):

1. Create `src/analysis/` directory
2. Add header and source files
3. Update `Makefile`:
   - Add `ANALYSIS_OBJ := $(BUILD_DIR)/analysis.o`
   - Add CPPFLAGS: `-Isrc/analysis`
   - Add compilation rule for analysis.c
   - Add object to relevant target dependency
4. Include headers as needed: `#include "analysis_header.h"`

## Testing

Run tests with:
```bash
make test_inverse          # Build and create executable
./test_inverse             # Run the test
```

## Cleanup

```bash
make clean                 # Remove build artifacts
```

Complete cleanup (including output):
```bash
rm -rf build output/*
```
