#include "audio_io.h"
#include <stdio.h>

int main() {
    // Initialize PortAudio
    if (audio_init() != 0) {
        fprintf(stderr, "Failed to initialize audio system.\n");
        return 1;
    }

    printf("Audio system initialized successfully.\n");

    // List available audio devices
    int num_devices = audio_list_devices();
    if (num_devices < 0) {
        fprintf(stderr, "Failed to list audio devices.\n");
        audio_terminate();
        return 1;
    }

    if (audio_terminate() != 0) {
        fprintf(stderr, "Failed to terminate audio system.\n");
        return 1;
    }

    printf("Audio system terminated successfully.\n");

    return 0;
}