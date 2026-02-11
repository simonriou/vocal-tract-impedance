#include "user_interface.h"
#include "audio_io.h"
#include <stdio.h>
#include <stdlib.h>

int select_audio_devices(AudioConfig *audio_cfg, int num_devices) {
    printf("\n--- Audio Device Selection ---\n");
    
    printf("Enter input device index: ");
    scanf("%d", &audio_cfg->input_device);
    
    if (audio_cfg->input_device < 0 || audio_cfg->input_device >= num_devices) {
        fprintf(stderr, "Invalid input device index\n");
        return -1;
    }

    const PaDeviceInfo *input_info = audio_get_device_info(audio_cfg->input_device);
    if (!input_info || input_info->maxInputChannels < NUM_CHANNELS) {
        fprintf(stderr, "Selected input device does not support %d channel(s)\n", NUM_CHANNELS);
        return -1;
    }
    
    printf("Enter output device index: ");
    scanf("%d", &audio_cfg->output_device);
    
    if (audio_cfg->output_device < 0 || audio_cfg->output_device >= num_devices) {
        fprintf(stderr, "Invalid output device index\n");
        return -1;
    }

    const PaDeviceInfo *output_info = audio_get_device_info(audio_cfg->output_device);
    if (!output_info || output_info->maxOutputChannels < NUM_CHANNELS) {
        fprintf(stderr, "Selected output device does not support %d channel(s)\n", NUM_CHANNELS);
        return -1;
    }
    
    return 0;
}

int get_chirp_parameters(ChirpParams *chirp_params) {
    printf("\n--- Chirp Parameters ---\n");
    
    printf("Enter chirp duration in seconds: ");
    scanf("%f", &chirp_params->duration);
    if (chirp_params->duration <= 0) {
        fprintf(stderr, "Invalid chirp duration\n");
        return -1;
    }
    
    printf("Enter chirp start frequency (Hz): ");
    scanf("%f", &chirp_params->start_freq);
    if (chirp_params->start_freq <= 0 || chirp_params->start_freq >= SAMPLE_RATE / 2) {
        fprintf(stderr, "Invalid chirp start frequency\n");
        return -1;
    }
    
    printf("Enter chirp end frequency (Hz): ");
    scanf("%f", &chirp_params->end_freq);
    if (chirp_params->end_freq <= 0 || chirp_params->end_freq >= SAMPLE_RATE / 2) {
        fprintf(stderr, "Invalid chirp end frequency\n");
        return -1;
    }
    
    printf("Enter chirp type (linear: l, exponential: e): ");
    char chirp_type;
    scanf(" %c", &chirp_type);
    if (chirp_type == 'e') {
        chirp_params->type = 1;
    } else if (chirp_type == 'l') {
        chirp_params->type = 0;
    } else {
        fprintf(stderr, "Invalid chirp type\n");
        return -1;
    }
    
    printf("Enter chirp amplitude: ");
    scanf("%f", &chirp_params->amplitude);
    if (chirp_params->amplitude < 0.0f) {
        fprintf(stderr, "Invalid chirp amplitude\n");
        return -1;
    }
    
    printf("Enter silence padding duration in seconds (Tgap): ");
    scanf("%f", &chirp_params->Tgap);
    if (chirp_params->Tgap < 0.0f) {
        fprintf(stderr, "Invalid silence padding duration\n");
        return -1;
    }
    
    printf("Enter fade-in/fade-out duration in seconds (Tfade): ");
    scanf("%f", &chirp_params->Tfade);
    if (chirp_params->Tfade < 0.0f) {
        fprintf(stderr, "Invalid fade duration\n");
        return -1;
    }
    
    return 0;
}

int confirm_and_preview(PaDeviceIndex output_device, const float *chirp_buffer, int n_samples) {
    printf("Chirp preview? (y/n): ");
    char preview_choice;
    scanf(" %c", &preview_choice);
    
    if (preview_choice == 'y' || preview_choice == 'Y') {
        if (audio_play(output_device, SAMPLE_RATE, chirp_buffer, n_samples, NUM_CHANNELS) != 0) {
            fprintf(stderr, "Failed to play chirp preview\n");
            return -1;
        }
    }
    
    return 0;
}

void prompt_ready(const char *mode_name) {
    printf("\n%s MODE: Please ensure the microphone/speaker are properly positioned.\n", mode_name);
    printf("When ready, press Enter to start...");
    fflush(stdout);
    
    getchar(); /* Consume leftover newline from previous scanf */
    getchar(); /* Wait for user to press Enter */
}

int prompt_mode_selection(void) {
    int choice;
    printf("\n--- Processing Mode Selection ---\n");
    printf("1. Calibration\n");
    printf("2. Measurement\n");
    printf("3. Processing\n");
    printf("Enter choice: ");
    scanf("%d", &choice);
    
    if (choice < 1 || choice > 3) {
        fprintf(stderr, "Invalid choice\n");
        return -1;
    }
    
    return choice;
}
