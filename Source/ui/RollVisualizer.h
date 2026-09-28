#pragma once

#include "Widgets.h"

class RollsKillaProcessor;

namespace rk::ui
{

/**
    The hero window: the pattern as velocity bars, rolls as glowing clusters, pitch as colour,
    a playhead synced to the host (or preview). Click = mute/unmute a note, drag up/down = velocity,
    double-click a roll = change its speed,
    right-click = delete a note or the whole roll, lock icons = keep bars when pressing KILL.
*/
class RollVisualizer : public juce::Component, private juce::Timer
{
public:
    explicit RollVisualizer (RollsKillaProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Rectangle<float> noteArea() const;
    juce::Rectangle<float> headerArea() const;
    float beatToX (double beat) const;
    int hitNote (juce::Point<float> p) const;
    int hitLock (juce::Point<float> p) const;
    juce::Rectangle<float> lockBounds (int bar) const;
    int rollFirstTick (int noteIndex) const;
    void showNoteMenu (int noteIndex);

    RollsKillaProcessor& processor;
    SegmentedChoice barsChoice;

    int shownVersion = -1;
    double lastPlayhead = -1.0;
    bool lastWaiting = false;
    int hoverNote = -1, hoverLock = -1;

    // drag state
    int dragNote = -1;
    int dragStartVel = 0;
    bool dragging = false;
};

} // namespace rk::ui
