#include "processing.h"
#include <string.h>
#include <stdio.h>

// --- Constants ---
#define EPSILON_DIVISION_BY_ZERO 1e-15
#define EPSILON_MAGNITUDE_THRESHOLD 1e-12
#define EPSILON_TRANSITION_HZ 50.0
#define DEFAULT_IR_LENGTH 8192
#define DEFAULT_FADE_LENGTH 16

int calculate_next_power_of_two(int n) {
    int nfft = 1;
    while (nfft < n) {
        nfft *= 2;
    }
    return nfft;
}

void generate_chirp(float *buffer, float A, float f0, float f1, float T, float fs, int type, float Tgap, float Tfade) {
    int n_samples_total = (int)((T + Tgap) * fs);
    int n_samples_gap_half = (int)((Tgap / 2) * fs);
    int n_samples_chirp = (int)(T * fs);
    int n_samples_fade = (int)(Tfade * fs);
    
    // Initialize buffer with zeros
    memset(buffer, 0, n_samples_total * sizeof(float));
    
    // Generate chirp in the middle (after gap/2) with fade envelope
    int chirp_start = n_samples_gap_half;
    
    if (type == 0) { // Linear chirp
        for (int t_idx = 0; t_idx < n_samples_chirp; t_idx++) {
            double t = (double)t_idx / fs;
            float chirp_sample = A * (float) sin(M_PI * (2 * f0 * t + (f1 - f0) * t * t / T));
            
            // Apply fade envelope
            float envelope = 1.0f;
            
            // Fade-in at start of chirp
            if (t_idx < n_samples_fade) {
                double t_fade = (double)t_idx / fs;
                envelope *= 0.5f * (1.0f - cosf((float)M_PI * t_fade / Tfade));
            }
            
            // Fade-out at end of chirp
            int samples_from_end = n_samples_chirp - t_idx - 1;
            if (samples_from_end < n_samples_fade) {
                double t_fade = (double)samples_from_end / fs;
                envelope *= 0.5f * (1.0f - cosf((float)M_PI * t_fade / Tfade));
            }
            
            buffer[chirp_start + t_idx] = chirp_sample * envelope;
        }
    } else if (type == 1) { // Exponential chirp
        double L = (1 / f0) * ceil(f0 * T / log(f1 / f0));
        
        for (int t_idx = 0; t_idx < n_samples_chirp; t_idx++) {
            double t = (double)t_idx / fs;
            float chirp_sample = A * (float) sin(2 * M_PI * f0 * L * exp(t / L));
            
            // Apply fade envelope
            float envelope = 1.0f;
            
            // Fade-in at start of chirp
            if (t_idx < n_samples_fade) {
                double t_fade = (double)t_idx / fs;
                envelope *= 0.5f * (1.0f - cosf((float)M_PI * t_fade / Tfade));
            }
            
            // Fade-out at end of chirp
            int samples_from_end = n_samples_chirp - t_idx - 1;
            if (samples_from_end < n_samples_fade) {
                double t_fade = (double)samples_from_end / fs;
                envelope *= 0.5f * (1.0f - cosf((float)M_PI * t_fade / Tfade));
            }
            
            buffer[chirp_start + t_idx] = chirp_sample * envelope;
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

static kiss_fft_cpx cpx_inv(kiss_fft_cpx z) {
    kiss_fft_cpx res;
    double denom = z.r * z.r + z.i * z.i;
    
    if (denom < EPSILON_DIVISION_BY_ZERO) {
        res.r = 0;
        res.i = 0;
    } else {
        res.r = z.r / denom;
        res.i = -z.i / denom;
    }
    return res;
}

void generate_inverse_filter(kiss_fft_cpx *filter, float A, float f0, float f1, float T, float fs, int nfft, int type) {
    // Allocate temporary buffers
    float *temp_chirp = (float*)calloc(nfft, sizeof(float)); // Zero init
    kiss_fft_cpx *temp_fft = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    kiss_fft_cfg cfg = kiss_fft_alloc(nfft, 0, NULL, NULL);

    if (!temp_chirp || !temp_fft || !cfg) {
        fprintf(stderr, "Allocation failed in generate_inverse_filter\n");
        free(temp_chirp);
        free(temp_fft);
        free(cfg);
        return;
    }

    // Generate the reference chirp (time domain)
    // Use the same function as playback to ensure perfect match
    generate_chirp(temp_chirp, A, f0, f1, T, fs, type, 0.0f, 0.0f);

    // Convert to frequency domain
    for (int i = 0; i < nfft; i++) {
        temp_fft[i].r = temp_chirp[i];
        temp_fft[i].i = 0.0f;
    }
    kiss_fft(cfg, temp_fft, temp_fft);

    // Compute inverse filter: 1 / Chirp_Spectrum
    // Only invert inside the active bandwidth to avoid amplifying noise
    for (int k = 0; k <= nfft / 2; k++) {
        double f = (double)k * fs / nfft;
        
        if (f >= f0 && f <= f1) {
            filter[k] = cpx_inv(temp_fft[k]);
        } else {
            filter[k].r = 0.0f;
            filter[k].i = 0.0f;
        }
    }

    // Enforce Hermitian symmetry for real IFFT result
    for (int k = nfft / 2 + 1; k < nfft; k++) {
        int sym_k = nfft - k;
        filter[k].r = filter[sym_k].r;
        filter[k].i = -filter[sym_k].i;
    }

    free(temp_chirp);
    free(temp_fft);
    free(cfg);
}

void generate_exponential_inverse_filter(kiss_fft_cpx *filter, float f0, float f1, float T, float fs, int nfft) {
    // Calculate L parameter from Python: L = floor(f0 * T / log(f1/f0)) / f0
    double L = floor(f0 * T / log(f1 / f0)) / f0;
    
    // For each frequency bin
    for (int k = 0; k <= nfft / 2; k++) {
        double freq = (double)k * fs / nfft;
        
        if (freq < EPSILON_DIVISION_BY_ZERO) {
            // DC component
            filter[k].r = 0.0f;
            filter[k].i = 0.0f;
            continue;
        }
        
        if (freq >= f0 && freq <= f1) {
            // Inside the active bandwidth
            // Compute: 2*sqrt(1.j*freq/L) * exp(-2.j*pi*freq*L*(1-log(freq/f0)))
            
            double freq_div_L = freq / L;
            
            // sqrt(i * freq/L):
            // i*x has magnitude x and angle pi/2
            // sqrt(i*x) = sqrt(x) * exp(i*pi/4) = sqrt(x) * (1/sqrt(2) + i/sqrt(2))
            //           = sqrt(x/2) * (1 + i)
            double sqrt_mag = sqrt(freq_div_L / 2.0);  // magnitude = sqrt(freq/(2*L))
            kiss_fft_cpx sqrt_term;
            sqrt_term.r = sqrt_mag;    // real and imaginary parts are equal
            sqrt_term.i = sqrt_mag;
            
            // Compute exp(-2*pi*freq*L*(1 - log(freq/f0)))
            double log_ratio = log(freq / f0);
            double exp_arg = -2.0 * M_PI * freq * L * (1.0 - log_ratio);
            double exp_real = cos(exp_arg);
            double exp_imag = sin(exp_arg);
            
            // Multiply: 2 * sqrt_term * exp_term
            // Complex multiplication: (a+bi)(c+di) = (ac-bd) + (ad+bc)i
            double prod_real = sqrt_term.r * exp_real - sqrt_term.i * exp_imag;
            double prod_imag = sqrt_term.r * exp_imag + sqrt_term.i * exp_real;
            
            filter[k].r = 2.0f * prod_real;
            filter[k].i = 2.0f * prod_imag;
        } else {
            // Outside the active bandwidth
            filter[k].r = 0.0f;
            filter[k].i = 0.0f;
        }
    }
    
    // Enforce Hermitian symmetry for real IFFT result
    for (int k = nfft / 2 + 1; k < nfft; k++) {
        int sym_k = nfft - k;
        filter[k].r = filter[sym_k].r;
        filter[k].i = -filter[sym_k].i;
    }
}

double transition_function(double f, double fa, double fb) {
    if (f <= fmin(fa, fb) || f >= fmax(fa, fb)) {
        if (fa < fb) {
            if (f <= fa) return 0.0;
            if (f >= fb) return 1.0;
        } else {
            if (f >= fa) return 0.0;
            if (f <= fb) return 1.0;
        }
    }

    double term1 = 1.0 / (fa - f);
    double term2 = 1.0 / (f - fb);
    return 0.5 * (1.0 + tanh(term1 + term2));
}

void generate_epsilon(float *epsilon, float f0, float f1, float fs, int nfft) {
    double fa0 = f0;
    double fb0 = f0 - EPSILON_TRANSITION_HZ;
    double fa1 = f1;
    double fb1 = f1 + EPSILON_TRANSITION_HZ;

    for (int k = 0; k < nfft; k++) {
        double f = (double)k * fs / nfft;
        double weight = 0.0;

        if (f < f0) {
            weight = transition_function(f, fa0, fb0);
        } else if (f > f1) {
            weight = transition_function(f, fa1, fb1);
        }

        // epsilon[k] = (float)(weight * We);
        epsilon[k] = (float)(weight);
    }
}

void perform_deconvolution(kiss_fft_cpx *spectrum, const kiss_fft_cpx *inverse_filter, int nfft) {
    for (int k = 0; k < nfft; k++) {
        double complex z = kiss_to_c99(spectrum[k]);
        double complex x_inv = kiss_to_c99(inverse_filter[k]);
        spectrum[k] = c99_to_kiss(z * x_inv);
    }
}

void compute_h_lips(kiss_fft_cpx *h_out, const kiss_fft_cpx *p_open, const kiss_fft_cpx *p_closed, const float *epsilon, int nfft) {
    for (int k = 0; k < nfft; k++) {
        double complex top = kiss_to_c99(p_open[k]);
        double complex bot = kiss_to_c99(p_closed[k]);
        double complex numerator = top * conj(bot);
        
        double mag_sq = bot * conj(bot);
        double denominator = mag_sq + epsilon[k];
        
        if (denominator < EPSILON_MAGNITUDE_THRESHOLD) {
            denominator = EPSILON_MAGNITUDE_THRESHOLD;
        }
        
        h_out[k] = c99_to_kiss(numerator / denominator);
    }
}

void generate_tukey_window(float *window, int nfade_pre, int nfade_post, int len_window) {
    // between nfade_pre and 2*nfade_pre, 0.5 * (1 - np.cos(np.linspace(0, np.pi, nfade_pre)))
    for (int i = 0; i < nfade_pre; i++) {
        window[i] = 0.5f * (1.0f - cosf((float)M_PI * i / (float)nfade_pre));
    }

    // after impulse (after end-nfade_post), 0.5 * (1 + np.cos(np.linspace(0, np.pi, nfade_post)))
    for (int i = 0; i < nfade_post; i++) {
        window[len_window - nfade_post + i] = 0.5f * (1.0f + cosf((float)M_PI * i / (float)nfade_post));
    }
    // in the middle, constant 1.0 (between 2*nfade_pre and nimp_pre + nfade_post)
    for (int i = 2*nfade_pre; i < len_window - nfade_post; i++) {
        window[i] = 1.0f;
    }
}

void extract_linear_ir(kiss_fft_cpx *spectrum, kiss_fft_cfg cfg_inv, kiss_fft_cfg cfg_fft, int nfft, int n_samples_chirp, int nimp_pre, int nimp_post, double fs) {
    kiss_fft_cpx *time_buf = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    if (!time_buf) return;

    kiss_fft(cfg_inv, spectrum, time_buf);

    kiss_fft_cpx *circ_buf = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * calculate_next_power_of_two(nimp_pre + nimp_post));
    if (!circ_buf) return;

    // Save intermediate results for debugging
    FILE *time_calib_file = fopen("output/time_domain_calibration_response.raw", "wb");
    if (time_calib_file) {
        fwrite(time_buf, sizeof(float), n_samples_chirp, time_calib_file);
        fclose(time_calib_file);
        printf("Time-domain calibration response saved for debugging.\n");
    } else {
        fprintf(stderr, "Failed to save time-domain calibration response for debugging\n");
    }

    // Put into circ_buf the nimp_pre last samples of time_buf followed by the nimp_post first samples
    for (int i = 0; i < nimp_pre; i++) {
        circ_buf[i] = time_buf[n_samples_chirp - nimp_pre + i];
    }
    for (int i = 0; i < nimp_post; i++) {
        circ_buf[nimp_pre + i] = time_buf[i];
    }

    // Design the window with fade-in/out at boundaries
    int nfade_pre = (int)nimp_pre / 2;
    int nfade_post = (int)nimp_post / 2;

    float *window = (float*)calloc(calculate_next_power_of_two(nimp_pre + nimp_post), sizeof(float));

    generate_tukey_window(window, nfade_pre, nfade_post, calculate_next_power_of_two(nimp_pre + nimp_post));

    // apply window to circ_buf
    for (int i = 0; i < nimp_pre + nimp_post; i++) {
        circ_buf[i].r *= window[i];
        circ_buf[i].i *= window[i];
    }

    // Save windowed result for debugging
    FILE *windowed_calib_file = fopen("output/windowed_calibration_response.raw", "wb");
    if (windowed_calib_file) {
        fwrite(circ_buf, sizeof(kiss_fft_cpx), calculate_next_power_of_two(nimp_pre + nimp_post), windowed_calib_file);
        fclose(windowed_calib_file);
        printf("Windowed calibration response saved for debugging.\n");
    } else {
        fprintf(stderr, "Failed to save windowed calibration response for debugging\n");
    }

    kiss_fft(cfg_fft, circ_buf, spectrum);

    // Apply phase correction * exp(2j pi f nimp_pre / fs)
    for (int k = 0; k < nfft; k++) {
        double f = (double)k * fs / nfft;
        double phase_correction = 2.0 * M_PI * f * nimp_pre / fs;
        double cos_phase = cos(phase_correction);
        double sin_phase = sin(phase_correction);
        double real_part = spectrum[k].r * cos_phase - spectrum[k].i * sin_phase;
        double imag_part = spectrum[k].r * sin_phase + spectrum[k].i * cos_phase;
        spectrum[k].r = real_part;
        spectrum[k].i = imag_part;
    }

    free(window);
    free(time_buf);
    free(circ_buf);
}