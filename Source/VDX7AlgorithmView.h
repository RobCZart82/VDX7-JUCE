#pragma once
#include <JuceHeader.h>
#include "VDX7Algorithms.h"

class VDX7AlgorithmView final : public juce::Component
{
public:
    VDX7AlgorithmView() { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    std::function<void(int)> onOperatorSelected;
    void setState(int algorithm, int selected, int feedback)
    {
        algorithm = juce::jlimit(1,32,algorithm);
        selected = juce::jlimit(0,5,selected);
        feedback = juce::jlimit(0,7,feedback);
        if (algorithm_==algorithm && selected_==selected && feedback_==feedback) return;
        algorithm_=algorithm; selected_=selected; feedback_=feedback;
        repaint();
    }
    int selectedOperator() const noexcept { return selected_; }
    int algorithm() const noexcept { return algorithm_; }
    int operatorAt(juce::Point<float> position) const
    {
        for (int op=0;op<6;++op) if (nodeBounds(op).contains(position)) return op;
        return -1;
    }
    juce::Rectangle<float> nodeBounds(int op) const
    {
        auto levels=VDX7Algorithms::levels(VDX7Algorithms::get(algorithm_));
        auto columns=VDX7Algorithms::columns(VDX7Algorithms::get(algorithm_));
        const float sx=getWidth()/200.0f, sy=getHeight()/124.0f;
        return {(columns[op]*180.0f-3.0f)*sx,(90.0f-levels[op]*26.0f)*sy,22.0f*sx,16.0f*sy};
    }
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (!isEnabled() || !event.mods.isLeftButtonDown()) return;
        const int op=operatorAt(event.position);
        if (op>=0 && onOperatorSelected) onOperatorSelected(op);
    }
    void paint(juce::Graphics& g) override
    {
        const auto& graph=VDX7Algorithms::get(algorithm_);
        const float sx=getWidth()/200.0f, sy=getHeight()/124.0f;
        const auto cyan=juce::Colour(0xff00e7e7);
        const auto wire=juce::Colour(0xff82a4ac);
        const float stroke=juce::jmax(0.8f,sx);
        auto line=[&](juce::Point<float> a,juce::Point<float> b)
        { g.drawLine({a,b},stroke); };
        auto arrow=[&](juce::Point<float> tip)
        {
            juce::Path p;
            p.addTriangle(tip.x,tip.y,tip.x-2.5f*sx,tip.y-4*sy,tip.x+2.5f*sx,tip.y-4*sy);
            g.fillPath(p);
        };
        g.setColour(isEnabled() ? wire : wire.withAlpha(0.3f));
        for (int target=0;target<6;++target)
            for (int source=0;source<6;++source)
                if (graph.inputs[target] & (1<<source))
                {
                    const auto a=nodeBounds(source), b=nodeBounds(target);
                    const auto end=juce::Point<float>(b.getCentreX(),b.getY());
                    line({a.getCentreX(),a.getBottom()},end);
                    arrow(end);
                }
        const float busY=112*sy;
        float left=200*sx,right=0;
        for (int op=0;op<6;++op)
            if (graph.carriers & (1<<op))
            {
                auto b=nodeBounds(op);
                left=std::min(left,b.getCentreX()); right=std::max(right,b.getCentreX());
                line({b.getCentreX(),b.getBottom()},{b.getCentreX(),busY});
            }
        line({left,busY},{right,busY});
        // Feedback is drawn around the source/target, never mistaken for a
        // forward modulation link. F=0 remains visible but subdued.
        const auto from=nodeBounds(graph.feedbackFrom), to=nodeBounds(graph.feedbackTo);
        g.setColour(feedback_>0 ? cyan : juce::Colour(0xff42575c));
        const float feedbackX=std::max(from.getRight(),to.getRight())+9*sx;
        const float feedbackY=to.getY()-9*sy;
        juce::Path loop;
        loop.startNewSubPath(from.getRight(),from.getCentreY());
        loop.lineTo(feedbackX,from.getCentreY());
        loop.lineTo(feedbackX,feedbackY);
        loop.lineTo(to.getCentreX(),feedbackY);
        loop.lineTo(to.getCentreX(),to.getY());
        g.strokePath(loop,juce::PathStrokeType(stroke));
        arrow({to.getCentreX(),to.getY()});
        for (int op=0;op<6;++op)
        {
            const auto b=nodeBounds(op);
            const bool selected=op==selected_, carrier=(graph.carriers & (1<<op))!=0;
            g.setColour(selected ? cyan : juce::Colour(carrier ? 0xff1a4249 : 0xff101f25));
            g.fillRoundedRectangle(b,2*sx);
            g.setColour(selected ? cyan : wire);
            g.drawRoundedRectangle(b,2*sx,stroke);
            g.setColour(selected ? juce::Colour(0xff052026) : juce::Colour(0xffe0eef0));
            g.setFont(juce::Font(juce::FontOptions(juce::jmax(8.0f,12*sy),juce::Font::bold)));
            g.drawText(juce::String(op+1),b,juce::Justification::centred);
        }
        g.setColour(wire);
        g.setFont(juce::Font(juce::FontOptions(juce::jmax(7.0f,8*sy))));
        g.drawText("OUT",juce::Rectangle<float>(left-10*sx,114*sy,20*sx,10*sy),juce::Justification::centred);
        g.drawText("F"+juce::String(feedback_),juce::Rectangle<float>(174*sx,110*sy,26*sx,14*sy),juce::Justification::centred);
    }
private:
    int algorithm_=1, selected_=0, feedback_=0;
};
