# Circular Array Beamforming and Direction of Arrival (DOA) Estimation

An educational and technical implementation of **Uniform Circular Array (UCA) Beamforming and Direction-of-Arrival (DOA) Estimation** with 36 sensors, planar wave propagation simulation, and animated scientific visualization.

---

## Quick Start (One Command)

In your terminal (Command Prompt or PowerShell) inside this directory, run:

```cmd
run.bat
```

This single command:
1. Compiles the C++ engine using **CMake** and **Ninja**.
2. Runs the automated verification suite (`build\test_geometry.exe`).
3. Picks a **genuinely random incoming direction** $\theta \in [0.0^\circ, 360.0^\circ)$, simulates planar wave propagation, calculates the $36 \times 36$ spatial covariance matrix, and executes Bartlett beamforming to identify the signal origin.
4. Opens the **interactive animated visualization** window with real-time controls.

---

## Dynamic Random Signal & Interactive Controls

- **Automatic Random Selection:** Every time the simulation runs, a random azimuth angle is chosen using hardware entropy (`std::random_device`).
- **Interactive Re-Roll (`R` key or Button):** In the animation window, click **"New Random Signal (R)"** or press the **`R`** key on your keyboard. The application will instantly re-simulate a brand new random incoming direction, recompute propagation delays, re-calculate the Bartlett spatial spectrum, and update all three panels live!
- **Pause / Play (`P` key or Button):** Pause wavefront propagation and live cursors.
- **Manual Override (Optional):** You can also pass any custom angle directly via CLI:
  ```cmd
  build\beamforming_app.exe 135.5
  ```

---

## What the Interactive Animation Displays

1. **Wavefront Propagation & Sensor Capture:**
   - Incoming planar wavefronts moving across space towards the 36-sensor array.
   - Dynamic illumination of sensors as wave crests strike them.
   - Highlights the sensors that capture the signal first (**Sensor 4 and Sensor 5** at $-1.49\text{ ms}$) vs last (**Sensor 22 and Sensor 23** at $+1.49\text{ ms}$).
2. **Bartlett Beamforming & Peak DOA Detection:**
   - Polar / Cartesian spatial spectrum sweeping $0^\circ \to 360^\circ$.
   - Pinpoints the maximum power peak at **$45.0^\circ$** with $0.00^\circ$ error, locating where the signal came from.
3. **Oscilloscope (Phase Difference):**
   - Live time-domain waveforms comparing the first sensor captured vs the last sensor captured across the $1.029\text{ m}$ circular aperture.

---

## Project Structure

```
├── CMakeLists.txt              # CMake build configuration
├── requirements.txt            # Python dependencies (numpy, matplotlib, scipy)
├── run.bat                     # Single master runner script
├── .gitignore                  # Git ignore rules for build and cache files
├── include/
│   ├── config.hpp              # Global parameters (M=36, c=343 m/s, f=1000 Hz, R=1.5λ)
│   ├── circular_array.hpp      # Circular array geometry class
│   ├── signal_generator.hpp    # Planar wave propagation & delay synthesizer
│   ├── steering_vector.hpp     # Spatial steering vector calculator
│   ├── covariance.hpp          # 36 x 36 spatial covariance matrix calculator
│   └── bartlett.hpp            # Bartlett beamformer & peak DOA detector
├── src/
│   ├── circular_array.cpp
│   ├── signal_generator.cpp
│   ├── steering_vector.cpp
│   ├── covariance.cpp
│   ├── bartlett.cpp
│   └── main.cpp                # Core simulation runner
├── tests/
│   └── test_geometry.cpp       # Automated unit test suite
├── visualization/
│   ├── animated_wavefronts.py  # Interactive animated dashboard & oscilloscope
│   └── plot_array.py           # Static dual-pane array geometry plot
├── data/                       # Exported CSV datasets (positions, delays, waveforms, covariance, beam power)
└── docs/                       # High-resolution animation previews & reports
```
