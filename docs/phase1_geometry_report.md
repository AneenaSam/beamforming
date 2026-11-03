# Phase 1: 36-Sensor Uniform Circular Array Geometry Report

## 1. Environment & Toolchain Inspection
- **Operating System:** Windows 11 AMD64
- **Host GPU:** NVIDIA GeForce RTX 4050 Laptop GPU (Driver 581.86, CUDA 13.0)
- **C++ Compiler:** GCC 16.2.0 (C++17/C++20/C++23 compliant)
- **Build System:** CMake 4.4.4 with Ninja 1.13.2
- **Python Environment:** Python 3.12.10 (NumPy 1.26.4, SciPy 1.17.1, Matplotlib 3.11.0)
- **SYCL Architecture Strategy:**
  - Dual-target design: Full SYCL 2020 kernel structure.
  - Native SYCL compilation supported with Intel oneAPI DPC++ (`icpx -fsycl`).
  - Spec-compliant SYCL 2020 standard host runtime enabled for GCC/Clang/MSVC environments.

---

## 2. Mathematical Foundation

### 2.1 Sensor Distribution
A Uniform Circular Array (UCA) consists of $M = 36$ omnidirectional acoustic sensors uniformly distributed around a circle of radius $R$ in the $xy$-plane ($z=0$).

The angular azimuth step between adjacent sensors is:
$$\Delta \theta = \frac{2\pi}{M} = \frac{2\pi}{36} = \frac{\pi}{18} \text{ rad} = 10^\circ$$

For each sensor index $m \in \{0, 1, 2, \dots, 35\}$:
$$\theta_m = m \cdot \Delta \theta = \frac{2\pi m}{M}$$

The Cartesian coordinates $(x_m, y_m)$ of sensor $m$ are:
$$\mathbf{r}_m = \begin{bmatrix} x_m \\ y_m \end{bmatrix} = \begin{bmatrix} R \cos(\theta_m) \\ R \sin(\theta_m) \end{bmatrix}$$

### 2.2 Wavelength and Array Aperture
- **Propagation medium:** Standard acoustic air environment at $20^\circ\text{C}$ with speed of sound $c = 343\text{ m/s}$.
- **Nominal carrier frequency:** $f = 1000\text{ Hz}$ ($1\text{ kHz}$).
- **Acoustic wavelength:**
  $$\lambda = \frac{c}{f} = \frac{343\text{ m/s}}{1000\text{ s}^{-1}} = 0.3430\text{ m} = 34.30\text{ cm}$$
- **Configured radius:** $R = 1.5 \lambda = 1.5 \times 0.3430\text{ m} = 0.5145\text{ m} = 51.45\text{ cm}$.
- **Array diameter (Aperture):** $D = 2R = 3.0\lambda = 1.029\text{ m}$.

### 2.3 Spatial Sampling & Nyquist Anti-Aliasing Criterion
The inter-element arc distance along the circular perimeter between adjacent sensors is:
$$d = R \Delta \theta = 0.5145\text{ m} \times \frac{2\pi}{36}\text{ rad} \approx 0.0898\text{ m} = 8.98\text{ cm}$$

Expressing inter-element spacing in terms of wavelength $\lambda$:
$$\frac{d}{\lambda} = \frac{0.0898}{0.3430} \approx 0.2618$$

Because $d \approx 0.2618 \lambda < 0.5 \lambda$, the spatial Nyquist sampling criterion is fully satisfied ($d < \frac{\lambda}{2}$). Consequently, the array has no grating lobes (spatial aliasing) across the entire $360^\circ$ azimuthal scanning range.

---

## 3. Sensor Geometry Verification Results

The test suite (`build/test_geometry.exe`) validated the mathematical properties:

