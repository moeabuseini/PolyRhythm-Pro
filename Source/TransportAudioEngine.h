#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <cstdint>

class TransportAudioEngine final : public juce::AudioSource
{
public:
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void setPattern(int voiceA, int voiceB, double bpm) noexcept;
    void setPlaying(bool shouldPlay) noexcept;
    void reset() noexcept;
    void setMasterVolume(float volume) noexcept;

    bool isPlaying() const noexcept { return playing.load(std::memory_order_relaxed); }
    bool isAudioReady() const noexcept { return audioReady.load(std::memory_order_relaxed); }
    double elapsedSeconds() const noexcept
    {
        return resetRequested.load(std::memory_order_relaxed) ? 0.0 : elapsed.load(std::memory_order_relaxed);
    }
    double cyclePhase() const noexcept;
    int cycleCount() const noexcept;

private:
    std::atomic<int> countA { 3 };
    std::atomic<int> countB { 2 };
    std::atomic<double> tempo { 96.0 };
    std::atomic<float> masterVolume { 0.65f };
    std::atomic<bool> playing { false };
    std::atomic<bool> audioReady { false };
    std::atomic<double> elapsed { 0.0 };
    std::atomic<int> publishedCycle { 0 };
    std::atomic<bool> resetRequested { false };

    double sampleRate = 44100.0;
    double phaseA = 0.0;
    double phaseB = 0.0;
    float envelopeA = 0.0f;
    float envelopeB = 0.0f;
    std::int64_t lastEventA = -1;
    std::int64_t lastEventB = -1;
    double lastTime = 0.0;
    bool wasPlaying = false;
};
