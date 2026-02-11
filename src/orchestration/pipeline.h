#ifndef PIPELINE_H
#define PIPELINE_H

#include "config.h"

/**
 * Calculates the next power of 2 greater than or equal to n.
 * 
 * Parameters:
 *   n: Input value
 * 
 * Returns:
 *   Smallest power of 2 >= n
 */
int calculate_next_power_of_two(int n);

/**
 * Saves recorded response and chirp to binary files.
 * 
 * Parameters:
 *   response_buffer: Recorded audio data
 *   chirp_buffer: Sent chirp data
 *   n_samples: Number of samples
 *   is_calibration: 1 for calibration mode, 0 for measurement
 * 
 * Returns:
 *   0 on success, -1 on failure
 */
int save_response_files(const float *response_buffer, const float *chirp_buffer, 
                       int n_samples, int is_calibration);

/**
 * Saves calibration parameters to text file.
 * 
 * Parameters:
 *   chirp_params: Chirp parameters to save
 * 
 * Returns:
 *   0 on success, -1 on failure
 */
int save_calibration_parameters(const ChirpParams *chirp_params);

/**
 * Runs the calibration workflow.
 * Records system response with closed mouth configuration.
 * 
 * Parameters:
 *   audio_cfg: Audio device configuration
 *   chirp_params: Chirp parameters
 *   recording_duration: Total recording duration in seconds
 * 
 * Returns:
 *   0 on success, -1 on failure
 */
int run_calibration_mode(const AudioConfig *audio_cfg, const ChirpParams *chirp_params, 
                        float recording_duration);

/**
 * Runs the measurement workflow.
 * Records system response with open mouth configuration.
 * 
 * Parameters:
 *   audio_cfg: Audio device configuration
 *   chirp_params: Chirp parameters
 *   recording_duration: Total recording duration in seconds
 * 
 * Returns:
 *   0 on success, -1 on failure
 */
int run_measurement_mode(const AudioConfig *audio_cfg, const ChirpParams *chirp_params, 
                        float recording_duration);

/**
 * Runs the processing workflow.
 * Loads calibration and measurement data, performs analysis.
 * 
 * Parameters:
 *   chirp_params: Chirp parameters (for inverse filter generation)
 * 
 * Returns:
 *   0 on success, -1 on failure
 */
int run_processing_mode(const ChirpParams *chirp_params);

#endif
