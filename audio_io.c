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
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info) {
            printf("Device %d: %s\n", i, info->name);
            printf("  Max input channels: %d\n", info->maxInputChannels);
            printf("  Max output channels: %d\n", info->maxOutputChannels);
            printf("  Default sample rate: %.2f\n", info->defaultSampleRate);
            printf("  Default latency (input): %.2f ms\n", info->defaultLowInputLatency * 1000);
            printf("  Default latency (output): %.2f ms\n", info->defaultLowOutputLatency * 1000);
            printf("  Is default input: %s\n", (i == Pa_GetDefaultInputDevice()) ? "Yes" : "No");
            printf("  Is default output: %s\n", (i == Pa_GetDefaultOutputDevice()) ? "Yes" : "No");
        }
    }
    return num_devices;
}