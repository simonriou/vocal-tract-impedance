# Overall Pipeline

1. **Signal Acquisition**: Send chirp to output device and record the response from the input device simultaneously (using duplex callback).

2. **Time Alignment**: Align the recorded response with the original chirp signal in the time domain using cross-correlation. Find the peak to determine the time delay.

3. **Inverse Filter**: Depending on the chirp type (linear or exponential), compute the inverse filter of the chirp in the frequency domain.

4. **Impulse Response**: Convolve the recorded response with the inverse filter to obtain the impulse response of the system:
   $$G_1(\omega) \cdot P_{\text{closed}}(\omega)$$
   where $G_1$ is the gain associated with the linear part of the system and $P_{\text{closed}}$ is the transfer function of the closed/open tract.

5. **Regularization**: Compute the regularization $\epsilon(\omega)$:
   - $\epsilon(\omega) = 0$ inside $[f_0, f_1]$
   - $\epsilon(\omega) = W_e \cdot T(f)$ outside $[f_0, f_1]$
   
   where $W_e = \int_0^{f_s/2} |G_1(f) P_{\text{open}}(f)|^2 \, df$ and $T(f)$ is a transition function:
   $$T(f) = \frac{1}{2} \left(1 + \tanh\left(\frac{1}{f_a - f} + \frac{1}{f_b - f}\right)\right)$$
   with $f_{a0} = f_0$ and $f_{b0} = f_0 - 50 \text{ Hz}$, then at the end of the recording, $f_{a1} = f_1$ and $f_{b1} = f_1 + 50 \text{ Hz}$.

6. **Frequency Response**: Compute the frequency response function $H_{\text{lips}}$:
   $$H_{\text{lips}} = \frac{P_{\text{open}} \cdot \overline{P_{\text{closed}}}}{|P_{\text{closed}}|^2 + \epsilon}$$

# Credits
The FFT logic is taken from [KissFFT](https://github.com/mborgerding/kissfft), and the audio processing is done using [PortAudio](http://www.portaudio.com/). The code is written in C. Visualisation is done in Python using Matplotlib.