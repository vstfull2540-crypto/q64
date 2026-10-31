#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <NAM/get_dsp.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iterator>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <stdexcept>

class NamModelPair {
public:
    std::unique_ptr<nam::DSP> left;
    std::unique_ptr<nam::DSP> right;
    juce::String fileName;
};

static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters() {
    using Float = juce::AudioParameterFloat;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<Float>("gain", "Gain", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));
    p.push_back(std::make_unique<Float>("peak", "Peak Reduction", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 25.0f));
    p.push_back(std::make_unique<Float>("attack", "Attack", juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f), 1.0f));
    p.push_back(std::make_unique<Float>("release", "Release", juce::NormalisableRange<float>(50.0f, 5000.0f, 1.0f), 500.0f));
    p.push_back(std::make_unique<juce::AudioParameterBool>("power", "Power", true));
    p.push_back(std::make_unique<juce::AudioParameterBool>("limit", "Limit / Compress", false));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("meter", "Meter",
        juce::StringArray { "Input", "Output", "Gain Reduction" }, 2)); // UI switch uses only Output (UP) and Gain Reduction (DOWN).
    return { p.begin(), p.end() };
}

ArmNongMuangLA2AAudioProcessor::ArmNongMuangLA2AAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", makeParameters()) {}

void ArmNongMuangLA2AAudioProcessor::prepareToPlay(double sr, int maxBlock) {
    currentSampleRate = juce::jmax(8000.0, sr);
    currentMaxBlockSize = juce::jmax(1, maxBlock);
    const auto n = static_cast<size_t>(currentMaxBlockSize);
    namInputLeft.assign(n, 0.0); namInputRight.assign(n, 0.0);
    namOutputLeft.assign(n, 0.0); namOutputRight.assign(n, 0.0);
    envelopeLeft = envelopeRight = 0.0;
    if (auto model = std::atomic_load(&namModel)) {
        try {
            if (model->left) model->left->Reset(currentSampleRate, currentMaxBlockSize);
            if (model->right) model->right->Reset(currentSampleRate, currentMaxBlockSize);
        } catch (...) {
            namStatus = "NAM reset failed; using built-in compressor";
            namLoaded.store(false, std::memory_order_relaxed);
            std::atomic_store(&namModel, std::shared_ptr<NamModelPair>{});
        }
    }
}

void ArmNongMuangLA2AAudioProcessor::releaseResources() {}

