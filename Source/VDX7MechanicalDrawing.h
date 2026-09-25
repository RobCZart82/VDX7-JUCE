#pragma once
#include <JuceHeader.h>
#include <cmath>

// Value-driven geometry: no independent animation clock, no audio-thread work.
namespace VDX7MechanicalDrawing
{
inline void wheel(juce::Graphics& g, juce::Rectangle<float> b, float value, bool hover)
{
    juce::Graphics::ScopedSaveState save(g);
    const float w = b.getWidth(), h = b.getHeight(), cx = b.getCentreX();
    const float unit = w / 48.0f;
    const auto outer = b.withWidth(w - 4.0f * unit).withCentre(b.getCentre()).reduced(0.45f * unit);

    g.setColour(juce::Colours::black.withAlpha(0.48f));
    g.fillRoundedRectangle(outer.translated(0.7f * unit, 1.35f * unit), 3.8f * unit);
    juce::ColourGradient bezel(juce::Colour(0xff151b1d), outer.getX(), outer.getCentreY(),
                               juce::Colour(0xff0d1112), outer.getRight(), outer.getCentreY(), false);
    bezel.addColour(0.10, juce::Colour(0xff465052));
    bezel.addColour(0.20, juce::Colour(0xff202829));
    bezel.addColour(0.48, juce::Colour(0xff606969));
    bezel.addColour(0.57, juce::Colour(0xff252c2d));
    bezel.addColour(0.86, juce::Colour(0xff414a4b));
    g.setGradientFill(bezel);
    g.fillRoundedRectangle(outer, 3.8f * unit);
    g.setColour(juce::Colour(hover ? 0xff7d9798 : 0xff687a7d));
    g.drawRoundedRectangle(outer.reduced(0.55f * unit), 3.2f * unit, 0.72f * unit);

    const auto cavity = outer.reduced(3.1f * unit, 2.7f * unit);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff020506), cavity.getTopLeft(),
        juce::Colour(0xff101719), cavity.getBottomLeft(), false));
    g.fillRoundedRectangle(cavity, 2.3f * unit);
    g.setColour(juce::Colour(0xff18252a));
    g.drawRoundedRectangle(cavity, 2.3f * unit, 0.65f * unit);

    const auto body = cavity.reduced(w * 0.12f, h * 0.045f);
    const float cy = body.getCentreY(), radius = body.getHeight() * 0.5f;
    juce::ColourGradient rubber(juce::Colour(0xff06090a), body.getX(), cy,
                                juce::Colour(0xff0a0f10), body.getRight(), cy, false);
    rubber.addColour(0.14, juce::Colour(0xff303735));
    rubber.addColour(0.45, juce::Colour(0xff535955));
    rubber.addColour(0.72, juce::Colour(0xff1d2524));
    rubber.addColour(0.94, juce::Colour(0xff090d0e));
    g.setGradientFill(rubber);
    g.fillRoundedRectangle(body, 2.4f * unit);
    g.setColour(juce::Colour(0xff8a9189).withAlpha(0.16f));
    g.drawRoundedRectangle(body.reduced(0.7f * unit), 1.8f * unit, 0.45f * unit);

    g.reduceClipRegion(body.toNearestInt());
    const float rotation = (0.5f - juce::jlimit(0.0f, 1.0f, value)) * 2.16f;
    // Each rib lives at a fixed angle on the cylinder. Sine projection compresses
    // spacing toward the far ends while the angular phase follows the value.
    for (int rib = -48; rib <= 48; ++rib)
    {
        const float angle = rib * 0.075f + rotation;
        if (std::abs(angle) > 1.46f) continue;
        const float y = cy + radius * std::sin(angle);
        const float depth = std::cos(angle);
        const float inset = (1.0f - depth) * 2.1f * unit;
        const float thickness = juce::jmax(0.45f, 1.55f * unit * depth);
        g.setColour(juce::Colour(0xff030605).withAlpha(0.8f));
        g.fillRect(body.getX() + inset, y, body.getWidth() - 2.0f * inset, thickness);
        g.setColour(juce::Colour(0xffa7ada5).withAlpha(0.22f * depth));
        g.fillRect(body.getX() + inset, y - thickness * 0.55f,
                   body.getWidth() - 2.0f * inset, thickness * 0.38f);
    }

    // The ridge phase and indicator share one value: mouse drag or wheel input
    // visibly turns the cylinder rather than merely moving an unrelated marker.
    const float markerY = cy + radius * std::sin(rotation);
    const float markerH = juce::jmax(1.35f, 2.45f * unit * std::cos(rotation));
    juce::ColourGradient shade(juce::Colours::black.withAlpha(0.68f), cx, body.getY(),
                               juce::Colours::black.withAlpha(0.68f), cx, body.getBottom(), false);
    shade.addColour(0.22,juce::Colours::black.withAlpha(0.04f));
    shade.addColour(0.68,juce::Colours::black.withAlpha(0.04f));
    g.setGradientFill(shade);
    g.fillRect(body);
    const auto marker = juce::Rectangle<float>(body.getX() + 2.0f * unit,
                                                markerY - markerH * 0.5f,
                                                body.getWidth() - 4.0f * unit, markerH);
    g.setColour(juce::Colour(0xff087b82).withAlpha(0.9f));
    g.fillRoundedRectangle(marker.expanded(0.9f * unit, 0.5f * unit), 0.8f * unit);
    juce::ColourGradient markerFace(juce::Colour(0xff68c7bb), marker.getX(), marker.getY(),
                                    juce::Colour(0xff368f88), marker.getX(), marker.getBottom(), false);
    g.setGradientFill(markerFace);
    g.fillRoundedRectangle(marker, 0.55f * unit);
    g.setColour(juce::Colour(0xff68c7bb).withAlpha(0.78f));
    g.drawLine(marker.getX() + unit, marker.getY() + 0.55f * unit,
               marker.getRight() - unit, marker.getY() + 0.55f * unit, 0.55f * unit);
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
    g.setColour(juce::Colour(0xff68c7bb));
    const float y=top.getY()+top.getHeight()*0.48f;
    g.fillRect(top.getX()+u,y,top.getWidth()-2*u,1.6f*u);
    g.setColour(juce::Colour(0xff00797d));
    g.drawLine(top.getRight()-u,y,top.getRight()-2*u,top.getBottom(),1.1f*u);
}
}
