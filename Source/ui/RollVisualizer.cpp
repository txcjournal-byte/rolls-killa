#include "RollVisualizer.h"

#include "../PluginProcessor.h"

namespace rk::ui
{

namespace
{
    constexpr float kHeaderH = 26.0f;
    constexpr float kFooterH = 22.0f;
    constexpr float kPadX = 10.0f;
    constexpr int kBarsSelectorW = 146;
}

RollVisualizer::RollVisualizer (RollsKillaProcessor& p)
    : processor (p),
      barsChoice (p.getState(), params::bars, params::barsChoices)
{
    addAndMakeVisible (barsChoice);
    setRepaintsOnMouseActivity (false);
    startTimerHz (60);
}

void RollVisualizer::resized()
{
    barsChoice.setBounds (getLocalBounds().removeFromTop ((int) kHeaderH).reduced (6, 3)
                              .removeFromRight (kBarsSelectorW).withTrimmedLeft (40));
}

juce::Rectangle<float> RollVisualizer::headerArea() const
{
    return getLocalBounds().toFloat().removeFromTop (kHeaderH).reduced (kPadX, 0.0f);
}

juce::Rectangle<float> RollVisualizer::noteArea() const
{
    auto r = getLocalBounds().toFloat();
    r.removeFromTop (kHeaderH + 6.0f);
    r.removeFromBottom (kFooterH);
    return r.reduced (kPadX, 0.0f);
}

float RollVisualizer::beatToX (double beat) const
{
    const auto area = noteArea();
    const auto len = processor.getModel().getPattern().lengthBeats();
    return area.getX() + (float) (beat / juce::jmax (1.0, len)) * area.getWidth();
}

juce::Rectangle<float> RollVisualizer::lockBounds (int bar) const
{
    const auto x = beatToX (bar * (double) kBeatsPerBar);
    const auto header = headerArea();
    const auto bars = processor.getModel().getPattern().bars;
    const auto labelW = bars > 4 ? 22.0f : 48.0f;
    return { x + labelW, header.getY() + 5.0f, 16.0f, 16.0f };
}

void RollVisualizer::timerCallback()
{
    const auto version = processor.getPatternVersion();
    const auto playhead = processor.getPlayheadBeat();

    const auto waiting = processor.isWaitingForMidi();

    if (version != shownVersion || std::abs (playhead - lastPlayhead) > 1.0e-9 || waiting != lastWaiting)
    {
        lastWaiting = waiting;
        shownVersion = version;
        lastPlayhead = playhead;
        repaint();
    }
}

void RollVisualizer::paint (juce::Graphics& g)
{
    const auto& pattern = processor.getModel().getPattern();
    const auto bounds = getLocalBounds().toFloat();
    drawPanel (g, bounds, 8.0f);

    const auto area = noteArea();
    const auto len = pattern.lengthBeats();
    const auto bars = pattern.bars;
    const auto locks = processor.getLockedBars();
    const auto playhead = processor.getPlayheadBeat();
    const auto header = headerArea();
    const auto barsRect = barsChoice.getBounds().toFloat().withTrimmedLeft (-40.0f);

    // subtle backdrop inside the note area
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0b0b0d), area.getX(), area.getY(),
                                             juce::Colour (0xff111114), area.getX(), area.getBottom(), false));
    g.fillRect (area.expanded (kPadX - 2.0f, 4.0f));

    // grid
    for (int step = 0; step <= (int) (len * 4); ++step)
    {
        const auto beat = step * 0.25;
        const auto x = beatToX (beat);
        const auto isBar = step % 16 == 0;
        const auto isBeat = step % 4 == 0;
        g.setColour (isBar ? colours::outlineLight : isBeat ? colours::outline : colours::outline.withAlpha (0.35f));
        g.drawVerticalLine (juce::roundToInt (x), isBar ? header.getY() + 2.0f : area.getY(), isBar ? bounds.getBottom() - 4.0f : area.getBottom());
    }
    g.setColour (colours::outline);
    g.drawHorizontalLine (juce::roundToInt (area.getBottom()), area.getX(), area.getRight());

    // header: BAR n + lock
    for (int bar = 0; bar < bars; ++bar)
    {
        const auto x = beatToX (bar * (double) kBeatsPerBar);
        const auto lb = lockBounds (bar);
        if (lb.intersects (barsRect))
            continue;

        const auto locked = (locks & (1u << bar)) != 0;
        g.setColour (colours::textDim);
        g.setFont (labelFont (11.0f));
        g.drawText ((bars > 4 ? "" : "BAR ") + juce::String (bar + 1), juce::Rectangle<float> (x + 8.0f, header.getY(), 50.0f, kHeaderH),
                    juce::Justification::centredLeft, false);
        drawIcon (g, locked ? Icon::lock : Icon::unlock, lb.reduced (1.0f),
                  locked ? colours::accent : (hoverLock == bar ? colours::text : colours::textDark));
        if (locked)
        {
            g.setColour (colours::accent.withAlpha (0.05f));
            g.fillRect (juce::Rectangle<float>::leftTopRightBottom (x, area.getY(), beatToX ((bar + 1) * (double) kBeatsPerBar), area.getBottom()));
        }
    }
    g.setColour (colours::textDim);
    g.setFont (labelFont (11.0f));
    g.drawText ("BARS", barsRect.withWidth (40.0f), juce::Justification::centredLeft, false);

    // footer: beat numbers
    g.setFont (uiFont (11.0f, false));
    for (int beat = 0; beat < (int) len; ++beat)
    {
        const auto x0 = beatToX (beat), x1 = beatToX (beat + 1);
        g.setColour (colours::textDark);
        g.drawText (juce::String (beat % kBeatsPerBar + 1), juce::Rectangle<float> (x0, area.getBottom() + 3.0f, x1 - x0, kFooterH - 4.0f),
                    juce::Justification::centred, false);
    }

    // roll clusters glow
    int currentRoll = -2;
    double rollStart = 0.0;
    auto flushRoll = [&] (double endBeat)
    {
        const auto x0 = beatToX (rollStart) - 3.0f, x1 = beatToX (endBeat) + 3.0f;
        const auto r = juce::Rectangle<float>::leftTopRightBottom (x0, area.getY() + area.getHeight() * 0.1f, x1, area.getBottom());
        g.setGradientFill (juce::ColourGradient (colours::accent.withAlpha (0.0f), r.getX(), r.getY(),
                                                 colours::accent.withAlpha (0.16f), r.getX(), r.getBottom(), false));
        g.fillRect (r);
    };

    for (size_t i = 0; i < pattern.notes.size(); ++i)
    {
        const auto& n = pattern.notes[i];
        if (n.rollId != currentRoll)
        {
            if (currentRoll >= 0)
                flushRoll (pattern.notes[i - 1].beat);
            currentRoll = n.rollId;
            rollStart = n.beat;
        }
    }
    if (currentRoll >= 0 && ! pattern.notes.empty())
        flushRoll (pattern.notes.back().beat);

    // notes
    const auto pxPerBeat = area.getWidth() / (float) juce::jmax (1.0, len);
    for (size_t i = 0; i < pattern.notes.size(); ++i)
    {
        const auto& n = pattern.notes[i];
        const auto next = i + 1 < pattern.notes.size() ? pattern.notes[i + 1].beat : len;
        const auto spacingPx = (float) (next - n.beat) * pxPerBeat;
        const auto w = juce::jlimit (1.5f, n.rollId >= 0 ? 3.2f : 4.5f, spacingPx * 0.62f);
        const auto x = beatToX (n.beat);
        const auto h = juce::jmax (3.0f, area.getHeight() * 0.94f * (float) n.vel / 127.0f);
        const auto bar = juce::Rectangle<float> (x - w * 0.5f, area.getBottom() - h, w, h);

        auto colour = pitchColour (n.pitch);
        float hot = 0.0f;
        if (playhead >= 0.0)
        {
            auto age = playhead - n.beat;
            if (age < 0.0)
                age += len;
            if (age >= 0.0 && age < 0.35)
                hot = 1.0f - (float) (age / 0.35);
        }

        if (n.muted)
        {
            g.setColour (colours::textDark);
            g.drawRect (bar, 1.0f);
            continue;
        }

        if (hot > 0.0f)
        {
            g.setColour (colour.withAlpha (0.25f * hot));
            g.fillRoundedRectangle (bar.expanded (3.0f + 3.0f * hot, 3.0f), 3.0f);
            colour = colour.interpolatedWith (juce::Colours::white, 0.45f * hot);
        }

        g.setGradientFill (juce::ColourGradient (colour, bar.getX(), bar.getY(), colour.withMultipliedBrightness (0.55f),
                                                 bar.getX(), bar.getBottom(), false));
        g.fillRect (bar);

        if ((int) i == hoverNote)
        {
            g.setColour (juce::Colours::white);
            g.drawRect (bar.expanded (1.5f), 1.0f);
        }
    }

    // playhead
    if (playhead >= 0.0)
    {
        const auto x = beatToX (playhead);
        for (int i = 3; i > 0; --i)
        {
            g.setColour (colours::accent.withAlpha (0.12f));
            g.fillRect (juce::Rectangle<float> (x - (float) i * 1.5f, area.getY() - 4.0f, (float) i * 3.0f, area.getHeight() + 4.0f));
        }
        g.setColour (colours::accent);
        g.fillRect (juce::Rectangle<float> (x - 1.0f, area.getY() - 4.0f, 2.0f, area.getHeight() + 4.0f));
        juce::Path tri;
        tri.addTriangle (x - 6.0f, area.getY() - 10.0f, x + 6.0f, area.getY() - 10.0f, x, area.getY() - 3.0f);
        g.fillPath (tri);
    }

    // MIDI play mode, host running, no note held: explain the silence
    if (processor.isWaitingForMidi())
    {
        auto box = area.withSizeKeepingCentre (juce::jmin (area.getWidth() - 20.0f, 560.0f), 46.0f);
        g.setColour (colours::background.withAlpha (0.88f));
        g.fillRoundedRectangle (box, 6.0f);
        g.setColour (colours::accent.withAlpha (0.8f));
        g.drawRoundedRectangle (box, 6.0f, 1.0f);
        g.setColour (colours::text);
        g.setFont (labelFont (12.0f));
        g.drawText ("PLAY: MIDI - WAITING FOR A NOTE ON THIS CHANNEL", box.removeFromTop (26.0f), juce::Justification::centredBottom, false);
        g.setColour (colours::textDim);
        g.setFont (uiFont (11.5f, false));
        g.drawText ("put one long note in the piano roll, or switch PLAY to HOST", box, juce::Justification::centredTop, false);
    }

    // hover info
    if (hoverNote >= 0 && hoverNote < (int) pattern.notes.size())
    {
        const auto& n = pattern.notes[(size_t) hoverNote];
        juce::String info;
        info << "vel " << n.vel;
        if (n.pitch != 0)
            info << "   pitch " << (n.pitch > 0 ? "+" : "") << n.pitch;
        if (n.rollId >= 0)
        {
            const auto next = (size_t) hoverNote + 1 < pattern.notes.size() ? pattern.notes[(size_t) hoverNote + 1].beat : len;
            const auto prev = hoverNote > 0 ? pattern.notes[(size_t) hoverNote - 1].beat : -1.0;
            auto rate = rateIndexForStep (next - n.beat);
            if (rate < 0 && prev >= 0.0)
                rate = rateIndexForStep (n.beat - prev);
            if (rate >= 0)
                info << "   roll " << kRollRateNames[(size_t) rate];
        }
        if (n.muted)
            info << "   (muted)";
        g.setColour (colours::textDim);
        g.setFont (uiFont (11.0f, false));
        g.drawText (info, area.withHeight (14.0f).translated (0.0f, -2.0f), juce::Justification::topRight, false);
    }
}

