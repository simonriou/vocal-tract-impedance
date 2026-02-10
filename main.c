#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "audio_io.h"
#include "processing.h"

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1

int main() {
    // --- initialisation audio ---
    if (audio_init() != 0) {
        fprintf(stderr, "Failed to initialize audio system\n");
        return -1;
    }

    printf("Audio system initialized successfully.\n");

    // --- sélection des pérophériques i/o ---
    int num_devices = audio_list_devices();
    if (num_devices <= 0) {
        fprintf(stderr, "No audio devices found\n");
        audio_terminate();
        return -1;
    }
    
    PaDeviceIndex input_device = -1;
    PaDeviceIndex output_device = -1;
    printf("Enter input device index: ");
    scanf("%d", &input_device);

    if (input_device < 0 || input_device >= num_devices) {
        fprintf(stderr, "Invalid input device index\n");
        audio_terminate();
        return -1;
    }

    printf("Enter output device index: ");
    scanf("%d", &output_device);

    if (output_device < 0 || output_device >= num_devices) {
        fprintf(stderr, "Invalid output device index\n");
        audio_terminate();
        return -1;
    }

    // --- paramètres du chirp (durée, type, amplitude, fréquences) ---
    printf("Enter chirp duration in seconds: ");
    float chirp_duration;
    scanf("%f", &chirp_duration);
    if (chirp_duration <= 0) {
        fprintf(stderr, "Invalid chirp duration\n");
        audio_terminate();
        return -1;
    }

    printf("Enter recording duration in seconds: ");
    float recording_duration;
    scanf("%f", &recording_duration);
    if (recording_duration <= 0) {
        fprintf(stderr, "Invalid recording duration\n");
        audio_terminate();
        return -1;
    }

    if (recording_duration < chirp_duration) {
        printf("Warning: recording duration is shorter than chirp duration. Recording will be truncated.\n");
    }

    printf("Enter chirp start frequency (Hz): ");
    float chirp_start_freq;
    scanf("%f", &chirp_start_freq);
    if (chirp_start_freq <= 0 || chirp_start_freq >= SAMPLE_RATE / 2) {
        fprintf(stderr, "Invalid chirp start frequency\n");
        audio_terminate();
        return -1;
    }

    printf("Enter chirp end frequency (Hz): ");
    float chirp_end_freq;
    scanf("%f", &chirp_end_freq);
    if (chirp_end_freq <= 0 || chirp_end_freq >= SAMPLE_RATE / 2) {
        fprintf(stderr, "Invalid chirp end frequency\n");
        audio_terminate();
        return -1;
    }

    printf("Enter chirp type (linear: l, exponential: e): ");
    char chirp_type;
    scanf(" %c", &chirp_type);
    if (chirp_type != 'l' && chirp_type != 'e') {
        fprintf(stderr, "Invalid chirp type\n");
        audio_terminate();
        return -1;
    }

    printf("Enter chirp amplitude: ");
    float chirp_amplitude;
    scanf("%f", &chirp_amplitude);
    if (chirp_amplitude < 0.0f) {
        fprintf(stderr, "Invalid chirp amplitude\n");
        audio_terminate();
        return -1;
    }

    // --- allocation chirp ---
    int n_samples_chirp = (int)(SAMPLE_RATE * chirp_duration);
    int n_samples_record = (int)(SAMPLE_RATE * recording_duration);
    
    float *chirp_buffer = (float*)malloc(sizeof(float) * n_samples_record);
    if (!chirp_buffer) {
        fprintf(stderr, "Failed to allocate chirp buffer\n");
        audio_terminate();
        return -1;
    }

    // Generate chirp in the first part of the buffer
    generate_chirp(chirp_buffer, chirp_amplitude, chirp_start_freq, chirp_end_freq, chirp_duration, SAMPLE_RATE, (chirp_type == 'l') ? 0 : 1);
    
    // Fill the rest with silence if recording_duration > chirp_duration
    for (int i = n_samples_chirp; i < n_samples_record; i++) {
        chirp_buffer[i] = 0.0f;
    }

    // --- preview du chirp dans le device de sortie ---
    printf("Chirp preview? (y/n): ");
    char preview_choice;
    scanf(" %c", &preview_choice);
    if (preview_choice == 'y' || preview_choice == 'Y') {
        if (audio_play(output_device, SAMPLE_RATE, chirp_buffer, n_samples_chirp, NUM_CHANNELS) != 0) {
            fprintf(stderr, "Failed to play chirp preview\n");
            free(chirp_buffer);
            audio_terminate();
            return -1;
        }
    }

    // --- envoi du chirp et enregistrement de la réponse (simultané, duplex) ---
    float *record_buffer = (float*)malloc(sizeof(float) * n_samples_record * NUM_CHANNELS);
    if (!record_buffer) {
        fprintf(stderr, "Failed to allocate record buffer\n");
        free(chirp_buffer);
        audio_terminate();
        return -1;
    }

    // --- choix de script: calibration ou mesure ---
    int choice;
    printf("Select mode:\n1. Calibration\n2. Measurement\n3. Processing\nEnter choice: ");
    scanf("%d", &choice);

    if (choice == 1 || choice == 2) {
        if (choice == 1) {
            printf("Running calibration...\n");
        } else {
            printf("Running measurement...\n");
        }

        // --- calibration ---
        if (choice == 1) {
            printf("CALIBRATON MODE: Please ensure the lips of the tract are CLOSED and the microphone / speaker are properly positioned.\n");
            printf("When ready, press Enter to start calibration...");
        } else {
            printf("MEASUREMENT MODE: Please ensure the lips of the tract are OPEN and the microphone / speaker are properly positioned.\n");
            printf("When ready, press Enter to start measurement...");
        }

        getchar(); // Consume leftover newline
        getchar(); // Wait for user to press Enter

        // on envoie chirp_buffer et on enregistre dans record_buffer simultanément
        printf("Starting full-duplex audio (play chirp and record response)...\n");
        if (audio_duplex_callback(output_device, input_device, SAMPLE_RATE, chirp_buffer, record_buffer, n_samples_record, NUM_CHANNELS) != 0) {
            fprintf(stderr, "Failed to perform full-duplex audio\n");
            free(chirp_buffer);
            free(record_buffer);
            audio_terminate();
            return -1;
        }
        printf("Full-duplex audio completed successfully.\n");

        printf("Now estimating delay and aligning recorded response with chirp...\n");

        // --- calcul du retard entre enregistrement et envoi ---
        int delay_samples = -estimate_delay(record_buffer, chirp_buffer, n_samples_record);
        printf("Estimated delay: %d samples\n", delay_samples);

        // --- décalage du signal enregistré pour aligner avec chirp ---
        for (int i = 0; i < n_samples_record - delay_samples; i++) {
            record_buffer[i] = record_buffer[i + delay_samples];
        }

        printf("Shifted recorded response to align with chirp.\n");

        // --- on transfère les données vers des buffers avec moins de silence ---
        // nouveau buffer d'enregistrement de taille n_samples_chirp
        float *record_buffer_final = (float*)malloc(sizeof(float) * n_samples_chirp * NUM_CHANNELS);
        if (!record_buffer_final) {
            fprintf(stderr, "Failed to allocate final record buffer\n");
            free(chirp_buffer);
            free(record_buffer);
            audio_terminate();
            return -1;
        }
        memcpy(record_buffer_final, record_buffer, sizeof(float) * n_samples_chirp * NUM_CHANNELS);
        free(record_buffer); // Free original buffer with more silence

        // idem pour le chirp
        float *chirp_buffer_final = (float*)malloc(sizeof(float) * n_samples_chirp * NUM_CHANNELS);
        if (!chirp_buffer_final) {
            fprintf(stderr, "Failed to allocate final chirp buffer\n");
            free(chirp_buffer);
            free(record_buffer_final);
            audio_terminate();
            return -1;
        }
        memcpy(chirp_buffer_final, chirp_buffer, sizeof(float) * n_samples_chirp * NUM_CHANNELS);
        free(chirp_buffer); // Free original buffer with more silence
        
        char *filename = (choice == 1) ? "output/calibration_response.raw" : "output/measurement_response.raw";

        // --- sauvegarde de la réponse enregistrée pour traitement ultérieur ---
        FILE *record_file = fopen(filename, "wb");
        if (!record_file) {
            fprintf(stderr, "Failed to open file for writing calibration response\n");
            free(chirp_buffer_final);
            free(record_buffer_final);
            audio_terminate();
            return -1;
        }
        fwrite(record_buffer_final, sizeof(float), n_samples_chirp * NUM_CHANNELS, record_file);
        fclose(record_file);
        printf("%s response saved to '%s'\n", (choice == 1) ? "Calibration" : "Measurement", filename);

        // --- savegarde du chirp ---
        char *chirp_filename = (choice == 1) ? "output/calibration_chirp.raw" : "output/measurement_chirp.raw";
        FILE *chirp_file = fopen(chirp_filename, "wb");
        if (!chirp_file) {
            fprintf(stderr, "Failed to open file for writing calibration chirp\n");
            free(chirp_buffer_final);
            free(record_buffer_final);
            audio_terminate();
            return -1;
        }
        fwrite(chirp_buffer_final, sizeof(float), n_samples_chirp * NUM_CHANNELS, chirp_file);
        fclose(chirp_file);
        printf("%s chirp saved to '%s'\n", (choice == 1) ? "Calibration" : "Measurement", chirp_filename);

        // --- sauvegarde paramètres de calibration dans un fichier texte ---
        if (choice == 1) {
            FILE *param_file = fopen("output/calibration_parameters.txt", "w");
            if (!param_file) {
                fprintf(stderr, "Failed to open file for writing calibration parameters\n");
                free(chirp_buffer_final);
                free(record_buffer_final);
                audio_terminate();
                return -1;
            }

            fprintf(param_file, "Chirp Duration: %.2f seconds\n", chirp_duration);
            fprintf(param_file, "Chirp Start Frequency: %.2f Hz\n", chirp_start_freq);
            fprintf(param_file, "Chirp End Frequency: %.2f Hz\n", chirp_end_freq);
            fprintf(param_file, "Chirp Type: %s\n", (chirp_type == 'l') ? "Linear" : "Exponential");
            fprintf(param_file, "Chirp Amplitude: %.2f\n", chirp_amplitude);
            fclose(param_file);
            printf("Calibration parameters saved to 'output/calibration_parameters.txt'\n");
        }

        if (choice == 1) {
            printf("Calibration completed successfully. You can now run the measurement mode using the same parameters.\n");
        } else {
            printf("Measurement completed successfully. You can now run the processing mode to analyze the results.\n");
        }
        
        free(chirp_buffer_final);
        free(record_buffer_final);
        audio_terminate();
        
    } else if (choice == 3) {
        printf("Running processing...\n");
        // --- traitement ---
    } else {
        fprintf(stderr, "Invalid choice\n");
        audio_terminate();
        return -1;
    }

    return 0;
} 