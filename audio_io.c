#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "audio_io.h"

int audio_init(void) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio initialization failed: %s\n", Pa_GetErrorText(err));
        return -1;
    }
    return 0;
}

int audio_terminate(void) {
    PaError err = Pa_Terminate();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio termination failed: %s\n", Pa_GetErrorText(err));
        return -1;
    }
    return 0;
}

int audio_list_devices(void) {
    int num_devices = Pa_GetDeviceCount();
    if (num_devices < 0) {
        fprintf(stderr, "Error getting device count: %s\n", Pa_GetErrorText(num_devices));
        return -1;
    }

    printf("Available audio devices:\n");
    for (int i = 0; i < num_devices; i++) {
        const PaDeviceInfo* info = audio_get_device_info(i);
        if (info) {
            printf("Device %d: %s\n", i, info->name);
            printf("  ------ Max input channels: %d\n", info->maxInputChannels);
            printf("  ------ Max output channels: %d\n", info->maxOutputChannels);
            printf("  ------ Default sample rate: %.2f\n", info->defaultSampleRate);
            printf("  ------ Default latency (input): %.2f ms\n", info->defaultLowInputLatency * 1000);
            printf("  ------ Default latency (output): %.2f ms\n", info->defaultLowOutputLatency * 1000);
            printf("  ------ Is default input: %s\n", (i == Pa_GetDefaultInputDevice()) ? "Yes" : "No");
            printf("  ------ Is default output: %s\n", (i == Pa_GetDefaultOutputDevice()) ? "Yes" : "No");
        }
    }
    return num_devices;
}

const PaDeviceInfo* audio_get_device_info(PaDeviceIndex device_index) {
    const PaDeviceInfo* info = Pa_GetDeviceInfo(device_index);
    if (!info) {
        fprintf(stderr, "Invalid device index %d: %s\n", device_index, Pa_GetErrorText(device_index));
        return NULL;
    }
    return info;
}

int audio_play(PaDeviceIndex output_device, float sample_rate, 
               const float *buffer, int num_samples, int num_channels) {
    if (!buffer || num_samples <= 0 || num_channels <= 0) {
        fprintf(stderr, "audio_play: Invalid parameters\n");
        return -1;
    }

    // Open output stream
    PaStream *stream;
    PaStreamParameters output_params;
    
    output_params.device = output_device;
    output_params.channelCount = num_channels;
    output_params.sampleFormat = paFloat32;
    output_params.suggestedLatency = Pa_GetDeviceInfo(output_device)->defaultLowOutputLatency;
    output_params.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
        &stream,
        NULL,  // No input
        &output_params,
        sample_rate,
        FRAMES_PER_BUFFER,
        paClipOff,  // Don't clip
        NULL,       // Use blocking I/O
        NULL
    );

    if (err != paNoError) {
        fprintf(stderr, "Failed to open output stream: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    // Start stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "Failed to start output stream: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        return -1;
    }

    // Write audio data in chunks
    int samples_written = 0;
    int total_frames = num_samples;
    
    while (samples_written < total_frames) {
        int frames_to_write = (total_frames - samples_written < FRAMES_PER_BUFFER) 
                               ? (total_frames - samples_written) 
                               : FRAMES_PER_BUFFER;
        
        err = Pa_WriteStream(stream, 
                            &buffer[samples_written * num_channels], 
                            frames_to_write);
        
        if (err != paNoError) {
            fprintf(stderr, "Error writing to output stream: %s\n", Pa_GetErrorText(err));
            Pa_StopStream(stream);
            Pa_CloseStream(stream);
            return -1;
        }
        
        samples_written += frames_to_write;
    }

    // Stop and close stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "Error stopping output stream: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        return -1;
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "Error closing output stream: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    return 0;
}

