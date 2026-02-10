#include "processing.h"

void generate_chirp(float *buffer, float A, float f0, float f1, float T, float fs, int type) {
    int n_samples = (int)(T * fs);
    
    if (type == 0) { // Linear chirp
        for (int t_idx = 0; t_idx < n_samples; t_idx++) {
            double t = (double)t_idx / fs;
            buffer[t_idx] = A * (float) sin(M_PI * (2 * f0 * t + (f1 - f0) * t * t / T));
        }
    } else if (type == 1) { // Exponential chirp
        double L = (1 / f0) * ceil(f0 * T / log(f1 / f0));
        
        for (int t_idx = 0; t_idx < n_samples; t_idx++) {
            double t = (double)t_idx / fs;
            buffer[t_idx] = A * (float) sin(2 * M_PI * f0 * L * exp(t / L));
        }
    } else {
        fprintf(stderr, "Invalid chirp type: %d\n", type);
    }

}

float find_peak_amplitude(const float *buffer, int n_samples) {
    float max_amp = 0.0f;
    for (int i = 0; i < n_samples; i++) {
        float abs_val = fabsf(buffer[i]);
        if (abs_val > max_amp) {
            max_amp = abs_val;
        }
    }
    return max_amp;
}

int estimate_delay(const float *signal, const float *reference, int n_samples) {
    // Simple cross-correlation based delay estimation
    int max_lag = n_samples / 2; // Max lag to search (half the signal length)
    int best_lag = 0;
    float max_corr = -1e30f; // Very small number

    for (int lag = -max_lag; lag <= max_lag; lag++) {
        float corr = 0.0f;
        for (int i = 0; i < n_samples; i++) {
            int ref_idx = i + lag;
            if (ref_idx >= 0 && ref_idx < n_samples) {
                corr += signal[i] * reference[ref_idx];
            }
        }
        if (corr > max_corr) {
            max_corr = corr;
            best_lag = lag;
        }
    }

    return best_lag;
}

kiss_fft_cpx cpx_inv(kiss_fft_cpx z) {
    kiss_fft_cpx res;
    double denom = z.r * z.r + z.i * z.i;
    // Avoid division by zero
    if (denom < 1e-15) {
        res.r = 0;
        res.i = 0;
    } else {
        res.r = z.r / denom;
        res.i = -z.i / denom;
    }
    return res;
}

void generate_linear_inverse_filter(kiss_fft_cpx *filter, float A, float f0, float f1, float T, float fs, int nfft) {
    // double w0 = 2.0 * M_PI * f0;
    // double w1 = 2.0 * M_PI * f1;
    // double beta = (w1 - w0) / T; // Chirp rate
    // double coeff = (1.0 / A) * sqrt(beta / (2.0 * M_PI));

    // // 1. Compute for Positive Frequencies (0 to Nyquist)
    // for (int k = 0; k <= nfft / 2; k++) {
    //     double f = (double)k * fs / nfft;
    //     double w = 2.0 * M_PI * f;

    //     // Apply only within the excited frequency range
    //     if (f >= f0 && f <= f1) {
    //         // Eq (II.9): Minus sign is outside the bracket in exp{-j[...]}
    //         double phase_arg = (w - w0) * (w - w0) / (2.0 * beta) + M_PI / 4.0;
    //         double complex val = coeff * cexp(-I * phase_arg); 
            
    //         filter[k] = c99_to_kiss(val);
    //     } else {
    //         filter[k].r = 0.0f;
    //         filter[k].i = 0.0f;
    //     }
    // }

    // // 2. Enforce Hermitian Symmetry for Negative Frequencies
    // // This ensures the IFFT results in a REAL time-domain signal.
    // for (int k = nfft / 2 + 1; k < nfft; k++) {
    //     int sym_k = nfft - k; // The corresponding positive frequency index
        
    //     // Conjugate: Re part is same, Im part is negated
    //     filter[k].r = filter[sym_k].r;
    //     filter[k].i = -filter[sym_k].i;
    // }

    // 1. Allocate temporary buffers
    float *temp_chirp = (float*)calloc(nfft, sizeof(float)); // Zero init
    kiss_fft_cpx *temp_fft = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    kiss_fft_cfg cfg = kiss_fft_alloc(nfft, 0, NULL, NULL);

    if (!temp_chirp || !temp_fft || !cfg) {
        fprintf(stderr, "Allocation failed in generate_numerical_inverse_filter\n");
        // Clean up and return (in production code, handle error propagation)
        free(temp_chirp); free(temp_fft); free(cfg);
        return;
    }

    // 2. Generate the Reference Chirp (Time Domain)
    // We use the EXACT same function as the playback to ensure perfect match
    // Note: ensure generate_chirp writes up to T*fs, and the rest of nfft is 0 (handled by calloc)
    generate_chirp(temp_chirp, A, f0, f1, T, fs, 0); // 0 = Linear

    // 3. Convert to Frequency Domain
    for (int i = 0; i < nfft; i++) {
        temp_fft[i].r = temp_chirp[i];
        temp_fft[i].i = 0.0f;
    }
    kiss_fft(cfg, temp_fft, temp_fft);

    // 4. Compute Inverse Filter: 1 / Chirp_Spectrum
    // We only invert inside the active bandwidth to avoid blowing up noise
    for (int k = 0; k <= nfft / 2; k++) {
        double f = (double)k * fs / nfft;
        
        // Add a small margin or strictly respect f0/f1
        if (f >= f0 && f <= f1) {
            filter[k] = cpx_inv(temp_fft[k]);
        } else {
            // Outside bandwidth: Zero the filter (Bandpass)
            filter[k].r = 0.0f;
            filter[k].i = 0.0f;
        }
    }

    // 5. Enforce Hermitian Symmetry (for real IFFT result)
    for (int k = nfft / 2 + 1; k < nfft; k++) {
        int sym_k = nfft - k;
        filter[k].r = filter[sym_k].r;
        filter[k].i = -filter[sym_k].i; // Conjugate
    }

    // Cleanup
    free(temp_chirp);
    free(temp_fft);
    free(cfg);
}

