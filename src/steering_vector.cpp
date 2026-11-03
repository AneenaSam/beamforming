#include "steering_vector.hpp"
#include <cmath>

namespace beamforming {

SteeringVectorCalculator::SteeringVectorCalculator(const CircularArray& array)
    : array_(array) {}

std::vector<Complex> SteeringVectorCalculator::compute(double theta_deg) const {
    const auto& sensors = array_.getSensors();
    const size_t M = sensors.size();
    const double k = array_.getConfig().wavenumber();
    const double theta_rad = theta_deg * (M_PI / 180.0);
    const double ux = std::cos(theta_rad);
    const double uy = std::sin(theta_rad);

    std::vector<Complex> a(M);
    for (size_t m = 0; m < M; ++m) {
        double proj = sensors[m].x * ux + sensors[m].y * uy;
        double phase = -k * proj;
        a[m] = Complex(std::cos(phase), std::sin(phase));
    }
    return a;
}

std::vector<std::vector<Complex>> SteeringVectorCalculator::computeGrid(
    double start_deg, double end_deg, double step_deg, std::vector<double>& out_angles_deg) const {
    
    out_angles_deg.clear();
    std::vector<std::vector<Complex>> steering_matrix;

    for (double theta = start_deg; theta < end_deg; theta += step_deg) {
        out_angles_deg.push_back(theta);
        steering_matrix.push_back(compute(theta));
    }

    return steering_matrix;
}

} // namespace beamforming

