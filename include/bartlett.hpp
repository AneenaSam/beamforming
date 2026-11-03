#pragma once

#include "steering_vector.hpp"
#include "covariance.hpp"
#include <vector>
#include <string>

namespace beamforming {

struct BeamformingResult {
    std::vector<double> angles_deg;
    std::vector<double> power_linear;
    std::vector<double> power_db;
    double max_power_linear = 0.0;
    double estimated_doa_deg = 0.0;
    size_t peak_index = 0;
};

/**
 * @brief Conventional Bartlett Beamformer and Peak DOA Detector.
 */
class BartlettBeamformer {
public:
    explicit BartlettBeamformer(const CircularArray& array);

    // Compute Bartlett spatial spectrum across all scanning angles
    BeamformingResult scan(const CovarianceMatrix& cov,
                           double start_deg = 0.0,
                           double end_deg = 360.0,
                           double step_deg = 1.0) const;

    // Export spatial spectrum to CSV
    static bool exportToCSV(const BeamformingResult& result,
                            double true_doa_deg,
                            const std::string& filepath);

private:
    CircularArray array_;
    SteeringVectorCalculator steering_calc_;
};

} // namespace beamforming

