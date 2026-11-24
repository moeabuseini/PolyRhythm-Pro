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

    // Message-thread snapshots of the journal are taken under the device callback lock.
    struct ScoreNote {double seconds, duration;int layer, pitch;float velocity;};
    std::vector<ScoreNote> journal=std::vector<ScoreNote>(65536);
    size_t journalWrite=0, journalCount=0;
    double journalOrigin=0,lastMeeting=-1,previousMeeting=-1;
    std::atomic<int> bassSeed{-1},bassStart{0},melodyBefore{-1},melodyAfter{-1},melodySwitch{0};
    std::atomic<bool> roadmap{false}, resolveCue{false}, spaceCue{false};
    std::array<std::atomic<int>,4> chordRoots{{2,10,5,0}}, chordKinds{{1,2,0,4}};
    std::atomic<float> outputPower{0};
    bool suppressAttacks=false; // offline renderer only
    std::function<void(int,const juce::AudioBuffer<float>&,int)> stemSink;
    void copySetupTo(TransportAudioEngine&) const;
    int nextNodePitch(int layer,int body,double position) const;
    void audition(int layer,int midi);
  private:
    void remember(double seconds,double duration,int layer,int midi,float strength);
    int pitchAt(const studio::BeatGrid&,int layer,int64_t step,int body,double pos,bool tonic,int stackIndex,int companion) const;
    int harmonicPitch(int midi,int layer,double position,bool tonic) const;
    double spaceUntil=-1;
    std::atomic<int> auditionRequest{-1};

    struct Note
    {
        float phase{}, stringPhase{}, step{}, amp{}, fall{}, attack{}, rise{}, brightness{};
        int sound{}, delaySamples{};
    };
    Note previewNote;
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
        float tapFrom{}, tapTo{}, tapBlend{1.f}, mixGain{};
        float low{}, high{}, filtered{}, modPhase{};
        int64_t lastStep = -1;
    };
    void trigger(int layer, int64_t step, double eventPosition, double eventTime, int instrument,
                 const studio::Values &, const studio::BeatGrid &, int globalShift, bool onBoundary);
    void soundReunion(Channel &, int layer, int degree, int transpose,double position,double seconds,bool tonic);
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
