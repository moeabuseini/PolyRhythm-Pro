#include "RhythmScene.h"
#include "Landscape.h"
#include <cmath>
namespace
{
constexpr float tau = juce::MathConstants<float>::twoPi;
float noise(unsigned x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    return (x >> 8) / 16777216.f;
}
void halo(juce::Graphics &g, juce::Point<float> p, float radius, juce::Colour c)
{
    g.setGradientFill({c, p, c.withAlpha(0.f), p.translated(radius, 0), true});
    g.fillEllipse(p.x - radius, p.y - radius, 2 * radius, 2 * radius);
}
} // namespace
namespace
{
float smoothNoise(float x, float y)
{
    int col = static_cast<int>(std::floor(x)), row = static_cast<int>(std::floor(y));
    float u = x - col, v = y - row;
    u = u * u * (3 - 2 * u);
    v = v * v * (3 - 2 * v);
    auto corner = [&](int dx, int dy)
    {
        return noise(static_cast<unsigned>(col + dx) * 73856093u ^
                     static_cast<unsigned>(row + dy) * 19349663u);
    };
    return juce::jmap(v, juce::jmap(u, corner(0, 0), corner(1, 0)),
                      juce::jmap(u, corner(0, 1), corner(1, 1)));
}
float mistAt(float x, float y)
{
    return smoothNoise(x, y) * 0.57f + smoothNoise(x * 2.1f + 10, y * 2.1f) * 0.28f +
           smoothNoise(x * 4.2f, y * 4.2f) * 0.15f;
}
} // namespace
void RhythmScene::setEnvironment(int index)
{
    environment = juce::jlimit(0, 2, index);
    juce::Logger::writeToLog("Environment "+juce::String(environment)+"; nodes/layer="+juce::String(grid.bodies)+"; reunion passes="+juce::String(grid.passesPerReunion)+"; equal speed="+juce::String(int(equalSpeed)));
    backdrop = {};
    repaint();
}
void RhythmScene::buildBackdrop()
{
    cachedHue = static_cast<int>(settings()[0]);
    cachedRange = motion().hueRange;
    juce::Image basePixels(juce::Image::RGB, 384, 216, false, juce::SoftwareImageType());
    juce::Image huePixels(juce::Image::RGB, 384, 216, false, juce::SoftwareImageType());
    juce::Image::BitmapData alternate(huePixels,juce::Image::BitmapData::writeOnly);
    juce::Image::BitmapData pixels(basePixels, juce::Image::BitmapData::writeOnly);
    constexpr float shifts[] = {0.08f, -0.18f, -0.03f};
    const float hue = settings()[0] / 360.f + shifts[environment] + 1;
    for (int y = 0; y < pixels.height; ++y)
        for (int x = 0; x < pixels.width; ++x)
        {
            float u = static_cast<float>(x) / pixels.width, v = static_cast<float>(y) / pixels.height;
            const float dust = mistAt(4 * u + environment * 13.f, 4 * v);
            float ridge = 0, grain = dust;
            if (environment == 0)
            {
                ridge = std::exp(-5.f * std::abs(v - (0.83f - 0.7f * u + 0.34f * (dust - 0.5f))));
            }
            else if (environment == 1)
            {
                ridge = std::exp(-4.f * std::abs(v - (0.32f + 0.18f * std::sin(6 * u) + 0.13f * dust)));
                grain *= 0.3f + 0.7f * smoothNoise(42 * u + 1.4f * v, 1);
            }
            else
            {
                ridge = std::exp(-5.f * std::abs(v - 0.63f));
                if (v > 0.63f)
                    grain *= smoothNoise(u * 9, v * 95);
            }
            if(environment==0) {
                // Dark extinction lanes and wisps in front of a brighter tilted dust band.
                float lane=v+0.68f*u-0.79f+0.10f*(mistAt(9*u,6*v)-0.5f);
                float extinction=std::exp(-30.f*std::abs(lane))*(0.55f+0.35f*grain);
                float wisps=mistAt(25*u+3*grain,11*v);
                ridge=(ridge+0.32f*wisps*std::exp(-6.f*std::abs(lane+0.08f)))*(1.f-extinction);
            }
            const float intensity = juce::jlimit(0.f, 1.f, 0.035f + 0.15f * dust + ridge * grain * 0.86f);
            auto base=juce::Colour::fromHSV(std::fmod(hue+0.12f*dust+0.12f*ridge,1.f),0.46f+0.38f*grain,intensity,1.f);
            pixels.setPixelColour(x,y,base);
            alternate.setPixelColour(x,y,base.withRotatedHue(cachedRange/360.f));
        }
    backdrop=juce::NativeImageType{}.convert(basePixels);
    hueBackdrop=juce::NativeImageType{}.convert(huePixels);
}
void RhythmScene::paintStars(juce::Graphics &g, juce::Rectangle<float> b)
{
    const int countStars = juce::jlimit(0, maxStars, static_cast<int>(settings()[1]));
    if (countStars == 0)
        return;

    // Keep distant pinpricks in one bounded atlas. Moving the density slider
    // reveals more of the same sky instead of scattering the existing stars.
    if (cachedStars != countStars)
    {
        cachedStars = countStars;
        distantStars = juce::Image(juce::Image::ARGB, 1024, 640, true, juce::NativeImageType());
        juce::Graphics atlas(distantStars);
        for (int i = 0; i < countStars; ++i)
        {
            if (i % 12 == 0)
                continue; // foreground stars are drawn below
            float x = noise(4 * i + 1) * 1024, y = noise(4 * i + 2) * 640;
            float size = 0.8f + noise(4 * i + 3) * 1.05f;
            atlas.setColour(juce::Colour(0xffdbe9ff).withAlpha(0.22f + 0.55f * noise(4 * i + 4)));
            atlas.fillEllipse(x, y, size, size);
        }
    }
    g.setColour(juce::Colours::white);
    g.drawImage(distantStars, b, juce::RectanglePlacement::stretchToFit);
    const float depth = settings()[twinkleSetting] * 0.01f;
    for (int i = 0; i < countStars; i += 12)
    {
        const float x = noise(4 * i + 1) * b.getWidth(), y = noise(4 * i + 2) * b.getHeight();
        const float starPhase = noise(4 * i + 3) * tau;
        const float pulse = static_cast<float>(std::sin(time * (1.15 + 2.7 * noise(4 * i + 4)) + starPhase));
        const float brightness = 0.56f + 0.43f * depth * pulse;
        const float radius = (1.1f + 0.8f * noise(i + 51)) * (1.f + 0.22f * depth * pulse);
        g.setColour(juce::Colour(0xffe2efff).withAlpha(brightness));
        g.fillEllipse(x - radius, y - radius, 2 * radius, 2 * radius);
        if (i % 48 == 0 && pulse > 0 && depth > 0)
        {
            float glint = depth * pulse * pulse;
            g.setColour(juce::Colours::white.withAlpha(glint * 0.72f));
            g.drawLine(x - 4 * glint, y, x + 4 * glint, y, 0.8f);
            g.drawLine(x, y - 5 * glint, x, y + 5 * glint, 0.8f);
        }
    }
}
void RhythmScene::paintMoon(juce::Graphics& g,juce::Point<float> centre,float radius)
{
    if(!lunarSurface.isValid()) lunarSurface=juce::NativeImageType{}.convert(landscape::lunarDisc());
    g.setColour(juce::Colours::white);
    g.drawImage(lunarSurface,{centre.x-radius,centre.y-radius,radius*2,radius*2});
}
void RhythmScene::paintMountains(juce::Graphics& g,juce::Rectangle<float> b,float shore)
{
    const float h=b.getHeight(), w=b.getWidth();
    for(int layer=0;layer<int(ridges.size());++layer) {
        if(!ridges[layer].isValid()) ridges[layer]=juce::NativeImageType{}.convert(landscape::mountainRange(layer));
        float depth=(layer+1)*0.25f;
        float height=h*(0.28f-0.037f*layer);
        float bottom=shore-h*(3-layer)*0.014f;
        auto dest=juce::Rectangle<float>(-w*0.08f+cameraOffset.x*depth,bottom-height,w*1.16f,height);
        g.setColour(juce::Colours::white);
        g.drawImage(ridges[layer],dest,juce::RectanglePlacement::stretchToFit);
        {
            juce::Graphics::ScopedSaveState save(g);
            g.addTransform(juce::AffineTransform::scale(1,-0.42f).translated(0,1.42f*shore));
            g.setOpacity(0.16f+0.05f*layer); g.drawImage(ridges[layer],dest,juce::RectanglePlacement::stretchToFit);
        }
    }
}
void RhythmScene::paintAtmosphere(juce::Graphics &g, juce::Rectangle<float> b)
{
    float w = b.getWidth(), h = b.getHeight(), t = static_cast<float>(time * 0.28 * settings()[3] / 55);
    const float age = static_cast<float>(time - reunionTime);
    const bool celebrating = age >= 0 && age < 3.2f;
    const float fade = celebrating ? 1.f - age / 3.2f : 0.f;
    const float bloom = celebrating ? std::min(1.f, 0.4f + age * 7.f) * fade * fade : 0.f;
    if (celebrating)
    {
        constexpr juce::uint32 colours[]{0xffffc497, 0xff98ffde, 0xffe5d9ff};
        const auto ink = juce::Colour(colours[environment]);
        const auto centre = juce::Point<float>(w * 0.5f, h * 0.45f);
        halo(g, centre, w * 0.78f, ink.withAlpha(0.37f * bloom));
        const float radius = h * (0.04f + 0.87f * age);
        g.setColour(ink.withAlpha(0.44f * bloom));
        g.drawEllipse(centre.x-radius, centre.y-radius, 2*radius, 2*radius, 1.6f);
    }
    if (environment == 0)
    {
        auto sun = juce::Point<float>{0.72f*w+cameraOffset.x*0.24f,0.28f*h+cameraOffset.y*0.24f};
        float r = 0.15f * h;
        auto light = juce::Colour(0xffffb9a4);
        const float corona = settings()[4] * 0.01f, etching = settings()[5] * 0.01f;
        halo(g, sun, r * (1.15f + 2.f * corona), light.withAlpha(0.5f));
        for (int i = 0; i < 8; ++i)
        {
            juce::Path filament;
            for (int j = 0; j <= 120; ++j)
            {
                float a = tau * j / 120,
                      flare = 0.5f + 0.5f * std::sin(5 * a + t + i * 0.22f) * std::cos(3 * a - t * 0.6f);
                float radius = r * (1 + corona * (0.025f * i + 0.21f * flare * flare));
                auto p = sun.translated(radius * std::cos(a), radius * std::sin(a));
                if (j == 0)
                    filament.startNewSubPath(p);
                else
                    filament.lineTo(p);
            }
            g.setColour(light.withAlpha(corona * (0.23f - 0.02f * i)));
            g.strokePath(filament, juce::PathStrokeType(1));
        }
        if (celebrating) {
            halo(g, sun, r * (3.8f + age), light.withAlpha(0.88f * bloom));
            for (int i = 0; i < 7; ++i) {
                const float f = i / 6.f;
                const float lift = r * (0.36f + 0.8f * std::sin(f * 2.7f) + 0.36f * age);
                const auto start = sun.translated(-r * 0.91f, -r * 0.24f);
                const auto end = sun.translated(-r * 0.29f, -r * 0.94f);
                juce::Path loop;
                loop.startNewSubPath(start);
                loop.cubicTo(start.translated(-lift, -lift * 0.6f), end.translated(-lift, -lift), end);
                g.setColour(light.withAlpha(0.15f * bloom));
                g.strokePath(loop, juce::PathStrokeType(r * 0.077f));
                g.setColour(juce::Colour(0xffffe5af).withAlpha(0.90f * bloom));
                g.strokePath(loop, juce::PathStrokeType(1.2f));
            }
        }
        paintMoon(g,sun,r);
        {
            juce::Graphics::ScopedSaveState save(g);
            juce::Path disc;
            disc.addEllipse(sun.x - r, sun.y - r, 2 * r, 2 * r);
            g.reduceClipRegion(disc);
            for (int band = 0; band < 31; ++band)
            {
                juce::Path contour;
                for (int j = 0; j <= 50; ++j)
                {
                    float u = j / 25.f - 1, v = band / 15.f - 1;
                    float y = v + 0.19f * std::sin(u * 3.5f + v * 2 + t * 0.19f) +
                              0.09f * std::sin(7 * u - 3 * v) + 0.26f * u;
                    auto p = sun.translated(u * r, y * r);
                    if (!j)
                        contour.startNewSubPath(p);
                    else
                        contour.lineTo(p);
                }
                g.setColour(light.withAlpha(etching * (0.055f + 0.04f * std::cos(band * 0.32f))));
                g.strokePath(contour, juce::PathStrokeType(band % 5 == 0 ? 1.7f : 0.75f));
            }
        }
        for (int i = 0; i < 3; ++i)
        {
            float angle = t * 0.35f + i * 2.3f;
            auto sat = sun.translated(r * 1.58f * std::cos(angle),
                                      r * (0.44f * std::sin(angle) - 0.36f * std::cos(angle)));
            halo(g, sat, 6, light.withAlpha(0.4f));
            g.setColour(light);
            g.fillEllipse(sat.x - 1.6f, sat.y - 1.6f, 3.2f, 3.2f);
        }
        juce::Path rim;
        rim.addCentredArc(sun.x, sun.y, r, r, 0, -2.2f + t * 0.04f, 0.28f + t * 0.04f, true);
        g.setColour(light);
        g.strokePath(rim, juce::PathStrokeType(1.7f));
    }
    else if (environment == 1)
    {
        juce::Graphics::ScopedSaveState saved(g);
        g.addTransform(juce::AffineTransform::scale(1.08f,1.f).translated(-w*0.04f+cameraOffset.x*0.35f,cameraOffset.y*0.35f));
        // Every silk strand samples the same horizontal wave with a different
        // phase. Factor sin(a+b) so the expensive oscillators run per column.
        struct Column { float x, sine, cosine, fold, base, flareSin, flareCos; };
        std::array<Column,71> columns{};
        const float lift = 0.04f + 0.002f * settings()[4];
        for (int i=0;i<=70;++i) {
            const float u=i/70.f, a=u*(3+0.06f*settings()[5])+t*0.7f;
            columns[i]={w*u, lift*std::sin(a), lift*std::cos(a),
                        0.08f+0.10f*std::cos(u*10-t), 0.26f+0.04f*std::sin(u*12+t),
                        celebrating ? std::sin(9*u-4*age) : 0.f, celebrating ? std::cos(9*u-4*age) : 0.f};
        }
        for (int strand = 0; strand < 32; ++strand)
        {
            float v = strand / 31.f;
            juce::Path p;
            p.preallocateSpace(3*71);
            const float cs=std::cos(v*0.8f), sn=std::sin(v*0.8f);
            const float flareCs=celebrating ? std::cos(v*2) : 0.f, flareSn=celebrating ? std::sin(v*2) : 0.f;
            const float shuttleColumn=70*std::fmod(0.5f+v+t*0.08f,1.f);
            const int leftColumn=juce::jmin(69,static_cast<int>(shuttleColumn));
            juce::Point<float> shuttle;
            float previousY=0;
            for (int i = 0; i <= 70; ++i)
            {
                const auto& col=columns[i];
                float y=h*(col.base+col.sine*cs+col.cosine*sn+v*col.fold);
                if (celebrating) {
                    const float crest = col.flareSin*flareCs+col.flareCos*flareSn;
                    y += h * bloom * (0.10f * crest + 0.09f * (v - 0.5f));
                    if (strand == 0) {
                        const float height = h * (0.08f + 0.07f * (1 + crest));
                        g.setGradientFill({juce::Colour(0xffdaffae).withAlpha(bloom * 0.40f), col.x, y,
                                           juce::Colour(0x0095f2e9), col.x, y-height, false});
                        g.fillRect(col.x, y-height, w/70.f+1.f, height);
                    }
                }
                if (i == 0)
                    p.startNewSubPath(0, y);
                else
                    p.lineTo(col.x, y);
                if(i==leftColumn+1)
                    shuttle={w*shuttleColumn/70.f,previousY+(shuttleColumn-leftColumn)*(y-previousY)};
                previousY=y;
            }
            g.setColour(juce::Colour(0xff69ffd2)
                            .interpolatedWith(juce::Colour(0xffa39cff), v)
                            .withAlpha(0.055f + 0.14f * v * v + 0.35f * bloom));
            g.strokePath(p, juce::PathStrokeType(1 + 2 * v));
            if (strand % 3 == 0)
            {
                halo(g, shuttle, 8, juce::Colour(0x997bf8e7));
                g.setColour(juce::Colour(0xffc9fff3));
                g.fillEllipse(shuttle.x - 2, shuttle.y - 1, 4, 2);
            }
        }
    }
    else
    {
        float horizon=h*0.66f+cameraOffset.y*0.55f,x=w*0.29f+cameraOffset.x*0.22f,r=h*0.069f;
        juce::Colour moon(0xffffdae0);
        halo(g, {x, horizon - 5.8f * r}, r * 4, moon.withAlpha(0.45f));
        paintMoon(g,{x,horizon-5.8f*r},r);
        if (celebrating)
            halo(g, {x,horizon-5.8f*r}, r * 4.8f, juce::Colour(0xfffff2da).withAlpha(0.85f * bloom));
        g.setGradientFill({juce::Colour(0xff46445d),0,horizon,juce::Colour(0xff0e1a30),0,h,false});
        g.fillRect(juce::Rectangle<float>(0,horizon,w,h-horizon));
        paintMountains(g,b,horizon);
        if (celebrating) {
            // The flock lifts in a staggered sweep, not a repeating idle animation.
            for (int i = 0; i < 13; ++i) {
                const float flight = age - 0.38f * noise(19+i);
                if (flight <= 0) continue;
                const float xBird = w * (0.33f + 0.19f*noise(41+i) + flight*(0.07f+0.025f*noise(87+i)));
                const float yBird = horizon - h * (0.11f + 0.075f*noise(52+i) + 0.14f*flight);
                const float span = h * (0.008f + 0.006f*noise(61+i));
                const float wing = span * std::sin(10*flight+i);
                juce::Path wings;
                wings.startNewSubPath(xBird-span,yBird-wing);
                wings.quadraticTo(xBird-span*0.4f,yBird-wing*0.8f,xBird,yBird);
                wings.quadraticTo(xBird+span*0.4f,yBird-wing*0.8f,xBird+span,yBird-wing);
                const float visibility = std::min(1.f,flight*8) * fade;
                g.setColour(moon.withAlpha(0.62f*visibility));
                g.strokePath(wings,juce::PathStrokeType(2.6f));
                g.setColour(juce::Colour(0xff171829).withAlpha(visibility));
                g.strokePath(wings,juce::PathStrokeType(1.5f));
            }
        }
        for (int crest = 0; crest < 14; ++crest)
        {
            float depth = (crest + 1) / 15.f;
            juce::Path breaker;
            for (int j = 0; j <= 80; ++j)
            {
                float u = j / 80.f;
                float y = horizon + (h - horizon) * depth +
                          h * depth * settings()[4] * 0.00035f * std::sin(13 * u + depth * 8 - t * 1.6f);
                if (!j)
                    breaker.startNewSubPath(0, y);
                else
                    breaker.lineTo(u * w, y);
            }
            g.setColour(moon.withAlpha(0.05f + 0.08f * depth));
            g.strokePath(breaker, juce::PathStrokeType(0.8f));
        }
        // Short moonlit wavelets pack tightly at the horizon and widen toward the observer.
        for(int i=0;i<590;++i) {
            float distance=noise(i+852); distance*=distance;
            float extent=w*(0.016f+0.13f*distance);
            float dx=(noise(i+1234)+noise(i+4321)-1.f)*extent;
            float sway=std::sin(t*(0.7f+distance)+i);
            float px=x+dx+sway*extent*0.12f, py=horizon+(h-horizon)*distance;
            float shine=(1-std::abs(dx)/extent)*(0.5f+0.5f*sway);
            g.setColour(moon.withAlpha(shine*(settings()[5]/100.f)*(0.68f+0.30f*bloom)));
            g.fillRoundedRectangle(px,py,0.85f+distance*(1+6*shine),0.48f+distance*0.9f,0.5f);
        }
        for (int i = 0; i < 125; ++i)
        {
            float v = noise(5 * i + 13), u = noise(5 * i + 12), length = w * (0.005f + 0.05f * v);
            float xx = w*u+length*std::sin(t+i)+cameraOffset.x*v, yy = horizon + (h - horizon) * v;
            float reflection = std::exp(-10.f * std::abs((xx-x)/w));
            g.setColour(moon.withAlpha(0.04f + settings()[5] * 0.007f * reflection));
            g.drawLine(xx, yy, xx + length, yy, 0.7f + v);
        }
    }
}
void RhythmScene::update(double elapsed, double position)
{
    const double dt=std::clamp(elapsed-time,0.,0.1);
    constexpr double periods[]{120,48,16};
    if(motion().hueCycle) hueClock+=dt/periods[std::clamp(motion().hueSpeed,0,2)];
    hueBlend=motion().hueCycle ? static_cast<float>(0.5-0.5*std::cos(hueClock*juce::MathConstants<double>::twoPi)) : 0.f;
    cameraClock+=dt*0.17;
    pointerSmooth+=(pointerTarget-pointerSmooth)*0.12f;
    auto direction=motion().camera==2 ? pointerSmooth : juce::Point<float>(static_cast<float>(std::sin(cameraClock)),static_cast<float>(std::sin(cameraClock*0.71)));
    cameraOffset=motion().camera==0 ? juce::Point<float>() : direction*(motion().depth*0.01f)*juce::Point<float>(getWidth()*0.045f,getHeight()*0.024f);
    time = elapsed;
    phase = position;
    repaint();
}
void RhythmScene::mouseMove(const juce::MouseEvent& e)
{
    pointerTarget={2.f*e.position.x/std::max(1,getWidth())-1.f,2.f*e.position.y/std::max(1,getHeight())-1.f};
}
void RhythmScene::strike(studio::Hit h)
{
    laneHit[h.layer]=h.time;
    if (h.fullReunion) reunionTime = h.time;
    history[cursor] = h;
    cursor = (cursor + 1) % static_cast<int>(history.size());
    count = std::min(count + 1, static_cast<int>(history.size()));
}
juce::Point<float> RhythmScene::point(double cycles, int layer, int strand) const
{
    const auto flight = grid.flight(cycles, layer, strand);
    return pathPoint(travelProgress(float(flight.phase),strand), layer, strand, flightRadius(flight.cycles));
}
float RhythmScene::flightRadius(double duration) const
{
    if (!equalSpeed) return 0;
    // A guaranteed phrase attack bounds every flight. Use the same scale
    // for every node: circumference is proportional to time until impact.
    int slowest = *std::max_element(grid.divisions.begin(), grid.divisions.end());
    for (int i=0;i<studio::layerCount;++i)
        if (grid.probability[i]>0) slowest = std::min(slowest,grid.divisions[i]);
    return float(duration * slowest / (2.0 * grid.phraseLength()))
           * (orbitScale*0.0022f) * float(std::min(getWidth(),getHeight()));
}
float RhythmScene::travelProgress(float phaseOnPath,int strand) const
{
    if (equalSpeed) return phaseOnPath;
    // A monotone time warp staggers travel without shifting either audible endpoint.
    const float laneOffset = grid.bodies > 1 ? 2.f*strand/(grid.bodies-1)-1.f : 0.f;
    const float u = phaseOnPath + timingOffset*0.009f*laneOffset*phaseOnPath*(1-phaseOnPath);
    return u;
}
juce::Point<float> RhythmScene::pathPoint(float progress, int layer, int body, float radius) const
{
    const auto b = getLocalBounds().toFloat();
    const float w = b.getWidth(), h = b.getHeight();
    const auto focus = juce::Point<float>(w * 0.5f, h * 0.45f);
    int slot=layer, slots=studio::layerCount;
    if (autoBalance) {
        slot=0; slots=0;
        for(int i=0;i<studio::layerCount;++i) if(grid.probability[i]>0) {
            ++slots;
            if(i<layer) ++slot;
        }
    }
    const int rank = body * slots + slot;
    const float spreadAmount = fanSpread/100.f;
    const float index = 0.5f + ((rank + 0.5f)/(std::max(1,slots)*grid.bodies)-0.5f)*spreadAmount;
    const float lineIndex = juce::jlimit(0.f,1.f,index+linePlacement[layer]*0.005f);
    // Centre the active fan on the vertical axis so the point target can
    // occupy both halves of the stage even when only two nodes are playing.
    const float angleOffset = pointPlacement[layer]*0.005f;
    const float pointIndex = equalSpeed && pointTarget ? 0.5f + ((rank+0.5f)/(std::max(1,slots)*grid.bodies)-0.5f)*pointCoverage/100.f : index;
    const float bearing = tau*(pointIndex+angleOffset+(autoBalance ? 0.25f : 0.f))
                        + (autoBalance ? 0.f : 0.3f*environment);
    const float side = rank % 2 ? -1.f : 1.f;
    if (equalSpeed) {
        const float angle = tau * progress;
        const float direction = pointTarget ? bearing : (side < 0 ? tau*0.5f : 0.f);
        const auto origin = pointTarget ? focus : juce::Point<float>{focus.x,h*(0.16f+lineIndex*0.58f)};
        const float along = radius*(1.f-std::cos(angle));
        const float across = radius*std::sin(angle);
        // Keep line-mode orbits on one side: only the endpoints touch the gate.
        return origin.translated(along*std::cos(direction)-across*std::sin(direction),
                                 along*std::sin(direction)+across*std::cos(direction));
    }
    // Keep 72% of the linear phase speed at each strike, with a softer mid-flight arc.
    const float eased = progress + 0.28f * progress * (1.f - progress) * (2.f * progress - 1.f);
    const float arc = juce::MathConstants<float>::pi * eased;
    const float petal = std::sin(arc);
    const float curl = std::sin(2 * arc);
    if (pointTarget)
    {
        // Each closed path passes through the same eye, with two axes that
        // both vanish at the endpoints. No phase offset or free-running drift.
        float petalRadius = petal * (0.70f + spreadAmount*0.26f * std::sin(1.3f * rank));
        const float mirror = autoBalance && index < 0.5f ? -1.f : 1.f;
        float twist = mirror * (0.35f + 0.24f * environment) * curl;
        return focus.translated(w * 0.45f * petalRadius * std::cos(bearing + twist),
                                h * 0.38f * petalRadius * std::sin(bearing + twist));
    }
    const float spread = w * (0.29f + 0.13f * spreadAmount * std::sin(tau*index));
    const float lift = environment == 1 ? -0.11f : environment == 2 ? 0.09f : 0.f;
    return {focus.x + side * spread * petal, h * (0.16f + lineIndex * 0.58f + lift * petal + 0.075f * curl)};
}
void RhythmScene::paintTarget(juce::Graphics &g, juce::Rectangle<float> b) const
{
    auto eye = juce::Point<float>(b.getWidth() * 0.5f, b.getHeight() * 0.45f);
    const juce::Colour opal(0xffd6fff4);
    if (!pointTarget)
    {
        float top = b.getHeight() * 0.10f, bottom = b.getHeight() * 0.80f;
        for (float width : {14.f, 5.f, 1.2f})
        {
            g.setColour(opal.withAlpha(width > 6 ? 0.045f : width > 2 ? 0.16f : 0.88f));
            g.drawLine(eye.x, top, eye.x, bottom, width);
        }
        for (int i = 0; i < 17; ++i)
        {
            float y = top + (bottom - top) * i / 16;
            g.drawLine(eye.x - 3, y, eye.x + 3, y, 1);
        }
    }
    else
    {
        halo(g, eye, 28, opal.withAlpha(0.48f));
        for (float r : {8.f, 14.f})
        {
            g.setColour(opal.withAlpha(0.8f));
            g.drawEllipse(eye.x - r, eye.y - r, 2 * r, 2 * r, 1.1f);
        }
        g.drawLine(eye.x - 22, eye.y, eye.x + 22, eye.y, 1);
        g.drawLine(eye.x, eye.y - 22, eye.x, eye.y + 22, 1);
    }
}
void RhythmScene::paintSky(juce::Graphics& g,juce::Rectangle<float> b)
{
    const float drift = static_cast<float>(time * 0.007 * settings()[3] / 55);
    auto tint = juce::Colour::fromHSV(std::fmod(settings()[0] / 360 + hueBlend*motion().hueRange/360.f, 1.f),
                                      0.75f, 0.65f, 1.f);
    if (!backdrop.isValid() || cachedHue != static_cast<int>(settings()[0]) || cachedRange!=motion().hueRange)
        buildBackdrop();
    auto sky=b.expanded(b.getWidth()*0.016f,b.getHeight()*0.012f).translated(cameraOffset.x*0.2f,cameraOffset.y*0.2f);
    g.setOpacity(1.f); g.drawImage(backdrop,sky,juce::RectanglePlacement::stretchToFit);
    if(hueBlend>0) {
        g.setOpacity(hueBlend); g.drawImage(hueBackdrop,sky,juce::RectanglePlacement::stretchToFit); g.setOpacity(1.f);
    }
    for (int i = 0; i < 5; ++i)
    {
        juce::Point<float> p{b.getWidth() * (0.12f + i * 0.19f + 0.08f * std::sin(drift + i)),
                             b.getHeight() * (0.5f + 0.34f * std::cos(drift + i))};
        halo(g, p, b.getHeight() * 0.78f, tint.withRotatedHue(0.055f * i).withAlpha(0.31f));
    }
    {
        juce::Graphics::ScopedSaveState save(g);
        g.addTransform(juce::AffineTransform::scale(1.02f,1.02f).translated(-0.01f*b.getWidth()+cameraOffset.x*0.10f,-0.01f*b.getHeight()+cameraOffset.y*0.10f));
        paintStars(g,b);
    }
    paintAtmosphere(g, b);
    // Pearlescent ribbons hover behind the instrument, separate from note trails.
    for (int band = 0; band < 5; ++band)
    {
        juce::Path veil;
        const float flow = static_cast<float>(time * 0.09 * settings()[3] / 55);
        for (int j = 0; j <= 80; ++j)
        {
            float u = j / 80.f;
            float crest = 0.26f * std::sin(3.5f * u + flow + environment) + 0.04f * std::sin(11 * u - flow);
            float y = b.getHeight() * (0.35f + crest + band * 0.014f);
            if (j == 0)
                veil.startNewSubPath(0, y);
            else
                veil.lineTo(b.getWidth() * u, y);
        }
        g.setGradientFill(
            {juce::Colour(0x057affdf), 0.f, 0.f, juce::Colour(0x32d4bcff), b.getWidth(), 0.f, false});
        g.strokePath(veil, juce::PathStrokeType(1.4f + band * 0.75f));
    }
}
void RhythmScene::paint(juce::Graphics &g)
{
    auto b = getLocalBounds().toFloat();
    int mask=0;
    if(autoBalance) for(int i=0;i<studio::layerCount;++i)
        if(grid.probability[i]>0) mask|=1<<i;
    const std::array<int,6> geometry{getWidth(),getHeight(),grid.bodies,environment,int(pointTarget),mask};
    const auto& placement = pointTarget ? pointPlacement : linePlacement;
    if(geometry!=orbitKey || orbitSpread!=fanSpread || placement!=orbitPlacement){
        orbitKey=geometry;orbitSpread=fanSpread;orbitPlacement=placement;
        for(auto& path:orbitInk)path.clear();
    }

    // The slow-moving sky need not be rebuilt on every 5 ms node update.
    // Composite a reusable native surface; note trails, hits and the highway
    // still draw at full display resolution on every refresh.
    if(!cacheSky)paintSky(g,b);
    else {
    const float scale=std::min(1.f,1600.f/std::max(1.f,b.getWidth()));
    const int sw=juce::jmax(1,juce::roundToInt(b.getWidth()*scale));
    const int sh=juce::jmax(1,juce::roundToInt(b.getHeight()*scale));
    if(skyFrame.getWidth()!=sw || skyFrame.getHeight()!=sh)
        skyFrame=juce::Image(juce::Image::RGB,sw,sh,false,juce::NativeImageType());
    const double nowMs=juce::Time::getMillisecondCounterHiRes();
    if(nowMs>=nextSkyMs || skyEnvironment!=environment || skyReunion!=reunionTime || skyTime>time || skySize!=getLocalBounds()){
        juce::Graphics sky(skyFrame);
        sky.addTransform(juce::AffineTransform::scale(sw/b.getWidth(),sh/b.getHeight()));
        paintSky(sky,b);
        nextSkyMs=nowMs+30.;skyEnvironment=environment;skyReunion=reunionTime;skyTime=time;skySize=getLocalBounds();
    }
    g.setColour(juce::Colours::white);g.drawImage(skyFrame,b);
    }
    paintTarget(g, b);
    const int active = static_cast<int>(std::count_if(grid.probability.begin(), grid.probability.end(),
                                                      [](float value) { return value > 0; }));
    if (active > 0 && !cleanStage)
    {
        const double recentWindow=phase-0.55/pattern.cycleSeconds();
        const bool justAligned=recentWindow<0 || grid.nextMeeting(recentWindow,true)<=phase;
        const double until = (grid.nextMeeting(phase, true) - phase) * pattern.cycleSeconds();
        const auto label = justAligned ? juce::String(active * grid.bodies) + " points aligned"
                                        : "All points in " + juce::String(until, 1) + " s";
        auto badge = juce::Rectangle<float>(12, 10, 160, 23);
        g.setColour(juce::Colour(0xb30a1020));
        g.fillRoundedRectangle(badge, 11);
        g.setColour(juce::Colour(0xffdaf4ee));
        g.setFont(juce::FontOptions(12.f));
        g.drawText(label, badge, juce::Justification::centred);
    }
    for (int l = 0; l < studio::layerCount; ++l)
    {
        auto colour = juce::Colour(studio::colours[l]);
        if (grid.probability[l] <= 0)
            continue;
        for (int s = 0; s < grid.bodies; ++s)
        {
            auto& outline=orbitInk[l*studio::BeatGrid::maxBodies+s];
            if(!equalSpeed && outline.isEmpty()){
                juce::Path orbit;
                orbit.startNewSubPath(pathPoint(0, l, s));
                for(int j=1;j<=48;++j)orbit.lineTo(pathPoint(j/48.f,l,s));
                juce::PathStrokeType(0.75f).createStrokedPath(outline,orbit);
            }
            g.setColour(colour.withAlpha(0.13f));
            if(!equalSpeed) g.fillPath(outline);
            // Sample only the current trip: never bridge across an attack.
            const auto flight = grid.flight(phase,l,s);
            const float radius = flightRadius(flight.cycles);
            const float u = travelProgress(float(flight.phase),s), tail=std::max(0.f,u-0.18f);
            auto curve=[&](float t){return pathPoint(t,l,s,radius);};
            const auto p=curve(u);
            if(equalSpeed && radius > 1) {
                const auto centre = (curve(0.5f)+curve(0))*0.5f;
                g.drawEllipse(centre.x-radius,centre.y-radius,2*radius,2*radius,0.75f);
            }
            auto tangent=[&](float t){
                float a=std::max(0.f,t-0.001f), b=std::min(1.f,t+0.001f);
                return (curve(b)-curve(a))/(b-a);
            };
            juce::Path trail;
            trail.startNewSubPath(curve(tail));
            const float delta=(u-tail)/4.f;
            for(int j=0;j<4;++j) {
                float a=tail+j*delta,end=a+delta;
                trail.cubicTo(curve(a)+tangent(a)*(delta/3),curve(end)-tangent(end)*(delta/3),curve(end));
            }
            g.setColour(colour.withAlpha(0.60f));
            g.strokePath(trail,juce::PathStrokeType(1.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
            float r = (l == 1 ? 5.5f : 2.5f) + 1.7f*(grid.bodies-s)/grid.bodies;
            halo(g, p, r * 3, colour.withAlpha(0.38f));
            g.setColour(colour.withAlpha(0.84f));
            g.drawEllipse(p.x - r, p.y - r, r * 2, r * 2, 1);
        }
    }
    const float sparks = settings()[2] * 0.01f;
    const float canvasScale = juce::jlimit(0.75f, 1.6f, std::min(b.getWidth(), b.getHeight()) / 440.f);
    const float reach = canvasScale * (1.f + 4.f * sparks * sparks * sparks);
    for (int i = 0; i < count; ++i)
    {
        auto &h = history[i];
        const float age = static_cast<float>(time - h.time);
        if (h.body >= grid.bodies || grid.probability[h.layer] <= 0 || age < 0 || age >= 1.25f)
            continue;
        auto p = pathPoint(0.f, h.layer, h.body);
        auto c = juce::Colour(studio::colours[h.layer]);
        float life = 1 - age / 1.25f;
        // Link neighbouring attacks in the exact same sample, including individual chord tones.
        for (int j = i - 1; j >= 0; --j)
        {
            const auto &other = history[j];
            if (other.body >= grid.bodies || other.time != h.time || grid.probability[other.layer] <= 0)
                continue;
            auto q = point(phase, other.layer, other.body);
            auto end = point(phase, h.layer, h.body);
            juce::Path thread;
            thread.startNewSubPath(q);
            auto mid = (end + q) * 0.5f;
            thread.quadraticTo(mid.translated(0, -0.08f * end.getDistanceFrom(q)), end);
            g.setColour(c.withAlpha(0.18f * life * life));
            g.strokePath(thread, juce::PathStrokeType(5 * life + 1));
            g.setColour(juce::Colours::ivory.withAlpha(0.85f * life * life));
            g.strokePath(thread, juce::PathStrokeType(1.1f));
            break;
        }
        float r = (h.layer == 1 ? 10.f : 5.f) + age * 27;
        halo(g, p, r * 2, c.withAlpha(life * 0.45f));
        g.setColour(c.withAlpha(life * life));
        g.drawEllipse(p.x - r, p.y - r, 2 * r, 2 * r, 1.4f);
        for (int k = 0; k < juce::roundToInt(sparks * 18.f); ++k)
        {
            unsigned key = h.note * 57 + k * 3 + h.body * 101 + h.layer * 79;
            const float a = noise(key) * tau;
            const float speed = (18 + noise(key + 1) * 66) * h.velocity * reach;
            const juce::Point<float> velocity(std::cos(a) * speed, std::sin(a) * speed);
            auto q = (p + velocity * age).translated(0.f, 12.f * age * age);
            float rr = life * (1 + noise(key + 2) * 2);
            g.setColour(c.interpolatedWith(juce::Colours::white, life * 0.8f).withAlpha(life));
            g.drawLine(juce::Line<float>(q - velocity * std::min(age, 0.018f * sparks), q), 0.8f * life);
            g.fillEllipse(q.x - rr, q.y - rr, rr * 2, rr * 2);
        }
    }
    if (showForecast) paintForecast(g, b);
    if(showFps && !cleanStage){
        double now=juce::Time::getMillisecondCounterHiRes();
        if(fpsStart==0)fpsStart=now;
        ++fpsFrames;
        if(now-fpsStart>=1000){
            measuredFps=juce::roundToInt(1000*fpsFrames/(now-fpsStart));fpsStart=now;fpsFrames=0;
        }
        auto badge=b.removeFromTop(32).removeFromRight(104).reduced(3);
        g.setColour(juce::Colour(0xcf0b1622));g.fillRoundedRectangle(badge,9);
        g.setColour(juce::Colours::white);g.setFont(juce::FontOptions(14.f));
        g.drawText(juce::String(measuredFps)+" stage FPS",badge,juce::Justification::centred);
    } else {fpsStart=0;fpsFrames=0;}
    for(const auto& [caption, opacity]:captions){
        const int width = juce::roundToInt(b.getWidth()*0.78f);
        auto box = juce::Rectangle<int>(width, 84).withCentre(
            {juce::roundToInt(b.getCentreX()), juce::roundToInt(b.getHeight()*0.10f+12-5*opacity)});
        g.setFont(juce::FontOptions("Georgia",juce::jlimit(20.f,42.f,b.getHeight()*0.035f),juce::Font::plain));
        g.setColour(juce::Colours::black.withAlpha(0.46f*opacity));
        g.drawFittedText(caption,box.translated(0,2),juce::Justification::centred,2);
        g.setColour(juce::Colours::ivory.withAlpha(opacity));
        g.drawFittedText(caption,box,juce::Justification::centred,2);
    }
}

void RhythmScene::paintForecast(juce::Graphics &g, juce::Rectangle<float> canvas) const
{
    const auto rail=canvas.reduced(16,10).removeFromBottom(std::min(trackHeight,canvas.getHeight()*0.38f));
    g.setColour(juce::Colour(0xe00d1421));g.fillRoundedRectangle(rail,12);
    const float gate=rail.getX()+112,span=rail.getRight()-gate-12,mid=rail.getCentreY();
    auto yAt=[&](int lane,float u){return mid+(lane-2.f)*rail.getHeight()*0.18f*(1-0.46f*u);};
    for(int lane=0;lane<studio::layerCount;++lane){
        const auto ink=juce::Colour(studio::colours[lane]);
        const float y=yAt(lane,0);
        const float flash=grid.probability[lane]>0 ? std::clamp(float(1-(time-laneHit[lane])/0.13),0.f,1.f) : 0.f;
        g.setColour(ink.withAlpha(0.12f+0.65f*flash));
        g.drawLine(gate,y,gate+span,yAt(lane,1),1.5f);
        const float radius=std::clamp(rail.getHeight()*0.055f,2.8f,8.f);
        if(flash>0) halo(g,{gate,y},radius*3,ink.withAlpha(flash));
        g.setColour(ink.interpolatedWith(juce::Colours::white,flash));
        g.drawEllipse(gate-radius,y-radius,2*radius,2*radius,1.4f);
        g.setFont(juce::FontOptions(std::clamp(rail.getHeight()*0.15f,8.f,15.f),flash>0.2f?juce::Font::bold:juce::Font::plain));
        g.drawFittedText(lane==3 && rail.getHeight()<62 ? "Atmo" : studio::layers[lane],
                         juce::Rectangle<float>(rail.getX()+8,y-12,91,24).toNearestInt(),
                         juce::Justification::centredLeft,1);
        const double density=grid.divisions[lane]*0.5;
        for(auto step=int64_t(std::ceil(phase*density-1.e-9));step<=int64_t((phase+1)*density);++step){
            if(!grid.sounds(lane,step))continue;
            const float u=std::clamp(float(step/density-phase),0.f,1.f);
            const float r=radius*(1-0.5f*u),x=gate+span*u,cy=yAt(lane,u);
            g.setColour(ink.withAlpha(1-0.50f*u));g.fillEllipse(x-r,cy-r,2*r,2*r);
            g.setColour(juce::Colours::white.withAlpha(0.8f));g.fillEllipse(x-r*0.42f,cy-r*0.62f,r*0.84f,r*0.52f);
        }
    }
}
