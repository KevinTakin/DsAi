#include "AILuj1.h"
#include <iostream>


void AILuj::init(const char* param_path, const char* bin_path) {
    opt.use_vulkan_compute = false;
    net.opt = opt;
    net.load_param(param_path);
    net.load_model(bin_path);

    input = ncnn::Mat(2);
    prev_coord = { 0, 0 };
}

std::vector<std::pair<double, double>> AILuj::predict(double x, double y) {
    input[0] = x - prev_coord[0];
    input[1] = y - prev_coord[1];

    ncnn::Extractor ex = net.create_extractor();
    ex.input("input", input);

    ncnn::Mat output;
    ex.extract("output", output);

    std::vector<std::vector<double>> coordinates;
    for (int i = 0; i < output.h; i++) {
        std::vector<double> coord;
        for (int j = 0; j < output.w; j++) {
            coord.push_back(output[i * output.w + j]);
        }
        coordinates.push_back(coord);
    }

    std::vector<std::pair<double, double>> relativePath;
    std::pair<double, double> origin = { 0.0, 0.0 };
    std::pair<double, double> extraNumbers = { 0.0, 0.0 };
    std::pair<double, double> totalOffset = { 0.0, 0.0 };

    for (const auto& point : coordinates) {
        std::pair<double, double> currentPoint = { point[0], point[1] };
        std::pair<double, double> offset = {
            currentPoint.first - origin.first,
            currentPoint.second - origin.second
        };

        extraNumbers.first += offset.first;
        extraNumbers.second += offset.second;

        if (std::abs(extraNumbers.first) >= 0.05) {
            double roundedValue = std::round(extraNumbers.first);
            relativePath.push_back({ roundedValue, 0.0 });
            totalOffset.first += roundedValue;
            extraNumbers.first -= roundedValue;
        }

        if (std::abs(extraNumbers.second) >= 0.05) {
            double roundedValue = std::round(extraNumbers.second);
            relativePath.push_back({ 0.0, roundedValue });
            totalOffset.second += roundedValue;
            extraNumbers.second -= roundedValue;
        }

        origin = currentPoint;
    }

    if (std::abs(extraNumbers.first) > 0.0) {
        relativePath.push_back({ extraNumbers.first, 0.0 });
        totalOffset.first += extraNumbers.first;
    }
    if (std::abs(extraNumbers.second) > 0.0) {
        relativePath.push_back({ 0.0, extraNumbers.second });
        totalOffset.second += extraNumbers.second;
    }

    // ¼ÆËãÎó²î
    double xError = input[0] - totalOffset.first;
    double yError = input[1] - totalOffset.second;

    if (std::abs(xError) > 1e-6 || std::abs(yError) > 1e-6) {
        if (std::abs(xError) > 1e-6) {
            relativePath.push_back({ xError, 0.0 });
            totalOffset.first += xError;
        }

        if (std::abs(yError) > 1e-6) {
            relativePath.push_back({ 0.0, yError });
            totalOffset.second += yError;
        }
    }

    return relativePath;
}