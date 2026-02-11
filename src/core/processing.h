#ifndef PROCESSING_H
#define PROCESSING_H

#include <math.h>
#include <complex.h>
#include "complex_utils.h"

int calculate_next_power_of_two(int n);

/**
 * Generates the chirp (time domain).
 * Parameters:
 * - buffer: Output buffer for the chirp (must be pre-allocated with n_samples = (T + Tgap) * fs size)
 * - A: Amplitude
 * - f0, f1: Initial and final freqs (Hz)
 * - T: Chirp duration (s)
 * - fs: Sampling freq. (Hz)
 * - type: 0 for linear, 1 for exponential
 * - Tgap: Silence padding duration (s) - split equally before and after chirp (total signal = T + Tgap)
 * - Tfade: Fade-in/fade-out duration (s) - cosine taper applied at chirp boundaries using 0.5*(1-cos(pi*t/Tfade))
 */
void generate_chirp(float *buffer, float A, float f0, float f1, float T, float fs, int type, float Tgap, float Tfade);

/**
 * Finds the peak amplitude in a buffer.
 * Parameters:
 * - buffer: Input buffer
 * - n_samples: Number of samples in the buffer
 * Returns:
 * - Peak amplitude (max absolute value)
 */
float find_peak_amplitude(const float *buffer, int n_samples);

/**
 * Finds the delay in samples between two signals by cross-correlation.
 * Parameters:
 * - signal: Input signal (e.g., recorded response)
 * - reference: Reference signal (e.g., original chirp)
 * - n_samples: Number of samples in each signal
 * Returns:
 * - Estimated delay in samples (positive if signal is delayed relative to reference)
 */
int estimate_delay(const float *signal, const float *reference, int n_samples);

/**
 * Generates the Inverse Filter spectrum bin by bin.
 * Parameters:
 * - A: Amplitude
 * - f0, f1: Initial and final freqs (Hz)
 * - T: Chirp duration (s)
 * - fs: Sampling freq. (Hz)
 * - nfft: Number of FFT bins
 */

void generate_inverse_filter(kiss_fft_cpx *filter, float A, float f0, float f1, float T, float fs, int nfft, int type);

/**
 * Generates the Exponential Inverse Filter spectrum bin by bin using Python-exact logic.
 * Computes: 2*sqrt(1.j*freq/L) * exp(-2.j*pi*freq*L*(1-log(freq/f0)))
 * where L = floor(f0 * T / log(f1/f0)) / f0
 * Parameters:
 * - filter: Output filter array (frequency domain)
 * - A: Amplitude (currently unused for frequency-domain approach)
 * - f0, f1: Initial and final freqs (Hz)
 * - T: Chirp duration (s)
 * - fs: Sampling freq. (Hz)
 * - nfft: Number of FFT bins
 */
void generate_exponential_inverse_filter(kiss_fft_cpx *filter, float f0, float f1, float T, float fs, int nfft);

/**
 * Calculates the transition factor T(f) for regularization.
 * Transitions from 0 to 1 between fa and fb.
 * Cf. Eq. II.19
 */
double transition_function(double f, double fa, double fb);

/**
 * Generates the regularisation vector epsilon(omega).
 * Parameters:
 * - f0, f1: Chirp freq. range (Hz)
 * - We: Energy weight (usually sum of ||^2 of the signal to invert)
 * - fs: Sampling rate (Hz)
 * - nfft: FFT size
 */
void generate_epsilon(float *epsilon, float f0, float f1, float fs, int nfft);

/**
 * Performs frequency domain deconvolution.
 * Z_out(w) = Z_in(w) * X_inverse(w)
 */
void perform_deconvolution(kiss_fft_cpx *spectrum, const kiss_fft_cpx *inverse_filter, int nfft);

/**
 * Computes the final transfer function H_lips
 * Parameters:
 * - h_out: Output buffer for H_lips
 * - p_open: Deconvolved Open Mouth spectrum (G1 * P_open)
 * - p_closed: Deconvolved Closed Mouth spectrum (G1 * P_closed)
 * - epsilon: Regularisation vector
 */
void compute_h_lips(kiss_fft_cpx *h_out, const kiss_fft_cpx *p_open, const kiss_fft_cpx *p_closed, const float *epsilon, int nfft);

/**
 * Applies a one-sided Tukey window to the time domain signal.
 * Preserves the start of the signal (Linear IR) and fades out to zero.
 * Parameters:
 * - time_signal: Buffer containing the IR
 * - nfft: Total nb of samples
 * - ir_len: Length of the IR to keep (before fading out)
 * - fade_len: Length of the cosine taper (fade duration)
 */
void apply_tukey_window(kiss_fft_cpx *time_signal, int nfft, int ir_len, int fade_len);

/**
 * Generates a Tukey window for impulse response extraction.
 * Window has fade-in at start (nfade_pre), flat region at 1.0, and fade-out at end (nfade_post).
 * Parameters:
 * - window: Output buffer (must be pre-allocated with len_window samples)
 * - nfade_pre: Number of samples for fade-in at the start
 * - nfade_post: Number of samples for fade-out at the end
 * - len_window: Total length of the window buffer
 */
void generate_tukey_window(float *window, int nfade_pre, int nfade_post, int len_window);

/**
 * Coordinates the extraction of the linear part (F -> T -> Window -> F)
 * 1. IFFT of the raw deconvolved spectrum.
 * 2. Windowing in time domain -> no non-linearities.
 * 3. FFT to get the clean freq. Response Function.
 * Parameters:
 * - spectrum: I/O buffer
 * - cfg_inv: Config for IFFT (kissfft, inverse_fft = 1)
 * - cfg_fft: Config for FFT (kissfft, inverse_fft = 0)
 * - nfft: FFT size
 * - ir_len, fade_len: Windowing params
 */
void extract_linear_ir(kiss_fft_cpx *spectrum, kiss_fft_cfg cfg_inv, kiss_fft_cfg cfg_fft, int nfft, int n_samples_chirp, int nimp_pre, int nimp_post, double fs);

#endif