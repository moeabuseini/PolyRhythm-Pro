#include "TransportAudioEngine.h"
#include "NodeVoicing.h"
#include <cmath>

using namespace studio;
namespace
{
unsigned mix(unsigned x) noexcept { return BeatGrid::hash(x); }
float unit(unsigned x) noexcept { return static_cast<float>(mix(x) >> 8) / 16777216.0f; }
constexpr int roots[]{0, 3, 5, 4};
} // namespace
TransportAudioEngine::TransportAudioEngine()
{
    for (size_t i = 0; i < sine.size(); ++i)
        sine[i] = static_cast<float>(std::sin(juce::MathConstants<double>::twoPi * i / 4096.0));
    constexpr int patches[]{0, 2, 3, 4, 11};
    constexpr float gains[]{82, 65, 45, 36, 42};
    for (int l = 0; l < layerCount; ++l)
    {
        auto &s = strips[l];
        for (int k = 0; k < parameterCount; ++k)
            s.values[k] = dials[k].initial;
        s.sound = patches[l];
        s.values[gain] = gains[l];
    }
    strips[1].values[wet] = 9;
    strips[1].values[delay] = 0;
    strips[2].values[decay] = 2.5f;
    strips[3].values[decay] = 4;
    strips[3].values[chorus] = 45;
    strips[3].values[cloud] = 30;
}
float TransportAudioEngine::wave(float phase) const noexcept
{
    const float index = (phase - std::floor(phase)) * 4096.0f;
    const int i = static_cast<int>(index) & 4095;
    return sine[i] + (sine[i + 1] - sine[i]) * (index - std::floor(index));
}
void TransportAudioEngine::setPattern(int a, int b, double bpm) noexcept
{
    countA = juce::jlimit(2, 9, a);
    countB = juce::jlimit(2, 9, b);
    tempo = juce::jlimit(1., 180., bpm);
}
BeatGrid TransportAudioEngine::grid() const noexcept
{
    BeatGrid result;
    result.seed = seed.load();
    result.bodies = std::clamp(nodesPerLayer.load(), 1, BeatGrid::maxBodies);
    result.passesPerReunion = std::clamp(reunionPasses.load(), 2, 16);
    result.divisions = {2 * countA.load(), 2 * countB.load(), 2, 1, 4};
    for (int layer = 0; layer < layerCount; ++layer)
    {
        const auto &params = strips[layer].values;
        result.divisions[layer] *= static_cast<int>(params[rate].load());
        result.probability[layer] =
            strips[layer].enabled.load() && params[gain].load() > 0 ? params[chance].load() : 0;
    }
    return result;
}
void TransportAudioEngine::clear()
{
    stampDuration=0;
    for (auto &c : channels)
    {
        c.notes = {};
        c.resonance = {};
        c.grains = {};
        c.verb.reset();
        c.ensemble.reset();
        std::fill(c.history.begin(), c.history.end(), 0.f);
        c.write = c.nextNote = c.nextGrain = c.countdown = 0;
        c.grainNumber = 0;
        c.low = c.high = c.filtered = c.modPhase = 0;
        c.lastStep = -1;
    }
    cycles = clockTime = 0;
    position = 0;
    elapsed = 0;
}
void TransportAudioEngine::prepareToPlay(int block, double sr)
{
    sampleRate = sr;
    scratch.setSize(2, juce::jmax(1, block));
    for (auto &c : channels)
    {
        c.history.resize(static_cast<size_t>(sr * 2.5) + 4);
        c.verb.setSampleRate(sr);
        c.ensemble.prepare({sr, static_cast<juce::uint32>(scratch.getNumSamples()), 2});
    }
    clear();
    audioReady = true;
}
bool TransportAudioEngine::popHit(Hit &hit)
{
    auto read = fifo.read(1);
    if (!read.blockSize1)
        return false;
    hit = hits[read.startIndex1];
    return true;
}
void TransportAudioEngine::trigger(int layer, int64_t step, double eventPosition, double eventTime,
                                   int instrument, const Values &v, const BeatGrid &score, int globalShift,
                                   bool onBoundary)
{
    auto &c = channels[layer];
    const auto seedValue = score.seed;
    const auto key = score.key(layer, step);
    const auto mask = score.attackMask(layer, step);
    if (mask == 0)
        return;
    const int root = roots[(static_cast<int>(eventPosition) / 2 + seedValue % 4) % 4];
    constexpr int registers[]{62, 38, 50, 62, 62};
    constexpr int motif[]{0, 2, 4, 6, 4, 2, 1, 4};
    const int variant = static_cast<int>(mix(key + 1) % 3);
    int degree = root;
    if (layer == 0)
        degree += motif[mix(static_cast<unsigned>(step % (5 + seedValue % 4)) + seedValue) % 8] +
                  (variant == 2 ? 2 : 0);
    if (layer == 1 && step % 4 == 3)
        degree += 4;
    if (layer == 3)
        degree += 4 + variant * 2;
    const float strength = (0.77f + 0.22f * unit(key + 2)) * score.accent(layer, step);
    const bool tutti = step % score.phraseLength() == 0;
    const int mode = strips[layer].pitchMode.load();
    const int copies = std::clamp(strips[layer].nodesPerPitch.load(), 1, score.bodies);
    const int transpose = static_cast<int>(v[pitch] + 12 * v[octave]);
    const bool reunion = onBoundary && score.isFullReunion(layer, step);
    if (reunion)
        soundReunion(c, layer, mode == ensemble ? root : nodeRoot(seedValue), transpose + globalShift);
    const int shape = chordShape.load();
    const int touch = chordTouch.load();
    const bool voiced = layer == 2 && mode != ensemble && touch != single && !tutti && !reunion;
    const bool stackedChord = mode == ensemble && layer == 2;
    const int notes = tutti ? score.bodies : (stackedChord || voiced ? 3 : 1);
    const float attackLevel = reunion ? 0.58f / std::pow(static_cast<float>(notes), 0.72f)
                                     : tutti ? 0.36f / score.bodies : notes > 1 ? 0.10f : 0.23f;
    for (int n = 0; n < notes; ++n)
    {
        const int body = tutti ? n : score.soloBody(step);
        const int layerPitch = mode != ensemble
            ? nodePitch(layer, body, score.bodies, copies, mode, seedValue, transpose, shape, voiced ? n : 0)
            : juce::jlimit(
            21, 105,
            registers[layer] + scaleInterval(degree + (tutti ? 2 * (n % (layer == 1 ? 2 : 4)) : 2 * n)) +
                transpose);
        const int midi = layerPitch + globalShift;
        auto &note = c.notes[c.nextNote];
        c.nextNote = (c.nextNote + 1) % static_cast<int>(c.notes.size());
        note = {};
        note.sound = instrument;
        note.step = static_cast<float>(440 * std::exp2((midi - 69) / 12.) / sampleRate);
        note.amp = strength * attackLevel;
        if (voiced && touch == rolled)
        {
            // Keep the last rolled tone inside this attack's subdivision.
            const double spacing = std::min(0.042, 0.4 * 120.0 * countB.load() / (tempo.load() * score.divisions[layer]));
            note.delaySamples = juce::roundToInt(n * spacing * sampleRate);
        }
        note.fall = static_cast<float>(std::exp(-4.0 / (v[decay] * sampleRate)));
        constexpr float attacks[]{0.004f, 0.009f, 0.016f, 0.09f,  0.32f, 0.022f,
                                  0.018f, 0.008f, 0.003f, 0.007f, 0.055f, 0.047f, 0.009f, 0.14f};
        note.rise = static_cast<float>(1 / (attacks[instrument] * sampleRate));
        note.brightness = 1;
        // Companion tones colour the node; they aren't additional crossings.
        if (voiced && n != 0) continue;
        auto write = fifo.write(1);
        if (write.blockSize1)
            hits[write.startIndex1] = {eventTime, eventPosition,
                                       layer,     midi,
                                       strength,  body, reunion && n == 0};
    }
}
void TransportAudioEngine::soundReunion(Channel &c, int layer, int degree, int transpose)
{
    constexpr int registers[]{62, 38, 50, 74, 62};
    const double beat = std::clamp(60.0 / tempo.load(), 0.32, 0.85);
    // Foundation, fifth, upper neighbour, then its resolution an octave
    // above the foundation. Each layer resonates through its own FX and fader.
    constexpr int degrees[]{0, 4, 8, 7};
    constexpr float levels[]{0.21f, 0.095f, 0.080f, 0.095f};
    const double lengths[]{3.8 * beat, 3.4 * beat, 0.68 * beat, 2.8 * beat};
    for (int i = 0; i < 4; ++i)
    {
        auto &n = c.resonance[i];
        n = {};
        const int midi = juce::jlimit(16, 120, registers[layer] + scaleInterval(degree + degrees[i]) + transpose);
        n.step = static_cast<float>(440 * std::exp2((midi - 69) / 12.) / sampleRate);
        n.amp = levels[i];
        n.fall = static_cast<float>(std::exp(-3.0 / (sampleRate * lengths[i])));
        n.rise = static_cast<float>(1.0 / (sampleRate * (i == 0 ? 0.010 : 0.030)));
        n.sound = i == 0 ? 6 : 10;
        if (i == 3) n.delaySamples = juce::roundToInt(0.52 * beat * sampleRate);
    }
}
float TransportAudioEngine::synthNote(Note &n)
{
    if (n.amp < 0.00002f) return 0;
    if (n.delaySamples > 0) { --n.delaySamples; return 0; }
    const float p = n.phase, fundamental = wave(p);
    float tone = fundamental;
    switch (n.sound)
    {
    case 0:
        tone = 0.67f * fundamental + 0.33f * wave(n.stringPhase) +
               n.brightness * (0.5f * wave(2 * p) + 0.27f * wave(3 * p) + 0.12f * wave(5 * p));
        break;
    case 1:
        tone = wave(p + 0.28f * n.brightness * wave(3 * p));
        break;
    case 2:
        tone += 0.28f * wave(2 * p);
        break;
    case 3:
        tone += 0.32f * wave(2 * p) + 0.17f * wave(3 * p);
        break;
    case 4:
        tone =
            0.6f * fundamental + 0.4f * wave(n.stringPhase) + 0.25f * wave(3 * p) + 0.18f * wave(5 * p);
        break;
    case 5:
        tone += 0.42f * wave(3 * p) + 0.14f * wave(5 * p);
        break;
    case 6:
        tone = 0.95f * fundamental + 0.05f * wave(2 * p);
        break;
    case 7:
        tone = wave(p + 0.20f * n.brightness * wave(p)) + 0.22f * wave(2 * p);
        break;
    case 8:
        tone = 0.69f * fundamental + 0.31f * wave(n.stringPhase);
        break;
    case 9:
        tone = wave(p + 0.17f * n.brightness * wave(2 * p)) + 0.15f * wave(3 * p);
        break;
    case 10:
        tone = 0.84f * fundamental + 0.14f * wave(2 * p) + 0.08f * wave(3 * p);
        break;
    case 11: // Solar brass: attack-driven harmonic bloom.
        tone = 0.64f * fundamental + 0.20f * wave(n.stringPhase) +
               n.brightness * (0.32f * wave(2 * p) + 0.24f * wave(3 * p) + 0.12f * wave(5 * p));
        break;
    case 12: // Prism lead: a bright FM transient over a stable fundamental.
        tone = 0.26f * fundamental + 0.74f * wave(p + (0.07f + 0.23f * n.brightness) * wave(2 * p));
        break;
    case 13: // Titan strings: opposing detune and sustained odd harmonics.
        tone = 0.40f * fundamental + 0.24f * wave(n.stringPhase) +
               0.20f * wave(2 * p - n.stringPhase) + 0.22f * wave(3 * p) + 0.09f * wave(5 * p);
        break;
    }
    n.stringPhase += n.step * (n.sound == 8 ? 2.756f : n.sound == 13 ? 1.007f : n.sound == 4 ? 1.005f : 1.0014f);
    n.stringPhase -= std::floor(n.stringPhase);
    n.phase += n.step;
    n.phase -= std::floor(n.phase);
    n.attack = std::min(1.f, n.attack + n.rise);
    const float value = tone * n.attack * n.amp;
    n.amp *= n.fall;
    n.brightness *= n.fall;
    return value;
}
float TransportAudioEngine::synth(Channel &c)
{
    float sum = 0;
    for (auto &note : c.notes) sum += synthNote(note);
    for (auto &tone : c.resonance) sum += synthNote(tone);
    return sum;
}
float TransportAudioEngine::tap(const Channel &c, float offset) const noexcept
{
    const int size = static_cast<int>(c.history.size());
    float pos = c.write - juce::jlimit(1.f, static_cast<float>(size - 2), offset);
    if (pos < 0)
        pos += size;
    int a = std::min(static_cast<int>(pos), size - 1);
    return c.history[a] + (c.history[(a + 1) % size] - c.history[a]) * (pos - a);
}
void TransportAudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo &out)
{
    juce::ScopedNoDenormals noDenormals;
    out.clearActiveBufferRegion();
    const auto score = grid();
    const int globalShift = masterTranspose.load();
    const double target = desiredPosition.exchange(-1);
    if (target >= 0)
    {
        clear();
        cycles = target;
        clockTime = target * 60. * countB.load() / tempo.load();
        position = cycles;
        elapsed = clockTime;
        for (int i = 0; i < layerCount; ++i)
            channels[i].lastStep =
                static_cast<int64_t>(std::ceil((target - 1.e-9) * score.divisions[i] * 0.5)) - 1;
    }
    if (!playing || !audioReady)
        return;
    const double bpm = tempo.load(), cycleInc = bpm / (60 * countB.load() * sampleRate);

    constexpr float pan[] = {-0.25f, 0, 0.18f, 0.36f, -0.12f};
    const auto seedValue = score.seed;
    const float volume = masterVolume.load();
    // At very slow tempi, choose a shorter musical subdivision rather than
    // clipping the tap to the last sample of the delay line.
    double echoSeconds = 45.0 / bpm;
    while (echoSeconds > 2.4)
        echoSeconds *= 0.5;
    const auto pole = [this](float hz)
    { return static_cast<float>(1 - std::exp(-juce::MathConstants<double>::twoPi * hz / sampleRate)); };
    const float bassPole = pole(280), highPole = pole(3200);
    for (int base = 0; base < out.numSamples; base += scratch.getNumSamples())
    {
        const int len = std::min(scratch.getNumSamples(), out.numSamples - base);
        const double startTime = clockTime;
        for (int l = 0; l < layerCount; ++l)
        {
            auto &c = channels[l];
            const auto v = strips[l].snapshot();
            const int patch = strips[l].sound.load();
            const auto eventsPerCycle = score.divisions[l] * 0.5;
            const float filterPole = pole(v[cutoff]), lo = juce::Decibels::decibelsToGain(v[lowEQ]) - 1,
                        hi = juce::Decibels::decibelsToGain(v[highEQ]) - 1;
            juce::Reverb::Parameters rv;
            rv.damping = v[damping] * 0.01f;
            rv.width = v[stereoWidth] * 0.01f;
            c.ensemble.setRate(v[modulation]);
            c.ensemble.setMix(v[chorus] * 0.01f);
            c.ensemble.setDepth(v[chorusDepth] * 0.01f);
            c.ensemble.setCentreDelay(v[chorusCentre]);
            c.ensemble.setFeedback(v[chorusFeedback] * 0.01f);
            rv.roomSize = v[room] * 0.01f;
            rv.wetLevel = v[wet] * 0.007f;
            rv.dryLevel = 0.5f;
            c.verb.setParameters(rv);
            auto *left = scratch.getWritePointer(0);
            auto *right = scratch.getWritePointer(1);
            for (int i = 0; i < len; ++i)
            {
                const double now = cycles + i * cycleInc;
                const auto step = static_cast<int64_t>((now + 1.e-9) * eventsPerCycle);
                if (step != c.lastStep)
                {
                    const double eventTime = startTime + i / sampleRate;
                    const bool onBoundary = now - step / eventsPerCycle <= cycleInc + 1.e-9;
                    trigger(l, step, now + 1.e-9, eventTime, patch, v, score, globalShift, onBoundary);
                    c.lastStep = step;
                }
                const float dry = synth(c);
                c.filtered += filterPole * (dry - c.filtered);
                c.low += bassPole * (c.filtered - c.low);
                c.high += highPole * (c.filtered - c.high);
                const float input = c.filtered + lo * c.low + hi * (c.filtered - c.high);
                const float echo = tap(c, static_cast<float>(sampleRate * echoSeconds));
                float mist = 0;
                if (v[cloud] > 0)
                {
                    if (--c.countdown <= 0)
                    {
                        const unsigned k = mix(++c.grainNumber + seedValue + l);
                        const float length = static_cast<float>(sampleRate * 0.001 * v[grainLength]);
                        auto &grain = c.grains[c.nextGrain];
                        c.nextGrain = (c.nextGrain + 1) % static_cast<int>(c.grains.size());
                        float speed = std::exp2((2 * unit(k) - 1) * v[grainRange] / 12);
                        if (unit(k + 1) < v[shimmer] * 0.01f)
                            speed *= 2;
                        if (v[reverse] > 0)
                            speed = -speed;
                        grain = {length * std::max(0.f, speed - 1) + 64, 1 - speed, 0, 1 / length};
                        c.countdown = static_cast<int>(sampleRate / v[grainDensity]);
                    }
                    for (auto &grain : c.grains)
                        if (grain.increment > 0 && grain.phase < 1)
                        {
                            const float window = wave(grain.phase * 0.5f);
                            mist += tap(c, grain.offset) * window * window;
                            grain.offset += grain.stride;
                            grain.phase += grain.increment;
                        }
                }
                c.history[c.write] =
                    std::tanh(input + (echo + mist * v[cloud] * 0.002f) * v[feedback] * 0.01f);
                const float body = input + echo * v[delay] * 0.01f + mist * v[cloud] * 0.003f;
                left[i] = right[i] = body;
                if (++c.write == static_cast<int>(c.history.size()))
                    c.write = 0;
            }
            auto audio = juce::dsp::AudioBlock<float>(scratch).getSubBlock(0, static_cast<size_t>(len));
            c.ensemble.process(juce::dsp::ProcessContextReplacing<float>(audio));
            c.verb.processStereo(left, right, len);
            for (int ch = 0; ch < std::min(2, out.buffer->getNumChannels()); ++ch)
                out.buffer->addFrom(ch, out.startSample + base, scratch, ch, 0, len,
                                    (strips[l].enabled.load() ? v[gain] : 0.f) * 0.01f * volume *
                                        (1 + (ch ? pan[l] : -pan[l])));
        }
        cycles += len * cycleInc;
        clockTime = startTime + len / sampleRate;
    }
    for (int ch = 0; ch < std::min(2, out.buffer->getNumChannels()); ++ch)
    {
        auto *samples = out.buffer->getWritePointer(ch, out.startSample);
        for (int i = 0; i < out.numSamples; ++i)
            samples[i] = std::tanh(samples[i]);
    }
    elapsed = clockTime;
    position = cycles;
    // Publish the rendered block as one clock sample. The display can move
    // within it without locking the audio callback or predicting future notes.
    ++stampVersion;
    stampWall=juce::Time::getMillisecondCounterHiRes()*0.001;
    stampSeconds=clockTime;
    stampCycles=cycles;
    stampDuration=out.numSamples/sampleRate;
    stampRate=cycleInc*sampleRate;
    ++stampVersion;
}

TransportAudioEngine::DisplayTime TransportAudioEngine::displayTime() const noexcept
{
    if(!isPlaying() || hasPendingSeek()) return {elapsedSeconds(),playhead()};
    for(int attempt=0;attempt<3;++attempt){
        const unsigned version=stampVersion.load();
        if(version&1u)continue;
        const double wall=stampWall.load(), seconds=stampSeconds.load(), pos=stampCycles.load();
        const double duration=stampDuration.load(), rate=stampRate.load();
        if(version!=stampVersion.load() || duration<=0)continue;
        const double remaining=std::clamp(duration-(juce::Time::getMillisecondCounterHiRes()*0.001-wall),0.,duration);
        return {seconds-remaining,pos-remaining*rate};
    }
    return {elapsedSeconds(),playhead()};
}
