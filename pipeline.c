#include "pipeline.h"
#include "audio_io.h"
#include "processing.h"
#include "user_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int calculate_next_power_of_two(int n) {
    int nfft = 1;
    while (nfft < n) {
        nfft *= 2;
    }
    return nfft;
}

int save_response_files(const float *response_buffer, const float *chirp_buffer, 
                       int n_samples, int is_calibration) {
    const char *response_filename = is_calibration ? "output/calibration_response.raw" : "output/measurement_response.raw";
    const char *chirp_filename = is_calibration ? "output/calibration_chirp.raw" : "output/measurement_chirp.raw";
    
    FILE *response_file = fopen(response_filename, "wb");
    if (!response_file) {
        fprintf(stderr, "Failed to open file for writing response\n");
        return -1;
    }
    fwrite(response_buffer, sizeof(float), n_samples * NUM_CHANNELS, response_file);
    fclose(response_file);
    printf("%s response saved to '%s'\n", is_calibration ? "Calibration" : "Measurement", response_filename);
    
    FILE *chirp_file = fopen(chirp_filename, "wb");
    if (!chirp_file) {
        fprintf(stderr, "Failed to open file for writing chirp\n");
        return -1;
    }
    fwrite(chirp_buffer, sizeof(float), n_samples * NUM_CHANNELS, chirp_file);
    fclose(chirp_file);
    printf("%s chirp saved to '%s'\n", is_calibration ? "Calibration" : "Measurement", chirp_filename);
    
    return 0;
}

int save_calibration_parameters(const ChirpParams *chirp_params) {
    FILE *param_file = fopen("output/calibration_parameters.txt", "w");
    if (!param_file) {
        fprintf(stderr, "Failed to open file for writing calibration parameters\n");
        return -1;
    }
    
    fprintf(param_file, "Chirp Duration: %.2f seconds\n", chirp_params->duration);
    fprintf(param_file, "Chirp Start Frequency: %.2f Hz\n", chirp_params->start_freq);
    fprintf(param_file, "Chirp End Frequency: %.2f Hz\n", chirp_params->end_freq);
    fprintf(param_file, "Chirp Type: %s\n", (chirp_params->type == 0) ? "Linear" : "Exponential");
    fprintf(param_file, "Chirp Amplitude: %.2f\n", chirp_params->amplitude);
    fclose(param_file);
    printf("Calibration parameters saved to 'output/calibration_parameters.txt'\n");
    
    return 0;
}

static int perform_duplex_and_align(const AudioConfig *audio_cfg, const float *chirp_buffer, 
                                   float *record_buffer, int n_samples_record) {
    printf("Starting full-duplex audio (play chirp and record response)...\n");
    if (audio_duplex_callback(audio_cfg->output_device, audio_cfg->input_device, SAMPLE_RATE, 
                             chirp_buffer, record_buffer, n_samples_record, NUM_CHANNELS) != 0) {
        fprintf(stderr, "Failed to perform full-duplex audio\n");
        return -1;
    }
    printf("Full-duplex audio completed successfully.\n");
    
    printf("Estimating delay and aligning recorded response with chirp...\n");
    int delay_samples = -estimate_delay(record_buffer, chirp_buffer, n_samples_record);
    printf("Estimated delay: %d samples\n", delay_samples);
    
    for (int i = 0; i < n_samples_record - delay_samples; i++) {
        record_buffer[i] = record_buffer[i + delay_samples];
    }
    printf("Shifted recorded response to align with chirp.\n");
    
    return 0;
}

int run_calibration_mode(const AudioConfig *audio_cfg, const ChirpParams *chirp_params, 
                        float recording_duration) {
    int n_samples_chirp = (int)(SAMPLE_RATE * chirp_params->duration);
    int n_samples_record = (int)(SAMPLE_RATE * recording_duration);
    
    /* Allocate buffers */
    float *chirp_buffer = (float*)malloc(sizeof(float) * n_samples_record);
    float *record_buffer = (float*)malloc(sizeof(float) * n_samples_record * NUM_CHANNELS);
    
    if (!chirp_buffer || !record_buffer) {
        fprintf(stderr, "Failed to allocate buffers\n");
        free(chirp_buffer);
        free(record_buffer);
        return -1;
    }
    
    /* Generate chirp */
    generate_chirp(chirp_buffer, chirp_params->amplitude, chirp_params->start_freq, 
                   chirp_params->end_freq, chirp_params->duration, SAMPLE_RATE, chirp_params->type);
    
    for (int i = n_samples_chirp; i < n_samples_record; i++) {
        chirp_buffer[i] = 0.0f;
    }
    
    /* Preview */
    if (confirm_and_preview(audio_cfg->output_device, chirp_buffer, n_samples_chirp) != 0) {
        free(chirp_buffer);
        free(record_buffer);
        return -1;
    }
    
    /* User confirmation */
    prompt_ready("CALIBRATION");
    
    /* Perform duplex and align */
    if (perform_duplex_and_align(audio_cfg, chirp_buffer, record_buffer, n_samples_record) != 0) {
        free(chirp_buffer);
        free(record_buffer);
        return -1;
    }
    
    /* Trim and save */
    float *record_buffer_final = (float*)malloc(sizeof(float) * n_samples_chirp * NUM_CHANNELS);
    float *chirp_buffer_final = (float*)malloc(sizeof(float) * n_samples_chirp * NUM_CHANNELS);
    
    if (!record_buffer_final || !chirp_buffer_final) {
        fprintf(stderr, "Failed to allocate final buffers\n");
        free(chirp_buffer);
        free(record_buffer);
        free(record_buffer_final);
        free(chirp_buffer_final);
        return -1;
    }
    
    memcpy(record_buffer_final, record_buffer, sizeof(float) * n_samples_chirp * NUM_CHANNELS);
    memcpy(chirp_buffer_final, chirp_buffer, sizeof(float) * n_samples_chirp * NUM_CHANNELS);
    
    if (save_response_files(record_buffer_final, chirp_buffer_final, n_samples_chirp, 1) != 0) {
        free(chirp_buffer);
        free(record_buffer);
        free(record_buffer_final);
        free(chirp_buffer_final);
        return -1;
    }
    
    if (save_calibration_parameters(chirp_params) != 0) {
        free(chirp_buffer);
        free(record_buffer);
        free(record_buffer_final);
        free(chirp_buffer_final);
        return -1;
    }
    
    printf("Calibration completed successfully.\n");
    
    free(chirp_buffer);
    free(record_buffer);
    free(record_buffer_final);
    free(chirp_buffer_final);
    
    return 0;
}

