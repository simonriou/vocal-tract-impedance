#!/usr/bin/env python3
"""
Plot the Tukey window values saved by test_window.c
"""

import numpy as np
import matplotlib.pyplot as plt
import sys

def plot_window(input_file, output_file=None):
    """
    Read and plot the window values from a raw binary file.
    
    Parameters:
    - input_file: Path to the raw binary file containing window values (float32)
    - output_file: Optional path to save the plot image
    """
    
    # Read the raw binary file
    try:
        window_values = np.fromfile(input_file, dtype=np.float32)
    except Exception as e:
        print(f"Error reading file {input_file}: {e}", file=sys.stderr)
        return False
    
    if len(window_values) == 0:
        print(f"No data read from {input_file}", file=sys.stderr)
        return False
    
    # Parameters (must match test_window.c)
    ir_len = 8192
    fade_len = 16
    nfft = len(window_values)
    
    # Create figure with subplots
    fig, axes = plt.subplots(2, 1, figsize=(14, 8))
    
    # Plot 1: Full window
    ax1 = axes[0]
    ax1.plot(window_values, linewidth=0.8, color='blue', label='Window')
    ax1.axvline(x=ir_len, color='red', linestyle='--', linewidth=1.5, label=f'IR end (n={ir_len})')
    ax1.axvline(x=ir_len + fade_len, color='green', linestyle='--', linewidth=1.5, label=f'Fade end (n={ir_len + fade_len})')
    ax1.fill_between([0, ir_len], 0, 1.2, alpha=0.1, color='blue', label='Flat region')
    ax1.fill_between([ir_len, ir_len + fade_len], 0, 1.2, alpha=0.1, color='orange', label='Fade region')
    ax1.set_xlabel('Sample Index')
    ax1.set_ylabel('Window Value')
    ax1.set_title('Tukey Window (Full)')
    ax1.grid(True, alpha=0.3)
    ax1.legend(loc='best')
    ax1.set_ylim([0, 1.2])
    
    # Plot 2: Zoomed in on fade region
    ax2 = axes[1]
    fade_start = max(0, ir_len - 100)
    fade_end = min(nfft, ir_len + fade_len + 100)
    fade_region = window_values[fade_start:fade_end]
    indices = np.arange(fade_start, fade_end)
    
    ax2.plot(indices, fade_region, linewidth=1.5, color='orange', marker='o', markersize=3, label='Window (zoomed)')
    ax2.axvline(x=ir_len, color='red', linestyle='--', linewidth=1.5, label=f'Flat end / Fade start')
    ax2.axvline(x=ir_len + fade_len, color='green', linestyle='--', linewidth=1.5, label=f'Fade end')
    ax2.set_xlabel('Sample Index')
    ax2.set_ylabel('Window Value')
    ax2.set_title('Tukey Window (Zoomed on Fade Region)')
    ax2.grid(True, alpha=0.3)
    ax2.legend(loc='best')
    ax2.set_ylim([0, 1.1])
    
    plt.tight_layout()
    
    # Save or show
    if output_file:
        try:
            plt.savefig(output_file, dpi=150, bbox_inches='tight')
            print(f"Plot saved to {output_file}")
        except Exception as e:
            print(f"Error saving plot to {output_file}: {e}", file=sys.stderr)
            return False
    else:
        plt.show()
    
    # Print statistics
    print(f"\nWindow Statistics:")
    print(f"  Total samples: {len(window_values)}")
    print(f"  IR length (flat region): {ir_len}")
    print(f"  Fade length: {fade_len}")
    print(f"  Min value: {np.min(window_values):.6f}")
    print(f"  Max value: {np.max(window_values):.6f}")
    print(f"\nKey points:")
    print(f"  window[0] = {window_values[0]:.6f} (start)")
    print(f"  window[{ir_len-1}] = {window_values[ir_len-1]:.6f} (end of flat region)")
    print(f"  window[{ir_len}] = {window_values[ir_len]:.6f} (start of fade)")
    print(f"  window[{ir_len + fade_len//2}] = {window_values[ir_len + fade_len//2]:.6f} (middle of fade)")
    print(f"  window[{ir_len + fade_len - 1}] = {window_values[ir_len + fade_len - 1]:.6f} (end of fade)")
    if ir_len + fade_len < len(window_values):
        print(f"  window[{ir_len + fade_len}] = {window_values[ir_len + fade_len]:.6f} (after fade)")
    
    return True

if __name__ == '__main__':
    # Default paths
    input_path = 'output/tukey_window_values.raw'
    output_path = 'output/tukey_window_plot.png'
    
    # Parse command line arguments
    if len(sys.argv) > 1:
        input_path = sys.argv[1]
    if len(sys.argv) > 2:
        output_path = sys.argv[2]
    
    success = plot_window(input_path, output_path)
    sys.exit(0 if success else 1)
