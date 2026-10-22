#include "App/EdgeStreamingService.hpp"
#include "App/TelemetrySerializer.hpp"
#include <chrono>
#include <thread>
#include <iostream>

EdgeStreamingService::EdgeStreamingService(IVideoSource& videoSource,
    IDetector& detector,
    ITelemetryProvider& telemetryProvider,
    ITelemetrySink& telemetrySink,
    IVideoSink& videoSink,
    const Config& config)
    : videoSource(videoSource), detector(detector), telemetryProvider(telemetryProvider),
    telemetrySink(telemetrySink), videoSink(videoSink), config(config) {
}

void EdgeStreamingService::run(std::atomic<bool>& stopFlag) {
    using clock = std::chrono::steady_clock;

    const auto videoPeriod = std::chrono::milliseconds(1000 / config.videoTargetFps);
    const auto minInferenceInterval = std::chrono::milliseconds(100);

    auto lastInferenceTime = clock::now() - minInferenceInterval;

    ImageData frame;

    while (!stopFlag) {
        auto loopStart = clock::now();

        if (!videoSource.read(frame)) {
            std::cerr << "[WARN] No se pudo leer frame de video." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (clock::now() - lastInferenceTime >= minInferenceInterval) {
            cachedDetections = detector.detect(frame);
            lastInferenceTime = clock::now();
        }

        TelemetryData telemetry = telemetryProvider.sampleTelemetry();
        telemetry.detections = cachedDetections;
        telemetrySink.publish(telemetryToJson(telemetry));

        videoSink.publish(frame);

        auto elapsed = clock::now() - loopStart;
        auto sleepTime = videoPeriod - elapsed;
        if (sleepTime > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
}