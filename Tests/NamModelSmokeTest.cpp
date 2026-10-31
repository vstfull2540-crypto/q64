#include <NAM/get_dsp.h>
#include <NAM/dsp.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

int main(int argc, char** argv)
{
    try
    {
        const std::filesystem::path modelPath = argc > 1
            ? std::filesystem::path(argv[1])
            : std::filesystem::path("Models/preset.NAM");

        if (!std::filesystem::exists(modelPath))
            throw std::runtime_error("NAM model file does not exist: " + modelPath.string());

        nam::DspLoadOptions options;
        options.prewarm = false;
        auto dsp = nam::get_dsp(modelPath, options);
        if (!dsp)
            throw std::runtime_error("NAM Core returned a null DSP instance.");
        if (dsp->NumInputChannels() != 1 || dsp->NumOutputChannels() != 1)
            throw std::runtime_error("Expected mono-in / mono-out model.");

        constexpr int blockSize = 512;
        constexpr double sampleRate = 48000.0;
        dsp->Reset(sampleRate, blockSize);

        std::vector<NAM_SAMPLE> input(blockSize, NAM_SAMPLE{});
        std::vector<NAM_SAMPLE> output(blockSize, NAM_SAMPLE{});
        for (int i = 0; i < blockSize; ++i)
        {
            const double t = static_cast<double>(i) / sampleRate;
            input[static_cast<size_t>(i)] = static_cast<NAM_SAMPLE>(0.1 * std::sin(2.0 * 3.141592653589793 * 997.0 * t));
        }
        NAM_SAMPLE* inputs[] = { input.data() };
        NAM_SAMPLE* outputs[] = { output.data() };
        dsp->process(inputs, outputs, blockSize);

        bool hasFiniteOutput = false;
        for (const auto sample : output)
        {
            if (!std::isfinite(static_cast<double>(sample)))
                throw std::runtime_error("NAM model produced NaN or infinite output.");
            hasFiniteOutput = hasFiniteOutput || std::abs(static_cast<double>(sample)) > 1.0e-12;
        }
        if (!hasFiniteOutput)
            throw std::runtime_error("NAM model produced only zero output for the smoke-test signal.");

        std::cout << "NAM model load/process test passed: " << modelPath.string() << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "NAM model load/process test FAILED: " << e.what() << std::endl;
        return 1;
    }
}
