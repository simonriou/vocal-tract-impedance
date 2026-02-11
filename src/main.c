#include <stdio.h>
#include <stdlib.h>

#include "audio_io.h"
#include "config.h"
#include "user_interface.h"
#include "pipeline.h"

int main() {
    /* Initialize audio system */
    if (audio_init() != 0) {
        fprintf(stderr, "Failed to initialize audio system\n");
        return -1;
    }
    printf("Audio system initialized successfully.\n");
    
    /* List and select devices */
    int num_devices = audio_list_devices();
    if (num_devices <= 0) {
        fprintf(stderr, "No audio devices found\n");
        audio_terminate();
        return -1;
    }
    
    AudioConfig audio_cfg;
    if (select_audio_devices(&audio_cfg, num_devices) != 0) {
        audio_terminate();
        return -1;
    }
    
    /* Get chirp parameters */
    ChirpParams chirp_params;
    if (get_chirp_parameters(&chirp_params) != 0) {
        audio_terminate();
        return -1;
    }
    
    /* Prompt for recording duration */
    printf("\nEnter recording duration in seconds: ");
    float recording_duration;
    scanf("%f", &recording_duration);
    if (recording_duration <= 0) {
        fprintf(stderr, "Invalid recording duration\n");
        audio_terminate();
        return -1;
    }
    
    if (recording_duration < chirp_params.duration) {
        printf("Warning: recording duration is shorter than chirp duration.\n");
    }
    
    /* Select processing mode */
    int mode = prompt_mode_selection();
    if (mode < 0) {
        audio_terminate();
        return -1;
    }
    
    int ret = 0;
    
    /* Execute selected mode */
    switch (mode) {
        case MODE_CALIBRATION:
            ret = run_calibration_mode(&audio_cfg, &chirp_params, recording_duration);
            break;
        case MODE_MEASUREMENT:
            ret = run_measurement_mode(&audio_cfg, &chirp_params, recording_duration);
            break;
        case MODE_PROCESSING:
            ret = run_processing_mode(&chirp_params);
            break;
        default:
            fprintf(stderr, "Invalid mode\n");
            ret = -1;
            break;
    }
    
    /* Cleanup */
    audio_terminate();
    
    return ret;
}