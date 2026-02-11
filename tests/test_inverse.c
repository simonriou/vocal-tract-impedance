#include "processing.h"
#include "audio_io.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

void test_inverse_filter_quality(void) {
    int nfft = 131072;
    int fs = 44100;
    float f0 = 200.0f;
    float f1 = 1200.0f;
    float T = 1.5f;
    float A = 1.0f;

    // Allocate buffers
    kiss_fft_cpx *chirp_spectrum = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    kiss_fft_cpx *inv_filter = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    kiss_fft_cpx *time_result = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    float *chirp_time = (float*)malloc(sizeof(float) * nfft);

    // 1. Generate ideal Time-Domain Chirp
    memset(chirp_time, 0, sizeof(float) * nfft);
    generate_chirp(chirp_time, A, f0, f1, T, fs, 0, 0.0f, 0.0f); // 0 = linear, no gap, no fade

    // 2. Convert Chirp to Frequency Domain
    kiss_fft_cfg cfg_fwd = kiss_fft_alloc(nfft, 0, NULL, NULL);
    kiss_fft_cfg cfg_inv = kiss_fft_alloc(nfft, 1, NULL, NULL);

    for (int i = 0; i < nfft; i++) {
        chirp_spectrum[i].r = chirp_time[i];
        chirp_spectrum[i].i = 0.0f;
    }
    kiss_fft(cfg_fwd, chirp_spectrum, chirp_spectrum);

    // 3. Generate Inverse Filter
    generate_inverse_filter(inv_filter, A, f0, f1, T, fs, nfft, 0); // 0 = linear

    // 4. Perform Deconvolution (Multiply in Freq Domain)
    perform_deconvolution(chirp_spectrum, inv_filter, nfft);

    // 5. Inverse FFT to get Impulse Response
    kiss_fft(cfg_inv, chirp_spectrum, time_result);

    // 6. Analyze the Result (Find Peak)
    float max_val = 0.0f;
    int max_idx = -1;
    for (int i = 0; i < nfft; i++) {
        // Normalize
        time_result[i].r /= nfft; 
        
        float mag = fabs(time_result[i].r); // Real part check for Dirac
        if (mag > max_val) {
            max_val = mag;
            max_idx = i;
        }
    }

    printf("--- INVERSE FILTER TEST ---\n");
    printf("Expected Peak: Index 0\n");
    printf("Actual Peak:   Index %d\n", max_idx);
    printf("Peak Amplitude: %f (Should be approx 1.0)\n", max_val);
    
    // Check width of peak (should be sharp)
    float side_val = fabs(time_result[(max_idx + 1) % nfft].r);
    printf("Side Lobe Level: %f (Should be small)\n", side_val);

    free(chirp_spectrum); 
    free(inv_filter); 
    free(time_result); 
    free(chirp_time);
    kiss_fft_free(cfg_fwd); 
    kiss_fft_free(cfg_inv);
}

int main() {
    test_inverse_filter_quality();
    return 0;
}