int audio_record(PaDeviceIndex input_device, float sample_rate,
                 float *buffer, int num_samples, int num_channels) {
    if (!buffer || num_samples <= 0 || num_channels <= 0) {
        fprintf(stderr, "audio_record: Invalid parameters\n");
        return -1;
    }

    // Open input stream
    PaStream *stream;
    PaStreamParameters input_params;
    
    input_params.device = input_device;
    input_params.channelCount = num_channels;
    input_params.sampleFormat = paFloat32;
    input_params.suggestedLatency = Pa_GetDeviceInfo(input_device)->defaultLowInputLatency;
    input_params.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
        &stream,
        &input_params,
        NULL,  // No output
        sample_rate,
        FRAMES_PER_BUFFER,
        paClipOff,
        NULL,  // Use blocking I/O
        NULL
    );

    if (err != paNoError) {
        fprintf(stderr, "Failed to open input stream: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    // Start stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "Failed to start input stream: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        return -1;
    }

    // Read audio data in chunks
    int samples_read = 0;
    int total_frames = num_samples;
    
    while (samples_read < total_frames) {
        int frames_to_read = (total_frames - samples_read < FRAMES_PER_BUFFER) 
                              ? (total_frames - samples_read) 
                              : FRAMES_PER_BUFFER;
        
        err = Pa_ReadStream(stream, 
                           &buffer[samples_read * num_channels], 
                           frames_to_read);
        
        if (err != paNoError) {
            fprintf(stderr, "Error reading from input stream: %s\n", Pa_GetErrorText(err));
            Pa_StopStream(stream);
            Pa_CloseStream(stream);
            return -1;
        }
        
        samples_read += frames_to_read;
    }

    // Stop and close stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "Error stopping input stream: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        return -1;
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "Error closing input stream: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    return samples_read;
}

int audio_duplex(PaDeviceIndex output_device, PaDeviceIndex input_device,
                                   float sample_rate,
                                   const float *playback_buffer, float *record_buffer,
                                   int num_samples, int num_channels) {
    if (!playback_buffer || !record_buffer || num_samples <= 0 || num_channels <= 0) {
        fprintf(stderr, "audio_duplex: Invalid parameters\n");
        return -1;
    }

    // Open full-duplex stream
    PaStream *stream;
    PaStreamParameters input_params, output_params;
    
    // Configure input parameters
    input_params.device = input_device;
    input_params.channelCount = num_channels;
    input_params.sampleFormat = paFloat32;
    input_params.suggestedLatency = Pa_GetDeviceInfo(input_device)->defaultLowInputLatency;
    input_params.hostApiSpecificStreamInfo = NULL;

    // Configure output parameters
    output_params.device = output_device;
    output_params.channelCount = num_channels;
    output_params.sampleFormat = paFloat32;
    output_params.suggestedLatency = Pa_GetDeviceInfo(output_device)->defaultLowOutputLatency;
    output_params.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
        &stream,
        &input_params,
        &output_params,
        sample_rate,
        FRAMES_PER_BUFFER,
        paClipOff,
        NULL,  // Use blocking I/O
        NULL
    );

    if (err != paNoError) {
        fprintf(stderr, "Failed to open full-duplex stream: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    // Start stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "Failed to start full-duplex stream: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        return -1;
    }

    // Perform simultaneous playback and recording
    int samples_processed = 0;
    int total_frames = num_samples;
    
    while (samples_processed < total_frames) {
        int frames_to_process = (total_frames - samples_processed < FRAMES_PER_BUFFER) 
                                 ? (total_frames - samples_processed) 
                                 : FRAMES_PER_BUFFER;
        
        // Read from input
        err = Pa_ReadStream(stream,
                           &record_buffer[samples_processed * num_channels],
                           frames_to_process);
        
        if (err != paNoError) {
            fprintf(stderr, "Error reading from full-duplex stream: %s\n", Pa_GetErrorText(err));
            Pa_StopStream(stream);
            Pa_CloseStream(stream);
            return -1;
        }

        // Write to output
        err = Pa_WriteStream(stream,
                            &playback_buffer[samples_processed * num_channels],
                            frames_to_process);
        
        if (err != paNoError) {
            fprintf(stderr, "Error writing to full-duplex stream: %s\n", Pa_GetErrorText(err));
            Pa_StopStream(stream);
            Pa_CloseStream(stream);
            return -1;
        }
        
        samples_processed += frames_to_process;
    }

    // Stop and close stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "Error stopping full-duplex stream: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        return -1;
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "Error closing full-duplex stream: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    return 0;
}

// Callback data structure
typedef struct {
    const float *playback_buffer;
    float *record_buffer;
    int frame_index;
    int max_frames;
    int num_channels;
    int finished;
} CallbackData;