int RollVisualizer::hitNote (juce::Point<float> p) const
{
    const auto area = noteArea();
    if (! area.expanded (4.0f, 8.0f).contains (p))
        return -1;

    const auto& notes = processor.getModel().getPattern().notes;
    int best = -1;
    float bestDist = 7.0f;
    for (size_t i = 0; i < notes.size(); ++i)
    {
        const auto d = std::abs (beatToX (notes[i].beat) - p.x);
        if (d < bestDist)
        {
            bestDist = d;
            best = (int) i;
        }
    }
    return best;
}

int RollVisualizer::hitLock (juce::Point<float> p) const
{
    const auto bars = processor.getModel().getPattern().bars;
    for (int bar = 0; bar < bars; ++bar)
        if (lockBounds (bar).expanded (4.0f).contains (p))
            return bar;
    return -1;
}

int RollVisualizer::rollFirstTick (int noteIndex) const
{
    const auto& notes = processor.getModel().getPattern().notes;
    if (noteIndex < 0 || noteIndex >= (int) notes.size() || notes[(size_t) noteIndex].rollId < 0)
        return -1;

    auto i = noteIndex;
    while (i > 0 && notes[(size_t) i - 1].rollId == notes[(size_t) noteIndex].rollId)
        --i;
    return RollModel::editKeyOf (notes[(size_t) i]);
}

