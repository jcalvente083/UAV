#include <iostream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <map>

#include "Detection/OnnxYoloAdapter.hpp" 

OnnxYoloAdapter::OnnxYoloAdapter()
    : env(ORT_LOGGING_LEVEL_WARNING, "UAV_Edge"), model_loaded(false)
{
    session_options.SetIntraOpNumThreads(1);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
}

OnnxYoloAdapter::~OnnxYoloAdapter() {

}

bool OnnxYoloAdapter::loadModel(const std::string& modelPath) {
    try {
        std::filesystem::path platform_path(modelPath);
        session = std::make_unique<Ort::Session>(env, platform_path.c_str(), session_options);

        Ort::AllocatorWithDefaultOptions allocator;
        inputName = session->GetInputNameAllocated(0, allocator).get();
        outputName = session->GetOutputNameAllocated(0, allocator).get();

        model_loaded = true;
        std::cout << "[INFO] Adaptador ONNX: Modelo cargado (" << modelPath << ")" << std::endl;
        return true;

    }
    catch (const Ort::Exception& e) {
        std::cerr << "[ERROR] Adaptador ONNX falló: " << e.what() << std::endl;
        model_loaded = false;
        return false;
    }
}

bool OnnxYoloAdapter::isLoaded() const {
    return model_loaded;
}

std::vector<Detection> OnnxYoloAdapter::detect(const ImageData& image) {
    if (!model_loaded) {
        return {};
    }

    auto t0 = std::chrono::steady_clock::now();

    LetterboxInfo letterboxInfo;
    cv::Mat blob = preprocess(image, letterboxInfo);

    auto t1 = std::chrono::steady_clock::now();

    std::vector<int64_t> inputShape = { 1, 3, 480, 480 };
    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo,
        reinterpret_cast<float*>(blob.data),
        blob.total(),
        inputShape.data(),
        inputShape.size()
    );

    const char* inputNames[] = { inputName.c_str() };
    const char* outputNames[] = { outputName.c_str() };

    auto outputTensors = session->Run(
        Ort::RunOptions{ nullptr },
        inputNames, &inputTensor, 1,
        outputNames, 1
    );

    auto t2 = std::chrono::steady_clock::now();

    double preprocessMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double inferenceMs = std::chrono::duration<double, std::milli>(t2 - t1).count();

    float* rawOutput = outputTensors[0].GetTensorMutableData<float>();

    std::cout << "[DEBUG] preprocesado: " << preprocessMs << " ms | inferencia: " << inferenceMs
        << " ms | rawOutput[0..4]: "
        << rawOutput[0] << " " << rawOutput[1] << " " << rawOutput[2] << " "
        << rawOutput[3] << " " << rawOutput[4] << std::endl;

    return processOutput(rawOutput, letterboxInfo, image.width, image.height);
}

std::vector<Detection> OnnxYoloAdapter::processOutput(float* raw_data, const LetterboxInfo& letterboxInfo, int origWidth, int origHeight) {
    constexpr int NUM_CLASSES = 80;
    constexpr int NUM_ANCHORS = 4725;
    constexpr float CONF_THRESHOLD = 0.25f;
    constexpr float IOU_THRESHOLD = 0.45f;

    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;
    std::vector<int> classIds;

    for (int i = 0; i < NUM_ANCHORS; ++i) {
        int bestClass = 0;
        float bestScore = raw_data[4 * NUM_ANCHORS + i];
        for (int c = 1; c < NUM_CLASSES; ++c) {
            float score = raw_data[(4 + c) * NUM_ANCHORS + i];
            if (score > bestScore) {
                bestScore = score;
                bestClass = c;
            }
        }

        if (bestScore < CONF_THRESHOLD) {
            continue;
        }

        float cx = raw_data[i];
        float cy = raw_data[NUM_ANCHORS + i];
        float w = raw_data[2 * NUM_ANCHORS + i];
        float h = raw_data[3 * NUM_ANCHORS + i];

        float x1 = (cx - 0.5f * w - letterboxInfo.padX) / letterboxInfo.scale;
        float y1 = (cy - 0.5f * h - letterboxInfo.padY) / letterboxInfo.scale;
        float boxW = w / letterboxInfo.scale;
        float boxH = h / letterboxInfo.scale;

        boxes.emplace_back(static_cast<int>(x1), static_cast<int>(y1),
            static_cast<int>(boxW), static_cast<int>(boxH));
        confidences.push_back(bestScore);
        classIds.push_back(bestClass);
    }

    std::map<int, std::vector<int>> boxesByClass;
    for (size_t i = 0; i < classIds.size(); ++i) {
        boxesByClass[classIds[i]].push_back(static_cast<int>(i));
    }

    std::vector<int> keep;
    for (const auto& [classId, indices] : boxesByClass) {
        std::vector<cv::Rect> classBoxes;
        std::vector<float> classConfidences;
        for (int idx : indices) {
            classBoxes.push_back(boxes[idx]);
            classConfidences.push_back(confidences[idx]);
        }

        std::vector<int> classKeep;
        cv::dnn::NMSBoxes(classBoxes, classConfidences, CONF_THRESHOLD, IOU_THRESHOLD, classKeep);

        for (int localIdx : classKeep) {
            keep.push_back(indices[localIdx]);
        }
    }

    std::vector<Detection> detections;
    detections.reserve(keep.size());
    for (int idx : keep) {
        cv::Rect clamped = boxes[idx] & cv::Rect(0, 0, origWidth, origHeight);

        Detection det;
        det.x = static_cast<double>(clamped.x) / origWidth;
        det.y = static_cast<double>(clamped.y) / origHeight;
        det.width = static_cast<double>(clamped.width) / origWidth;
        det.height = static_cast<double>(clamped.height) / origHeight;
        det.confidence = confidences[idx];
        det.class_id = classIds[idx];

        detections.push_back(det);
    }

    return detections;
}

cv::Mat OnnxYoloAdapter::preprocess(const ImageData& image, LetterboxInfo& outInfo) {
    cv::Mat src(image.height, image.width, CV_8UC3, const_cast<uint8_t*>(image.data.data()));

    const int targetSize = 480;
    float scale = std::min(
        static_cast<float>(targetSize) / image.width,
        static_cast<float>(targetSize) / image.height
    );

    int newW = static_cast<int>(image.width * scale);
    int newH = static_cast<int>(image.height * scale);

    cv::Mat resized;
    cv::resize(src, resized, cv::Size(newW, newH));

    int padX = (targetSize - newW) / 2;
    int padY = (targetSize - newH) / 2;

    cv::Mat letterboxed(targetSize, targetSize, CV_8UC3, cv::Scalar(114, 114, 114));
    resized.copyTo(letterboxed(cv::Rect(padX, padY, newW, newH)));

    outInfo = { scale, padX, padY };

    cv::Mat blob;
    cv::dnn::blobFromImage(letterboxed, blob, 1.0 / 255.0, cv::Size(targetSize, targetSize),
        cv::Scalar(), true, false, CV_32F);

    return blob;
}