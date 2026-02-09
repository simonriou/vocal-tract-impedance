#ifndef PROCESSING_H
#define PROCESSING_H

#include <math.h>
#include <complex.h>
#include "complex_utils.h"

/**
 * Generates the Linear Inverse Filter spectrum bin by bin.
 * Parameters:
 * - A: Amplitude
 * - f0, f1: Initial and final freqs (Hz)
 * - T: Chirp duration (s)
 * - fs: Sampling freq. (Hz)
 * - nfft: Number of FFT bins
 */

void generate_linear_inverse_filter(kiss_fft_cpx *filter, float A, float f0, float f1, float T, float fs, int nfft);

/**
 * Generates the Exponential Inverse Filter spectrum bin by bin.
 * Parameters:
 * - A: Amplitude
 * - f0, f1: Initial and effective final freqs. (Hz)
 * - L: Chirp log-rate increase
 */
void generate_exponential_inverse_filter(kiss_fft_cpx *filter, float A, float f0, float f1, float L, float fs, int nfft);

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
void generate_epsilon(float *epsilon, float f0, float f1, float We, float fs, int nfft);

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
void extract_linear_ir(kiss_fft_cpx *spectrum, kiss_fft_cfg cfg_inv, kiss_fft_cfg cfg_fft, int nfft, int ir_len, int fade_len);

#endif