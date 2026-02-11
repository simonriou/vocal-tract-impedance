#ifndef CONFIG_H
#define CONFIG_H

#include <portaudio.h>

/* Audio configuration */
typedef struct {
    PaDeviceIndex input_device;
    PaDeviceIndex output_device;
} AudioConfig;

/* Chirp parameters */
typedef struct {
    float amplitude;
    float start_freq;
    float end_freq;
    float duration;
    int type; /* 0 = linear, 1 = exponential */
    float Tgap; /* Silence padding (s) - split equally before and after chirp */
    float Tfade; /* Fade-in/fade-out duration (s) */
} ChirpParams;

/* Processing modes */
typedef enum {
    MODE_CALIBRATION = 1,
    MODE_MEASUREMENT = 2,
    MODE_PROCESSING = 3
} ProcessingMode;

/* Global constants */
#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1
#define DEFAULT_FFT_PADDING_FACTOR 1 /* FFT size = smallest power of 2 >= n_samples */

#endif