int run_measurement_mode(const AudioConfig *audio_cfg, const ChirpParams *chirp_params, 
                        float recording_duration) {
    int n_samples_chirp = (int)(SAMPLE_RATE * chirp_params->duration);
    int n_samples_record = (int)(SAMPLE_RATE * recording_duration);
    
    /* Allocate buffers */
    float *chirp_buffer = (float*)malloc(sizeof(float) * n_samples_record);
    float *record_buffer = (float*)malloc(sizeof(float) * n_samples_record * NUM_CHANNELS);
    
    if (!chirp_buffer || !record_buffer) {
        fprintf(stderr, "Failed to allocate buffers\n");
        free(chirp_buffer);
        free(record_buffer);
        return -1;
    }
    
    /* Generate chirp */
    generate_chirp(chirp_buffer, chirp_params->amplitude, chirp_params->start_freq, 
                   chirp_params->end_freq, chirp_params->duration, SAMPLE_RATE, chirp_params->type);
    
    for (int i = n_samples_chirp; i < n_samples_record; i++) {
        chirp_buffer[i] = 0.0f;
    }
    
    /* Preview */
    if (confirm_and_preview(audio_cfg->output_device, chirp_buffer, n_samples_chirp) != 0) {
        free(chirp_buffer);
        free(record_buffer);
        return -1;
    }
    
    /* User confirmation */
    prompt_ready("MEASUREMENT");
    
    /* Perform duplex and align */
    if (perform_duplex_and_align(audio_cfg, chirp_buffer, record_buffer, n_samples_record) != 0) {
        free(chirp_buffer);
        free(record_buffer);
        return -1;
    }
    
    /* Trim and save */
    float *record_buffer_final = (float*)malloc(sizeof(float) * n_samples_chirp * NUM_CHANNELS);
    float *chirp_buffer_final = (float*)malloc(sizeof(float) * n_samples_chirp * NUM_CHANNELS);
    
    if (!record_buffer_final || !chirp_buffer_final) {
        fprintf(stderr, "Failed to allocate final buffers\n");
        free(chirp_buffer);
        free(record_buffer);
        free(record_buffer_final);
        free(chirp_buffer_final);
        return -1;
    }
    
    memcpy(record_buffer_final, record_buffer, sizeof(float) * n_samples_chirp * NUM_CHANNELS);
    memcpy(chirp_buffer_final, chirp_buffer, sizeof(float) * n_samples_chirp * NUM_CHANNELS);
    
    if (save_response_files(record_buffer_final, chirp_buffer_final, n_samples_chirp, 0) != 0) {
        free(chirp_buffer);
        free(record_buffer);
        free(record_buffer_final);
        free(chirp_buffer_final);
        return -1;
    }
    
    printf("Measurement completed successfully.\n");
    
    free(chirp_buffer);
    free(record_buffer);
    free(record_buffer_final);
    free(chirp_buffer_final);
    
    return 0;
}

