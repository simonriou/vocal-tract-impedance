import pandas as pd
import matplotlib.pyplot as plt

def plot_frf():
    # Load the data
    try:
        df = pd.read_csv('vocal_tract_frf.csv')
    except FileNotFoundError:
        print("Error: 'vocal_tract_frf.csv' not found. Run the C program first.")
        return

    # Plot Magnitude
    plt.figure(figsize=(10, 6))
    
    # Magnitude Plot
    plt.subplot(2, 1, 1)
    plt.semilogx(df['Frequency_Hz'], df['Magnitude_dB'], label='Measured H_lips', color='blue')
    plt.axvline(x=1500, color='r', linestyle='--', alpha=0.5, label='Target Resonance (1500 Hz)')
    plt.axhline(y=10, color='g', linestyle='--', alpha=0.5, label='Target Gain (+10 dB)')
    plt.title('Vocal Tract Frequency Response Function')
    plt.ylabel('Magnitude (dB)')
    plt.grid(True, which="both", ls="-")
    plt.legend()
    plt.xlim(100, 5000) # Zoom to relevant range
    plt.ylim(-5, 15)    # Zoom to expected gain range

    # Phase Plot
    plt.subplot(2, 1, 2)
    plt.semilogx(df['Frequency_Hz'], df['Phase_Rad'], label='Phase', color='orange')
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Phase (radians)')
    plt.grid(True, which="both", ls="-")
    plt.xlim(100, 5000)

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    plot_frf()