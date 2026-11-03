#include "bartlett.hpp"
#include <fstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace beamforming {

BartlettBeamformer::BartlettBeamformer(const CircularArray& array)
    : array_(array), steering_calc_(array) {}

BeamformingResult BartlettBeamformer::scan(
    const CovarianceMatrix& cov, double start_deg, double end_deg, double step_deg) const {
    
    BeamformingResult result;
    const auto& Rxx = cov.getMatrix();
    const size_t M = Rxx.size();
    if (M == 0) return result;

    const double inv_M2 = 1.0 / (static_cast<double>(M) * static_cast<double>(M));

    std::vector<double> scan_angles;
    auto steering_grid = steering_calc_.computeGrid(start_deg, end_deg, step_deg, scan_angles);
    const size_t num_angles = scan_angles.size();

    result.angles_deg = scan_angles;
    result.power_linear.resize(num_angles);
    result.power_db.resize(num_angles);

    double max_p = -1e30;
    size_t peak_idx = 0;

    for (size_t a = 0; a < num_angles; ++a) {
        const auto& steer = steering_grid[a];

        // Compute v = Rxx * a(theta)
        // Then P(theta) = steer^H * v / M^2
        Complex p_complex(0.0, 0.0);
        for (size_t i = 0; i < M; ++i) {
            Complex v_i(0.0, 0.0);
            for (size_t j = 0; j < M; ++j) {
                v_i += Rxx[i][j] * steer[j];
            }
            // steer^H[i] * v_i = conj(steer[i]) * v_i
            p_complex += std::conj(steer[i]) * v_i;
        }

        double p = p_complex.real() * inv_M2;
        if (p < 0.0) p = 0.0; // Enforce non-negativity against numerical precision limits

        result.power_linear[a] = p;
        if (p > max_p) {
            max_p = p;
            peak_idx = a;
        }
    }

    result.max_power_linear = max_p;
    result.peak_index = peak_idx;
    result.estimated_doa_deg = scan_angles[peak_idx];

    // Compute normalized power in decibels (max is 0 dB)
    const double eps = 1e-12;
    for (size_t a = 0; a < num_angles; ++a) {
        double ratio = (result.power_linear[a] + eps) / (max_p + eps);
        result.power_db[a] = 10.0 * std::log10(ratio);
    }

    return result;
}

bool BartlettBeamformer::exportToCSV(
    const BeamformingResult& result, double true_doa_deg, const std::string& filepath) {
    
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "# Bartlett Beamforming Spatial Spectrum\n";
    file << "# TrueDOADeg=" << true_doa_deg << "\n";
    file << "# EstimatedDOADeg=" << result.estimated_doa_deg << "\n";
    file << "# AngularErrorDeg=" << std::abs(result.estimated_doa_deg - true_doa_deg) << "\n";
    file << "# MaxPowerLinear=" << result.max_power_linear << "\n";
    file << "angle_deg,power_linear,power_db\n";

    for (size_t i = 0; i < result.angles_deg.size(); ++i) {
        file << std::fixed << std::setprecision(2) << result.angles_deg[i] << ","
             << std::scientific << std::setprecision(6) << result.power_linear[i] << ","
             << std::fixed << std::setprecision(4) << result.power_db[i] << "\n";
    }

    file.close();
    return true;
}

} // namespace beamforming

