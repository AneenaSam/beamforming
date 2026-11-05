#include "circular_array.hpp"
#include "signal_generator.hpp"
#include "covariance.hpp"
#include "bartlett.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "====================================================================\n";
    std::cout << "   CIRCULAR ARRAY BEAMFORMING & DOA ESTIMATION SIMULATION ENGINE   \n";
    std::cout << "====================================================================\n\n";

    // 1. Array Configuration
    beamforming::ArrayConfig config;
    config.num_sensors = 36;
    config.speed_of_sound = 343.0;        // m/s
    config.signal_frequency = 1000.0;     // 1 kHz carrier
    config.radius_wavelength_factor = 1.5; // R = 1.5 * lambda = 0.5145 m
    config.num_snapshots = 512;           // N snapshots
    config.snr_db = 20.0;                 // 20 dB SNR
    config.sampling_rate = 8000.0;        // 8 kHz sampling

    // Determine Incoming Signal Direction (Random or User-specified)
    if (argc > 1) {
        try {
            config.actual_doa_deg = std::stod(argv[1]);
            std::cout << "[CONFIG] Using user-specified incoming signal DOA: " 
                      << config.actual_doa_deg << " deg\n";
        } catch (...) {
            std::cerr << "[WARNING] Invalid angle argument. Generating random angle instead.\n";
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<double> dist(0.0, 360.0);
            config.actual_doa_deg = std::round(dist(gen) * 10.0) / 10.0;
        }
    } else {
        // True random angle selection across [0.0, 360.0) degrees
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist(0.0, 360.0);
        // Round to 1 decimal place (e.g. 137.4 deg)
        config.actual_doa_deg = std::round(dist(gen) * 10.0) / 10.0;
        std::cout << ">>> [RANDOM GENERATOR] Selected random incoming signal DOA: " 
                  << std::fixed << std::setprecision(1) << config.actual_doa_deg << " deg <<<\n";
    }

    beamforming::CircularArray array(config);
    array.printSummary(std::cout);

    // Export array geometry
    array.exportToCSV("data/sensor_positions.csv");

    // 2. Incoming Signal Simulation & Propagation
    std::cout << "\n[1/4] Simulating Incoming Planar Wavefront...\n";
    std::cout << "  - Source Azimuth Angle (DOA): " << config.actual_doa_deg << " deg\n";
    std::cout << "  - Carrier Frequency         : " << config.signal_frequency << " Hz\n";
    std::cout << "  - SNR                       : " << config.snr_db << " dB\n";
    std::cout << "  - Number of Snapshots (N)   : " << config.num_snapshots << "\n";

    beamforming::SignalGenerator sig_gen(array, config);
    sig_gen.exportDelaysToCSV("data/sensor_delays.csv");
    sig_gen.exportWaveformsToCSV("data/received_signals.csv", 256);

    // Display Top 5 Sensors that capture the wave first
    const auto& delays = sig_gen.getSensorDelays();
    std::vector<beamforming::SensorDelayInfo> sorted_delays = delays;
    std::sort(sorted_delays.begin(), sorted_delays.end(), [](const auto& a, const auto& b) {
        return a.arrival_rank < b.arrival_rank;
    });

    std::cout << "\n>>> SENSOR CAPTURE ORDER (Which sensors capture the signal first) <<<\n";
    std::cout << " Rank | Sensor ID | Azimuth (deg) | Time Delay (ms) | Phase Shift (deg) | Status\n";
    std::cout << "------+-----------+---------------+-----------------+-------------------+-------------------\n";
    const auto& sensors = array.getSensors();
    for (size_t i = 0; i < 5; ++i) {
        const auto& d = sorted_delays[i];
        size_t s_id = d.sensor_id;
        std::cout << "  #" << d.arrival_rank << "  | Sensor " << std::setw(2) << s_id << " | "
                  << std::setw(11) << std::fixed << std::setprecision(1) << sensors[s_id].angle_deg << " deg | "
                  << std::setw(11) << std::fixed << std::setprecision(4) << (d.time_delay_s * 1000.0) << " ms | "
                  << std::setw(13) << std::setprecision(2) << d.phase_shift_deg << " deg | FIRST TO CAPTURE\n";
    }
    std::cout << "  ... |    ...    |      ...      |       ...       |        ...        |        ...\n";
    for (size_t i = sorted_delays.size() - 2; i < sorted_delays.size(); ++i) {
        const auto& d = sorted_delays[i];
        size_t s_id = d.sensor_id;
        std::cout << " #" << d.arrival_rank << "  | Sensor " << std::setw(2) << s_id << " | "
                  << std::setw(11) << std::fixed << std::setprecision(1) << sensors[s_id].angle_deg << " deg | "
                  << std::setw(11) << std::fixed << std::setprecision(4) << (d.time_delay_s * 1000.0) << " ms | "
                  << std::setw(13) << std::setprecision(2) << d.phase_shift_deg << " deg | LAST TO CAPTURE\n";
    }

    // 3. Covariance Matrix Calculation
    std::cout << "\n[2/4] Computing 36 x 36 Spatial Covariance Matrix Rxx...\n";
    beamforming::CovarianceMatrix cov;
    cov.compute(sig_gen.getComplexSnapshots());
    cov.exportToCSV("data/covariance_real.csv", "data/covariance_imag.csv");
    std::cout << "  - Matrix Dimension: " << cov.getDimension() << " x " << cov.getDimension() << "\n";
    std::cout << "  - Hermitian Check: " << (cov.verifyHermitian() ? "PASSED [Hermitian]" : "FAILED") << "\n";
    std::cout << "  - Positive Diagonal Check: " << (cov.verifyPositiveDiagonal() ? "PASSED" : "FAILED") << "\n";

    // 4. Conventional Bartlett Beamforming & DOA Estimation
    std::cout << "\n[3/4] Running Bartlett Beamforming Angular Scan (0 deg to 360 deg)...\n";
    beamforming::BartlettBeamformer beamformer(array);
    auto result = beamformer.scan(cov, 0.0, 360.0, 1.0);
    beamforming::BartlettBeamformer::exportToCSV(result, config.actual_doa_deg, "data/beam_power.csv");

    // 5. Results & DOA Identification
    double raw_error = std::abs(result.estimated_doa_deg - config.actual_doa_deg);
    double error = std::min(raw_error, 360.0 - raw_error); // Angular distance on circle
    std::cout << "\n====================================================================\n";
    std::cout << "             DIRECTION OF ARRIVAL (DOA) ESTIMATION RESULTS          \n";
    std::cout << "====================================================================\n";
    std::cout << "  Random Incoming Signal Direction : " << std::fixed << std::setprecision(2) 
              << config.actual_doa_deg << " deg\n";
    std::cout << "  Bartlett Estimated Direction     : " << result.estimated_doa_deg << " deg\n";
    std::cout << "  Angular Estimation Error         : " << error << " deg\n";
    std::cout << "  Peak Beamforming Power           : " << result.max_power_linear << "\n";
    std::cout << "  Identification Status            : " 
              << (error <= 1.0 ? "SUCCESSFULLY IDENTIFIED [MATCH!]" : "APPROXIMATE (within beamwidth)") << "\n";
    std::cout << "====================================================================\n";

    return 0;
}