void generate_exponential_inverse_filter(kiss_fft_cpx *filter, float A, float f0, float f1, float L, float fs, int nfft) {
    double w0 = 2.0 * M_PI * f0;

    for (int k = 0; k < nfft; k++) {
        double f = (double)k * fs / nfft;
        double w = 2.0 * M_PI * f;

        // Filter is defined for positive freqs within range
        if (f >= f0 && f <= f1) {
            double mag = (2.0 / A) * sqrt(w / (2.0 * M_PI * L));
            double phase = -w * L * (1.0 - log(w / w0)) + M_PI / 4.0;

            double _Complex val = mag * cexp(I * phase);
            filter[k] = c99_to_kiss(val);
        } else {
            filter[k].r = 0.0f;
            filter[k].i = 0.0f;
        }
    }
}

double transition_function(double f, double fa, double fb) {
    // Check bounds to avoid division by zero / domain errors
    if (f <= fmin(fa, fb) || f >= fmax(fa, fb)) {
        // If f is exactly on the boundary or outside the transition interval:
        // The tanh argument would explode
        // So we define the behaviour based on the limit.
        if (fa < fb) {
            // Std case: fa (start) -> fb (end)
            if (f <= fa) return 0.0;
            if (f >= fb) return 1.0;
        } else {
            // Reverse case (if inputs are swapped): fa (end) <- fb (start)
            if (f >= fa) return 0.0;
            if (f <= fb) return 1.0;
        }
    }

    double term1 = 1.0 / (fa - f);
    double term2 = 1.0 / (f - fb);
    return 0.5 * (1.0 + tanh(term1 + term2));
}

void generate_epsilon(float *epsilon, float f0, float f1, float We, float fs, int nfft) {
    // Transition bandwidth defined as 50hz (text)
    double delta_f = 50.0;

    // Define transition boundaries
    // Start: fa0 = f0, fb0 = f0 - 50
    double fa0 = f0;
    double fb0 = f0 - delta_f;

    // End: fa1 = f1, fb1 = f1 + 50
    double fa1 = f1;
    double fb1 = f1 + delta_f;

    for (int k = 0; k < nfft; k++) {
        double f = (double)k * fs / nfft;

        // T(fa) = 0, T(fb) = 1
        // We want epsilon to be 0 INSIDE [f0, f1] and We OUTSIDE.

        double weight = 0.0;

        if (f < f0) {
            // Below f0: transition from We to 0
            // We use transition from fb0 (We) to fa0 (0)
            weight = transition_function(f, fa0, fb0);
        } else if (f > f1) {
            // Above f1: transition from 0 to We
            // We use transition from fa1 (0) to fb1 (We)
            weight = transition_function(f, fa1, fb1);
        } else {
            // Inside bandwidth
            weight = 0.0;
        }

        epsilon[k] = (float)(weight * We);
    }
}

