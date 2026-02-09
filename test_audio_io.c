#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "audio_io.h"

#define TEST_DURATION 2.0f
#define TEST_SAMPLE_RATE 44100.0f
#define NUM_CHANNELS 1

/**
 * Generates a simple sine wave test signal
 * Frequency: 440 Hz (A4 note)
 * Duration: TEST_DURATION seconds
 */
void generate_sine_wave(float *buffer, int num_samples, float freq, float amplitude) {
    for (int i = 0; i < num_samples; i++) {
        float t = (float)i / TEST_SAMPLE_RATE;
        buffer[i] = amplitude * sinf(2.0f * M_PI * freq * t);
    }
}

/**
 * Generates a chirp signal (frequency sweep)
 * Start: 100 Hz, End: 1000 Hz
 */
void generate_chirp_signal(float *buffer, int num_samples, float f_start, float f_end, float amplitude) {
    float duration = (float)num_samples / TEST_SAMPLE_RATE;
    for (int i = 0; i < num_samples; i++) {
        float t = (float)i / TEST_SAMPLE_RATE;
        float freq = f_start + (f_end - f_start) * (t / duration);
        float phase = 2.0f * M_PI * (f_start * t + (f_end - f_start) * t * t / (2.0f * duration));
        buffer[i] = amplitude * sinf(phase);
    }
}

/**
 * Test 1: List all available audio devices
 */
void test_list_devices(void) {
    printf("\n========== TEST: List Available Devices ==========\n");
    int num_devices = audio_list_devices();
    if (num_devices < 0) {
        fprintf(stderr, "Failed to list devices\n");
        return;
    }
    printf("Total devices found: %d\n\n", num_devices);
}

/**
 * Test 2: Playback test - Play a sine wave
 */
void test_playback(void) {
    printf("\n========== TEST: Playback Test ==========\n");
    printf("This test will play a 440 Hz sine wave for %.1f seconds\n", TEST_DURATION);
    
    int num_samples = (int)(TEST_SAMPLE_RATE * TEST_DURATION);
    float *buffer = (float *)malloc(num_samples * NUM_CHANNELS * sizeof(float));
    
    if (!buffer) {
        fprintf(stderr, "Failed to allocate playback buffer\n");
        return;
    }

    // Generate test signal
    printf("Generating test signal...\n");
    generate_sine_wave(buffer, num_samples, 440.0f, 0.3f);

    // Get device selection
    printf("\nSelect output device index (or -1 for default): ");
    PaDeviceIndex output_device;
    int result = scanf("%d", (int *)&output_device);
    if (result != 1) {
        fprintf(stderr, "Invalid input\n");
        free(buffer);
        return;
    }

    if (output_device == -1) {
        output_device = Pa_GetDefaultOutputDevice();
        printf("Using default output device: %d\n", output_device);
    }

    printf("Starting playback on device %d...\n", output_device);
    int ret = audio_play(output_device, TEST_SAMPLE_RATE, buffer, num_samples, NUM_CHANNELS);
    
    if (ret == 0) {
        printf("Playback completed successfully\n");
    } else {
        fprintf(stderr, "Playback failed\n");
    }

    free(buffer);
}

/**
 * Test 3: Recording test - Record audio
 */
