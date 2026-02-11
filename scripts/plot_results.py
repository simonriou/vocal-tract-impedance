import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import spectrogram

# Read the data from the file
ref_chirp = np.fromfile('../output/measurement_chirp.raw', dtype=np.float32)

data_measured = np.fromfile('../output/measurement_response.raw', dtype=np.float32)
data_calibration = np.fromfile('../output/calibration_response.raw', dtype=np.float32)

data_deconv_open = np.fromfile('../output/deconvolved_measurement_response.raw', dtype=np.float32)
data_deconv_closed = np.fromfile('../output/deconvolved_calibration_response.raw', dtype=np.float32)

data_deconv_temp = np.fromfile('../output/time_domain_calibration_response.raw', dtype=np.float32)
data_deconv_temp_windowed = np.fromfile('../output/windowed_calibration_response.raw', dtype=np.float32)


# Only keep 200 ms of the deconvolved responses, as the rest is none
# data_deconv_open = data_deconv_open[:int(0.2 * 44100)]
# data_deconv_closed = data_deconv_closed[:int(0.2 * 44100)]

# Save both data to .wav files for easier hearing
from scipy.io import wavfile
wavfile.write('../output/measurement_response.wav', 44100, data_measured)
wavfile.write('../output/calibration_response.wav', 44100, data_calibration)
wavfile.write('../output/measurement_chirp.wav', 44100, ref_chirp)

# Plot the time data (ref chirp, measure and calibration)
plt.figure(figsize=(12, 8))
plt.subplot(3, 1, 1)
plt.plot(ref_chirp, label='Reference Chirp')
plt.title('Reference Chirp')
plt.xlabel('Sample Index')
plt.ylabel('Amplitude')
plt.grid()
plt.legend()
plt.subplot(3, 1, 2)
plt.plot(data_measured, label='Measured Response')
plt.title('Measured Response')
plt.xlabel('Sample Index')
plt.ylabel('Amplitude')
plt.grid()
plt.legend()
plt.subplot(3, 1, 3)
plt.plot(data_calibration, label='Calibration Response')
plt.title('Calibration Response')
plt.xlabel('Sample Index')
plt.ylabel('Amplitude')
plt.grid()
plt.legend()
plt.tight_layout()
plt.show()

# Plot deconvolved temporal signals (not windowed, windowed)
plt.figure(figsize=(12, 8))
plt.subplot(2, 1, 1)
plt.plot(data_deconv_temp, label='Deconvolved Calibration Response (Time Domain)')
plt.title('Deconvolved Calibration Response (Time Domain)')
plt.xlabel('Sample Index')
plt.ylabel('Amplitude')
plt.grid()
plt.legend()
plt.subplot(2, 1, 2)
plt.plot(data_deconv_temp_windowed, label='Deconvolved Calibration Response (Windowed)')
plt.title('Deconvolved Calibration Response (Windowed)')
plt.xlabel('Sample Index')
plt.ylabel('Amplitude')
plt.grid()
plt.legend()
plt.tight_layout()
plt.show()

# Plot frequency: magnitude spectrogram of the measured reponse and the deconvolved response (open)
# The spectrogram of the deconvolved response must be zoomed in, as only 18 ms are kept
f_meas, t_meas, Sxx_meas = spectrogram(data_measured, fs=44100, nperseg=1024)
f_deconv, t_deconv, Sxx_deconv = spectrogram(data_deconv_open, fs=44100, nperseg=256)
plt.figure(figsize=(12, 8))
plt.subplot(2, 1, 1)
plt.pcolormesh(t_meas, f_meas, 10 * np.log10(Sxx_meas), shading='gouraud')
plt.title('Spectrogram of Measured Response')
plt.ylabel('Frequency [Hz]')
plt.xlabel('Time [sec]')
plt.colorbar(label='Intensity [dB]')
plt.subplot(2, 1, 2)
plt.pcolormesh(t_deconv, f_deconv, 10 * np.log10(Sxx_deconv), shading='gouraud')
plt.title('Spectrogram of Deconvolved Response')
plt.ylabel('Frequency [Hz]')
plt.xlabel('Time [sec]')
plt.colorbar(label='Intensity [dB]')
plt.tight_layout()
plt.show()