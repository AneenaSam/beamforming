#pragma once

#include "circular_array.hpp"
#include <complex>
#include <vector>
#include <random>

namespace beamforming {

using Complex = std::complex<double>;

/**
 * @brief Structure holding propagation delay metrics for each sensor.
 */
struct SensorDelayInfo {
    size_t sensor_id;
    double spatial_projection_m;  // r_m dot u(theta)
    double time_delay_s;          // tau_m = - (r_m dot u) / c
    double phase_shift_rad;       // phi_m = - k * (r_m dot u)
    double phase_shift_deg;
    size_t arrival_rank;          // 1 = first sensor hit, 36 = last sensor hit
};

/**
 * @brief Narrowband Acoustic Signal Generator.
 * 
 * Simulates an incoming planar wavefront arriving from a specified azimuth angle,
 * computes exact geometric propagation delays, and synthesizes time-domain
 * and complex baseband snapshot matrices with additive white Gaussian noise (AWGN).
 */
class SignalGenerator {
public:
    SignalGenerator(const CircularArray& array, const ArrayConfig& config);

    // Set or update signal source parameters
    void setSource(double source_angle_deg, double snr_db = 20.0, double amplitude = 1.0);

    // Compute propagation delays and reception order across all 36 sensors
    void computePropagationDelays();

    // Synthesize received signals matrix X (dimensions: M x N)
    void generateReceivedSignals();

    // Accessors
    const std::vector<std::vector<Complex>>& getComplexSnapshots() const { return X_complex_; }
    const std::vector<std::vector<double>>& getRealWaveforms() const { return X_real_; }
    const std::vector<SensorDelayInfo>& getSensorDelays() const { return delays_; }
    double getSourceAngleDeg() const { return source_angle_deg_; }
    double getSNRdB() const { return snr_db_; }
    size_t getSnapshotCount() const { return config_.num_snapshots; }

    // CSV Exporters for Python Visualization
    bool exportWaveformsToCSV(const std::string& filepath, size_t max_snapshots = 128) const;
    bool exportDelaysToCSV(const std::string& filepath) const;

private:
    CircularArray array_;
    ArrayConfig config_;
    double source_angle_deg_;
    double snr_db_;
    double amplitude_;

    std::vector<SensorDelayInfo> delays_;
    std::vector<std::vector<Complex>> X_complex_;  // M x N complex baseband matrix
    std::vector<std::vector<double>> X_real_;      // M x N real acoustic waveforms
};

} // namespace beamforming