void test_record(void) {
    printf("\n========== TEST: Recording Test ==========\n");
    printf("This test will record audio for %.1f seconds\n", TEST_DURATION);
    
    int num_samples = (int)(TEST_SAMPLE_RATE * TEST_DURATION);
    float *buffer = (float *)malloc(num_samples * NUM_CHANNELS * sizeof(float));
    
    if (!buffer) {
        fprintf(stderr, "Failed to allocate recording buffer\n");
        return;
    }

    // Zero the buffer
    memset(buffer, 0, num_samples * NUM_CHANNELS * sizeof(float));

    // Get device selection
    printf("\nSelect input device index (or -1 for default): ");
    PaDeviceIndex input_device;
    int result = scanf("%d", (int *)&input_device);
    if (result != 1) {
        fprintf(stderr, "Invalid input\n");
        free(buffer);
        return;
    }

    if (input_device == -1) {
        input_device = Pa_GetDefaultInputDevice();
        printf("Using default input device: %d\n", input_device);
    }

    printf("Starting recording on device %d... Please speak or make sounds!\n", input_device);
    int samples_recorded = audio_record(input_device, TEST_SAMPLE_RATE, buffer, num_samples, NUM_CHANNELS);
    
    if (samples_recorded > 0) {
        printf("Recording completed successfully: %d samples recorded\n", samples_recorded);
        
        // Calculate and display statistics
        float max_val = 0.0f, rms = 0.0f;
        for (int i = 0; i < samples_recorded; i++) {
            float val = fabsf(buffer[i]);
            if (val > max_val) max_val = val;
            rms += buffer[i] * buffer[i];
        }
        rms = sqrtf(rms / samples_recorded);
        
        printf("Recording statistics:\n");
        printf("  Peak level: %.4f\n", max_val);
        printf("  RMS level: %.4f\n", rms);
        printf("  Duration: %.2f seconds\n", (float)samples_recorded / TEST_SAMPLE_RATE);
        
        // Ask if user wants to playback the recording
        // Clear input buffer first
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        
        printf("\nWould you like to playback the recording? (y/n): ");
        char response;
        if (scanf("%c", &response) == 1) {
            // Clear input buffer
            while ((c = getchar()) != '\n' && c != EOF);
            
            if (response == 'y' || response == 'Y') {
                printf("Select output device for playback (or -1 for default): ");
                PaDeviceIndex playback_device;
                if (scanf("%d", (int *)&playback_device) == 1) {
                    // Clear input buffer
                    while ((c = getchar()) != '\n' && c != EOF);
                    
                    if (playback_device == -1) {
                        playback_device = Pa_GetDefaultOutputDevice();
                        printf("Using default output device: %d\n", playback_device);
                    }
                    printf("Playing back recording...\n");
                    if (audio_play(playback_device, TEST_SAMPLE_RATE, buffer, samples_recorded, NUM_CHANNELS) == 0) {
                        printf("Playback completed successfully\n");
                    } else {
                        fprintf(stderr, "Playback failed\n");
                    }
                }
            }
        }
    } else {
        fprintf(stderr, "Recording failed\n");
    }

    free(buffer);
}

/**
 * Test 4: Full-duplex test - Play while recording
 */
