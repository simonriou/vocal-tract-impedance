#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include "config.h"

/**
 * Prompts user to select input and output devices.
 * 
 * Parameters:
 *   audio_cfg: Output struct to store selected device indices
 *   num_devices: Number of available devices
 * 
 * Returns:
 *   0 on success, -1 on invalid selection
 */
int select_audio_devices(AudioConfig *audio_cfg, int num_devices);

/**
 * Prompts user for all chirp parameters.
 * Validates all inputs before storing.
 * 
 * Parameters:
 *   chirp_params: Output struct to store parameters
 * 
 * Returns:
 *   0 on success, -1 on invalid input
 */
int get_chirp_parameters(ChirpParams *chirp_params);

/**
 * Offers preview of the generated chirp.
 * 
 * Parameters:
 *   output_device: Device index for playback
 *   chirp_buffer: Audio buffer to preview
 *   n_samples: Number of samples
 * 
 * Returns:
 *   0 on success, -1 on failure
 */
int confirm_and_preview(PaDeviceIndex output_device, const float *chirp_buffer, int n_samples);

/**
 * Prompts user to confirm readiness and waits for Enter.
 * 
 * Parameters:
 *   mode_name: String describing the mode (e.g., "calibration", "measurement")
 */
void prompt_ready(const char *mode_name);

/**
 * Displays mode selection menu and returns user choice.
 * 
 * Returns:
 *   ProcessingMode enum value (MODE_CALIBRATION, MODE_MEASUREMENT, or MODE_PROCESSING)
 *   or -1 on invalid choice
 */
int prompt_mode_selection(void);

#endif
