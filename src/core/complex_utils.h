#include <complex.h>
#include "kiss_fft.h"

/**
 * Mapping: kiss_fft_cpx -> double complex
 * Converts the float-based KissFFT struct into a standard C99 complex type.
 */
static inline double complex kiss_to_c99(kiss_fft_cpx k) {
    return (double)k.r + (double)k.i * I;
}

/**
 * Mapping: double complex -> kiss_fft_cpx
 * Converts a standard complex back to the struct.
 * Note: kiss_fft_scalar is typically float, so this will cast down.
 */
static inline kiss_fft_cpx c99_to_kiss(double complex c) {
    kiss_fft_cpx k;
    k.r = (kiss_fft_scalar)creal(c);
    k.i = (kiss_fft_scalar)cimag(c);
    return k;
}

/**
 * Calculates the squared magnitude of a complex number.
 * Used in the denominator of the transfer function ratio (cf. eq. II.18).
 */
static inline float complex_squared_magnitude(kiss_fft_cpx z) {
    return (z.r * z.r) + (z.i * z.i);
}

/**
 * Calculates the complex conjugate.
 * Required for the numerator in the ratio method (cf. eq. II.18).
 */
static inline kiss_fft_cpx complex_conjugate(kiss_fft_cpx z) {
    kiss_fft_cpx res;
    res.r = z.r;
    res.i = -z.i;
    return res;
}

/**
 * Performs general complex division.
 * Uses the formula: (a.r + i*a.i) / (b.r + i*b.i)
 */
static inline kiss_fft_cpx complex_division(kiss_fft_cpx a, kiss_fft_cpx b) {
    float denom = complex_squared_magnitude(b);
    kiss_fft_cpx res;

    // Check for division by zero
    if (denom == 0.0f) {
        res.r = 0.0f; // or some error value
        res.i = 0.0f;
        return res;
    }

    res.r = (a.r * b.r + a.i * b.i) / denom;
    res.i = (a.i * b.r - a.r * b.i) / denom;
    return res;
}