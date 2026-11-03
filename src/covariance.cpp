#include "covariance.hpp"
#include <fstream>
#include <iomanip>
#include <cmath>

namespace beamforming {

void CovarianceMatrix::compute(const std::vector<std::vector<Complex>>& X) {
    if (X.empty() || X[0].empty()) return;

    const size_t M = X.size();
    const size_t N = X[0].size();
    const double inv_N = 1.0 / static_cast<double>(N);

    Rxx_.assign(M, std::vector<Complex>(M, Complex(0.0, 0.0)));

    for (size_t i = 0; i < M; ++i) {
        for (size_t j = i; j < M; ++j) {
            Complex sum(0.0, 0.0);
            for (size_t n = 0; n < N; ++n) {
                // X[i][n] * conj(X[j][n])
                sum += X[i][n] * std::conj(X[j][n]);
            }
            Complex val = sum * inv_N;
            Rxx_[i][j] = val;
            if (i != j) {
                Rxx_[j][i] = std::conj(val); // Exploit Hermitian symmetry
            }
        }
    }
}

bool CovarianceMatrix::verifyHermitian(double tolerance) const {
    const size_t M = Rxx_.size();
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < M; ++j) {
            Complex diff = Rxx_[i][j] - std::conj(Rxx_[j][i]);
            if (std::abs(diff) > tolerance) {
                return false;
            }
        }
    }
    return true;
}

bool CovarianceMatrix::verifyPositiveDiagonal() const {
    const size_t M = Rxx_.size();
    for (size_t i = 0; i < M; ++i) {
        if (Rxx_[i][i].real() < 0.0 || std::abs(Rxx_[i][i].imag()) > 1e-6) {
            return false;
        }
    }
    return true;
}

bool CovarianceMatrix::exportToCSV(const std::string& real_path, const std::string& imag_path) const {
    std::ofstream f_real(real_path);
    std::ofstream f_imag(imag_path);
    if (!f_real.is_open() || !f_imag.is_open()) return false;

    const size_t M = Rxx_.size();
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < M; ++j) {
            f_real << std::fixed << std::setprecision(6) << Rxx_[i][j].real();
            f_imag << std::fixed << std::setprecision(6) << Rxx_[i][j].imag();
            if (j + 1 < M) {
                f_real << ",";
                f_imag << ",";
            }
        }
        f_real << "\n";
        f_imag << "\n";
    }
    return true;
}

} // namespace beamforming

