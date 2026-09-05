#pragma once
#include <JuceHeader.h>
#include <cmath>

// Value-driven geometry: no independent animation clock, no audio-thread work.
namespace VDX7MechanicalDrawing
{
inline void wheel(juce::Graphics& g, juce::Rectangle<float> b, float value, bool hover)
{
    juce::Graphics::ScopedSaveState save(g);
    const float w=b.getWidth(), h=b.getHeight(), cx=b.getCentreX();
    const float unit=w/48.0f;
    g.setColour(juce::Colour(0xff030607)); g.fillRoundedRectangle(b,2*unit);
    g.setColour(juce::Colour(hover ? 0xff52757a : 0xff3b484a));
    g.drawRoundedRectangle(b.reduced(unit),2*unit,unit);
    const auto body=b.reduced(w*0.18f,h*0.06f);
    const float cy=body.getCentreY(), radius=body.getHeight()*0.5f;
    juce::ColourGradient rubber(juce::Colour(0xff080b0c),body.getX(),cy,
                               juce::Colour(0xff0c1011),body.getRight(),cy,false);
    rubber.addColour(0.16,juce::Colour(0xff292c2b));
    rubber.addColour(0.46,juce::Colour(0xff393c39));
    rubber.addColour(0.83,juce::Colour(0xff1b201f));
    g.setGradientFill(rubber); g.fillRoundedRectangle(body,3*unit);
    g.reduceClipRegion(body.toNearestInt());
    const float rotation=(0.5f-juce::jlimit(0.0f,1.0f,value))*2.16f;
    // Each rib lives at a fixed angle on the cylinder. Sine projection compresses
    // spacing toward the far ends while the angular phase follows the value.
    for (int rib=-48;rib<=48;++rib)
    {
        const float angle=rib*0.075f+rotation;
        if (std::abs(angle)>1.46f) continue;
        const float y=cy+radius*std::sin(angle);
        const float depth=std::cos(angle);
        const float inset=(1-depth)*2*unit;
        const float thickness=juce::jmax(0.45f,1.45f*unit*depth);
        g.setColour(juce::Colour(0xff030605).withAlpha(0.8f));
        g.fillRect(body.getX()+inset,y,body.getWidth()-2*inset,thickness);
        g.setColour(juce::Colour(0xff81857c).withAlpha(0.40f*depth));
        g.fillRect(body.getX()+inset,y-thickness*0.65f,body.getWidth()-2*inset,thickness*0.55f);
    }
    // A fixed transverse stripe on the same rotating surface as the ribs.
    const float markerY=cy+radius*std::sin(rotation);
    const float markerH=juce::jmax(1.1f,2.2f*unit*std::cos(rotation));
    juce::ColourGradient shade(juce::Colours::black.withAlpha(0.9f),cx,body.getY(),
                               juce::Colours::black.withAlpha(0.9f),cx,body.getBottom(),false);
    shade.addColour(0.22,juce::Colours::transparentBlack);
    shade.addColour(0.68,juce::Colours::transparentBlack);
    g.setGradientFill(shade); g.fillRect(body);
    // Retain a readable cyan position indicator even at either end stop.
    g.setColour(juce::Colour(0xff00e7e7).withAlpha(0.65f+0.35f*std::cos(rotation)));
    g.fillRoundedRectangle(body.getX()+2*unit,markerY-markerH/2,
                           body.getWidth()-4*unit,markerH,0.5f*unit);
}

inline void faderCap(juce::Graphics& g, juce::Rectangle<float> b, bool hover, bool down)
{
    const float u=b.getWidth()/58.0f;
    const auto r=b.reduced(2*u,1*u);
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.fillRoundedRectangle(r.translated(2*u,4*u),2*u);
    auto top=r.withHeight(r.getHeight()*0.56f).translated(0,down?u:0);
    juce::ColourGradient face(juce::Colour(hover?0xff444b4b:0xff383e3e),top.getX(),top.getY(),
                              juce::Colour(0xff151a1c),top.getX(),top.getBottom(),false);
    g.setGradientFill(face); g.fillRoundedRectangle(top,1.4f*u);
    juce::Path front;
    front.startNewSubPath(top.getX(),top.getBottom()-u);
    front.lineTo(top.getRight(),top.getBottom()-u);
    front.lineTo(r.getRight()-3*u,r.getBottom());
    front.lineTo(r.getX()+3*u,r.getBottom()); front.closeSubPath();
    g.setColour(juce::Colour(0xff080c0e)); g.fillPath(front);
    g.setColour(juce::Colour(0xff596162));
    g.drawLine(top.getX()+u,top.getY()+u,top.getRight()-u,top.getY()+u,0.7f*u);
    g.setColour(juce::Colour(0xff00e7e7));
    const float y=top.getY()+top.getHeight()*0.48f;
    g.fillRect(top.getX()+u,y,top.getWidth()-2*u,1.6f*u);
    g.setColour(juce::Colour(0xff00797d));
    g.drawLine(top.getRight()-u,y,top.getRight()-2*u,top.getBottom(),1.1f*u);
}
}