| Test ID | Objective | Expected Value | Measured Value | Result |
| :--- | :--- | :--- | :--- | :--- |
| **TEST 1** | Total Sensor Count ($M$) | 36 sensors | 36 sensors | **PASSED** |
| **TEST 2** | Radial Distance ($R$) | $0.5145\text{ m}$ ($1.5\lambda$) | $0.51450000\text{ m}$ (tol $< 10^{-6}$) | **PASSED** |
| **TEST 3** | Angular Separation ($\Delta \theta$) | $10.0^\circ$ ($0.174533\text{ rad}$) | $10.0000^\circ$ (tol $< 10^{-6}$) | **PASSED** |
| **TEST 4** | Cardinal Orientations | $0^\circ, 90^\circ, 180^\circ, 270^\circ$ | Sensors 0, 9, 18, 27 | **PASSED** |
| **TEST 5** | CSV Export Integrity | `data/sensor_positions.csv` | Valid headers & 36 rows | **PASSED** |

---

## 4. Calculated Sensor Coordinates Table

| Sensor ID ($m$) | Azimuth Angle $\theta_m$ ($^\circ$) | Azimuth Angle $\theta_m$ ($\text{rad}$) | $x_m$ ($\text{m}$) | $y_m$ ($\text{m}$) |
| :---: | :---: | :---: | :---: | :---: |
| 0 | 0.00 | 0.0000 | +0.51450 | +0.00000 |
| 1 | 10.00 | 0.1745 | +0.50668 | +0.08934 |
| 2 | 20.00 | 0.3491 | +0.48347 | +0.17597 |
| 3 | 30.00 | 0.5236 | +0.44557 | +0.25725 |
| 4 | 40.00 | 0.6981 | +0.39413 | +0.33071 |
| 5 | 50.00 | 0.8727 | +0.33071 | +0.39413 |
| 6 | 60.00 | 1.0472 | +0.25725 | +0.44557 |
| 7 | 70.00 | 1.2217 | +0.17597 | +0.48347 |
| 8 | 80.00 | 1.3963 | +0.08934 | +0.50668 |
| 9 | 90.00 | 1.5708 | +0.00000 | +0.51450 |
| 10 | 100.00 | 1.7453 | -0.08934 | +0.50668 |
| 11 | 110.00 | 1.9199 | -0.17597 | +0.48347 |
| 12 | 120.00 | 2.0944 | -0.25725 | +0.44557 |
| 13 | 130.00 | 2.2689 | -0.33071 | +0.39413 |
| 14 | 140.00 | 2.4435 | -0.39413 | +0.33071 |
| 15 | 150.00 | 2.6180 | -0.44557 | +0.25725 |
| 16 | 160.00 | 2.7925 | -0.48347 | +0.17597 |
| 17 | 170.00 | 2.9671 | -0.50668 | +0.08934 |
| 18 | 180.00 | 3.1416 | -0.51450 | +0.00000 |
| 19 | 190.00 | 3.3161 | -0.50668 | -0.08934 |
| 20 | 200.00 | 3.4907 | -0.48347 | -0.17597 |
| 21 | 210.00 | 3.6652 | -0.44557 | -0.25725 |
| 22 | 220.00 | 3.8397 | -0.39413 | -0.33071 |
| 23 | 230.00 | 4.0143 | -0.33071 | -0.39413 |
| 24 | 240.00 | 4.1888 | -0.25725 | -0.44557 |
| 25 | 250.00 | 4.3633 | -0.17597 | -0.48347 |
| 26 | 260.00 | 4.5379 | -0.08934 | -0.50668 |
| 27 | 270.00 | 4.7124 | +0.00000 | -0.51450 |
| 28 | 280.00 | 4.8869 | +0.08934 | -0.50668 |
| 29 | 290.00 | 5.0615 | +0.17597 | -0.48347 |
| 30 | 300.00 | 5.2360 | +0.25725 | -0.44557 |
| 31 | 310.00 | 5.4105 | +0.33071 | -0.39413 |
| 32 | 320.00 | 5.5851 | +0.39413 | -0.33071 |
| 33 | 330.00 | 5.7596 | +0.44557 | -0.25725 |
| 34 | 340.00 | 5.9341 | +0.48347 | -0.17597 |
| 35 | 350.00 | 6.1087 | +0.50668 | -0.08934 |