bool ArmNongMuangLA2AAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void ArmNongMuangLA2AAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0) return;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, samples);

    const bool powered = parameters.getRawParameterValue("power")->load(std::memory_order_relaxed) >= 0.5f;
    if (!powered) {
        buffer.clear(); vuValueDb.store(-60.0f, std::memory_order_relaxed);
        gainReductionDb.store(0.0f, std::memory_order_relaxed); return;
    }

    const float gain = parameters.getRawParameterValue("gain")->load(std::memory_order_relaxed);
    const float peak = parameters.getRawParameterValue("peak")->load(std::memory_order_relaxed);
    const float attackMs = parameters.getRawParameterValue("attack")->load(std::memory_order_relaxed);
    const float releaseMs = parameters.getRawParameterValue("release")->load(std::memory_order_relaxed);
    const bool limiter = parameters.getRawParameterValue("limit")->load(std::memory_order_relaxed) >= 0.5f;
    const float thresholdDb = -1.0f - peak * 0.29f;
    const double attackCoeff = std::exp(-1.0 / (std::max(0.1f, attackMs) * 0.001 * currentSampleRate));
    const double releaseCoeff = std::exp(-1.0 / (std::max(10.0f, releaseMs) * 0.001 * currentSampleRate));
    const float ratio = limiter ? 10.0f : 3.0f;
    const float outputGain = juce::Decibels::decibelsToGain((gain - 50.0f) * 0.24f);

    double inputSquares = 0.0, outputSquares = 0.0;
    float maxReduction = 0.0f;
    const int inputChannels = juce::jmin(channels, getTotalNumInputChannels());
    for (int ch = 0; ch < inputChannels; ++ch) {
        const float* raw = buffer.getReadPointer(ch);
        for (int i = 0; i < samples; ++i) inputSquares += static_cast<double>(raw[i]) * raw[i];
    }

    // Run each mono NAM model on its own channel instance; processing is chunked without audio-thread allocations.
    if (auto model = std::atomic_load(&namModel); model && model->left && model->right) {
        int offset = 0;
        while (offset < samples) {
            const int count = juce::jmin(samples - offset, currentMaxBlockSize);
            for (int ch = 0; ch < inputChannels; ++ch) {
                auto& in = (ch == 0 ? namInputLeft : namInputRight);
                auto& out = (ch == 0 ? namOutputLeft : namOutputRight);
                const float* source = buffer.getReadPointer(ch, offset);
                for (int i = 0; i < count; ++i) in[static_cast<size_t>(i)] = static_cast<NAM_SAMPLE>(source[i]);
                NAM_SAMPLE* inPtrs[1] = { in.data() };
                NAM_SAMPLE* outPtrs[1] = { out.data() };
                auto* dsp = (ch == 0 ? model->left.get() : model->right.get());
                dsp->process(inPtrs, outPtrs, count);
                float* destination = buffer.getWritePointer(ch, offset);
                for (int i = 0; i < count; ++i) destination[i] = static_cast<float>(out[static_cast<size_t>(i)]);
            }
            offset += count;
        }
    }

    for (int ch = 0; ch < inputChannels; ++ch) {
        float* data = buffer.getWritePointer(ch);
        double& envelope = (ch == 0 ? envelopeLeft : envelopeRight);
        for (int i = 0; i < samples; ++i) {
            const float x = data[i];
            const double detector = std::abs(static_cast<double>(x));
            const double coeff = detector > envelope ? attackCoeff : releaseCoeff;
            envelope = coeff * envelope + (1.0 - coeff) * detector;
            const float envDb = envelope > 1.0e-9 ? static_cast<float>(20.0 * std::log10(envelope)) : -180.0f;
            float reductionDb = 0.0f;
            if (envDb > thresholdDb) reductionDb = juce::jmax(0.0f, (envDb - thresholdDb) * (1.0f - 1.0f / ratio));
            maxReduction = juce::jmax(maxReduction, reductionDb);
            data[i] = x * juce::Decibels::decibelsToGain(-reductionDb) * outputGain;
            outputSquares += static_cast<double>(data[i]) * data[i];
        }
    }

    const double denom = static_cast<double>(juce::jmax(1, inputChannels * samples));
    const auto rmsDb = [denom](double sum) -> float {
        const double rms = std::sqrt(sum / denom);
        return rms > 1.0e-9 ? static_cast<float>(20.0 * std::log10(rms)) : -60.0f;
    };
    blockInputDb = rmsDb(inputSquares);
    blockOutputDb = rmsDb(outputSquares);
    gainReductionDb.store(maxReduction, std::memory_order_relaxed);
    const int mode = juce::jlimit(0, 2, juce::roundToInt(parameters.getRawParameterValue("meter")->load(std::memory_order_relaxed) * 2.0f));
    meterMode.store(mode, std::memory_order_relaxed);
    const float vu = (mode == 0 ? blockInputDb : (mode == 1 ? blockOutputDb : -maxReduction));
    vuValueDb.store(juce::jlimit(-20.0f, 3.0f, vu), std::memory_order_relaxed);
}