void test_duplex(void) {
    printf("\n========== TEST: Full-Duplex Test (Play & Record) ==========\n");
    printf("This test will play a chirp while recording for %.1f seconds\n", TEST_DURATION);
    
    int num_samples = (int)(TEST_SAMPLE_RATE * TEST_DURATION);
    float *playback_buffer = (float *)malloc(num_samples * NUM_CHANNELS * sizeof(float));
    float *record_buffer = (float *)malloc(num_samples * NUM_CHANNELS * sizeof(float));
    
    if (!playback_buffer || !record_buffer) {
        fprintf(stderr, "Failed to allocate buffers\n");
        free(playback_buffer);
        free(record_buffer);
        return;
    }

    // Generate test signal (chirp from 100 Hz to 1000 Hz)
    printf("Generating chirp signal (100 Hz -> 1000 Hz)...\n");
    generate_chirp_signal(playback_buffer, num_samples, 100.0f, 1000.0f, 1.0f);
    memset(record_buffer, 0, num_samples * NUM_CHANNELS * sizeof(float));

    // Get device selection
    printf("\nSelect output device index (or -1 for default): ");
    PaDeviceIndex output_device;
    int result = scanf("%d", (int *)&output_device);
    if (result != 1) {
        fprintf(stderr, "Invalid input\n");
        free(playback_buffer);
        free(record_buffer);
        return;
    }

    if (output_device == -1) {
        output_device = Pa_GetDefaultOutputDevice();
        printf("Using default output device: %d\n", output_device);
    }

    printf("Select input device index (or -1 for default): ");
    PaDeviceIndex input_device;
    result = scanf("%d", (int *)&input_device);
    if (result != 1) {
        fprintf(stderr, "Invalid input\n");
        free(playback_buffer);
        free(record_buffer);
        return;
    }

    if (input_device == -1) {
        input_device = Pa_GetDefaultInputDevice();
        printf("Using default input device: %d\n", input_device);
    }

    printf("Starting full-duplex operation with callback-based I/O...\n");
    int ret = audio_duplex_callback(output_device, input_device, 
                                     TEST_SAMPLE_RATE, 
                                     playback_buffer, record_buffer,
                                     num_samples, NUM_CHANNELS);
    
    if (ret == 0) {
        printf("Full-duplex operation completed successfully\n");
        
        // Calculate and display recording statistics
        float max_val = 0.0f, rms = 0.0f;
        for (int i = 0; i < num_samples; i++) {
            float val = fabsf(record_buffer[i]);
            if (val > max_val) max_val = val;
            rms += record_buffer[i] * record_buffer[i];
        }
        rms = sqrtf(rms / num_samples);
        
        printf("Recording statistics:\n");
        printf("  Peak level: %.4f\n", max_val);
        printf("  RMS level: %.4f\n", rms);
        
        // Ask if user wants to playback the recording
        // Ask if user wants to playback the recorded audio
        // Clear input buffer first
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        
        printf("\nWould you like to playback the recorded audio? (y/n): ");
        char response;
        if (scanf("%c", &response) == 1) {
            // Clear input buffer
            while ((c = getchar()) != '\n' && c != EOF);
            
            if (response == 'y' || response == 'Y') {
                printf("Select output device for playback (or -1 for default): ");
                PaDeviceIndex playback_device;
                if (scanf("%d", (int *)&playback_device) == 1) {
                    // Clear input buffer
                    while ((c = getchar()) != '\n' && c != EOF);
                    
                    if (playback_device == -1) {
                        playback_device = Pa_GetDefaultOutputDevice();
                        printf("Using default output device: %d\n", playback_device);
                    }
                    printf("Playing back recorded audio...\n");
                    if (audio_play(playback_device, TEST_SAMPLE_RATE, record_buffer, num_samples, NUM_CHANNELS) == 0) {
                        printf("Playback completed successfully\n");
                    } else {
                        fprintf(stderr, "Playback failed\n");
                    }
                }
            }
        }
    } else {
        fprintf(stderr, "Full-duplex operation failed\n");
    }

    free(playback_buffer);
    free(record_buffer);
}

/**
 * Display test menu
 */
void display_menu(void) {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║         Audio I/O Module Test.         ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n");
    printf("Available Tests:\n");
    printf("  1 - List available audio devices\n");
    printf("  2 - Test audio playback (sine wave)\n");
    printf("  3 - Test audio recording\n");
    printf("  4 - Test full-duplex (play & record)\n");
    printf("  0 - Exit\n");
    printf("\n");
}

int main(void) {
    printf("Initializing PortAudio...\n");
    if (audio_init() != 0) {
        fprintf(stderr, "Failed to initialize PortAudio\n");
        return EXIT_FAILURE;
    }

    int choice = -1;
    while (choice != 0) {
        display_menu();
        printf("Enter test number: ");
        int result = scanf("%d", &choice);
        
        // Clear input buffer
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        
        if (result != 1) {
            fprintf(stderr, "Invalid input\n");
            continue;
        }

        switch (choice) {
            case 1:
                test_list_devices();
                break;
            case 2:
                test_playback();
                break;
            case 3:
                test_record();
                break;
            case 4:
                test_duplex();
                break;
            case 0:
                printf("Exiting...\n");
                break;
            default:
                fprintf(stderr, "Invalid choice\n");
        }
    }

    printf("Terminating PortAudio...\n");
    audio_terminate();
    
    printf("Test suite completed\n");
    return EXIT_SUCCESS;
}