void perform_deconvolution(kiss_fft_cpx *spectrum, const kiss_fft_cpx *inverse_filter, int nfft) {
    for (int k = 0; k < nfft; k++) {
        // Convert C99 -> cx for easy math
        double complex z = kiss_to_c99(spectrum[k]);
        double complex x_inv = kiss_to_c99(inverse_filter[k]);

        double complex result = z * x_inv;

        // Store back in C99
        spectrum[k] = c99_to_kiss(result);
    }
}

void compute_h_lips(kiss_fft_cpx *h_out, const kiss_fft_cpx *p_open, const kiss_fft_cpx *p_closed, const float *epsilon, int nfft) {
    for (int k = 0; k < nfft; k++) {
        double complex top = kiss_to_c99(p_open[k]);
        double complex bot = kiss_to_c99(p_closed[k]);

        // Num: P_open * conj(P_closed)
        double complex numerator = top * conj(bot);

        // Denom: |P_closed|^2 + epsilon
        // cabs(bot) returns mag, so we square it
        double mag_sq = pow(cabs(bot), 2.0);
        double denominator = mag_sq + epsilon[k];

        // Final division, avoiding by zero is epsilon is 0 and signal is 0
        // This should not happen as in practice there will be noise floor
        if (denominator < 1e-12) denominator = 1e-12;

        double complex h = numerator / denominator;

        h_out[k] = c99_to_kiss(h);
    }
}

void apply_tukey_window(kiss_fft_cpx *time_signal, int nfft, int ir_len, int fade_len) {
    for (int k = 0; k < nfft; k++) {
        double window_val = 0.0;

        if (k < ir_len) {
            // Flat top part: Keep signal as is
            window_val = 1.0;
        } else if (k < ir_len + fade_len) {
            // Fade out part: cosine taper
            // n goes from 0 to fade_len
            double n = (double)(k - ir_len);
            window_val = 0.5 * (1.0 + cos(M_PI * n / (double)fade_len));
        } else {
            // Zero out the rest (where non linear harms and noise are)
            window_val = 0.0;
        }

        // Aplly window to time signal
        time_signal[k].r *= (kiss_fft_scalar)window_val;
        time_signal[k].i *= (kiss_fft_scalar)window_val;
    }
}

void extract_linear_ir(kiss_fft_cpx *spectrum, kiss_fft_cfg cfg_inv, kiss_fft_cfg cfg_fft, int nfft, int n_samples_chirp, int ir_len, int fade_len) {
    kiss_fft_cpx *time_buf = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    if (!time_buf) return;

    kiss_fft(cfg_inv, spectrum, time_buf);

    // 3. Normalize IFFT result
    float max_mag = 0.0f;
    int peak_index = 0;

    for (int k = 0; k < nfft; k++) {
        time_buf[k].r /= (kiss_fft_scalar)nfft;
        time_buf[k].i /= (kiss_fft_scalar)nfft;

        // debug
        float mag = time_buf[k].r * time_buf[k].r + time_buf[k].i * time_buf[k].i;
        if (mag > max_mag) {
            max_mag = mag;
            peak_index = k;
        }
    }

    printf("[DEBUG] IR Peak Index: %d (Magnitude: %.2e)\n", peak_index, max_mag);

    // --- sauvegarde intermédiaire pour débug ---
    FILE *time_calib_file = fopen("output/time_domain_calibration_response.raw", "wb");
    if (time_calib_file) {
        fwrite(time_buf, sizeof(float), n_samples_chirp, time_calib_file);
        fclose(time_calib_file);
        printf("Time-domain calibration response saved for debugging.\n");
    } else {
        fprintf(stderr, "Failed to save time-domain calibration response for debugging\n");
    }

    apply_tukey_window(time_buf, nfft, ir_len, fade_len);

    // --- sauvegarde intermédiaire pour débug ---
    FILE *windowed_calib_file = fopen("output/windowed_calibration_response.raw", "wb");
    if (windowed_calib_file) {
        fwrite(time_buf, sizeof(float), n_samples_chirp, windowed_calib_file);
        fclose(windowed_calib_file);
        printf("Windowed calibration response saved for debugging.\n");
    } else {
        fprintf(stderr, "Failed to save windowed calibration response for debugging\n");
    }

    kiss_fft(cfg_fft, time_buf, spectrum); // On remet en domaine fréquentiel après fenêtrage
    free(time_buf);
}