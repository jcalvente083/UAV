#include "Telemetry/SimulatedTelemetryProvider.hpp"

#include <chrono>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>


SimulatedTelemetryProvider::SimulatedTelemetryProvider(double initialLat, double initialLon)
    : latitude(initialLat), longitude(initialLon) {}


TelemetryData SimulatedTelemetryProvider::sampleTelemetry() {
    TelemetryData data;

    auto now = std::chrono::system_clock::now();
    data.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();

    batteryLevel -= 0.01;
    batteryLevel = std::max(batteryLevel, 0.0);
    data.battery = batteryLevel;

    time += 0.05;

    data.altitude = 50.0 + std::sin(time) * 5.0;
    data.velocity = std::max(0.0, 10.0 + std::cos(time) * 3.0);

    latitude += std::sin(time) * 0.0001;
    longitude += std::cos(time) * 0.0001;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << latitude << "," << longitude;
    data.coordinates = oss.str();

    data.detections.clear();

    return data;
}