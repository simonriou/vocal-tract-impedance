#include "processing.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

void test_generate_tukey_window(void) {
    // Parameters matching the Python implementation
    int nimp_pre = 8192;   // Samples before the impulse
    int nimp_post = 200;   // Samples after the impulse
    int len_window = calculate_next_power_of_two(nimp_pre + nimp_post);
    
    int nfade_pre = nimp_pre / 2;   // Fade-in duration
    int nfade_post = nimp_post / 2; // Fade-out duration
    
    printf("Generating Tukey window with parameters:\n");
    printf("  nimp_pre: %d\n", nimp_pre);
    printf("  nimp_post: %d\n", nimp_post);
    printf("  len_window: %d (next power of 2)\n", len_window);
    printf("  nfade_pre: %d\n", nfade_pre);
    printf("  nfade_post: %d\n", nfade_post);
    
    // Allocate and initialize window array to zeros
    float *window = (float*)calloc(len_window, sizeof(float));
    if (!window) {
        fprintf(stderr, "Failed to allocate window buffer\n");
        return;
    }
    
    // Generate the Tukey window
    generate_tukey_window(window, nfade_pre, nfade_post, len_window);
    
    // Save window to file
    FILE *outfile = fopen("output/tukey_window_generated.raw", "wb");
    if (!outfile) {
        fprintf(stderr, "Failed to open output file\n");
        free(window);
        return;
    }
    
    size_t written = fwrite(window, sizeof(float), len_window, outfile);
    fclose(outfile);
    
    printf("\nWindow saved to output/tukey_window_generated.raw\n");
    printf("  Samples written: %zu\n", written);
    
    // Print sample values for verification
    printf("\nSample window values:\n");
    printf("  window[0] = %.6f (start, should be 0.0)\n", window[0]);
    printf("  window[%d] = %.6f (mid fade-in)\n", nfade_pre, window[nfade_pre]);
    printf("  window[%d] = %.6f (end fade-in, should be ~1.0)\n", 2*nfade_pre-1, window[2*nfade_pre-1]);
    printf("  window[%d] = %.6f (flat region, should be 1.0)\n", 2*nfade_pre, window[2*nfade_pre]);
    printf("  window[%d] = %.6f (mid flat region)\n", (2*nfade_pre + len_window - nfade_post)/2, 
           window[(2*nfade_pre + len_window - nfade_post)/2]);
    printf("  window[%d] = %.6f (start fade-out, should be 1.0)\n", len_window - nfade_post - 1, 
           window[len_window - nfade_post - 1]);
    printf("  window[%d] = %.6f (mid fade-out)\n", len_window - nfade_post/2, 
           window[len_window - nfade_post/2]);
    printf("  window[%d] = %.6f (end, should be 0.0)\n", len_window - 1, window[len_window - 1]);
    
    // Calculate statistics
    float min_val = window[0], max_val = window[0];
    for (int i = 0; i < len_window; i++) {
        if (window[i] < min_val) min_val = window[i];
        if (window[i] > max_val) max_val = window[i];
    }
    
    printf("\nStatistics:\n");
    printf("  Min value: %.6f\n", min_val);
    printf("  Max value: %.6f\n", max_val);
    
    free(window);
}

int main(void) {
    test_generate_tukey_window();
    return 0;
}
