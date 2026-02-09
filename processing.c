#include "processing.h"

void generate_linear_inverse_filter(kiss_fft_cpx *filter, float A, float f0, float f1, float T, float fs, int nfft) {
    double w0 = 2.0 * M_PI * f0;
    double w1 = 2.0 * M_PI * f1;
    double beta = (w1 - w0) / T; // Chirp rate (rad.s^-2)
    double coeff = (1.0 / A) * sqrt(beta / (2.0 * M_PI));

    for (int k = 0; k < nfft; k++) {
        double f = (double)k * fs / nfft;
        double w = 2.0 * M_PI * f;
        
        // Apply only within the excited frequency range
        if (f >= f0 && f <=f1) {
            double phase = -((w - w0) * (w - w0) / (2.0 * beta) + M_PI / 4.0);
            double _Complex val = coeff * cexp(I * phase);
            filter[k] = c99_to_kiss(val);
        } else {
            filter[k].r = 0.0f;
            filter[k].i = 0.0f;
        }
    }
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

void extract_linear_ir(kiss_fft_cpx *spectrum, kiss_fft_cfg cfg_inv, kiss_fft_cfg cfg_fft, int nfft, int ir_len, int fade_len) {
    // 1. Prepare temp time buffer
    kiss_fft_cpx *time_buf = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
    if (!time_buf) return; // Handle malloc failure

    // 2. Perform IFFT
    // Result is in time_buf
    kiss_fft(cfg_inv, spectrum, time_buf);

    // 3. Normalize IFFT result
    for (int k = 0; k < nfft; k++) {
        time_buf[k].r /= (kiss_fft_scalar)nfft;
        time_buf[k].i /= (kiss_fft_scalar)nfft;
    }

    // 4. Apply Tukey Window to isolate Linear IR
    apply_tukey_window(time_buf, nfft, ir_len, fade_len);

    // 5. Perform Forward FFT
    // Result is written back to 'spectrum', overwriting the noisy version
    kiss_fft(cfg_fft, time_buf, spectrum);

    free(time_buf);
}