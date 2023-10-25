# SOME-gui
SOME: Singing-Oriented MIDI Extractor.

This is an ONNX implementation of [openvpi/SOME](https://github.com/openvpi/SOME).

Place your ONNX models in `models` directory.

### Requirements

- Toolchains
    - A compiler that supports C++17
        - MSVC 2019 or later
        - GCC
        - Clang
    - [CMake](https://cmake.org/)
    - [vcpkg](https://github.com/microsoft/vcpkg) \(optional\)
    - [NuGet](https://www.nuget.org/) \(optional\)
- Third-party libraries:
    - [ONNX Runtime](https://onnxruntime.ai/)
        - MIT License
    - [DirectML](https://github.com/microsoft/DirectML) \(optional\)
        - MIT License
    - [libsndfile](https://github.com/libsndfile/libsndfile)
        - GNU LGPL v2.1 or later
    - [r8brain-free-src](https://github.com/avaneev/r8brain-free-src) \(optional\)
      - Sample rate converter designed by Aleksey Vaneev of Voxengo
      - MIT License
    - [libsamplerate](https://github.com/libsndfile/libsamplerate) \(optional\)
      - BSD-2-Clause license
    - [Midifile](https://github.com/craigsapp/midifile)
      - MIT License