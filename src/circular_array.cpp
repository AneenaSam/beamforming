#include "circular_array.hpp"
#include <fstream>
#include <iomanip>
#include <cmath>
#include <stdexcept>

namespace beamforming {

CircularArray::CircularArray(const ArrayConfig& config)
    : config_(config) {
    generateSensors();
}

void CircularArray::generateSensors() {
    sensors_.clear();
    sensors_.reserve(config_.num_sensors);

    const double R = config_.radius();
    const double d_theta = config_.angular_spacing_rad();

    for (size_t m = 0; m < config_.num_sensors; ++m) {
        double theta_m = static_cast<double>(m) * d_theta;
        double theta_deg = theta_m * (180.0 / M_PI);
        double x_m = R * std::cos(theta_m);
        double y_m = R * std::sin(theta_m);

        // Clamp floating point rounding artifacts near zero
        if (std::abs(x_m) < 1e-12) x_m = 0.0;
        if (std::abs(y_m) < 1e-12) y_m = 0.0;

        sensors_.push_back(Sensor{m, theta_m, theta_deg, x_m, y_m});
    }
}

void CircularArray::setConfig(const ArrayConfig& config) {
    config_ = config;
    generateSensors();
}

double CircularArray::getInterElementArcDistance() const {
    return config_.radius() * config_.angular_spacing_rad();
}

bool CircularArray::exportToCSV(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << filepath << std::endl;
        return false;
    }

    // Write metadata header comments
    file << "# Circular Array Sensor Positions\n";
    file << "# NumSensors=" << config_.num_sensors << "\n";
    file << "# FrequencyHz=" << config_.signal_frequency << "\n";
    file << "# SpeedOfSoundMps=" << config_.speed_of_sound << "\n";
    file << "# WavelengthM=" << std::setprecision(6) << config_.wavelength() << "\n";
    file << "# RadiusM=" << std::setprecision(6) << config_.radius() << "\n";
    file << "# RadiusFactorLambda=" << config_.radius_wavelength_factor << "\n";
    file << "# InterElementArcDistM=" << std::setprecision(6) << getInterElementArcDistance() << "\n";

    // Write column headers
    file << "sensor_id,angle_rad,angle_deg,x_m,y_m\n";

    // Write sensor records
    for (const auto& s : sensors_) {
        file << s.id << ","
             << std::fixed << std::setprecision(8) << s.angle_rad << ","
             << std::setprecision(4) << s.angle_deg << ","
             << std::setprecision(8) << s.x << ","
             << std::setprecision(8) << s.y << "\n";
    }

    file.close();
    return true;
}

void CircularArray::printSummary(std::ostream& os) const {
    os << "========================================================\n";
    os << "       CIRCULAR SENSOR ARRAY GEOMETRY CONFIGURATION     \n";
    os << "========================================================\n";
    os << std::fixed << std::setprecision(4);
    os << "  Number of Sensors (M)     : " << config_.num_sensors << "\n";
    os << "  Signal Frequency (f)      : " << config_.signal_frequency << " Hz\n";
    os << "  Propagation Speed (c)     : " << config_.speed_of_sound << " m/s\n";
    os << "  Acoustic Wavelength (λ)   : " << config_.wavelength() << " m (" 
       << config_.wavelength() * 100.0 << " cm)\n";
    os << "  Array Radius (R)          : " << config_.radius() << " m (" 
       << config_.radius_wavelength_factor << " λ)\n";
    os << "  Angular Separation (Δθ)   : " << config_.angular_spacing_deg() << " deg ("
       << config_.angular_spacing_rad() << " rad)\n";
    os << "  Inter-element Arc Distance: " << getInterElementArcDistance() << " m ("
       << getInterElementArcDistance() / config_.wavelength() << " λ)\n";
    os << "--------------------------------------------------------\n";
    os << " ID | Angle (deg) | Angle (rad) |   X (m)   |   Y (m)   \n";
    os << "----+-------------+-------------+-----------+-----------\n";

    for (const auto& s : sensors_) {
        os << " " << std::setw(2) << s.id << " | "
           << std::setw(11) << std::setprecision(2) << s.angle_deg << " | "
           << std::setw(11) << std::setprecision(4) << s.angle_rad << " | "
           << std::setw(9) << std::setprecision(5) << s.x << " | "
           << std::setw(9) << std::setprecision(5) << s.y << "\n";
    }
    os << "========================================================\n";
}

bool CircularArray::verifySensorCount() const {
    return sensors_.size() == config_.num_sensors;
}

bool CircularArray::verifyRadialDistance(double tolerance) const {
    const double expected_R = config_.radius();
    for (const auto& s : sensors_) {
        double dist = std::sqrt(s.x * s.x + s.y * s.y);
        if (std::abs(dist - expected_R) > tolerance) {
            return false;
        }
    }
    return true;
}

bool CircularArray::verifyAngularSeparation(double tolerance) const {
    const double expected_d_theta = config_.angular_spacing_rad();
    for (size_t i = 0; i < sensors_.size(); ++i) {
        size_t next_i = (i + 1) % sensors_.size();
        double diff = sensors_[next_i].angle_rad - sensors_[i].angle_rad;
        if (next_i == 0) {
            diff += 2.0 * M_PI; // Wrap around 2*pi
        }
        if (std::abs(diff - expected_d_theta) > tolerance) {
            return false;
        }
    }
    return true;
}

} // namespace beamforming

