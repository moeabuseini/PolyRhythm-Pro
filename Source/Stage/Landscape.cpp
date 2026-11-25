#include "Landscape.h"
#include <array>
#include <cmath>

namespace {
float hash(int x,int y)
{
    uint32_t k=static_cast<uint32_t>(x)*374761393u+static_cast<uint32_t>(y)*668265263u;
    k=(k^(k>>13))*1274126177u;
    return static_cast<float>((k^(k>>16))>>8)/16777216.f;
}
float noise(float x,float y)
{
    int i=static_cast<int>(std::floor(x)), j=static_cast<int>(std::floor(y));
    float a=x-i,b=y-j;
    a=a*a*(3-2*a); b=b*b*(3-2*b);
    return juce::jmap(b,juce::jmap(a,hash(i,j),hash(i+1,j)),juce::jmap(a,hash(i,j+1),hash(i+1,j+1)));
}
float terrain(float x,float y)
{
    return 0.54f*noise(x,y)+0.27f*noise(2*x+9,2*y)+0.13f*noise(4*x,4*y+5)+0.06f*noise(9*x,9*y);
}
}

juce::Image landscape::lunarDisc()
{
    constexpr int size=384;
    juce::Image image(juce::Image::ARGB,size,size,true,juce::SoftwareImageType());
    juce::Image::BitmapData pixels(image,juce::Image::BitmapData::writeOnly);
    struct Crater { float x,y,r; };
    std::array<Crater,76> craters;
    for(int i=0;i<static_cast<int>(craters.size());++i)
        craters[i]={2*hash(i,1)-1,2*hash(i,2)-1,0.012f+0.12f*std::pow(hash(i,3),3.f)};
    for(int y=0;y<size;++y) for(int x=0;x<size;++x) {
        float u=(2*x+1.f)/size-1,v=(2*y+1.f)/size-1, rr=u*u+v*v;
        if(rr>=1) continue;
        float z=std::sqrt(1-rr);
        float maria=terrain(4*u+2*z,4*v-z), grain=terrain(32*u,32*v);
        float relief=0;
        for(const auto& c:craters) {
            float dx=u-c.x,dy=v-c.y;
            if(std::abs(dx)>1.2f*c.r || std::abs(dy)>1.2f*c.r) continue;
            float d=std::sqrt(dx*dx+dy*dy)/c.r;
            float rim=std::max(0.f,1-5.f*std::abs(d-0.94f));
            relief+=rim*(0.15f-0.32f*(dx+dy)/c.r)-0.13f*std::max(0.f,1-d);
        }
        float diffuse=std::max(0.f,-0.80f*u-0.38f*v+0.36f*z);
        float albedo=juce::jlimit(0.15f,1.1f,0.32f+0.62f*maria+0.26f*grain+relief);
        float lum=juce::jlimit(0.f,1.f,(0.09f+diffuse)*albedo*1.35f);
        auto colour=juce::Colour(0xff161a31).interpolatedWith(juce::Colour(0xffffe6cb),lum);
        pixels.setPixelColour(x,y,colour.withAlpha(std::min(1.f,(1-rr)*size*0.5f)));
    }
    return image;
}

juce::Image landscape::mountainRange(int layer)
{
    constexpr int width=1000,height=160;
    juce::Image image(juce::Image::ARGB,width,height,true,juce::SoftwareImageType());
    juce::Image::BitmapData pixels(image,juce::Image::BitmapData::writeOnly);
    constexpr juce::uint32 shadow[]{0xff555a81,0xff444866,0xff292f49,0xff151f33};
    constexpr juce::uint32 light[]{0xffaeb1c8,0xff8b90a8,0xff656d84,0xff3a475a};
    auto ridge=[layer](float u){
        // Overlapping spurs produce hard summits and saddles; short octaves break up the skyline.
        const float along=u*7+layer*0.43f;
        const int nearest=static_cast<int>(std::floor(along));
        float massif=0;
        for(int j=nearest-1;j<=nearest+1;++j) {
            float summit=j+hash(j,layer+23)*0.72f;
            float height=0.32f+0.50f*hash(j,layer+41);
            massif=std::max(massif,height-0.66f*std::abs(along-summit));
        }
        return std::clamp(0.16f+0.78f*massif+0.27f*terrain(27*u,layer+9.f)
                          +0.046f*noise(140*u,layer+3.f),0.f,1.f);
    };
    for(int x=0;x<width;++x) {
        float u=x/static_cast<float>(width);
        float elevation=ridge(u), slope=ridge(u+0.003f)-ridge(u-0.003f);
        float top=height*(1-elevation);
        for(int y=std::max(0,static_cast<int>(top));y<height;++y) {
            float down=(y-top)/height;
            float folds=terrain(36*u+down*(0.7f+16*slope),8*down+layer);
            folds+=0.14f*(noise(140*u+down*15,33*down)-0.5f);
            float facet=juce::jlimit(0.f,1.f,0.35f-4.f*slope+1.3f*(folds-0.45f)-0.25f*down);
            auto colour=juce::Colour(shadow[layer]).interpolatedWith(juce::Colour(light[layer]),facet);
            if(layer<3) {
                float snow=juce::jlimit(0.f,1.f,5*(elevation-0.63f-down*1.9f)+1.8f*(folds-0.5f));
                colour=colour.interpolatedWith(juce::Colour(0xffd5cfda),snow*(0.22f+0.62f*facet));
            }
            pixels.setPixelColour(x,y,colour.withAlpha(juce::jlimit(0.f,1.f,1+y-top)));
        }
    }
    return image;
}