int run_processing_mode(const ChirpParams *chirp_params) {
    printf("PROCESSING MODE: Initializing processing pipeline...\n");
    
    int n_samples_chirp = (int)(SAMPLE_RATE * chirp_params->duration);
    int nfft = calculate_next_power_of_two(n_samples_chirp);
    printf("Using FFT size of %d for processing\n", nfft);
    
    /* Load calibration and measurement responses */
    float *calibration_response = (float*)malloc(sizeof(float) * n_samples_chirp * NUM_CHANNELS);
    float *measurement_response = (float*)malloc(sizeof(float) * n_samples_chirp * NUM_CHANNELS);
    
    if (!calibration_response || !measurement_response) {
        fprintf(stderr, "Failed to allocate buffers for processing\n");
        free(calibration_response);
        free(measurement_response);
        return -1;
    }
    
    FILE *calib_file = fopen("output/calibration_response.raw", "rb");
    if (!calib_file) {
        fprintf(stderr, "Failed to open calibration response file\n");
        free(calibration_response);
        free(measurement_response);
        return -1;
    }
    
    FILE *meas_file = fopen("output/measurement_response.raw", "rb");
    if (!meas_file) {
        fprintf(stderr, "Failed to open measurement response file\n");
        free(calibration_response);
        free(measurement_response);
        fclose(calib_file);
        return -1;
    }
    
    fread(calibration_response, sizeof(float), n_samples_chirp * NUM_CHANNELS, calib_file);
    fread(measurement_response, sizeof(float), n_samples_chirp * NUM_CHANNELS, meas_file);
    fclose(calib_file);
    fclose(meas_file);
    printf("Successfully loaded calibration and measurement responses.\n");
    
    /* Allocate FFT buffers */
    kiss_fft_cfg cfg_fwd = kiss_fft_alloc(nfft, 0, NULL, NULL);
    kiss_fft_cfg cfg_inv = kiss_fft_alloc(nfft, 1, NULL, NULL);
    
    kiss_fft_cpx *buf_closed = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    kiss_fft_cpx *buf_open = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    kiss_fft_cpx *inv_filter = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    kiss_fft_cpx *h_result = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    float *epsilon = (float*)malloc(sizeof(float) * nfft);
    
    if (!buf_closed || !buf_open || !inv_filter || !h_result || !epsilon) {
        fprintf(stderr, "Failed to allocate FFT buffers\n");
        free(buf_closed);
        free(buf_open);
        free(inv_filter);
        free(h_result);
        free(epsilon);
        free(calibration_response);
        free(measurement_response);
        return -1;
    }
    
    /* Convert to complex format */
    for (int i = 0; i < nfft; i++) {
        buf_closed[i].r = (i < n_samples_chirp) ? calibration_response[i] : 0.0f;
        buf_closed[i].i = 0.0f;
        buf_open[i].r = (i < n_samples_chirp) ? measurement_response[i] : 0.0f;
        buf_open[i].i = 0.0f;
    }
    
    free(calibration_response);
    free(measurement_response);
    
    /* Generate inverse filter and compute FFT */
    generate_inverse_filter(inv_filter, 1.0f, chirp_params->start_freq, chirp_params->end_freq, 
                           chirp_params->duration, SAMPLE_RATE, nfft, chirp_params->type);
    
    kiss_fft(cfg_fwd, buf_closed, buf_closed);
    kiss_fft(cfg_fwd, buf_open, buf_open);
    
    /* Perform deconvolution */
    perform_deconvolution(buf_closed, inv_filter, nfft);
    perform_deconvolution(buf_open, inv_filter, nfft);
    
    /* Estimate regularization parameter We */
    double We = 0.0;
    for (int i = 0; i < nfft / 2; i++) {
        double mag = sqrt(complex_squared_magnitude(buf_open[i]));
        We += mag * mag;
    }
    printf("Estimated We: %.6f\n", We);
    
    /* Generate regularization epsilon */
    generate_epsilon(epsilon, chirp_params->start_freq, chirp_params->end_freq, We, SAMPLE_RATE, nfft);
    
    /* Extract linear impulse response */
    extract_linear_ir(buf_closed, cfg_inv, cfg_fwd, nfft, n_samples_chirp, 8192, 16);
    extract_linear_ir(buf_open, cfg_inv, cfg_fwd, nfft, n_samples_chirp, 8192, 16);
    
    /* Compute final transfer function */
    compute_h_lips(h_result, buf_open, buf_closed, epsilon, nfft);
    
    /* Save results */
    FILE *fp = fopen("output/real_tract_frf.csv", "w");
    if (!fp) {
        fprintf(stderr, "Failed to open output CSV file\n");
        free(buf_closed);
        free(buf_open);
        free(inv_filter);
        free(h_result);
        free(epsilon);
        return -1;
    }
    
    fprintf(fp, "Frequency_Hz,Magnitude_dB,Resistance_dB,Reactance_dB,Phase_Rad\n");
    for (int i = 0; i < nfft / 2; i++) {
        double f = (double)i * SAMPLE_RATE / nfft;
        double mag = sqrt(complex_squared_magnitude(h_result[i]));
        
        if (mag < 1e-9) mag = 1e-9;
        double db = 20.0 * log10(mag);
        double phase = atan2(h_result[i].i, h_result[i].r);
        
        fprintf(fp, "%.2f,%.4f,%.4f,%.4f,%.4f\n", f, db, h_result[i].r, h_result[i].i, phase);
    }
    fclose(fp);
    printf("Results saved to 'output/real_tract_frf.csv'\n");
    
    /* Cleanup */
    free(buf_closed);
    free(buf_open);
    free(inv_filter);
    free(h_result);
    free(epsilon);
    kiss_fft_free(cfg_fwd);
    kiss_fft_free(cfg_inv);
    
    printf("Processing completed successfully.\n");
    return 0;
}
