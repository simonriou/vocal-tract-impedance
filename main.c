#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// External Library
#include "external/kiss_fft/kiss_fft.h"

// Project Headers
#include "processing.h"

// --- Simulation Parameters ---
#define SAMPLE_RATE 44100
#define DURATION    1.0     // Seconds
#define F_START     100.0   // Hz
#define F_END       5000.0  // Hz
#define AMP         0.95    // Signal Amplitude

// --- Signal Generator ---
// Generates a Linear Chirp with Harmonic Distortion
// Ref: MAISON PhD Eq II.7 and II.10
void generate_signals(float *y_closed, float *y_open, int n_samples) {
    double w0 = 2.0 * M_PI * F_START;
    double w1 = 2.0 * M_PI * F_END;
    double beta = (w1 - w0) / DURATION; // Chirp rate

    for (int t_idx = 0; t_idx < n_samples; t_idx++) {
        double t = (double)t_idx / SAMPLE_RATE;

        // 1. Calculate Instantaneous Phase for Linear Chirp (Eq II.7)
        double phase_main = w0 * t + (beta / 2.0) * t * t;

        // 2. Generate Distorted Excitation (Eq II.10)
        // Fundamental + Harmonics
        double clean = AMP * sin(phase_main);
        double dist2 = 0.4 * AMP * sin(2.0 * phase_main);
        double dist3 = 0.2 * AMP * sin(3.0 * phase_main);
        double dist4 = 0.1 * AMP * sin(4.0 * phase_main);
        
        double excitation = clean + dist2 + dist3 + dist4;

        // 3. Calibration Signal (Closed Mouth)
        y_closed[t_idx] = (float)excitation;

        // 4. Measurement Signal (Open Mouth)
        // VALIDATION CASE (G1 = 2):
        // The output is exactly 2 * the input (excitation) in dB ~ 6dB.
        y_open[t_idx] = 2.0 * (float)excitation; 
    }
}

// --- CSV Exporter ---
void save_results_csv(const char *filename, kiss_fft_cpx *h_lips, int nfft) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open output file");
        return;
    }

    fprintf(fp, "Frequency_Hz,Magnitude_dB,Phase_Rad\n");
    for (int i = 0; i < nfft / 2; i++) { // Only positive frequencies
        double f = (double)i * SAMPLE_RATE / nfft;
        double mag = sqrt(complex_squared_magnitude(h_lips[i]));
        
        // Clamp distinctively small values to avoid log(-inf)
        if (mag < 1e-9) mag = 1e-9;
        double db = 20.0 * log10(mag);
        
        double phase = atan2(h_lips[i].i, h_lips[i].r);
        
        fprintf(fp, "%.2f,%.4f,%.4f\n", f, db, phase);
    }
    fclose(fp);
    printf("Results saved to %s\n", filename);
}

// --- Main Pipeline ---
int main() {
    int n_samples = (int)(SAMPLE_RATE * DURATION);
    int nfft = 65536; 

    printf("Initializing Pipeline (Validation Mode G1=2):\n");
    printf("- Fs: %d Hz\n- Duration: %.1f s\n- FFT Size: %d\n", SAMPLE_RATE, DURATION, nfft);

    // 1. Allocations
    kiss_fft_cfg cfg_fwd = kiss_fft_alloc(nfft, 0, NULL, NULL);
    kiss_fft_cfg cfg_inv = kiss_fft_alloc(nfft, 1, NULL, NULL);

    kiss_fft_cpx *buf_closed = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    kiss_fft_cpx *buf_open   = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    kiss_fft_cpx *inv_filter = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    kiss_fft_cpx *h_result   = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    float *epsilon           = (float*)malloc(sizeof(float) * nfft);
    
    // Temporary simulation buffers
    float *sim_closed = (float*)calloc(n_samples, sizeof(float)); 
    float *sim_open   = (float*)calloc(n_samples, sizeof(float));

    if (!buf_closed || !buf_open || !sim_closed || !sim_open) return 1;

    // 2. Signal Generation
    generate_signals(sim_closed, sim_open, n_samples);

    // Convert to Complex FFT buffer
    for (int i = 0; i < nfft; i++) {
        buf_closed[i].r = (i < n_samples) ? sim_closed[i] : 0.0f;
        buf_closed[i].i = 0.0f;
        buf_open[i].r   = (i < n_samples) ? sim_open[i]   : 0.0f;
        buf_open[i].i   = 0.0f;
    }

    // 3. Pre-computation
    generate_linear_inverse_filter(inv_filter, AMP, F_START, F_END, DURATION, SAMPLE_RATE, nfft);
    generate_epsilon(epsilon, F_START, F_END, 10.0f, SAMPLE_RATE, nfft);

    // 4. Forward FFT
    kiss_fft(cfg_fwd, buf_closed, buf_closed);
    kiss_fft(cfg_fwd, buf_open, buf_open);

    // 5. Deconvolution
    perform_deconvolution(buf_closed, inv_filter, nfft);
    perform_deconvolution(buf_open, inv_filter, nfft);

    // 6. Extraction of Linear Impulse Response
    // The delay between linear IR and 2nd harmonic is ~20ms (100Hz / 4900Hz/s).
    // To see a flat response, we MUST cut before the harmonics arrive.
    // We set window to 15ms to be safe.
    int ir_len = (int)(0.015 * SAMPLE_RATE); // 15ms
    int fade_len = (int)(0.005 * SAMPLE_RATE); // 5ms
    
    extract_linear_ir(buf_closed, cfg_inv, cfg_fwd, nfft, ir_len, fade_len);
    extract_linear_ir(buf_open, cfg_inv, cfg_fwd, nfft, ir_len, fade_len);

    // 7. Calculate Ratio
    compute_h_lips(h_result, buf_open, buf_closed, epsilon, nfft);

    // 8. Output
    save_results_csv("vocal_tract_frf.csv", h_result, nfft);
    
    printf("Done. Check 'vocal_tract_frf.csv'.\n");

    // Cleanup
    free(buf_closed); free(buf_open); free(inv_filter); 
    free(h_result); free(epsilon);
    free(sim_closed); free(sim_open);
    kiss_fft_free(cfg_fwd); kiss_fft_free(cfg_inv);

    return 0;
}