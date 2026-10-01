#pragma once

#include <algorithm>
#include <chrono>
#include <vector>

// время выполнения в миллисекундах
template <typename F>
double MeasureMs(F&& action) {
    auto start = std::chrono::steady_clock::now();
    action();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// медиана нескольких запусков
template <typename F>
double MeasureMedianMs(F&& action, int repeats) {
    std::vector<double> times;
    for (int i = 0; i < repeats; ++i) {
        times.push_back(MeasureMs(action));
    }
    std::sort(times.begin(), times.end());
    return times[times.size() / 2];
}