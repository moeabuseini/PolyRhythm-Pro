#include "RhythmScene.h"
#include "../UI/GlassTheme.h"
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
// An individual feature lights up only as the reunion wave reaches it.
float passingLight(float age, float arrival, float duration)
{
    const float phase=(age-arrival)/duration;
    if(phase<=0 || phase>=1) return 0;
    const float envelope=std::sin(juce::MathConstants<float>::pi*phase);
    return envelope*envelope;
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
    environment = juce::jlimit(0, environmentCount-1, index);
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
    constexpr float shifts[] = {0.08f, -0.18f, -0.03f, -0.09f, 0.02f, -0.03f};
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
            else if(environment==3){
                ridge=(1-v)*(0.28f+0.70f*std::pow(smoothNoise(15*u+3*v,2*v),2.f));
            }
            else if(environment==4){
                ridge=0.96f*std::exp(-4.8f*std::abs(v-0.54f));
            }
            else if(environment==5){
                const float wall=std::abs(u-0.50f)*1.7f+0.21f*(dust-0.5f);
                ridge=std::min(1.f,wall*wall+0.28f*std::exp(-8.f*std::abs(v-0.73f)));
                grain=mistAt(13*u+4*dust,5*v);
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
            if(environment==3){
                const float caustic=std::exp(-12.f*std::abs(std::sin(27*u+7*mistAt(11*u,14*v))
                                              +0.66f*std::cos(31*v+11*u)));
                base=base.interpolatedWith(juce::Colour(0xff83cfc7),caustic*0.28f*std::exp(-9*v));
            }else if(environment==4){
                const float vapour=mistAt(5*u+3*dust,32*v);
                const float haze=std::exp(-6.f*std::abs(v-0.53f));
                base=base.interpolatedWith(juce::Colour(0xffe7ad87),haze*(0.20f+0.51f*vapour));
            }else if(environment==5){
                const float edge=std::pow(std::abs(2*u-1),1.7f);
                const float folds=mistAt(17*u+4*dust,4*v+8*(u-0.5f)*(u-0.5f));
                base=base.interpolatedWith(juce::Colour(0xff5cbace),edge*folds*0.64f);
            }
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
void RhythmScene::paintLanternDeep(juce::Graphics& g,float w,float h,float t,float bloom)
{
    const float light=settings()[5]*0.01f;
    const float age=float(time-reunionTime);
    // A feathered, diverging shaft. Its light is absorbed with depth, not cut off at a polygon edge.
    static const auto shaft=[]{
        juce::Image mask(juce::Image::ARGB,72,96,true,juce::SoftwareImageType());
        juce::Image::BitmapData pixels(mask,juce::Image::BitmapData::writeOnly);
        for(int y=0;y<96;++y) for(int x=0;x<72;++x){
            const float v=y/95.f, side=(x/71.f-0.5f)/(0.09f+v*0.30f);
            const float density=std::exp(-3.8f*side*side)*(1-v)*(1-v);
            pixels.setPixelColour(x,y,juce::Colour(0xff9ee7df).withAlpha(density));
        }
        return mask;
    }();
    for(int i=0;i<9;++i){
        const float x=w*(0.15f+0.69f*noise(i+217)+0.016f*std::sin(t*0.41f+i));
        const float width=w*(0.13f+0.14f*noise(i+71));
        const float bend=(x-w*0.66f)*0.78f+w*0.036f*std::sin(t*0.28f+i);
        g.setOpacity(light*(0.14f+0.19f*noise(i+22)));
        g.drawImageTransformed(shaft,juce::AffineTransform(width/72.f,bend/96.f,x-width*0.5f,
                                                           0,h/96.f,-h*0.015f));
    }
    g.setOpacity(1.f);
    // Successive reef shelves silhouette the lower water column at different depths.
    for(int shelf=0;shelf<3;++shelf){
        const float depth=(shelf+1)/3.f;
        juce::Path reef;reef.startNewSubPath(-2,h);
        for(int j=0;j<=84;++j){
            const float u=j/84.f;
            const float stone=mistAt(8*u+shelf-cameraOffset.x*depth/w*8,1.6f*shelf);
            const float y=h*(0.69f+0.11f*shelf+0.12f*std::sin(3.1f*u)+0.12f*(stone-0.5f));
            reef.lineTo(u*w,y+cameraOffset.y*depth);
        }
        reef.lineTo(w+2,h);reef.closeSubPath();
        g.setGradientFill({juce::Colour(0xff226070).darker(depth),0,h*0.65f,juce::Colour(0xff071625),0,h,false});
        g.fillPath(reef);
    }
    for(int i=0;i<115;++i){
        const float depth=noise(i+349), u=std::fmod(noise(i+249)+t*(0.003f+0.008f*depth),1.f);
        const float y=h*(noise(i+570)+0.009f*std::sin(t+i))+cameraOffset.y*depth;
        const float wave=passingLight(age,u*1.75f,0.94f);
        const float r=0.45f+1.6f*depth+1.6f*wave;
        g.setColour(juce::Colour(0xff8eeed8).withAlpha(0.05f+0.15f*depth+0.58f*wave));
        g.fillEllipse(u*w,y,r,r);
    }
    const int count=juce::roundToInt(settings()[4]*0.14f);
    for(int i=0;i<count;++i){
        const float depth=0.3f+0.7f*noise(i+427);
        const float cycle=t*(0.7f+depth*0.5f)+i*2.1f;
        const float pulse=std::sin(cycle);
        const float x=w*(0.05f+0.9f*noise(i+71)+0.024f*std::sin(t*0.2f+i))+cameraOffset.x*depth;
        const float rise=std::fmod(noise(i+23)+t*(0.009f+depth*0.005f),1.f);
        const float y=h*(1.20f-1.45f*rise)+cameraOffset.y*depth;
        const float r=h*(0.013f+0.035f*depth)*(1+0.09f*pulse);
        const float flash=passingLight(age,1.75f*x/w,1.10f);
        const float crown=r*(0.96f-0.16f*pulse);
        const auto ink=juce::Colour::fromHSV(0.45f+0.28f*noise(i+633),0.40f,1.f,1.f);
        halo(g,{x,y},r*(2.2f+bloom),ink.withAlpha(0.12f+0.43f*flash));
        for(int j=-2;j<=2;++j){
            const float root=x+r*j*0.31f, sway=r*0.38f*std::sin(cycle+j);
            juce::Path filament;filament.startNewSubPath(root,y);
            filament.cubicTo(root+sway,y+r,root-sway,y+r*1.65f,root+sway*0.6f,y+r*(2.8f-0.25f*std::abs(j)));
            g.setColour(ink.withAlpha(0.21f+0.25f*bloom));g.strokePath(filament,juce::PathStrokeType(0.7f+depth));
        }
        juce::Path bell;bell.startNewSubPath(x-r,y);
        bell.cubicTo(x-r,y-crown*1.45f,x+r,y-crown*1.45f,x+r,y);
        bell.quadraticTo(x,y+r*0.42f,x-r,y);bell.closeSubPath();
        g.setGradientFill({ink.withAlpha(0.48f+0.46f*flash),x,y-crown,ink.withAlpha(0.09f),x,y,false});
        g.fillPath(bell);
        for(int rib=-2;rib<=2;++rib){
            juce::Path seam;seam.startNewSubPath(x,y-crown);
            seam.quadraticTo(x+rib*r*0.48f,y-crown*0.61f,x+rib*r*0.36f,y+r*0.06f);
            g.setColour(ink.withAlpha(0.18f+0.4f*flash));g.strokePath(seam,juce::PathStrokeType(0.7f));
        }
        g.setColour(ink.withAlpha(0.56f));g.strokePath(bell,juce::PathStrokeType(0.8f));
    }
}

void RhythmScene::paintDunes(juce::Graphics& g,float w,float h,float t,float bloom)
{
    const float relief=settings()[4]*0.01f, wind=settings()[5]*0.01f;
    const juce::Point<float> sun{w*0.76f+cameraOffset.x*0.13f,h*(0.56f-0.042f*relief)+cameraOffset.y*0.13f};
    const float r=h*0.066f;
    halo(g,sun,r*(4.5f+2*bloom),juce::Colour(0xffffb66c).withAlpha(0.42f+0.25f*bloom));
    g.setGradientFill({juce::Colour(0xffffe3a1),sun.translated(0,-r),juce::Colour(0xffdd816b),sun.translated(0,r),false});
    g.fillEllipse(sun.x-r,sun.y-r,2*r,2*r);
    // Dust veils interrupt the lower limb; the distant crest then occludes the sun naturally.
    for(int i=0;i<16;++i){
        const float y=sun.y+r*(0.18f+i*0.048f);
        const float half=std::sqrt(std::max(0.f,r*r-(y-sun.y)*(y-sun.y)));
        g.setColour(juce::Colour(0xffab696d).withAlpha(0.06f+0.12f*noise(i+91)));
        g.drawLine(sun.x-half,y,sun.x+half,y,0.6f+noise(i+35));
    }
    const float age=float(time-reunionTime);
    for(int band=0;band<5;++band){
        const float d=(band+1)*0.2f;
        auto crest=[&](float u){
            const float phase=u*5.7f+band*1.9f-cameraOffset.x*d/w*5.7f;
            return h*(0.55f+band*0.092f+relief*(0.022f+0.055f*d)*(std::sin(phase)+0.36f*std::sin(2*phase+1)))+cameraOffset.y*d;
        };
        juce::Path ridge;ridge.startNewSubPath(0,crest(0));
        for(int j=1;j<=56;++j)ridge.lineTo(w*j/56.f,crest(j/56.f));
        auto sand=ridge;sand.lineTo(w,h);sand.lineTo(0,h);sand.closeSubPath();
        const auto ochre=juce::Colour(0xffc99483).interpolatedWith(juce::Colour(0xff674455),d);
        g.setGradientFill({ochre,0,crest(0)-h*0.04f,juce::Colour(0xff251e37),0,crest(0)+h*0.13f,false});
        g.fillPath(sand);
        g.setColour(juce::Colour(0xffffd29b).withAlpha(0.22f+0.45f*bloom));g.strokePath(ridge,juce::PathStrokeType(1.25f));
        // Broken wind ripples follow the slip face rather than a screen-space grid.
        for(int i=0;i<64;++i){
            const unsigned seed=unsigned(97*band+i);
            const float u=noise(seed+527), width=(0.003f+0.023f*noise(seed+178))*d;
            const float y=crest(u)+h*(0.005f+0.095f*noise(seed+888));
            juce::Path ripple;ripple.startNewSubPath(w*u,y);
            ripple.quadraticTo(w*(u+width*0.55f),y+h*0.002f,w*(u+width),y+crest(u+width)-crest(u));
            g.setColour(juce::Colour(0xfff6cd9b).withAlpha(0.07f+0.12f*d));
            g.strokePath(ripple,juce::PathStrokeType(0.65f+d*0.55f));
        }
        if(age>=0 && age<3.2f){
            // A saltation front crosses the lit crest, spilling downwind before it settles.
            for(int i=0;i<130;++i){
                const unsigned seed=unsigned(i+band*149);
                const float u=noise(seed+419), gust=passingLight(age,u*1.72f+band*0.13f,0.88f);
                if(gust<=0) continue;
                const float loft=h*d*gust*(0.015f+0.055f*noise(i+band*89));
                const float lift=loft*noise(seed+88);
                const auto grain=juce::Point<float>{w*u+loft*0.9f,crest(u)-lift};
                g.setColour(juce::Colour(0xffffd9a0).withAlpha(0.26f*gust));
                g.drawLine(grain.x,grain.y,grain.x+1.f+gust*d*6.f,grain.y-0.7f,0.7f+d*0.5f);
            }
        }
        const int grains=juce::roundToInt(38*wind+26*bloom);
        for(int i=0;i<grains;++i){
            const unsigned id=unsigned(i+band*89);
            const float u=std::fmod(noise(id+770)+t*(0.015f+0.035f*d),1.f);
            const float loft=noise(id+930)*(0.6f+0.4f*std::sin(t+i));
            const float y=crest(u)-h*loft*(0.018f*wind+0.085f*bloom);
            g.setColour(juce::Colour(0xffffdaa2).withAlpha(0.12f+0.24f*loft));
            g.drawLine(w*u,y,w*u+1+3*d,y-0.6f,0.85f);
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
        constexpr juce::uint32 colours[]{0xffffc497, 0xff98ffde, 0xffe5d9ff, 0xff8cffe4, 0xffffcf82, 0xffa0edff};
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
            // Both feet of each prominence sit inside the limb: painting the
            // lunar disc last hides their roots instead of leaving detached arcs.
            for(float side:{-1.f,1.f}){
                for(int strand=0;strand<5;++strand){
                    const float f=strand/4.f;
                    const float heat=passingLight(age,0.04f*f+(side>0 ? 0.08f : 0.f),1.32f);
                    if(heat==0) continue;
                    const float rise=r*heat*(0.38f+0.56f*f)*(0.55f+corona);
                    auto root=sun.translated(side*r*0.88f,r*0.30f);
                    auto tip=sun.translated(side*r*0.71f,-r*0.61f);
                    juce::Path loop;loop.startNewSubPath(root);
                    loop.cubicTo(root.translated(side*rise,-rise*0.12f),
                                 tip.translated(side*rise*1.25f,-rise*(side>0 ? 0.52f : 0.86f)),tip);
                    g.setColour(light.withAlpha(0.16f*heat));g.strokePath(loop,juce::PathStrokeType(r*0.075f));
                    g.setColour(juce::Colour(0xffffe1a5).withAlpha(heat*0.88f));g.strokePath(loop,juce::PathStrokeType(1.2f));
                }
            }
            // Find the centre of the same extinction lane used by buildBackdrop.
            // Cache in texture coordinates so the pulse follows its bends and parallax.
            static const auto fault=[]{
                std::array<float,129> samples{};
                for(size_t i=0;i<samples.size();++i){
                    const float u=float(i)/(samples.size()-1);
                    float v=0.79f-0.68f*u;
                    for(int step=0;step<5;++step)
                        v=0.79f-0.68f*u-0.10f*(mistAt(9*u,6*v)-0.5f);
                    samples[i]=v;
                }
                return samples;
            }();
            const auto sky=b.expanded(w*0.016f,h*0.012f).translated(cameraOffset.x*0.2f,cameraOffset.y*0.2f);
            auto fissure=[&](size_t i){
                return sky.getRelativePoint(float(i)/(fault.size()-1),fault[i]);
            };
            for(size_t i=1;i<fault.size();++i){
                const float u=float(i)/(fault.size()-1);
                const float arrival=0.31f+2.45f*std::abs(u-0.72f);
                const float wave=passingLight(age,arrival,0.72f)+0.27f*passingLight(age,arrival+0.58f,0.49f);
                if(wave==0) continue;
                const juce::Line<float> segment(fissure(i-1),fissure(i));
                g.setColour(light.withAlpha(0.13f*wave));g.drawLine(segment,h*0.012f);
                g.setColour(juce::Colour(0xfff4c8fa).withAlpha(0.72f*wave));g.drawLine(segment,1.35f);
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
            const float unfurl=celebrating ? passingLight(age,0.22f+2.35f*std::abs(u-0.5f),1.05f) : 0.f;
            columns[i]={w*u, lift*std::sin(a), lift*std::cos(a),
                        0.08f+0.10f*std::cos(u*10-t)+0.095f*unfurl, 0.26f+0.04f*std::sin(u*12+t)-0.055f*unfurl,
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
    else if(environment==3) paintLanternDeep(g,w,h,t,bloom);
    else if(environment==4) paintDunes(g,w,h,t,bloom);
    else if(environment==5){
        const float shimmer=settings()[4]*0.01f, frost=settings()[5]*0.01f;
        // Nested ice vaults frame the stage; the middle remains open for the score.
        const auto ice=juce::Colour::fromHSV(std::fmod(settings()[0]/360.f+hueBlend*motion().hueRange/360.f,1.f),
                                            0.48f,0.93f,1.f);
        for(int depth=0;depth<4;++depth){
            const float near=(depth+1)*0.25f;
            const float inset=(1.f-near)*0.30f;
            const auto shift=cameraOffset*near;
            auto at=[&](float x,float y){return juce::Point<float>{w*x,h*y}+shift;};
            const float foot=0.68f+near*0.23f, crown=0.20f-near*0.31f;
            juce::Path arch;
            arch.startNewSubPath(at(inset-0.04f,foot));
            arch.cubicTo(at(inset+0.03f,foot*0.60f),at(inset+0.02f,crown+0.16f),at(0.49f,crown));
            arch.cubicTo(at(0.89f-inset,crown+0.07f),at(0.90f-inset,foot*0.50f),at(1.f-inset,foot));
            const float pulse=0.5f+0.5f*std::sin(t*0.58f-depth*0.8f);
            const float resonance=passingLight(age,depth*0.49f,1.32f);
            auto shell=arch;
            shell.lineTo(at(1.1f,-0.2f));shell.lineTo(at(-0.1f,-0.2f));shell.closeSubPath();
            g.setGradientFill({ice.darker(1.2f).withAlpha(0.27f),at(inset,crown),
                               ice.darker(0.5f).withAlpha(0.08f),at(0.5f,foot),false});
            g.fillPath(shell);
            g.setColour(ice.withAlpha(0.09f+shimmer*pulse*0.20f+0.56f*resonance));
            g.strokePath(arch,juce::PathStrokeType(0.8f+near));
            // Compressed layers of ice catch light at different angles along each wall.
            for(int vein=1;vein<=7;++vein){
                const float k=vein/8.f;
                juce::Path stratum;
                stratum.startNewSubPath(at(inset-0.05f*k,foot+0.018f*k));
                stratum.cubicTo(at(inset-0.07f*k,foot*0.58f),at(inset+0.04f,crown+0.15f-0.08f*k),
                                at(0.49f+0.08f*k,crown-0.055f*k));
                stratum.cubicTo(at(0.83f-inset,crown+0.01f-0.05f*k),at(0.94f-inset+0.04f*k,foot*0.48f),
                                at(1.f-inset+0.028f*k,foot-0.035f*k));
                const float glint=0.5f+0.5f*std::cos(t*0.81f-depth-vein*0.73f);
                g.setColour(ice.withAlpha(0.035f+0.09f*glint*shimmer+0.29f*resonance*(1-k)));
                g.strokePath(stratum,juce::PathStrokeType(0.8f+near*k));
            }
            // Broken reflections anchor the vaults without mirroring the node paths.
            for(int ripple=0;ripple<10;++ripple){
                float y=foot+ripple*0.005f;
                float sway=0.006f*std::sin(t+ripple+depth);
                for(float bank:{inset,1.f-inset}){
                    auto centre=at(bank+sway,y);
                    float half=w*(0.009f+0.014f*noise(depth*19+ripple));
                    g.setColour(ice.withAlpha((0.09f+0.42f*resonance)*(1.f-ripple*0.09f)));
                    g.drawLine(centre.x-half,centre.y,centre.x+half,centre.y,0.85f);
                }
            }
        }
        for(int i=0;i<juce::roundToInt(96*frost);++i){
            const float depth=0.2f+0.8f*noise(i+961);
            const float x=w*(noise(i+116)+0.018f*std::sin(t*0.42f+i))+cameraOffset.x*depth;
            const float y=h*std::fmod(noise(i+693)+t*depth*0.023f,1.f);
            const float radius=0.5f+1.1f*depth;
            g.setColour(juce::Colour(0xffbcefff).withAlpha(0.08f+0.22f*depth+0.35f*bloom));
            g.fillEllipse(x,y,radius*2,radius);
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
        // A long swell steepens in the foreground. The same displacement
        // carries the moon path, foam and lesser wavelets over its shoulder.
        auto swell=[&](float u,float depth){
            if(!celebrating) return 0.f;
            const float arrival=0.08f+1.68f*std::sqrt(depth)+0.32f*std::abs(u-x/w);
            return passingLight(age,arrival,0.88f)-0.30f*passingLight(age,arrival+0.56f,0.62f);
        };
        const float amplitude=h*(0.032f+0.00052f*settings()[4]);
        for (int crest = 0; crest < 14; ++crest)
        {
            float depth = (crest + 1) / 15.f;
            juce::Path breaker;
            for (int j = 0; j <= 80; ++j)
            {
                float u = j / 80.f;
                float y = horizon + (h - horizon) * depth - amplitude*depth*swell(u,depth) +
                          h * depth * settings()[4] * 0.00035f * std::sin(13 * u + depth * 8 - t * 1.6f);
                if (!j)
                    breaker.startNewSubPath(0, y);
                else
                    breaker.lineTo(u * w, y);
            }
            const float lip=std::max(0.f,swell(x/w,depth));
            if(lip>0){
                auto face=breaker;face.lineTo(w,h);face.lineTo(0,h);face.closeSubPath();
                const float surface=horizon+(h-horizon)*depth-amplitude*depth*lip;
                g.setGradientFill({juce::Colour(0xff86b5cf).withAlpha(0.38f*lip),x,surface,
                                   juce::Colours::transparentBlack,x,surface+h*(0.018f+0.045f*depth),false});
                g.fillPath(face);
            }
            g.setColour(moon.withAlpha(0.05f + 0.08f * depth + 0.35f*lip));
            g.strokePath(breaker, juce::PathStrokeType(0.8f));
        }
        // Short moonlit wavelets pack tightly at the horizon and widen toward the observer.
        for(int i=0;i<590;++i) {
            float distance=noise(i+852); distance*=distance;
            float extent=w*(0.016f+0.13f*distance);
            float dx=(noise(i+1234)+noise(i+4321)-1.f)*extent;
            float sway=std::sin(t*(0.7f+distance)+i);
            float px=x+dx+sway*extent*0.12f, py=horizon+(h-horizon)*distance-amplitude*distance*swell(px/w,distance);
            float shine=(1-std::abs(dx)/extent)*(0.5f+0.5f*sway);
            g.setColour(moon.withAlpha(shine*(settings()[5]/100.f)*(0.68f+0.30f*bloom)));
            g.fillRoundedRectangle(px,py,0.85f+distance*(1+6*shine),0.48f+distance*0.9f,0.5f);
        }
        for (int i = 0; i < 125; ++i)
        {
            float v = noise(5 * i + 13), u = noise(5 * i + 12), length = w * (0.005f + 0.05f * v);
            float xx = w*u+length*std::sin(t+i)+cameraOffset.x*v, yy = horizon + (h - horizon) * v-amplitude*v*swell(xx/w,v);
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
void RhythmScene::beginExposure()
{
    exposing=true;exposureEnd=-1;capturePending=false;
    exposureCount=exposureCursor=0;repaint();
}
void RhythmScene::endExposure()
{
    if(!exposing)return;
    exposing=false;exposureEnd=time;capturePending=exposureCount>0;repaint();
}
void RhythmScene::mouseDown(const juce::MouseEvent& event)
{
    if(cleanStage || !headsReady)return;
    float distance=18.f;
    pinLayer=-1;
    for(int l=0;l<studio::layerCount;++l)if(grid.probability[l]>0)
        for(int n=0;n<grid.bodies;++n){
            const auto p=paintedHeads[l*studio::BeatGrid::maxBodies+n].transformedBy(shot);
            const auto d=p.getDistanceFrom(event.position);
            if(d<distance){pinLayer=l;pinBody=n;distance=d;}
        }
    if(pinLayer>=0 && predictPitch && onAudition){
        pinPitch=predictPitch(pinLayer,pinBody,phase);
        onAudition(pinLayer,pinPitch);
    }
    repaint();
}
void RhythmScene::strike(studio::Hit h)
{
    laneHit[h.layer]=h.time;
    if((exposing || (exposureEnd>=0 && std::abs(h.time-exposureEnd)<0.00001)) && h.body<grid.bodies && getWidth()>0 && getHeight()>0){
        // Place each sounding note on its own strand by pitch: shared gates
        // stay legible as a constellation rather than one saturated pixel.
        auto p=pathPoint((h.note%24+1)/26.f,h.layer,h.body,flightRadius(grid.flight(h.position,h.layer,h.body).cycles,h.layer));
        exposurePoints[exposureCursor]={{p.x/getWidth(),p.y/getHeight()},h.layer,h.velocity};
        exposureCursor=(exposureCursor+1)%int(exposurePoints.size());
        exposureCount=std::min(exposureCount+1,int(exposurePoints.size()));
        if(h.fullReunion && exposing){endExposure();exposureEnd=h.time;}
    }
    if (h.fullReunion) reunionTime = h.time;
    linkedCount=-1;
    history[cursor] = h;
    cursor = (cursor + 1) % static_cast<int>(history.size());
    count = std::min(count + 1, static_cast<int>(history.size()));
}
// Precompute the same nearest simultaneous predecessor that the painter
// used to search for on every frame, including after the hit ring wraps.
void RhythmScene::refreshThreadLinks()
{
    int layers=0;
    for(int l=0;l<studio::layerCount;++l) if(grid.probability[l]>0) layers|=1<<l;
    if(linkedLayers==layers && linkedCount==count) return;
    linkedLayers=layers;linkedCount=count;
    std::array<int,256> order;
    int used=0;
    threadPredecessors.fill(-1);
    for(int i=0;i<count;++i)
        if((layers & (1<<history[i].layer)) && history[i].body<grid.bodies) order[used++]=i;
    std::sort(order.begin(),order.begin()+used,[&](int a,int b){
        return history[a].time==history[b].time ? a<b : history[a].time<history[b].time;
    });
    for(int j=1;j<used;++j)
        if(history[order[j-1]].time==history[order[j]].time) threadPredecessors[order[j]]=order[j-1];
}
float RhythmScene::flightRadius(double duration,int layer) const
{
    if (!equalSpeed) return 0;
    // A guaranteed phrase attack bounds every flight. Use the same scale
    // for every node: circumference is proportional to time until impact.
    int slowest = *std::max_element(grid.divisions.begin(), grid.divisions.end());
    for (int i=0;i<studio::layerCount;++i)
        if (grid.probability[i]>0) slowest = std::min(slowest,grid.divisions[i]);
    return float(duration * slowest / (2.0 * grid.phraseLength()))
           * (orbitScale*0.0022f*layerDistance[layer]*0.01f) * float(std::min(getWidth(),getHeight()));
}
float RhythmScene::travelProgress(float phaseOnPath,int strand) const
{
    if (equalSpeed) return phaseOnPath;
    // A monotone time warp staggers travel without shifting either audible endpoint.
    const float laneOffset = grid.bodies > 1 ? 2.f*strand/(grid.bodies-1)-1.f : 0.f;
    const float u = phaseOnPath + timingOffset*0.009f*laneOffset*phaseOnPath*(1-phaseOnPath);
    return u;
}
// Build each formation in stage pixels. A cumulative-length table lets the
// renderer sample the exact same curve for notes, tails and equal-speed travel.
juce::Point<float> RhythmScene::routeGate(int rank,float lane) const
{
    const bool across = formation==horizontal || formation==horizontalTrio;
    const bool trio = formation==horizontalTrio || formation==verticalTrio;
    const float rail=trio ? 0.26f+0.22f*(rank%3) : 0.48f;
    if(formation>=infinity){
        float height=0.45f;
        if(formation==sphere || formation==heart || formation==helix || formation==shellFan) height=0.76f;
        if(formation==square || formation==triangle || formation==pyramid || formation==hexagon || formation==compassStar)
            height=0.20f;
        if(formation==crescent) height=0.24f;
        return {getWidth()*0.5f,getHeight()*height};
    }
    if(trio) lane=((rank%3)+lane)/3.f;
    return across ? juce::Point<float>{getWidth()*lane,getHeight()*rail}
                  : juce::Point<float>{getWidth()*rail,getHeight()*lane};
}
namespace
{
// Each corner occupies an actual sample, including on triangles where 64
// intervals cannot be divided evenly. Flight sampling handles arc length later.
juce::Point<float> polygonSample(const juce::Point<float>* corners,int sides,int sample)
{
    const int edge=std::min(sides-1,sample*sides/64);
    const int start=(64*edge+sides-1)/sides, end=(64*(edge+1)+sides-1)/sides;
    const auto from=corners[edge], to=corners[(edge+1)%sides];
    return from+(to-from)*(float(sample-start)/(end-start));
}
}
void RhythmScene::buildRoute(int layer,int body)
{
    int slots=0,slot=0;
    for(int i=0;i<studio::layerCount;++i)
        if(!autoBalance || grid.probability[i]>0){ ++slots; if(i<layer) ++slot; }
    const int rank=body*slots+slot;
    const float spread=visibleSpread()*0.01f;
    const float index=(rank+0.5f)/(std::max(1,slots)*grid.bodies);
    const float lane=juce::jlimit(0.06f,0.94f,0.5f+(index-0.5f)*0.84f*spread+linePlacement[layer]*0.004f);
    auto& route=routes[layer*studio::BeatGrid::maxBodies+body];
    route.gate=routeGate(rank,lane);
    const float side=rank%2 ? -1.f : 1.f;
    const float extent=std::min(getWidth()*0.44f,getHeight()*0.34f);
    const float coverage=equalSpeed ? pointCoverage*0.01f : 1.f;
    const float bearing=tau*((index-0.5f)*spread*coverage+pointPlacement[layer]*0.005f);
    using Point=juce::Point<float>;
    std::array<Point,16> corners{};
    int sides=0;
    if(formation==cube){
        const Point axes[]={{0.83f,0.42f},{-0.83f,0.42f},{0,-0.91f}};
        const auto a=axes[rank%3], b=axes[(rank+1)%3];
        corners={Point{},a,a+b,b}; sides=4;
    }else if(formation==pyramid){
        const Point base[]={{-0.93f,1.22f},{-0.19f,1.01f},{0.91f,1.31f},{0.16f,1.72f}};
        corners={Point{},base[rank%4],base[(rank+1)%4]}; sides=3;
    }else if(formation==square){
        corners={Point{},Point{0.68f,0},Point{0.68f,1.36f},Point{-0.68f,1.36f},Point{-0.68f,0}};
        sides=5; // gate is at the centre of the top edge
    }else if(formation==triangle || formation==hexagon || formation==compassStar){
        sides=formation==triangle ? 3 : formation==hexagon ? 6 : 16;
        for(int j=0;j<sides;++j){
            const float angle=tau*j/sides;
            const float radius=formation==compassStar && j%2 ? 0.34f : 0.81f;
            corners[j]={radius*std::sin(angle),0.81f-radius*std::cos(angle)};
        }
    }
    const float nest=1.f-index*spread*coverage*0.46f;
    const float rotation=pointPlacement[layer]*0.005f*tau;
    route.guide.clear();
    for(int i=0;i<=64;++i){
        const float u=i/64.f, a=tau*u;
        const float reach=i==64 ? 0.f : std::sin(a*0.5f);
        const float loop=i==64 ? 0.f : std::sin(a);
        juce::Point<float> p;
        if(sides){
            p=polygonSample(corners.data(),sides,i);
        }else if(formation==heart){
            // Parametric heart, with its lower cusp at the strike point.
            const float c=std::cos(a);
            p={loop*loop*loop,(13*c+5*std::cos(2*a)-2*std::cos(3*a)+std::cos(4*a)-17)/17.f};
        }else if(formation==helix){
            // Full-height strands define the double helix; shorter journeys
            // cross a rung before returning down the opposite strand.
            const float height=rank%4==0 ? 1.f : 0.16f+0.80f*index;
            const float z=height*juce::jlimit(0.f,1.f,std::min(i,64-i)/28.f);
            const float cross=juce::jlimit(-1.f,1.f,(32-i)/4.f);
            p={side*cross*(0.12f+0.33f*spread*coverage)*std::sin(2.5f*tau*z),-1.67f*z};
        }else if(formation==crescent){
            const float y=(1.f-std::cos(a))*0.76f;
            p={std::abs(loop)*(loop>=0 ? 0.79f : 0.24f)-0.15f*y,y};
        }else if(formation==shellFan){
            const float rib=bearing*0.35f;
            p=Point{0.18f*loop*reach,-reach*(1.45f+0.10f*std::cos(7*rib))}.rotatedAboutOrigin(rib);
        }else if(formation==infinity){
            p={side*extent*reach,extent*loop*reach*(0.16f+0.32f*spread)};
            p=p.rotatedAboutOrigin(bearing*0.12f);
        }else if(formation==sphere){
            p={extent*loop*std::cos(bearing),-extent*(1.f-std::cos(a))*0.86f};
        }else if(formation==nautilus){
            p=juce::Point<float>{extent*reach,0}.rotatedAboutOrigin(bearing+a*0.76f);
        }else if(formation==wings){
            p={side*extent*reach*(0.67f+0.31f*loop),extent*reach*(0.64f*std::cos(a)+0.27f*side*loop)};
            p=p.rotatedAboutOrigin(bearing*0.28f);
        }else if(formation==ripple){
            const float radius=extent*reach*(0.74f+0.20f*std::cos(3*a));
            p=juce::Point<float>{0,-radius}.rotatedAboutOrigin(bearing+0.58f*loop);
        }else if(formation==lotus){
            p=juce::Point<float>{extent*0.24f*loop*reach,-extent*reach}.rotatedAboutOrigin(bearing);
        }else {
            const bool across=formation==horizontal || formation==horizontalTrio;
            const bool trio=formation==horizontalTrio || formation==verticalTrio;
            const float travel=side*(1.f-std::abs(2*u-1))*(across?getHeight():getWidth())*(trio?0.17f:0.31f);
            p=across ? juce::Point<float>{0,travel} : juce::Point<float>{travel,0};
        }
        if(formation>=heart){
            const float scale=formation==helix || formation==shellFan ? 1.f : nest;
            p=(p*(extent*scale)).rotatedAboutOrigin(formation==shellFan ? rotation*0.65f : rotation);
            if(i==0 || i==64) p={};
        }
        route.samples[i]=p;
        route.distance[i]=i ? route.distance[i-1]+p.getDistanceFrom(route.samples[i-1]) : 0.f;
        if(i) route.guide.lineTo(p); else route.guide.startNewSubPath(p);
    }
}
juce::Point<float> RhythmScene::pathPoint(float progress, int layer, int body, float radius) const
{
    if(formation>=horizontal){
        const auto& route=routes[layer*studio::BeatGrid::maxBodies+body];
        const float total=route.distance.back();
        if(total<=0) return route.gate;
        const float distance=juce::jlimit(0.f,1.f,progress)*total;
        const auto it=std::lower_bound(route.distance.begin()+1,route.distance.end(),distance);
        const int i=int(it-route.distance.begin());
        const float f=(distance-route.distance[i-1])/std::max(0.00001f,route.distance[i]-route.distance[i-1]);
        const auto offset=route.samples[i-1]+(route.samples[i]-route.samples[i-1])*f;
        return route.gate+offset*(equalSpeed ? tau*radius/total : 1.f);
    }
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
    const float spreadAmount = visibleSpread()/100.f;
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
    if(formation>=horizontal && formation<infinity){
        const int rails=(formation==horizontalTrio || formation==verticalTrio)?3:1;
        for(int i=0;i<rails;++i){
            const auto start=routeGate(i,0.04f), end=routeGate(i,0.96f);
            g.setColour(opal.withAlpha(0.08f));g.drawLine({start,end},5.f);
            g.setColour(opal.withAlpha(0.58f));g.drawLine({start,end},1.f);
        }
        return;
    }
    if(formation>=infinity) eye=routeGate(0,0);
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
    if(environment>=3) return; // Water shafts, dust and ice strata supply their own motion.
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
    if(b.isEmpty())return;
    int mask=0;
    if(autoBalance) for(int i=0;i<studio::layerCount;++i)
        if(grid.probability[i]>0) mask|=1<<i;
    const std::array<int,6> geometry{getWidth(),getHeight(),grid.bodies,environment,int(formation),mask | (int(equalSpeed)<<8)};
    const auto& placement = pointTarget ? pointPlacement : linePlacement;
    const auto now=juce::Time::getMillisecondCounterHiRes();
    if(geometry!=orbitKey || ((orbitSpread!=visibleSpread() || placement!=orbitPlacement || orbitCoverage!=pointCoverage) && now>=nextOrbitRefresh)){
        nextOrbitRefresh=now+33.;
        orbitKey=geometry;orbitSpread=visibleSpread();orbitPlacement=placement;orbitCoverage=pointCoverage;
        if(formation>=horizontal)
            for(int l=0;l<studio::layerCount;++l) for(int n=0;n<grid.bodies;++n) buildRoute(l,n);
        guideViewport={};
    }

    // The slow-moving sky need not be rebuilt on every 5 ms node update.
    // Composite a reusable native surface; note trails, hits and the highway
    // still draw at full display resolution on every refresh.
    if(pinLayer>=0 && (pinBody>=grid.bodies || grid.probability[pinLayer]<=0))pinLayer=-1;
    shot={};
    if(shotCamera){
        const double until=(grid.nextMeeting(phase,true)-phase)*pattern.cycleSeconds();
        const double since=time-reunionTime;
        constexpr double leads[]{3,6,10,16};constexpr float zooms[]{1.25f,1.5f,1.85f,2.25f};
        float t=float(std::clamp(until/leads[shotLead],0.,1.));
        if(reunionTime>=0)t=std::min(t,float(std::clamp((since-1.5)/4.,0.,1.)));
        const float zoom=1.f+(zooms[closeShot]-1)*t*t*(3-2*t);
        int focus=pinLayer;
        for(int l=0;focus<0 && l<studio::layerCount;++l)if(grid.probability[l]>0)focus=l;
        auto anchor=b.getCentre();
        if(focus>=0){
            const int n=pinLayer<0?0:pinBody;
            const auto flight=grid.flight(phase,focus,n);
            anchor=pathPoint(travelProgress(float(flight.phase),n),focus,n,flightRadius(flight.cycles,focus));
        }
        shot=juce::AffineTransform::scale(zoom).translated(
            std::clamp(b.getCentreX()-anchor.x*zoom,b.getWidth()*(1-zoom),0.f),
            std::clamp(b.getCentreY()-anchor.y*zoom,b.getHeight()*(1-zoom),0.f));
    }
    const float stageScale=g.getInternalContext().getPhysicalPixelScaleFactor();
    g.saveState();g.addTransform(shot);
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
    // Cache the faint, stationary guide strokes separately from full-rate moving notes.
    // The surface is stage-sized, never proportional to the largest orbit.
    std::array<studio::BeatGrid::Flight,nodeSlots> flights{};
    std::array<juce::Point<float>,nodeSlots> heads{};
    std::array<float,nodeSlots> radii{};
    GuideBounds guides{};
    std::array<float,studio::layerCount> hues{};
    for(int l=0;l<studio::layerCount;++l){
        hues[l]=brushes[l][tint];
        if(grid.probability[l]<=0) continue;
        const double pos=std::max(0.,phase)*grid.divisions[l]*0.5;
        for(int n=0;n<grid.bodies;++n){
            const int slot=l*studio::BeatGrid::maxBodies+n;
            auto& window=flightWindows[slot];
            if(pos<window.before || pos>=window.after)
                window=grid.flightWindow(static_cast<int64_t>(std::floor(pos)),l,n);
            const auto flight=flights[slot]=grid.sampleFlight(pos,l,window);
            const float r=radii[slot]=flightRadius(flight.cycles,l);
            if(equalSpeed && formation<horizontal){
                if(r<=1) continue;
                const auto centre=(pathPoint(0,l,n,r)+pathPoint(0.5f,l,n,r))*0.5f;
                guides[slot]={centre.x-r,centre.y-r,2*r,2*r};
            }else{
                // A positive extent marks an active route; variable-distance
                // guides must also invalidate when their next trip length changes.
                guides[slot]={0,0,equalSpeed ? r : 1.f,1.f};
            }
        }
    }
    const float guideScale=equalSpeed && formation<horizontal
        ? std::min(1.f,1280.f/std::max(1.f,b.getWidth()))
        : stageScale;
    const int gw=std::max(1,juce::roundToInt(b.getWidth()*guideScale));
    const int gh=std::max(1,juce::roundToInt(b.getHeight()*guideScale));
    if(hues!=guideHues || guides!=previousGuides || guideViewport!=getLocalBounds()
       || guideImage.getWidth()!=gw || guideImage.getHeight()!=gh){
        guideViewport=getLocalBounds();previousGuides=guides;guideHues=hues;
        if(guideImage.getWidth()!=gw || guideImage.getHeight()!=gh)
            guideImage=juce::Image(juce::Image::ARGB,gw,gh,true);
        else guideImage.clear(guideImage.getBounds());
        juce::Graphics ink(guideImage);
        ink.addTransform(juce::AffineTransform::scale(gw/b.getWidth(),gh/b.getHeight()));
        for(int l=0;l<studio::layerCount;++l){
            ink.setColour(nodeColour(l).withAlpha(0.13f));
            for(int n=0;n<grid.bodies;++n){
                const int slot=l*studio::BeatGrid::maxBodies+n;
                if(guides[slot].isEmpty()) continue;
                if(formation>=horizontal){
                    const auto& route=routes[slot];
                    const float scale=equalSpeed ? tau*radii[slot]/std::max(0.001f,route.distance.back()) : 1.f;
                    // Outline the first strand of each layer so that even the
                    // unoccupied faces are legible before the first attack.
                    const bool outline=formation>=heart && n==0;
                    if(formation>=heart)
                        ink.setColour(nodeColour(l).withAlpha(outline ? 0.48f : formation==helix ? 0.26f : 0.15f));
                    ink.strokePath(route.guide,juce::PathStrokeType(outline ? 1.25f : 0.7f),juce::AffineTransform::scale(scale).translated(route.gate));
                }else if(equalSpeed){
                    ink.drawEllipse(guides[slot],0.75f);
                }else{
                    juce::Path orbit;orbit.startNewSubPath(pathPoint(0,l,n));
                    for(int j=1;j<=48;++j) orbit.lineTo(pathPoint(j/48.f,l,n));
                    ink.strokePath(orbit,juce::PathStrokeType(0.75f));
                }
            }
        }
    }
    g.setColour(juce::Colours::white);g.drawImage(guideImage,b);
    for (int l = 0; l < studio::layerCount; ++l)
    {
        auto colour = nodeColour(l);
        if (grid.probability[l] <= 0)
            continue;
        for (int s = 0; s < grid.bodies; ++s)
        {
            const int slot=l*studio::BeatGrid::maxBodies+s;
            const auto flight=flights[slot];
            const float radius=radii[slot];
            const float u = travelProgress(float(flight.phase),s), tail=std::max(0.f,u-brushes[l][length]*0.01f);
            auto curve=[&](float t){return pathPoint(t,l,s,radius);};
            const auto p=heads[slot]=curve(u);
            // Arc length bounds every point of the tail, including its halo.
            if(equalSpeed && !b.expanded(80.f+radius*tau*(u-tail)).contains(p)) continue;
            juce::Path trail;
            if(equalSpeed && formation<horizontal){
                const auto& circle=previousGuides[l*studio::BeatGrid::maxBodies+s];
                if(!circle.isEmpty()){
                    const auto centre=circle.getCentre();
                    const auto gate=curve(0);
                    const float start=std::atan2(gate.x-centre.x,centre.y-gate.y);
                    trail.addCentredArc(centre.x,centre.y,radius,radius,0,start-tau*tail,start-tau*u,true);
                }
            }else if(!elasticTrails && u>tail){
                trail.startNewSubPath(curve(tail));
                if(formation>=horizontal){
                    // Walk exactly the cached guide vertices between these arc
                    // lengths. Joining only the two endpoints cuts off corners.
                    const auto& route=routes[slot];
                    const float total=route.distance.back();
                    const float scale=equalSpeed && total>0 ? tau*radius/total : 1.f;
                    const auto endDistance=u*total;
                    auto it=std::upper_bound(route.distance.begin(),route.distance.end(),tail*total);
                    for(;it!=route.distance.end() && *it<endDistance;++it){
                        auto vertex=route.samples[size_t(it-route.distance.begin())];
                        trail.lineTo(route.gate+vertex*scale);
                    }
                }else{
                    // The original variable-speed guides have 48 line segments.
                    for(int j=int(tail*48)+1;j<48 && j/48.f<u;++j) trail.lineTo(curve(j/48.f));
                }
                trail.lineTo(p);
            }else{
                auto tangent=[&](float t){
                    float a=std::max(0.f,t-0.001f), b=std::min(1.f,t+0.001f);
                    return (curve(b)-curve(a))/(b-a);
                };
                if(u>tail){
                    auto start=curve(tail);
                    trail.startNewSubPath(start);
                    const int segments=std::max(1,juce::roundToInt(std::ceil((u-tail)*20.f)));
                    const float delta=(u-tail)/segments;
                    auto slope=tangent(tail)*(delta/3);
                    for(int j=1;j<=segments;++j){
                        const float endT=tail+j*delta;
                        const auto end=j==segments ? p : curve(endT);
                        const auto endSlope=tangent(endT)*(delta/3);
                        trail.cubicTo(start+slope,end-endSlope,end);
                        start=end;slope=endSlope;
                    }
                }
            }
            const float bloom=brushes[l][glow]*0.01f;
            if(bloom>0 && u>tail){
                g.setColour(colour.withAlpha(0.17f*bloom));
                g.strokePath(trail,juce::PathStrokeType(2+6*bloom,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
            }
            g.setColour(colour.withAlpha(0.60f));
            g.strokePath(trail,juce::PathStrokeType(1.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
            float r = (l == 1 ? 5.5f : 2.5f) + 1.7f*(grid.bodies-s)/grid.bodies;
            r*=brushes[l][size]*0.01f;
            if(brushes[l][haloStrength]>0) halo(g,p,r*3,colour.withAlpha(0.0038f*brushes[l][haloStrength]));
            g.setColour(colour.withAlpha(0.84f));
            g.drawEllipse(p.x - r, p.y - r, r * 2, r * 2, brushes[l][stroke]*0.01f);
        }
    }
    const float sparks = settings()[2] * 0.01f;
    const float canvasScale = juce::jlimit(0.75f, 1.6f, std::min(b.getWidth(), b.getHeight()) / 440.f);
    const float reach = canvasScale * (1.f + 4.f * sparks * sparks * sparks);
    const int sparksPerHit=juce::roundToInt(sparks*18.f);
    const int visibleHits=int(std::count_if(history.begin(),history.begin()+count,[&](const auto& hit){
        return grid.probability[hit.layer]>0 && hit.body<grid.bodies && time>=hit.time && time-hit.time<1.25;
    }));
    const bool batch=visibleHits*sparksPerHit>=128;
    burst.clear();
    if(showSyncThreads) refreshThreadLinks();
    for (int i = 0; i < count; ++i)
    {
        auto &h = history[i];
        const float age = static_cast<float>(time - h.time);
        if (h.body >= grid.bodies || grid.probability[h.layer] <= 0 || age < 0 || age >= 1.25f)
            continue;
        auto p = pathPoint(0.f, h.layer, h.body);
        auto c = nodeColour(h.layer);
        float life = 1 - age / 1.25f;
        // Link neighbouring attacks in the exact same sample, including individual chord tones.
        if(const int j=threadPredecessors[i]; showSyncThreads && j>=0)
        {
            const auto &other = history[j];
            auto q = heads[other.layer*studio::BeatGrid::maxBodies+other.body];
            auto end = heads[h.layer*studio::BeatGrid::maxBodies+h.body];
            juce::Path thread;
            thread.startNewSubPath(q);
            auto mid = (end + q) * 0.5f;
            thread.quadraticTo(mid.translated(0, -0.08f * end.getDistanceFrom(q)), end);
            g.setColour(c.withAlpha(0.18f * life * life));
            g.strokePath(thread, juce::PathStrokeType(5 * life + 1));
            g.setColour(juce::Colours::ivory.withAlpha(0.85f * life * life));
            g.strokePath(thread, juce::PathStrokeType(1.1f));
        }
        float r = (h.layer == 1 ? 10.f : 5.f) + age * 27;
        halo(g, p, r * 2, c.withAlpha(life * 0.45f));
        g.setColour(c.withAlpha(life * life));
        g.drawEllipse(p.x - r, p.y - r, 2 * r, 2 * r, 1.4f);
        const auto sparkColour=c.interpolatedWith(juce::Colours::white,life*0.8f).withAlpha(life);
        g.setColour(sparkColour);
        for (int k = 0; k < sparksPerHit; ++k)
        {
            unsigned key = h.note * 57 + k * 3 + h.body * 101 + h.layer * 79;
            const float a = noise(key) * tau;
            const float speed = (18 + noise(key + 1) * 66) * h.velocity * reach;
            const juce::Point<float> velocity(std::cos(a) * speed, std::sin(a) * speed);
            auto q = (p + velocity * age).translated(0.f, 12.f * age * age);
            float rr = life * (1 + noise(key + 2) * 2);
            auto tail=q-velocity*std::min(age,0.018f*sparks);
            if(batch){burst.push_back({q,tail,rr,sparkColour});continue;}
            g.drawLine(juce::Line<float>(tail,q),0.8f*life);
            g.fillEllipse(q.x - rr, q.y - rr, rr * 2, rr * 2);
        }
    }
    if(!burst.empty()) compositeBurst(g,b);
    paintedHeads=heads;headsReady=true;
    if(pinLayer>=0 && !cleanStage){
        const int slot=pinLayer*studio::BeatGrid::maxBodies+pinBody;
        juce::Path route; route.startNewSubPath(pathPoint(0,pinLayer,pinBody,radii[slot]));
        for(int i=1;i<=64;++i)route.lineTo(pathPoint(i/64.f,pinLayer,pinBody,radii[slot]));
        g.fillAll(juce::Colours::black.withAlpha(0.38f));
        g.setColour(nodeColour(pinLayer));g.strokePath(route,juce::PathStrokeType(1.6f));
        g.drawEllipse(juce::Rectangle<float>(18,18).withCentre(heads[slot]),2);
    }
    const float exposureAlpha=exposing?1.f:exposureEnd>=0?float(std::clamp(1.-(time-exposureEnd)/5.,0.,1.)):0.f;
    if(exposureAlpha>0)for(int i=0;i<exposureCount;++i){
        auto star=exposurePoints[i];const float x=star.point.x*b.getWidth(),y=star.point.y*b.getHeight();
        g.setColour(nodeColour(star.layer).withAlpha(0.65f*exposureAlpha));
        const float r=1.f+star.energy*2;g.drawLine(x-r*2,y,x+r*2,y);g.drawLine(x,y-r*2,x,y+r*2);
    }
    g.restoreState();
    const int active = static_cast<int>(std::count_if(grid.probability.begin(), grid.probability.end(),
                                                      [](float value) { return value > 0; }));
    const auto& theme=GlassTheme::current();
    const auto& ui=theme.colours();
    if (active > 0 && !cleanStage)
    {
        const double recentWindow=phase-0.55/pattern.cycleSeconds();
        const bool justAligned=recentWindow<0 || grid.nextMeeting(recentWindow,true)<=phase;
        const double until = (grid.nextMeeting(phase, true) - phase) * pattern.cycleSeconds();
        const auto label = justAligned ? juce::String(active * grid.bodies) + " points aligned"
                                        : "All points in " + juce::String(until, 1) + " s";
        auto badge = juce::Rectangle<float>(12, 10, 160, 23);
        theme.surface(g,badge,ui.panel.withAlpha(0.88f),false,false,true);
        g.setColour(ui.text());
        g.setFont(theme.font(12.f));
        g.drawText(label, badge, juce::Justification::centred);
        badge.translate(0,25);
        theme.surface(g,badge,ui.panel.withAlpha(0.88f),false,false,true);
        g.setColour(ui.text());
        auto since = reunionTime>=0 ? juce::String(std::max(0.,time-reunionTime),1)+" s" : juce::String("--");
        g.drawText("Since convergence: "+since,badge,juce::Justification::centred);
    }
    if(portraitSafe){
        const float width=std::min(b.getWidth(),b.getHeight()*9.f/16);
        g.setColour(ui.text().withAlpha(0.4f));g.drawRect(juce::Rectangle<float>(width,width*16/9).withCentre(b.getCentre()),1);
    }
    if(pinLayer>=0 && !cleanStage){
        auto flight=flights[pinLayer*studio::BeatGrid::maxBodies+pinBody];
        if(predictPitch)pinPitch=predictPitch(pinLayer,pinBody,phase);
        auto label=juce::String(studio::layers[pinLayer])+" #"+juce::String(pinBody+1)+"   "+
            juce::MidiMessage::getMidiNoteName(pinPitch,true,true,4)+"   in "+juce::String((1-flight.phase)*flight.cycles*pattern.cycleSeconds(),2)+" s";
        auto area=b.reduced(8).removeFromTop(25).removeFromRight(198);
        theme.surface(g,area,ui.panel.withAlpha(0.9f),false,false,true);
        g.setColour(nodeColour(pinLayer));
        g.setFont(theme.font(13.f));g.drawText(label,area,juce::Justification::centred);
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
        theme.surface(g,badge,ui.panel.withAlpha(0.9f),false,false,true);
        g.setColour(ui.text());g.setFont(theme.font(14.f));
        g.drawText(juce::String(measuredFps)+" stage FPS",badge,juce::Justification::centred);
    } else {fpsStart=0;fpsFrames=0;}
    for(const auto& [caption, opacity]:captions){
        const int width = juce::roundToInt(b.getWidth()*0.78f);
        auto box = juce::Rectangle<int>(width, 84).withCentre(
            {juce::roundToInt(b.getCentreX()), juce::roundToInt(b.getHeight()*0.10f+12-5*opacity)});
        std::array<juce::Colour,studio::layerCount> palette;
        for(int l=0;l<studio::layerCount;++l)palette[l]=nodeColour(l);
        const juce::Point<int> dimensions{width,getHeight()};
        if(styledCaption!=caption || dimensions!=captionSize || palette!=captionPalette || captionVoices!=styledVoices){
            styledCaption=caption;captionSize=dimensions;captionPalette=palette;styledVoices=captionVoices;
            juce::AttributedString words(caption);
            words.setFont(juce::Font(juce::FontOptions("Georgia",juce::jlimit(20.f,42.f,b.getHeight()*0.035f),juce::Font::plain)));
            words.setJustification(juce::Justification::centred);
            words.setColour(juce::Colours::ivory);
            auto highlight=[&](const juce::String& term,int layer){
                for(int pos=0;(pos=caption.indexOfIgnoreCase(pos,term))>=0;pos+=term.length()){
                    const int end=pos+term.length();
                    if((pos==0 || !juce::CharacterFunctions::isLetter(caption[pos-1])) &&
                       (end==caption.length() || !juce::CharacterFunctions::isLetter(caption[end])))
                        words.setColour({pos,end},nodeColour(layer));
                }
            };
            static const char* aliases[]{"piano","keys","bass","synth","choir","reed","sub","copper","celesta","tine","flute","brass","lead","strings"};
            for(int l=0;l<studio::layerCount;++l){
                const int voice=captionVoices[l];
                highlight(aliases[voice],l);highlight(studio::sounds[voice],l);
            }
            for(int l=0;l<studio::layerCount;++l)highlight(studio::layers[l],l);
            captionLayout.createLayout(words,float(width));
        }
        g.beginTransparencyLayer(opacity);
        captionLayout.draw(g,box.toFloat());
        g.endTransparencyLayer();
    }
    if(cleanStage && videoOpacity<1) g.fillAll(juce::Colours::black.withAlpha(1-videoOpacity));
}

// Dense reunions produce thousands of tiny translucent primitives. Rasterise
// only their occupied bounds at device resolution, then submit one image.
// Premultiplied source-over keeps overlapping sparks luminous without thinning them.
void RhythmScene::compositeBurst(juce::Graphics& g,juce::Rectangle<float> canvas)
{
    const float scale=g.getInternalContext().getPhysicalPixelScaleFactor();
    juce::Rectangle<int> area;
    for(const auto& spark:burst){
        auto bounds=juce::Rectangle<float>(spark.head*scale,spark.tail*scale);
        const auto pixels=bounds.expanded(scale*spark.radius+1.f).getSmallestIntegerContainer();
        area=area.getUnion(pixels);
    }
    area=area.getIntersection((canvas*scale).getSmallestIntegerContainer());
    if(area.isEmpty())return;
    const int width=area.getWidth(),height=area.getHeight();
    if(burstSurface.getWidth()<width || burstSurface.getHeight()<height)
        burstSurface=juce::Image(juce::Image::ARGB,std::max(burstSurface.getWidth(),(width+63)&~63),
                                 std::max(burstSurface.getHeight(),(height+63)&~63),true,juce::SoftwareImageType());
    {
        juce::Image::BitmapData pixels(burstSurface,0,0,width,height,juce::Image::BitmapData::writeOnly);
        for(int y=0;y<height;++y) std::memset(pixels.getLinePointer(y),0,size_t(width)*sizeof(juce::PixelARGB));
        for(const auto& spark:burst){
            const auto head=spark.head*scale-area.getPosition().toFloat();
            const auto tail=spark.tail*scale-area.getPosition().toFloat();
            const auto vector=head-tail;
            const float length2=vector.getDotProduct(vector);
            const float invLength2=length2>0 ? 1.f/length2 : 0.f;
            const float tip=spark.radius*scale+0.5f;
            const float streak=0.4f*spark.colour.getFloatAlpha()*scale+0.5f;
            const auto rect=juce::Rectangle<float>(head,tail).expanded(std::max(tip,streak)).getSmallestIntegerContainer()
                                .getIntersection({0,0,width,height});
            const auto source=spark.colour.getPixelARGB();
            for(int y=rect.getY();y<rect.getBottom();++y){
                auto* row=reinterpret_cast<juce::PixelARGB*>(pixels.getLinePointer(y));
                for(int x=rect.getX();x<rect.getRight();++x){
                    const juce::Point<float> p{x+0.5f,y+0.5f};
                    const float along=juce::jlimit(0.f,1.f,(p-tail).getDotProduct(vector)*invLength2);
                    const float line2=p.getDistanceSquaredFrom(tail+vector*along);
                    const float dot2=p.getDistanceSquaredFrom(head);
                    if(line2<streak*streak)
                        row[x].blend(source,unsigned(255*std::min(1.f,streak-std::sqrt(line2))));
                    if(dot2<tip*tip)
                        row[x].blend(source,unsigned(255*std::min(1.f,tip-std::sqrt(dot2))));
                }
            }
        }
    }
    g.setColour(juce::Colours::white);
    g.drawImage(burstSurface.getClippedImage({0,0,width,height}),area.toFloat()/scale,juce::RectanglePlacement::stretchToFit);
}

void RhythmScene::paintForecast(juce::Graphics &g, juce::Rectangle<float> canvas) const
{
    const auto rail=canvas.reduced(16,10).removeFromBottom(std::min(trackHeight,canvas.getHeight()*0.38f));
    const auto& theme=GlassTheme::current();
    if(cleanStage)
    {
        g.setColour(juce::Colour(0xe00d1421));
        g.fillRoundedRectangle(rail,12);
    }
    else
        theme.surface(g,rail,theme.colours().background.withAlpha(0.88f),false,false,true);
    const float gate=rail.getX()+112,span=rail.getRight()-gate-12,mid=rail.getCentreY();
    auto yAt=[&](int lane,float u){return mid+(lane-2.f)*rail.getHeight()*0.18f*(1-0.46f*u);};
    for(int lane=0;lane<studio::layerCount;++lane){
        const auto ink=nodeColour(lane);
        const float y=yAt(lane,0);
        const float flash=grid.probability[lane]>0 ? std::clamp(float(1-(time-laneHit[lane])/0.13),0.f,1.f) : 0.f;
        g.setColour(ink.withAlpha(0.12f+0.65f*flash));
        g.drawLine(gate,y,gate+span,yAt(lane,1),1.5f);
        const float radius=std::clamp(rail.getHeight()*0.055f,2.8f,8.f);
        if(flash>0) halo(g,{gate,y},radius*3,ink.withAlpha(flash));
        g.setColour(ink.interpolatedWith(juce::Colours::white,flash));
        g.drawEllipse(gate-radius,y-radius,2*radius,2*radius,1.4f);
        const float fontHeight=std::clamp(rail.getHeight()*0.15f,8.f,15.f);
        const int fontStyle=flash>0.2f ? juce::Font::bold : juce::Font::plain;
        g.setFont(cleanStage ? juce::Font(juce::FontOptions("Bahnschrift",fontHeight,fontStyle))
                             : theme.font(fontHeight,fontStyle));
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
