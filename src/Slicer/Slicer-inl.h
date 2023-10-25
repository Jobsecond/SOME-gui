#ifndef SOME_GUI_SLICER_INL_H
#define SOME_GUI_SLICER_INL_H

#include <vector>
#include <cstddef>
#include <type_traits>


// DECLARATION //

template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
T divIntRound(T n, T d);

template<typename T, typename = std::enable_if_t<std::is_convertible_v<T, double>>>
std::vector<double> get_rms(const std::vector<T> &arr, std::size_t frame_length, std::size_t hop_length);

template<typename T>
std::size_t argmin_range_view(const std::vector<T>& v, std::size_t begin, std::size_t end);

template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
std::vector<T> multichannel_to_mono(const std::vector<T> &v, int channels);


// IMPLEMENTATION //

template<typename T, typename>
T divIntRound(T n, T d) {
    /*
     * Integer division rounding to the closest integer, without converting to floating point numbers.
     */
    // T should be an integer type (int, int64_t, qint64, ...)
    return ((n < 0) ^ (d < 0)) ? \
                               ((n - (d / 2)) / d) : \
                               ((n + (d / 2)) / d);
}

template<typename T, typename>
std::vector<double> get_rms(const std::vector<T> &arr, std::size_t frame_length, std::size_t hop_length) {
    std::size_t arr_length = arr.size();

    std::size_t padding = frame_length / 2;

    std::size_t rms_size = arr_length / hop_length + 1;

    std::vector<double> rms = std::vector<double>(rms_size);

    std::size_t left = 0;
    std::size_t right = 0;
    std::size_t hop_count = 0;

    std::size_t rms_index = 0;
    double val = 0;

    // Initial condition: the frame is at the beginning of padded array
    while ((right < padding) && (right < arr_length)) {
        val += static_cast<double>(arr[right]) * arr[right];
        right++;
    }
    rms[rms_index++] = std::sqrt(
            std::max(0.0, static_cast<double>(val) / static_cast<double>(frame_length)));

    // Left side or right side of the frame has not touched the sides of original array
    while ((right < frame_length) && (right < arr_length) && (rms_index < rms_size)) {
        val += static_cast<double>(arr[right]) * arr[right];
        hop_count++;
        if (hop_count == hop_length) {
            rms[rms_index++] = std::sqrt(
                    std::max(0.0, static_cast<double>(val) / static_cast<double>(frame_length)));
            hop_count = 0;
        }
        right++;  // Move right 1 step at a time.
    }

    if (frame_length < arr_length) {
        while ((right < arr_length) && (rms_index < rms_size)) {
            val += static_cast<double>(arr[right]) * arr[right] - static_cast<double>(arr[left]) * arr[left];
            hop_count++;
            if (hop_count == hop_length) {
                rms[rms_index++] = std::sqrt(
                        std::max(0.0, static_cast<double>(val) / static_cast<double>(frame_length)));
                hop_count = 0;
            }
            left++;
            right++;
        }
    } else {
        while ((right < frame_length) && (rms_index < rms_size)) {
            hop_count++;
            if (hop_count == hop_length) {
                rms[rms_index++] = std::sqrt(
                        std::max(0.0, static_cast<double>(val) / static_cast<double>(frame_length)));
                hop_count = 0;
            }
            right++;
        }
    }

    while ((left < arr_length) && (rms_index < rms_size)/* && (right < arr_length + padding)*/) {
        val -= static_cast<double>(arr[left]) * arr[left];
        hop_count++;
        if (hop_count == hop_length) {
            rms[rms_index++] = std::sqrt(
                    std::max(0.0, static_cast<double>(val) / static_cast<double>(frame_length)));
            hop_count = 0;
        }
        left++;
        right++;
    }

    return rms;
}

template<typename T>
std::size_t argmin_range_view(const std::vector<T> &v, std::size_t begin, std::size_t end) {
    // Ensure vector access is not out of bound
    auto size = v.size();
    if (begin > size)  begin = size;
    if (end > size)    end = size;
    if (begin >= end)  return 0;

    auto min_index = begin;
    T min_value = v[min_index];

    auto i = begin + 1;
    while (i < end) {
        if (v[i] < min_value) {
            min_value = v[i];
            min_index = i;
        }
        ++i;
    }

    return min_index - begin;
}

template<typename T, typename>
std::vector<T> multichannel_to_mono(const std::vector<T> &v, int channels) {
    auto frames = v.size() / channels;
    std::vector<T> out(frames);

    for (std::size_t i = 0; i < frames; i++) {
        T s = 0;
        for (int j = 0; j < channels; j++) {
            s += static_cast<T>(v[i * channels + j]) / static_cast<T>(channels);
        }
        out[i] = s;
    }

    return out;
}

#endif //SOME_GUI_SLICER_INL_H
