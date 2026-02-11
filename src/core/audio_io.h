#ifndef AUDIO_IO_H
#define AUDIO_IO_H

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 1024

#include <portaudio.h>

/**
 * Initializes PortAudio library.
 * Must be called before any other audio_io functions.
 * 
 * Returns:
 *   0 if initialization succeeds
 *   Non-zero if initialization fails
 */
int audio_init(void);

/**
 * Terminates the PortAudio library.
 * Should be called when done with audio operations.
 * 
 * Returns:
 *   0 if termination succeeds
 *   Non-zero if termination fails
 */
int audio_terminate(void);

/**
 * Enumerates all available audio devices and displays their information.
 * Prints to stdout:
 *   - Device index
 *   - Device name
 *   - Max input/output channels
 *   - Default sample rate
 *   - Default latency (input/output)
 *   - Whether device is default input/output
 * 
 * Returns:
 *   Number of devices found (>= 0)
 *   -1 if an error occurs
 */
int audio_list_devices(void);

/**
 * Gets detailed information about a specific device.
 * 
 * Parameters:
 *   device_index: Index of the device to query
 * 
 * Returns:
 *   Pointer to PaDeviceInfo structure (valid only until next Pa call)
 *   NULL if device_index is invalid
 * 
 * Note: Do not free the returned pointer
 */
const PaDeviceInfo* audio_get_device_info(PaDeviceIndex device_index);

/**
 * Plays audio data from a buffer to a specified output device.
 * 
 * Parameters:
 *   output_device: Device index for playback (use Pa_GetDefaultOutputDevice() for default)
 *   sample_rate: Sampling rate in Hz (e.g., 44100)
 *   buffer: Output audio data (mono, interleaved if stereo)
 *   num_samples: Number of samples in buffer
 *   num_channels: Number of channels (1 for mono, 2 for stereo)
 * 
 * Returns:
 *   0 if playback succeeds
 *   Non-zero if playback fails
 */
int audio_play(PaDeviceIndex output_device, float sample_rate, 
               const float *buffer, int num_samples, int num_channels);

/**
 * Records audio data from a specified input device into a buffer.
 * Blocks until recording is complete.
 * 
 * Parameters:
 *   input_device: Device index for recording (use Pa_GetDefaultInputDevice() for default)
 *   sample_rate: Sampling rate in Hz (e.g., 44100)
 *   buffer: Buffer to store recorded audio (caller must allocate)
 *   num_samples: Maximum number of samples to record
 *   num_channels: Number of channels (1 for mono, 2 for stereo)
 * 
 * Returns:
 *   Number of samples actually recorded (> 0 on success)
 *   -1 if recording fails
 */
int audio_record(PaDeviceIndex input_device, float sample_rate,
                 float *buffer, int num_samples, int num_channels);

/**
 * Performs simultaneous playback and recording (full-duplex operation).
 * 
 * Parameters:
 *   output_device: Device index for playback
 *   input_device: Device index for recording
 *   sample_rate: Sampling rate in Hz (e.g., 44100)
 *   playback_buffer: Audio data to play (caller must allocate)
 *   record_buffer: Buffer to store recorded audio (caller must allocate)
 *   num_samples: Number of samples to play/record
 *   num_channels: Number of channels (1 for mono, 2 for stereo)
 * 
 * Returns:
 *   0 if operation succeeds
 *   Non-zero if operation fails
 * 
 * Note: playback_buffer and record_buffer must be pre-allocated with
 *       at least (num_samples * num_channels) floats
 */
int audio_duplex(PaDeviceIndex output_device, PaDeviceIndex input_device,
                                   float sample_rate,
                                   const float *playback_buffer, float *record_buffer,
                                   int num_samples, int num_channels);

/**
 * Performs simultaneous playback and recording using callback-based I/O.
 * This version is better for async operation with different audio devices
 * as it handles clock domain mismatches more gracefully.
 * 
 * Parameters:
 *   output_device: Device index for playback
 *   input_device: Device index for recording
 *   sample_rate: Sampling rate in Hz (e.g., 44100)
 *   playback_buffer: Audio data to play (caller must allocate)
 *   record_buffer: Buffer to store recorded audio (caller must allocate)
 *   num_samples: Number of samples to play/record
 *   num_channels: Number of channels (1 for mono, 2 for stereo)
 * 
 * Returns:
 *   0 if operation succeeds
 *   Non-zero if operation fails
 * 
 * Note: Use this version when output_device != input_device
 */
int audio_duplex_callback(PaDeviceIndex output_device, PaDeviceIndex input_device,
                          float sample_rate,
                          const float *playback_buffer, float *record_buffer,
                          int num_samples, int num_channels);

#endif
