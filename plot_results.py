import numpy as np

import matplotlib.pyplot as plt

# Read the data from the file
data_recorded = np.fromfile('output/measurement_response.raw', dtype=np.float32)
data_sent = np.fromfile('output/measurement_chirp.raw', dtype=np.float32)

# Save both data to .wav files for easier hearing
from scipy.io import wavfile
wavfile.write('output/measurement_response.wav', 44100, data_recorded)
wavfile.write('output/measurement_chirp.wav', 44100, data_sent)

# Plot the data
plt.figure(figsize=(10, 6))
plt.plot(data_recorded, label='Recorded Response')  
plt.plot(data_sent, label='Sent Chirp', alpha=0.7)
plt.xlabel('Sample Index')
plt.ylabel('Amplitude')
plt.title('Measurement Response')
plt.grid(True)
plt.legend()
plt.show()