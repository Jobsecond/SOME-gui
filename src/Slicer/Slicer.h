#ifndef AUDIO_SLICER_SLICER_H
#define AUDIO_SLICER_SLICER_H

#include <vector>
#include <cstddef>
#include <string>

using MarkerList = std::vector<std::pair<std::size_t, std::size_t>>;

enum SlicerErrorCode {
    SLICER_OK = 0,
    SLICER_INVALID_ARGUMENT,
    SLICER_AUDIO_ERROR
};

class Slicer {
private:
    double m_threshold;
    std::size_t m_hopSize;
    std::size_t m_winSize;
    std::size_t m_minLength;
    std::size_t m_minInterval;
    std::size_t m_maxSilKept;
    SlicerErrorCode m_errCode;
    std::string m_errMsg;

public:
    explicit Slicer(int sr, double threshold = -40.0, std::size_t minLength = 5000, std::size_t minInterval = 300, std::size_t hopSize = 20, std::size_t maxSilKept = 5000);
    MarkerList slice(const std::vector<float> &waveform, int channels);
    SlicerErrorCode getErrorCode() const;
    std::string getErrorMsg() const;
};

#endif //AUDIO_SLICER_SLICER_H