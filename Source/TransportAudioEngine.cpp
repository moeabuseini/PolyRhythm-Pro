#include "TransportAudioEngine.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr double twoPi = juce::MathConstants<double>::twoPi;
constexpr double frequencyA = 523.25;
constexpr double frequencyB = 392.0;
}

void TransportAudioEngine::setPattern(int a, int b, double bpm) noexcept
{
    countA.store(juce::jlimit(2, 9, a), std::memory_order_relaxed);
    countB.store(juce::jlimit(2, 9, b), std::memory_order_relaxed);
    tempo.store(juce::jlimit(40.0, 220.0, bpm), std::memory_order_relaxed);
}

void TransportAudioEngine::setPlaying(bool shouldPlay) noexcept
{
    playing.store(shouldPlay, std::memory_order_relaxed);
}

void TransportAudioEngine::reset() noexcept
{
    resetRequested.store(true, std::memory_order_relaxed);
}

void TransportAudioEngine::setMasterVolume(float volume) noexcept
{
    masterVolume.store(juce::jlimit(0.0f, 1.0f, volume), std::memory_order_relaxed);
}

double TransportAudioEngine::cyclePhase() const noexcept
{
    const auto cycleSeconds = countB.load(std::memory_order_relaxed) * 60.0
                            / tempo.load(std::memory_order_relaxed);
    return cycleSeconds > 0.0 ? std::fmod(elapsedSeconds(), cycleSeconds) / cycleSeconds : 0.0;
}

int TransportAudioEngine::cycleCount() const noexcept
{
    return resetRequested.load(std::memory_order_relaxed) ? 0 : publishedCycle.load(std::memory_order_relaxed);
}

void TransportAudioEngine::prepareToPlay(int, double rate)
{
    sampleRate = rate > 1000.0 ? rate : 44100.0;
    audioReady.store(true, std::memory_order_relaxed);
}

void TransportAudioEngine::releaseResources()
{
    audioReady.store(false, std::memory_order_relaxed);
}

void TransportAudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* const* outputChannelData = bufferToFill.buffer->getArrayOfWritePointers();
    const auto numOutputChannels = bufferToFill.buffer->getNumChannels();
    const auto numSamples = bufferToFill.numSamples;
    const auto startSample = bufferToFill.startSample;
    const auto clearChannels = juce::jmin(2, numOutputChannels);
    const auto run = playing.load(std::memory_order_relaxed);
    const auto aCount = countA.load(std::memory_order_relaxed);
    const auto bCount = countB.load(std::memory_order_relaxed);
    const auto bpm = tempo.load(std::memory_order_relaxed);
    const auto volume = masterVolume.load(std::memory_order_relaxed);
    const auto cycleLength = bCount * 60.0 / bpm;
    const auto invRate = 1.0 / sampleRate;

    if (resetRequested.exchange(false, std::memory_order_relaxed))
    {
        lastTime = 0.0;
        elapsed.store(0.0, std::memory_order_relaxed);
        publishedCycle.store(0, std::memory_order_relaxed);
        lastEventA = lastEventB = -1;
        envelopeA = envelopeB = 0.0f;
    }
    if (run && !wasPlaying)
        lastEventA = lastEventB = -1;
    wasPlaying = run;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto time = run ? lastTime + invRate : lastTime;
        if (run)
        {
            const auto eventA = static_cast<std::int64_t>(std::floor(time * aCount / cycleLength + 1.0e-9));
            const auto eventB = static_cast<std::int64_t>(std::floor(time * bCount / cycleLength + 1.0e-9));
            if (eventA != lastEventA)
            {
                envelopeA = eventA == 0 || eventA % aCount == 0 ? 1.0f : 0.74f;
                lastEventA = eventA;
            }
            if (eventB != lastEventB)
            {
                envelopeB = eventB == 0 || eventB % bCount == 0 ? 1.0f : 0.78f;
                lastEventB = eventB;
            }
            lastTime = time;
        }

        const auto sampleA = std::sin(phaseA) * envelopeA;
        const auto sampleB = std::sin(phaseB) * envelopeB;
        phaseA += twoPi * frequencyA * invRate;
        phaseB += twoPi * frequencyB * invRate;
        if (phaseA >= twoPi) phaseA -= twoPi;
        if (phaseB >= twoPi) phaseB -= twoPi;
        envelopeA *= 0.99918f;
        envelopeB *= 0.99882f;

        const auto mixed = static_cast<float>((sampleA * 0.34 + sampleB * 0.31) * volume);
        for (int channel = 0; channel < numOutputChannels; ++channel)
            if (outputChannelData[channel] != nullptr)
                outputChannelData[channel][startSample + i] = mixed;
    }
    for (int channel = clearChannels; channel < numOutputChannels; ++channel)
        bufferToFill.buffer->clear(channel, startSample, numSamples);
    if (run)
    {
        elapsed.store(lastTime, std::memory_order_relaxed);
        publishedCycle.store(static_cast<int>(lastTime / cycleLength), std::memory_order_relaxed);
    }
}
