# Project Structure

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


# Overall Pipeline

1. **Signal Acquisition**: Send chirp to output device and record the response from the input device simultaneously (using duplex callback).

2. **Time Alignment**: Align the recorded response with the original chirp signal in the time domain using cross-correlation. Find the peak to determine the time delay.

3. **Inverse Filter**: Depending on the chirp type (linear or exponential), compute the inverse filter of the chirp in the frequency domain.

4. **Impulse Response**: Convolve the recorded response with the inverse filter to obtain the impulse response of the system:
   $$G_1(\omega) \cdot P_{\text{closed}}(\omega)$$
   where $G_1$ is the gain associated with the linear part of the system and $P_{\text{closed}}$ is the transfer function of the closed/open tract.

5. **Regularization**: Compute the regularization $\epsilon(\omega)$:
   - $\epsilon(\omega) = 0$ inside $[f_0, f_1]$
   - $\epsilon(\omega) = W_e \cdot T(f)$ outside $[f_0, f_1]$
   
   where $W_e = \int_0^{f_s/2} |G_1(f) P_{\text{open}}(f)|^2 \, df$ and $T(f)$ is a transition function:
   $$T(f) = \frac{1}{2} \left(1 + \tanh\left(\frac{1}{f_a - f} + \frac{1}{f_b - f}\right)\right)$$
   with $f_{a0} = f_0$ and $f_{b0} = f_0 - 50 \text{ Hz}$, then at the end of the recording, $f_{a1} = f_1$ and $f_{b1} = f_1 + 50 \text{ Hz}$.

6. **Frequency Response**: Compute the frequency response function $H_{\text{lips}}$:
   $$H_{\text{lips}} = \frac{P_{\text{open}} \cdot \overline{P_{\text{closed}}}}{|P_{\text{closed}}|^2 + \epsilon}$$

# Credits
This work is based on the thesis of Thimotée MAISON, "Towards the characterization of dynamical resonators: measuring vocal tract resonances in singing", 2023. The code structure and processing pipeline are inspired by the methodologies described in the thesis, with adaptations for real-time audio processing and user interaction.
The FFT logic is taken from [KissFFT](https://github.com/mborgerding/kissfft), and the audio processing is done using [PortAudio](http://www.portaudio.com/). The code is written in C. Visualisation is done in Python using Matplotlib.