#pragma once

#include "circular_array.hpp"
#include <complex>
#include <vector>

namespace beamforming {

using Complex = std::complex<double>;

/**
 * @brief Computes steering vectors a(theta) for any scanning direction theta.
 */
class SteeringVectorCalculator {
public:
    explicit SteeringVectorCalculator(const CircularArray& array);

    // Compute steering vector of size M for angle theta_deg
    std::vector<Complex> compute(double theta_deg) const;

    // Precompute steering vectors for a grid of angles [start_deg, end_deg) with step
    std::vector<std::vector<Complex>> computeGrid(double start_deg, double end_deg, double step_deg,
                                                  std::vector<double>& out_angles_deg) const;

private:
    CircularArray array_;
};

} // namespace beamforming

