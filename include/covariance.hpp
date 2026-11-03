#pragma once

#include "signal_generator.hpp"
#include <vector>
#include <string>

namespace beamforming {

/**
 * @brief Computes and validates the 36 x 36 spatial covariance matrix Rxx.
 */
class CovarianceMatrix {
public:
    CovarianceMatrix() = default;

    // Compute Rxx = (1/N) * X * X^H
    void compute(const std::vector<std::vector<Complex>>& X);

    // Accessors
    const std::vector<std::vector<Complex>>& getMatrix() const { return Rxx_; }
    size_t getDimension() const { return Rxx_.size(); }

    // Verification methods
    bool verifyHermitian(double tolerance = 1e-6) const;
    bool verifyPositiveDiagonal() const;

    // CSV Exporters
    bool exportToCSV(const std::string& real_path, const std::string& imag_path) const;

private:
    std::vector<std::vector<Complex>> Rxx_; // M x M matrix
};

} // namespace beamforming

