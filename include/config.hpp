#pragma once

#include <cmath>
#include <string>

namespace beamforming {

#ifndef M_PI
constexpr double M_PI = 3.14159265358979323846;
#endif

/**
 * @brief Global configuration structure for Circular Array Beamforming.
 */
struct ArrayConfig {
    // Array parameters
    size_t num_sensors = 36;             // Exact number of sensors (M = 36)
    double speed_of_sound = 343.0;       // Acoustic propagation speed (m/s)
    double signal_frequency = 1000.0;    // Default carrier frequency (Hz)
    double radius_wavelength_factor = 1.5; // Array radius R = 1.5 * lambda

    // Signal simulation parameters (used in subsequent phases)
    double actual_doa_deg = 45.0;        // Initial true angle of arrival
    size_t num_snapshots = 512;          // Number of temporal time-snapshots (N)
    double snr_db = 20.0;                // Signal-to-Noise Ratio (dB)
    double sampling_rate = 8000.0;       // Temporal sampling frequency (Hz)

    // Scanning parameters
    double scan_start_deg = 0.0;
    double scan_end_deg = 360.0;
    double scan_step_deg = 1.0;          // 1-degree angular resolution

    /**
     * @brief Calculate acoustic wavelength: lambda = c / f
     */
    double wavelength() const {
        return speed_of_sound / signal_frequency;
    }

    /**
     * @brief Calculate array radius: R = factor * lambda
     */
    double radius() const {
        return radius_wavelength_factor * wavelength();
    }

    /**
     * @brief Angular separation between adjacent sensors in radians
     */
    double angular_spacing_rad() const {
        return (2.0 * M_PI) / static_cast<double>(num_sensors);
    }

    /**
     * @brief Angular separation between adjacent sensors in degrees
     */
    double angular_spacing_deg() const {
        return 360.0 / static_cast<double>(num_sensors);
    }

    /**
     * @brief Wavenumber k = 2 * pi / lambda
     */
    double wavenumber() const {
        return (2.0 * M_PI) / wavelength();
    }
};

} // namespace beamforming