// Audio callback function for duplex operation
static int duplex_callback(const void *input_buffer, void *output_buffer,
                          unsigned long frames_per_buffer,
                          const PaStreamCallbackTimeInfo *time_info,
                          PaStreamCallbackFlags status_flags,
                          void *user_data) {
    CallbackData *data = (CallbackData *)user_data;
    const float *in = (const float *)input_buffer;
    float *out = (float *)output_buffer;
    
    (void)time_info; // Prevent unused variable warning
    
    // Check for buffer issues
    if (status_flags & paInputOverflow) {
        fprintf(stderr, "Warning: Input overflow detected in callback\n");
    }
    if (status_flags & paOutputUnderflow) {
        fprintf(stderr, "Warning: Output underflow detected in callback\n");
    }
    
    unsigned long frames_to_process = frames_per_buffer;
    unsigned long frames_left = data->max_frames - data->frame_index;
    
    if (frames_to_process > frames_left) {
        frames_to_process = frames_left;
    }
    
    // Copy input to record buffer
    if (in != NULL) {
        for (unsigned long i = 0; i < frames_to_process; i++) {
            for (int ch = 0; ch < data->num_channels; ch++) {
                data->record_buffer[(data->frame_index + i) * data->num_channels + ch] = 
                    in[i * data->num_channels + ch];
            }
        }
    }
    
    // Copy playback buffer to output
    if (out != NULL) {
        for (unsigned long i = 0; i < frames_to_process; i++) {
            for (int ch = 0; ch < data->num_channels; ch++) {
                out[i * data->num_channels + ch] = 
                    data->playback_buffer[(data->frame_index + i) * data->num_channels + ch];
            }
        }
        
        // Zero out remaining frames if we've reached the end
        if (frames_to_process < frames_per_buffer) {
            for (unsigned long i = frames_to_process; i < frames_per_buffer; i++) {
                for (int ch = 0; ch < data->num_channels; ch++) {
                    out[i * data->num_channels + ch] = 0.0f;
                }
            }
        }
    }
    
    data->frame_index += frames_to_process;
    
    // Check if we're done
    if (data->frame_index >= data->max_frames) {
        data->finished = 1;
        return paComplete;
    }
    
    return paContinue;
}

int audio_duplex_callback(PaDeviceIndex output_device, PaDeviceIndex input_device,
                          float sample_rate,
                          const float *playback_buffer, float *record_buffer,
                          int num_samples, int num_channels) {
    if (!playback_buffer || !record_buffer || num_samples <= 0 || num_channels <= 0) {
        fprintf(stderr, "audio_duplex_callback: Invalid parameters\n");
        return -1;
    }

    // Initialize callback data
    CallbackData callback_data;
    callback_data.playback_buffer = playback_buffer;
    callback_data.record_buffer = record_buffer;
    callback_data.frame_index = 0;
    callback_data.max_frames = num_samples;
    callback_data.num_channels = num_channels;
    callback_data.finished = 0;

    // Open full-duplex stream with callback
    PaStream *stream;
    PaStreamParameters input_params, output_params;
    
    // Configure input parameters with higher latency for different devices
    input_params.device = input_device;
    input_params.channelCount = num_channels;
    input_params.sampleFormat = paFloat32;
    // Always use high latency for better stability and to prevent overflow
    input_params.suggestedLatency = Pa_GetDeviceInfo(input_device)->defaultHighInputLatency;
    input_params.hostApiSpecificStreamInfo = NULL;

    // Configure output parameters
    output_params.device = output_device;
    output_params.channelCount = num_channels;
    output_params.sampleFormat = paFloat32;
    output_params.suggestedLatency = Pa_GetDeviceInfo(output_device)->defaultHighOutputLatency;
    output_params.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
        &stream,
        &input_params,
        &output_params,
        sample_rate,
        FRAMES_PER_BUFFER,
        paClipOff,  // Don't clip, let us handle it
        duplex_callback,
        &callback_data
    );

    if (err != paNoError) {
        fprintf(stderr, "Failed to open callback-based full-duplex stream: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    // Start stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "Failed to start callback-based full-duplex stream: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        return -1;
    }

    // Wait for stream to complete
    while (Pa_IsStreamActive(stream) == 1) {
        Pa_Sleep(10); // Sleep for 10ms to avoid busy waiting
    }

    // Stop and close stream
    err = Pa_StopStream(stream);
    if (err != paNoError && err != paStreamIsStopped) {
        fprintf(stderr, "Error stopping callback-based stream: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        return -1;
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "Error closing callback-based stream: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    return 0;
}