void RollVisualizer::mouseMove (const juce::MouseEvent& e)
{
    const auto note = hitNote (e.position);
    const auto lock = hitLock (e.position);
    if (note != hoverNote || lock != hoverLock)
    {
        hoverNote = note;
        hoverLock = lock;
        setMouseCursor (lock >= 0 || note >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void RollVisualizer::mouseExit (const juce::MouseEvent&)
{
    hoverNote = hoverLock = -1;
    repaint();
}

void RollVisualizer::mouseDown (const juce::MouseEvent& e)
{
    dragging = false;
    dragNote = -1;

    if (const auto lock = hitLock (e.position); lock >= 0)
    {
        processor.setBarLocked (lock, (processor.getLockedBars() & (1u << lock)) == 0);
        repaint();
        return;
    }

    if (e.mods.isPopupMenu())
    {
        showNoteMenu (hitNote (e.position));
        return;
    }

    dragNote = hitNote (e.position);
    if (dragNote >= 0)
    {
        const auto& n = processor.getModel().getPattern().notes[(size_t) dragNote];
        const auto edits = processor.getEdits();
        const auto it = edits.find (RollModel::editKeyOf (n));
        dragStartVel = it != edits.end() && it->second.vel > 0 ? it->second.vel : n.vel;
    }
}

void RollVisualizer::mouseDrag (const juce::MouseEvent& e)
{
    if (dragNote < 0)
        return;

    const auto& notes = processor.getModel().getPattern().notes;
    if (dragNote >= (int) notes.size())
        return;

    const auto tick = RollModel::editKeyOf (notes[(size_t) dragNote]);
    if (std::abs (e.getDistanceFromDragStartY()) < 3)
        return;

    dragging = true;
    auto edits = processor.getEdits();
    auto edit = edits.count (tick) > 0 ? edits[tick] : NoteEdit {};
    edit.vel = juce::jlimit (1, 127, dragStartVel - juce::roundToInt ((float) e.getDistanceFromDragStartY() * 127.0f / noteArea().getHeight()));
    processor.setNoteEdit (tick, edit);
}

void RollVisualizer::mouseUp (const juce::MouseEvent& e)
{
    if (dragNote >= 0 && ! dragging && e.mouseWasClicked() && e.getNumberOfClicks() == 1)
    {
        const auto& notes = processor.getModel().getPattern().notes;
        if (dragNote < (int) notes.size())
        {
            const auto tick = RollModel::editKeyOf (notes[(size_t) dragNote]);
            auto edits = processor.getEdits();
            auto edit = edits.count (tick) > 0 ? edits[tick] : NoteEdit {};
            edit.muted = ! notes[(size_t) dragNote].muted;
            processor.setNoteEdit (tick, edit);
        }
    }

    dragNote = -1;
    dragging = false;
}

void RollVisualizer::showNoteMenu (int note)
{
    const auto& notes = processor.getModel().getPattern().notes;
    const auto hasNote = note >= 0 && note < (int) notes.size();
    const auto inRoll = hasNote && notes[(size_t) note].rollId >= 0;
    const auto hasEdits = ! processor.getEdits().empty();

    juce::PopupMenu menu;
    menu.addSectionHeader (hasNote ? (inRoll ? "ROLL NOTE" : "NOTE") : "PATTERN");
    menu.addItem (1, "Delete note", hasNote);
    menu.addItem (2, "Delete whole roll", inRoll);
    menu.addItem (3, hasNote && notes[(size_t) note].muted ? "Unmute note" : "Mute note", hasNote);
    menu.addSeparator();
    menu.addItem (4, "Restore all removed / edited notes", hasEdits);

    const auto key = hasNote ? RollModel::editKeyOf (notes[(size_t) note]) : -1;
    const auto rollKey = inRoll ? rollFirstTick (note) : -1;
    const auto muted = hasNote && notes[(size_t) note].muted;

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(),
                        [safe = juce::Component::SafePointer<RollVisualizer> (this), key, rollKey, muted] (int result)
                        {
                            if (safe == nullptr || result == 0)
                                return;

                            auto& proc = safe->processor;
                            auto edits = proc.getEdits();
                            auto editFor = [&edits] (int k) { return edits.count (k) > 0 ? edits[k] : NoteEdit {}; };

                            if (result == 1 && key >= 0)
                            {
                                auto e = editFor (key);
                                e.deleted = true;
                                proc.setNoteEdit (key, e);
                            }
                            else if (result == 2 && rollKey >= 0)
                            {
                                auto e = editFor (rollKey);
                                e.removeRoll = true;
                                proc.setNoteEdit (rollKey, e);
                            }
                            else if (result == 3 && key >= 0)
                            {
                                auto e = editFor (key);
                                e.muted = ! muted;
                                proc.setNoteEdit (key, e);
                            }
                            else if (result == 4)
                            {
                                proc.clearEdits();
                            }

                            proc.commitUndoStep();
                            safe->hoverNote = -1;
                            safe->repaint();
                        });
}

void RollVisualizer::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto note = hitNote (e.position);
    const auto tick = rollFirstTick (note);
    if (tick < 0)
        return;

    // the first click of the double-click toggled the mute - undo that
    auto edits = processor.getEdits();
    const auto& notes = processor.getModel().getPattern().notes;
    const auto clickedTick = RollModel::editKeyOf (notes[(size_t) note]);
    if (edits.count (clickedTick) > 0 && edits[clickedTick].muted)
    {
        edits[clickedTick].muted = false;
        processor.setNoteEdit (clickedTick, edits[clickedTick]);
        edits = processor.getEdits();
    }

    // cycle the roll rate 1/24 -> 1/32 -> 1/48 -> 1/64 -> 1/96 -> 1/24
    int first = note;
    while (first > 0 && notes[(size_t) first - 1].rollId == notes[(size_t) note].rollId)
        --first;
    const auto step = (size_t) first + 1 < notes.size() ? notes[(size_t) first + 1].beat - notes[(size_t) first].beat : kRollSteps[0];
    auto rate = rateIndexForStep (step);
    auto edit = edits.count (tick) > 0 ? edits[tick] : NoteEdit {};
    if (edit.rate >= 0)
        rate = edit.rate;
    edit.rate = (juce::jmax (0, rate) + 1) % numRollRates;
    processor.setNoteEdit (tick, edit);
}

} // namespace rk::ui
