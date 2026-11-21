#include "RhythmScene.h"

#include <cmath>
#include <tuple>

namespace
{
const juce::Colour background { 0xff111720 };
const juce::Colour muted { 0xff8593a3 };
const juce::Colour aqua { 0xff59e0d0 };
const juce::Colour coral { 0xffffa66d };
}

void RhythmScene::setTransport(double phase, int cycle, bool active) noexcept
{
    transportPhase = phase;
    currentCycle = cycle;
    isActive = active;
    repaint();
}

void RhythmScene::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient wash(juce::Colour(0xff1d2a34), bounds.getCentreX(), bounds.getY(),
                              background, bounds.getRight(), bounds.getBottom(), true);
    g.setGradientFill(wash);
    g.fillRoundedRectangle(bounds, 22.0f);
    g.setColour(juce::Colour(0x183bc1b2));
    g.fillEllipse(bounds.withSizeKeepingCentre(bounds.getWidth() * 0.62f, bounds.getHeight() * 0.72f));

    const auto centre = bounds.getCentre();
    const auto radius = juce::jmin(bounds.getWidth() * 0.285f, bounds.getHeight() * 0.335f);
    const auto radiusA = radius * 0.69f;
    const auto radiusB = radius;
    const auto activeIndexA = static_cast<int>(std::floor(transportPhase * currentPattern.voiceA)) % currentPattern.voiceA;
    const auto activeIndexB = static_cast<int>(std::floor(transportPhase * currentPattern.voiceB)) % currentPattern.voiceB;
    const auto localA = transportPhase * currentPattern.voiceA - std::floor(transportPhase * currentPattern.voiceA);
    const auto localB = transportPhase * currentPattern.voiceB - std::floor(transportPhase * currentPattern.voiceB);
    const auto pulseA = isActive ? 1.0f - static_cast<float>(localA) : 0.0f;
    const auto pulseB = isActive ? 1.0f - static_cast<float>(localB) : 0.0f;

    for (const auto [trackRadius, colour, pulse] : { std::tuple<float, juce::Colour, float> { radiusB, coral, pulseB },
                                                     { radiusA, aqua, pulseA } })
    {
        g.setColour(colour.withAlpha(0.035f + 0.055f * pulse));
        g.fillEllipse(juce::Rectangle<float>(centre.x - trackRadius - 18.0f - pulse * 5.0f,
                                             centre.y - trackRadius - 18.0f - pulse * 5.0f,
                                             (trackRadius + 18.0f + pulse * 5.0f) * 2.0f,
                                             (trackRadius + 18.0f + pulse * 5.0f) * 2.0f));
        g.setColour(juce::Colour(0xff34414d));
        g.drawEllipse(centre.x - trackRadius, centre.y - trackRadius,
                      trackRadius * 2.0f, trackRadius * 2.0f, 1.0f);
        g.setColour(colour.withAlpha(0.12f + 0.28f * pulse));
        g.drawEllipse(centre.x - trackRadius, centre.y - trackRadius,
                      trackRadius * 2.0f, trackRadius * 2.0f, 1.5f + pulse * 1.5f);
    }

    const auto drawVoice = [&] (int count, float trackRadius, juce::Colour colour, int activeIndex)
    {
        for (int i = 0; i < count; ++i)
        {
            const auto angle = -juce::MathConstants<float>::halfPi + i * juce::MathConstants<float>::twoPi / count;
            const auto x = centre.x + std::cos(angle) * trackRadius;
            const auto y = centre.y + std::sin(angle) * trackRadius;
            const auto isCurrent = isActive && i == activeIndex;
            g.setColour(colour.withAlpha(isCurrent ? 0.19f : 0.13f));
            g.fillEllipse(x - 11.0f, y - 11.0f, 22.0f, 22.0f);
            g.setColour(colour.withAlpha(isCurrent ? 0.95f : 0.52f));
            g.fillEllipse(x - (isCurrent ? 5.2f : 3.4f), y - (isCurrent ? 5.2f : 3.4f),
                          isCurrent ? 10.4f : 6.8f, isCurrent ? 10.4f : 6.8f);
        }

        const auto headAngle = -juce::MathConstants<float>::halfPi
                             + transportPhase * juce::MathConstants<float>::twoPi;
        const auto x = centre.x + std::cos(headAngle) * trackRadius;
        const auto y = centre.y + std::sin(headAngle) * trackRadius;
        const auto glow = isActive ? 0.82f : 0.24f;
        g.setColour(colour.withAlpha(0.12f * glow));
        g.fillEllipse(x - 24.0f, y - 24.0f, 48.0f, 48.0f);
        g.setColour(colour.withAlpha(0.28f * glow));
        g.fillEllipse(x - 14.0f, y - 14.0f, 28.0f, 28.0f);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.fillEllipse(x - 6.0f, y - 6.0f, 12.0f, 12.0f);
        g.setColour(colour);
        g.drawEllipse(x - 8.0f, y - 8.0f, 16.0f, 16.0f, 2.0f);
    };

    drawVoice(currentPattern.voiceA, radiusA, aqua, activeIndexA);
    drawVoice(currentPattern.voiceB, radiusB, coral, activeIndexB);

    g.setColour(juce::Colour(0xff1a242e));
    g.fillEllipse(centre.x - 34.0f, centre.y - 34.0f, 68.0f, 68.0f);
    g.setColour(juce::Colour(0xff435260));
    g.drawEllipse(centre.x - 34.0f, centre.y - 34.0f, 68.0f, 68.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.92f));
    g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    g.drawFittedText(juce::String(currentPattern.voiceA) + ":" + juce::String(currentPattern.voiceB),
                     centre.x - 30.0f, centre.y - 14.0f, 60.0f, 28.0f,
                     juce::Justification::centred, 1);

    const auto timelineY = bounds.getBottom() - 35.0f;
    const auto timelineX = bounds.getX() + 24.0f;
    const auto timelineW = bounds.getWidth() - 48.0f;
    g.setColour(juce::Colour(0xff27333d));
    g.fillRoundedRectangle(timelineX, timelineY, timelineW, 3.0f, 2.0f);
    g.setColour(aqua.withAlpha(0.72f));
    g.fillRoundedRectangle(timelineX, timelineY, timelineW * static_cast<float>(transportPhase), 3.0f, 2.0f);
    for (int i = 0; i < currentPattern.voiceA; ++i)
    {
        const auto x = timelineX + timelineW * static_cast<float>(i) / currentPattern.voiceA;
        g.setColour(aqua.withAlpha(i == 0 ? 0.95f : 0.5f));
        g.fillEllipse(x - 3.0f, timelineY - 8.0f, 6.0f, 6.0f);
    }
    for (int i = 0; i < currentPattern.voiceB; ++i)
    {
        const auto x = timelineX + timelineW * static_cast<float>(i) / currentPattern.voiceB;
        g.setColour(coral.withAlpha(i == 0 ? 0.95f : 0.58f));
        g.fillEllipse(x - 2.5f, timelineY + 6.0f, 5.0f, 5.0f);
    }

    g.setColour(muted.withAlpha(0.9f));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("A", static_cast<int>(timelineX), static_cast<int>(timelineY - 21.0f), 14, 12, juce::Justification::centred);
    g.drawText("B", static_cast<int>(timelineX), static_cast<int>(timelineY + 14.0f), 14, 12, juce::Justification::centred);
    juce::ignoreUnused(background);
}
