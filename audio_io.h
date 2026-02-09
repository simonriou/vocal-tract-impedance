#ifndef AUDIO_IO_H
#define AUDIO_IO_H

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

#endif
