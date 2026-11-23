#pragma once
#include <JuceHeader.h>
#include "Instrument.h"
#include "BeatGrid.h"

class TransportAudioEngine final : public juce::AudioSource
{
  public:
    TransportAudioEngine();
    void prepareToPlay(int, double) override;
    void releaseResources() override { audioReady = false; }
    void getNextAudioBlock(const juce::AudioSourceChannelInfo &) override;
    void setPattern(int a, int b, double bpm) noexcept;
    void setPlaying(bool run) noexcept { playing = run; }
    void reset() noexcept { seek(0); }
    void seek(double target) noexcept { desiredPosition = std::max(0., target); }
    double playhead() const noexcept
    {
        double pending = desiredPosition.load();
        return pending >= 0 ? pending : position.load();
    }
    studio::BeatGrid grid() const noexcept;
    void setMasterVolume(float value) noexcept { masterVolume = value; }
    bool isPlaying() const noexcept { return playing.load(); }
    struct DisplayTime { double seconds, cycles; };
    DisplayTime displayTime() const noexcept;
    bool hasPendingSeek() const noexcept { return desiredPosition.load() >= 0; }
    bool isAudioReady() const noexcept { return audioReady.load(); }
    double elapsedSeconds() const noexcept { return elapsed.load(); }
    double cyclePhase() const noexcept
    {
        auto p = position.load();
        return p - std::floor(p);
    }
    int cycleCount() const noexcept { return static_cast<int>(position.load()); }
    bool popHit(studio::Hit &);
    std::array<studio::Strip, studio::layerCount> strips;
    std::atomic<int> seed{1709}, nodesPerLayer{8}, reunionPasses{8};
    std::atomic<int> masterTranspose{0};
    std::atomic<int> chordShape{0}, chordTouch{0};

  private:
    struct Note
    {
        float phase{}, stringPhase{}, step{}, amp{}, fall{}, attack{}, rise{}, brightness{};
        int sound{}, delaySamples{};
    };
    struct Grain
    {
        float offset{}, stride{}, phase{}, increment{};
    };
    struct Channel
    {
        std::array<Note, studio::BeatGrid::maxBodies> notes{};
        // Reinforcement has its own voices: it must not steal any of the
        // sixteen node attacks or disappear when a roll starts.
        std::array<Note, 4> resonance{};
        std::array<Grain, 12> grains{};
        juce::Reverb verb;
        juce::dsp::Chorus<float> ensemble;
        std::vector<float> history;
        int write{}, nextNote{}, nextGrain{}, countdown{};
        unsigned grainNumber{};
        float low{}, high{}, filtered{}, modPhase{};
        int64_t lastStep = -1;
    };
    void trigger(int layer, int64_t step, double eventPosition, double eventTime, int instrument,
                 const studio::Values &, const studio::BeatGrid &, int globalShift, bool onBoundary);
    void soundReunion(Channel &, int layer, int degree, int transpose);
    float synthNote(Note &);
    float synth(Channel &);
    float wave(float phase) const noexcept;
    float tap(const Channel &, float offset) const noexcept;
    void clear();
    std::array<Channel, studio::layerCount> channels;
    std::array<float, 4097> sine{};
    juce::AudioBuffer<float> scratch;
    juce::AbstractFifo fifo{512};
    std::array<studio::Hit, 512> hits{};
    std::atomic<int> countA{3}, countB{2};
    std::atomic<double> tempo{96}, elapsed{0}, position{0};
    std::atomic<float> masterVolume{0.65f};
    std::atomic<bool> playing{false}, audioReady{false};
    std::atomic<double> desiredPosition{-1};
    std::atomic<unsigned> stampVersion{};
    std::atomic<double> stampWall{}, stampSeconds{}, stampCycles{}, stampDuration{}, stampRate{};
    double sampleRate = 44100, clockTime = 0, cycles = 0;
};
