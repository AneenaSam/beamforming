#pragma once

#include "config.hpp"
#include <vector>
#include <string>
#include <iostream>

namespace beamforming {

/**
 * @brief Represents an individual sensor element in 2D space.
 */
struct Sensor {
    size_t id;          // Sensor index m (0 to M-1)
    double angle_rad;   // Angular azimuth position theta_m in radians
    double angle_deg;   // Angular azimuth position in degrees
    double x;           // Cartesian X-coordinate: R * cos(theta_m) (meters)
    double y;           // Cartesian Y-coordinate: R * sin(theta_m) (meters)
};

/**
 * @brief Circular Sensor Array manager.
 * 
 * Computes, validates, and manages geometry for uniformly spaced
 * circular arrays (UCA).
 */
class CircularArray {
public:
    explicit CircularArray(const ArrayConfig& config = ArrayConfig{});

    // Recomputes sensor positions based on current configuration
    void generateSensors();

    // Accessors
    const std::vector<Sensor>& getSensors() const { return sensors_; }
    const ArrayConfig& getConfig() const { return config_; }
    size_t getSensorCount() const { return sensors_.size(); }
    double getRadius() const { return config_.radius(); }
    double getWavelength() const { return config_.wavelength(); }
    double getInterElementArcDistance() const;

    // Mutator for configuration
    void setConfig(const ArrayConfig& config);

    // Export geometry to CSV for Python visualization
    bool exportToCSV(const std::string& filepath) const;

    // Print summary to console
    void printSummary(std::ostream& os = std::cout) const;

    // Verification methods
    bool verifySensorCount() const;
    bool verifyRadialDistance(double tolerance = 1e-6) const;
    bool verifyAngularSeparation(double tolerance = 1e-6) const;

private:
    ArrayConfig config_;
    std::vector<Sensor> sensors_;
};

} // namespace beamforming

