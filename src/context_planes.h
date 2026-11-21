#pragma once
#include <algorithm>
#include <stdexcept>

// Output: four contiguous row-major 5x5 float planes, appended after plane 30.
inline void encode_context_planes(float* output, int active_worker,
                                  int movement_origin, int previous_build,
                                  bool prometheus_prebuilt) {
    const int squares[3] = {active_worker, movement_origin, previous_build};
    std::fill(output, output + 100, 0.0f);
    for (int plane = 0; plane < 3; ++plane) {
        const int square = squares[plane];
        if (square < -1 || square >= 25)
            throw std::invalid_argument("Context square must be -1 or 0..24");
        if (square >= 0) output[plane * 25 + square] = 1.0f;
    }
    if (prometheus_prebuilt) std::fill(output + 75, output + 100, 1.0f);
}
