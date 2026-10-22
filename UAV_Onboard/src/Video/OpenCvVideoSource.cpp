#include "Video/OpenCvVideoSource.hpp"
#include <algorithm>
#include <cstring>

OpenCvVideoSource::OpenCvVideoSource(bool loop) : loopEnabled(loop) {}

bool OpenCvVideoSource::open(const std::string& source) {
    bool isNumeric = !source.empty() &&
        std::all_of(source.begin(), source.end(), ::isdigit);

    bool ok = isNumeric
        ? capture.open(std::stoi(source))
        : capture.open(source);

    return ok && capture.isOpened();
}

bool OpenCvVideoSource::isOpened() const {
    return capture.isOpened();
}

bool OpenCvVideoSource::read(ImageData& frame) {
    bool ok = capture.read(frameBuffer);

    if (!ok && loopEnabled) {
        capture.set(cv::CAP_PROP_POS_FRAMES, 0);
        ok = capture.read(frameBuffer);
    }

    if (!ok || frameBuffer.empty()) {
        return false;
    }

    if (!frameBuffer.isContinuous()) {
        frameBuffer = frameBuffer.clone();
    }

    frame.width = frameBuffer.cols;
    frame.height = frameBuffer.rows;
    frame.channels = frameBuffer.channels();

    size_t byteSize = frameBuffer.total() * frameBuffer.elemSize();
    frame.data.resize(byteSize);
    std::memcpy(frame.data.data(), frameBuffer.data, byteSize);

    return true;
}

void OpenCvVideoSource::close() {
    capture.release();
}