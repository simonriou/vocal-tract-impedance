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
        Path to the CSV file containing Frequency_Hz and other measurement columns
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
    
    # Validate that Frequency_Hz column exists
    if 'Frequency_Hz' not in df.columns:
        print(f"Error: CSV must contain 'Frequency_Hz' column")
        print(f"Found columns: {list(df.columns)}")
        sys.exit(1)
    
    # Extract frequency
    frequency = df['Frequency_Hz'].values
    
    # Get all measurement columns (everything except Frequency_Hz)
    all_columns = [col for col in df.columns if col != 'Frequency_Hz']

    # Get lower and upper frequency bounds for plotting in ../output/calibration_parameters.txt
    try:
        with open('../output/calibration_parameters.txt', 'r') as f:
            lines = f.readlines()
            lower_bound = None
            upper_bound = None
            for line in lines:
                if "Chirp Start Frequency" in line:
                    lower_bound = float(line.split(":")[1].strip().split()[0])
                elif "Chirp End Frequency" in line:
                    upper_bound = float(line.split(":")[1].strip().split()[0])
            else:
                print("Warning: Could not find frequency bounds in calibration parameters. Using default x-axis limits.")
    except Exception as e:
        print(f"Error reading calibration parameters: {e}")
        print("Using default x-axis limits.")
    
    # Filter to only plot Magnitude, Reactance, and Phase
    measurement_columns = [col for col in all_columns if any(keyword in col for keyword in ['Magnitude', 'Reactance', 'Phase'])]
    
    if not measurement_columns:
        print("Error: No measurement columns found in CSV")
        sys.exit(1)
    
    # Define color palette
    colors = ['b', 'r', 'g', 'orange', 'purple', 'brown', 'pink', 'gray']
    
    # Create figure with subplots for each measurement
    n_plots = len(measurement_columns)
    fig, axes = plt.subplots(n_plots, 1, figsize=(12, 4 * n_plots))
    
    # Handle single plot case (axes is not an array)
    if n_plots == 1:
        axes = [axes]
    
    # Plot each measurement column
    for idx, col in enumerate(measurement_columns):
        ax = axes[idx]
        color = colors[idx % len(colors)]
        values = df[col].values
        
        ax.plot(frequency, values, color=color, linewidth=1.5, label=col)
        ax.set_xlabel('Frequency (Hz)', fontsize=11)
        ax.set_ylabel(col, fontsize=11)
        ax.set_title(f'Vocal Tract FRF - {col}', fontsize=13, fontweight='bold')
        if lower_bound is not None and upper_bound is not None:
                print(f"Using frequency bounds from calibration parameters: {lower_bound} Hz to {upper_bound} Hz")
                ax.set_xlim(lower_bound, upper_bound)
        
        # # Set y-axis lower bound for Magnitude
        # if 'Magnitude' in col:
        #     current_ylim = ax.get_ylim()
        #     ax.set_ylim(-30, current_ylim[1])
        
        ax.grid(True, alpha=0.3)
        ax.legend(loc='best')
    
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
    
    for col in measurement_columns:
        values = df[col].values
        print(f"\n{col}:")
        print(f"  Min: {values.min():.4f}")
        print(f"  Max: {values.max():.4f}")
        print(f"  Mean: {values.mean():.4f}")


if __name__ == '__main__':
    csv_file = '../output/real_tract_frf.csv'
    
    if len(sys.argv) > 1:
        csv_file = sys.argv[1]
    
    plot_frf(csv_file)