bool ArmNongMuangLA2AAudioProcessor::loadNamFile(const juce::File& file, juce::String& result) {
    const auto extension = file.getFileExtension().toLowerCase();
    if (!file.existsAsFile() || (extension != ".nam" && extension != ".gen")) {
        result = "Choose a .nam or .gen model file.";
        namStatus = result;
        return false;
    }

    try {
        // Read a small prefix first. A .gen suffix is not a format specification:
        // only JSON files containing a valid NAM model can be loaded by NAM Core.
        juce::FileInputStream probe(file);
        if (!probe.openedOk())
            throw std::runtime_error("Cannot open model file.");
        const auto prefix = probe.readString().trimStart();
        const bool looksLikeJson = prefix.startsWithChar('{');
        probe.setPosition(0);

        if (extension == ".gen" && !looksLikeJson) {
            result = "Unsupported binary .gen format: this file is not a NAM JSON model. "
                     "A compatible .gen decoder/SDK is required.";
            namStatus = result;
            return false;
        }

        nam::DspLoadOptions options;
        options.prewarm = false;
        auto next = std::make_shared<NamModelPair>();
        const auto path = std::filesystem::path(file.getFullPathName().toWideCharPointer());

        if (extension == ".nam") {
            // Use NAM Core's canonical path loader so its own version/schema
            // validation and architecture registry are applied consistently.
            next->left = nam::get_dsp(path, options);
            next->right = nam::get_dsp(path, options);
        } else {
            // .gen is accepted only when its contents are actually a NAM JSON
            // document. NAM Core does not decode proprietary binary .gen files.
            std::ifstream input(path, std::ios::binary);
            if (!input.is_open())
                throw std::runtime_error("Unable to open .gen model file.");
            std::string contents((std::istreambuf_iterator<char>(input)),
                                 std::istreambuf_iterator<char>());
            const auto parsed = nlohmann::json::parse(contents);
            if (!parsed.is_object() || !parsed.contains("architecture") ||
                !parsed.contains("config") || !parsed.contains("weights"))
                throw std::runtime_error(".gen JSON does not contain a complete NAM model.");
            next->left = nam::get_dsp(parsed, options);
            next->right = nam::get_dsp(parsed, options);
        }

        if (!next->left || !next->right ||
            next->left->NumInputChannels() != 1 || next->left->NumOutputChannels() != 1 ||
            next->right->NumInputChannels() != 1 || next->right->NumOutputChannels() != 1) {
            result = "Unsupported model I/O: this plugin requires mono-in / mono-out NAM.";
            namStatus = result;
            return false;
        }

        next->left->Reset(currentSampleRate, currentMaxBlockSize);
        next->right->Reset(currentSampleRate, currentMaxBlockSize);
        next->fileName = file.getFileName();
        std::atomic_store(&namModel, next);
        namPath = file.getFullPathName();
        namLoaded.store(true, std::memory_order_relaxed);
        result = "Loaded: " + file.getFileName();
        namStatus = result;
        return true;
    } catch (const std::exception& e) {
        result = "Could not load " + file.getFileName() + ": " + juce::String(e.what());
        namStatus = result;
        return false;
    } catch (...) {
        result = "Could not load " + file.getFileName() + ": unknown model loading error";
        namStatus = result;
        return false;
    }
}

void ArmNongMuangLA2AAudioProcessor::unloadNamFile() {
    std::atomic_store(&namModel, std::shared_ptr<NamModelPair>{});
    namLoaded.store(false, std::memory_order_relaxed);
    namPath.clear();
    namStatus = "No model loaded | Built-in LA-2A compressor active";
}
juce::String ArmNongMuangLA2AAudioProcessor::getNamStatus() const { return namStatus; }
juce::String ArmNongMuangLA2AAudioProcessor::getLoadedNamName() const {
    if (auto model = std::atomic_load(&namModel)) return model->fileName;
    return {};
}

void ArmNongMuangLA2AAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = parameters.copyState();
    state.setProperty("namPath", namPath, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}
void ArmNongMuangLA2AAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        if (xml->hasTagName(parameters.state.getType())) {
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
            const juce::var savedPathValue = parameters.state.getProperty("namPath");
            const juce::String savedPath = savedPathValue.toString();
            if (savedPath.isNotEmpty() && juce::File(savedPath).existsAsFile()) {
                juce::String ignored; loadNamFile(juce::File(savedPath), ignored);
            }
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ArmNongMuangLA2AAudioProcessor(); }
juce::AudioProcessorEditor* ArmNongMuangLA2AAudioProcessor::createEditor() { return new ArmNongMuangLA2AAudioProcessorEditor(*this); }
