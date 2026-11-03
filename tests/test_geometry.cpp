#include "circular_array.hpp"
#include <iostream>
#include <cmath>
#include <cassert>
#include <fstream>

int main() {
    std::cout << "[TEST] Running Circular Array Geometry Tests...\n\n";

    beamforming::ArrayConfig config;
    config.num_sensors = 36;
    config.speed_of_sound = 343.0;
    config.signal_frequency = 1000.0;
    config.radius_wavelength_factor = 1.5;

    beamforming::CircularArray array(config);

    // TEST 1: Sensor Count
    std::cout << "TEST 1: Verifying exactly 36 sensors are generated... ";
    if (array.verifySensorCount() && array.getSensorCount() == 36) {
        std::cout << "PASSED (Count = " << array.getSensorCount() << ")\n";
    } else {
        std::cerr << "FAILED! Expected 36 sensors, got " << array.getSensorCount() << "\n";
        return 1;
    }

    // TEST 2: Radial Distance
    std::cout << "TEST 2: Verifying all sensors lie on configured radius R = " 
              << array.getRadius() << " m... ";
    if (array.verifyRadialDistance(1e-6)) {
        std::cout << "PASSED (All sensors within 1e-6 m tolerance)\n";
    } else {
        std::cerr << "FAILED! Sensor radial distance mismatch.\n";
        return 1;
    }

    // TEST 3: Angular Separation
    std::cout << "TEST 3: Verifying angular separation is 10.0 degrees (2*pi/36 rad)... ";
    if (array.verifyAngularSeparation(1e-6)) {
        std::cout << "PASSED (Separation = " << config.angular_spacing_deg() << " deg)\n";
    } else {
        std::cerr << "FAILED! Angular separation mismatch.\n";
        return 1;
    }

    // TEST 4: Cardinal Points Verification
    std::cout << "TEST 4: Verifying cardinal point positions (0 deg, 90 deg, 180 deg, 270 deg)... ";
    const auto& sensors = array.getSensors();
    const double R = array.getRadius();

    // Sensor 0: theta = 0 deg -> (R, 0)
    assert(std::abs(sensors[0].x - R) < 1e-6 && std::abs(sensors[0].y) < 1e-6);
    // Sensor 9: theta = 90 deg -> (0, R)
    assert(std::abs(sensors[9].x) < 1e-6 && std::abs(sensors[9].y - R) < 1e-6);
    // Sensor 18: theta = 180 deg -> (-R, 0)
    assert(std::abs(sensors[18].x - (-R)) < 1e-6 && std::abs(sensors[18].y) < 1e-6);
    // Sensor 27: theta = 270 deg -> (0, -R)
    assert(std::abs(sensors[27].x) < 1e-6 && std::abs(sensors[27].y - (-R)) < 1e-6);
    std::cout << "PASSED (Sensors 0, 9, 18, 27 match exact cardinal coordinates)\n";

    // TEST 5: CSV Export Verification
    std::cout << "TEST 5: Verifying CSV export to data/sensor_positions.csv... ";
    std::string csv_path = "data/sensor_positions.csv";
    if (array.exportToCSV(csv_path)) {
        std::ifstream f(csv_path);
        if (f.good()) {
            std::cout << "PASSED (Export succeeded and file is accessible)\n";
        } else {
            std::cerr << "FAILED! CSV file not accessible.\n";
            return 1;
        }
    } else {
        std::cerr << "FAILED! Export function returned false.\n";
        return 1;
    }

    std::cout << "\n>>> ALL PHASE 1 GEOMETRY TESTS PASSED SUCCESSFULLY! <<<\n";
    return 0;
}

