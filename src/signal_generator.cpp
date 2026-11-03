#include "signal_generator.hpp"
#include <fstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace beamforming {

SignalGenerator::SignalGenerator(const CircularArray& array, const ArrayConfig& config)
    : array_(array), config_(config), source_angle_deg_(config.actual_doa_deg),
      snr_db_(config.snr_db), amplitude_(1.0) {
    computePropagationDelays();
    generateReceivedSignals();
}

void SignalGenerator::setSource(double source_angle_deg, double snr_db, double amplitude) {
    source_angle_deg_ = source_angle_deg;
    snr_db_ = snr_db;
    amplitude_ = amplitude;
    computePropagationDelays();
    generateReceivedSignals();
}

void SignalGenerator::computePropagationDelays() {
    delays_.clear();
    const auto& sensors = array_.getSensors();
    const size_t M = sensors.size();
    delays_.reserve(M);

    const double theta_src_rad = source_angle_deg_ * (M_PI / 180.0);
    const double ux = std::cos(theta_src_rad);
    const double uy = std::sin(theta_src_rad);
    const double c = config_.speed_of_sound;
    const double k = config_.wavenumber();

    for (size_t m = 0; m < M; ++m) {
        // Projection: r_m dot u(theta)
        double proj = sensors[m].x * ux + sensors[m].y * uy;
        // Propagation time delay relative to array origin (0, 0)
        // Positive projection (sensor is closer to source) => wave arrives earlier (negative delay relative to origin)
        double tau = -proj / c;
        // Phase shift: phi_m = -k * proj
        double phi_rad = -k * proj;
        // Wrap phase to [-pi, pi]
        double phi_wrapped = std::remainder(phi_rad, 2.0 * M_PI);
        double phi_deg = phi_wrapped * (180.0 / M_PI);

        SensorDelayInfo info;
        info.sensor_id = m;
        info.spatial_projection_m = proj;
        info.time_delay_s = tau;
        info.phase_shift_rad = phi_wrapped;
        info.phase_shift_deg = phi_deg;
        info.arrival_rank = 0; // Assigned below
        delays_.push_back(info);
    }

    // Determine arrival ranking: the sensor with the largest projection is hit first (Rank 1)
    std::vector<size_t> indices(M);
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        return delays_[a].spatial_projection_m > delays_[b].spatial_projection_m;
    });

    for (size_t rank = 0; rank < M; ++rank) {
        delays_[indices[rank]].arrival_rank = rank + 1;
    }
}

void SignalGenerator::generateReceivedSignals() {
    const size_t M = config_.num_sensors;
    const size_t N = config_.num_snapshots;
    const double fs = config_.sampling_rate;
    const double f_sig = config_.signal_frequency;
    const double k = config_.wavenumber();
    const double theta_src_rad = source_angle_deg_ * (M_PI / 180.0);
    const double ux = std::cos(theta_src_rad);
    const double uy = std::sin(theta_src_rad);

    X_complex_.assign(M, std::vector<Complex>(N));
    X_real_.assign(M, std::vector<double>(N));

    // Signal power and noise variance: SNR = 10 * log10(P_signal / sigma^2)
    const double signal_power = amplitude_ * amplitude_;
    const double noise_power = signal_power / std::pow(10.0, snr_db_ / 10.0);
    const double noise_std_complex = std::sqrt(noise_power / 2.0); // Real and Imag each get half
    const double noise_std_real = std::sqrt(noise_power);

    std::mt19937_64 rng(42); // Deterministic seed for reproducible testing
    std::normal_distribution<double> dist(0.0, 1.0);

    const auto& sensors = array_.getSensors();

    // Precompute steering vector components a_m(theta_src) = exp(-j * k * r_m dot u)
    std::vector<Complex> steering_vec(M);
    for (size_t m = 0; m < M; ++m) {
        double proj = sensors[m].x * ux + sensors[m].y * uy;
        double phase = -k * proj;
        steering_vec[m] = Complex(std::cos(phase), std::sin(phase));
    }

    for (size_t n = 0; n < N; ++n) {
        double t = static_cast<double>(n) / fs;
        
        // Baseband complex envelope s(t)
        // For narrowband carrier, baseband envelope can be modulated or constant amplitude with initial phase
        Complex s_bb = amplitude_ * Complex(std::cos(2.0 * M_PI * 50.0 * t), std::sin(2.0 * M_PI * 50.0 * t));

        for (size_t m = 0; m < M; ++m) {
            // Complex baseband: X[m][n] = a_m * s(t_n) + noise
            Complex noise_c(dist(rng) * noise_std_complex, dist(rng) * noise_std_complex);
            X_complex_[m][n] = steering_vec[m] * s_bb + noise_c;

            // Real physical acoustic waveform: x_m(t) = A * cos(2*pi*f*(t - tau_m)) + noise
            double tau_m = delays_[m].time_delay_s;
            double wave_val = amplitude_ * std::cos(2.0 * M_PI * f_sig * (t - tau_m));
            double noise_r = dist(rng) * noise_std_real;
            X_real_[m][n] = wave_val + noise_r;
        }
    }
}

bool SignalGenerator::exportDelaysToCSV(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "# Sensor Propagation Delays and Arrival Ranking\n";
    file << "# SourceAngleDeg=" << source_angle_deg_ << "\n";
    file << "# SpeedOfSoundMps=" << config_.speed_of_sound << "\n";
    file << "# FrequencyHz=" << config_.signal_frequency << "\n";
    file << "# WavelengthM=" << config_.wavelength() << "\n";
    file << "sensor_id,spatial_projection_m,time_delay_s,time_delay_ms,phase_shift_rad,phase_shift_deg,arrival_rank\n";

    for (const auto& d : delays_) {
        file << d.sensor_id << ","
             << std::fixed << std::setprecision(6) << d.spatial_projection_m << ","
             << std::scientific << std::setprecision(6) << d.time_delay_s << ","
             << std::fixed << std::setprecision(4) << (d.time_delay_s * 1000.0) << ","
             << std::setprecision(6) << d.phase_shift_rad << ","
             << std::setprecision(2) << d.phase_shift_deg << ","
             << d.arrival_rank << "\n";
    }
    file.close();
    return true;
}

bool SignalGenerator::exportWaveformsToCSV(const std::string& filepath, size_t max_snapshots) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    const size_t M = config_.num_sensors;
    const size_t N = std::min(max_snapshots, config_.num_snapshots);
    const double fs = config_.sampling_rate;

    file << "# Real Received Acoustic Waveforms Across Sensors\n";
    file << "# NumSensors=" << M << "\n";
    file << "# Snapshots=" << N << "\n";
    file << "# SamplingRateHz=" << fs << "\n";
    file << "# SourceAngleDeg=" << source_angle_deg_ << "\n";

    // Header row: time_s,sensor_0,sensor_1,...,sensor_35
    file << "snapshot_idx,time_ms";
    for (size_t m = 0; m < M; ++m) {
        file << ",sensor_" << m;
    }
    file << "\n";

    for (size_t n = 0; n < N; ++n) {
        double t_ms = (static_cast<double>(n) / fs) * 1000.0;
        file << n << "," << std::fixed << std::setprecision(4) << t_ms;
        for (size_t m = 0; m < M; ++m) {
            file << "," << std::setprecision(5) << X_real_[m][n];
        }
        file << "\n";
    }
    file.close();
    return true;
}

} // namespace beamforming

