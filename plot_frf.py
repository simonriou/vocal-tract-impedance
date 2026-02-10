#!/usr/bin/env python3
"""
Plot Frequency Response Function (FRF) from real_tract_frf.csv
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
from pathlib import Path

def plot_frf(csv_file):
    """
    Load and plot FRF data from CSV file.
    
    Parameters:
    -----------
    csv_file : str
        Path to the CSV file containing Frequency_Hz, Magnitude_dB, Phase_Rad
    """
    
    # Check if file exists
    if not Path(csv_file).exists():
        print(f"Error: File '{csv_file}' not found")
        sys.exit(1)
    
    # Load data
    try:
        df = pd.read_csv(csv_file)
    except Exception as e:
        print(f"Error reading CSV file: {e}")
        sys.exit(1)
    
    # Validate columns
    required_columns = ['Frequency_Hz', 'Magnitude_dB', 'Phase_Rad']
    if not all(col in df.columns for col in required_columns):
        print(f"Error: CSV must contain columns: {required_columns}")
        print(f"Found columns: {list(df.columns)}")
        sys.exit(1)
    
    # Extract data
    frequency = df['Frequency_Hz'].values
    magnitude = df['Magnitude_dB'].values
    phase = df['Phase_Rad'].values
    
    # Create figure with two subplots
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    
    # Plot magnitude response
    ax1.plot(frequency, magnitude, 'b-', linewidth=1.5, label='Magnitude')
    ax1.set_xlabel('Frequency (Hz)', fontsize=11)
    ax1.set_ylabel('Magnitude (dB)', fontsize=11)
    ax1.set_title('Vocal Tract FRF - Magnitude Response', fontsize=13, fontweight='bold')
    ax1.grid(True, alpha=0.3)
    ax1.legend(loc='best')
    
    # Plot phase response
    ax2.plot(frequency, phase, 'r-', linewidth=1.5, label='Phase')
    ax2.set_xlabel('Frequency (Hz)', fontsize=11)
    ax2.set_ylabel('Phase (Radians)', fontsize=11)
    ax2.set_title('Vocal Tract FRF - Phase Response', fontsize=13, fontweight='bold')
    ax2.grid(True, alpha=0.3)
    ax2.legend(loc='best')
    
    plt.tight_layout()
    
    # Save figure
    output_file = csv_file.replace('.csv', '.png')
    try:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Plot saved to: {output_file}")
    except Exception as e:
        print(f"Error saving plot: {e}")
    
    # Show plot
    plt.show()
    
    # Print statistics
    print("\n=== FRF Data Statistics ===")
    print(f"Frequency range: {frequency.min():.2f} - {frequency.max():.2f} Hz")
    print(f"Number of points: {len(frequency)}")
    print(f"\nMagnitude:")
    print(f"  Min: {magnitude.min():.2f} dB")
    print(f"  Max: {magnitude.max():.2f} dB")
    print(f"  Mean: {magnitude.mean():.2f} dB")
    print(f"\nPhase:")
    print(f"  Min: {phase.min():.4f} rad")
    print(f"  Max: {phase.max():.4f} rad")
    print(f"  Mean: {phase.mean():.4f} rad")


if __name__ == '__main__':
    csv_file = 'output/real_tract_frf.csv'
    
    if len(sys.argv) > 1:
        csv_file = sys.argv[1]
    
    plot_frf(csv_file)
