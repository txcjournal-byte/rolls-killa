#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace rk
{

/** Snapshot-based undo/redo of the whole plugin state (last 20 changes). */
class UndoHistory
{
public:
    static constexpr int kMaxUndoSteps = 20;

    void reset (const juce::ValueTree& state)
    {
        states.clear();
        states.push_back (state.createCopy());
        index = 0;
    }

    /** Records a new state if it differs from the current one. Returns true if recorded. */
    bool push (const juce::ValueTree& state)
    {
        if (! states.empty() && states[(size_t) index].isEquivalentTo (state))
            return false;

        states.erase (states.begin() + index + 1, states.end());
        states.push_back (state.createCopy());
        while ((int) states.size() > kMaxUndoSteps + 1)
            states.erase (states.begin());
        index = (int) states.size() - 1;
        return true;
    }

    bool canUndo() const noexcept { return index > 0; }
    bool canRedo() const noexcept { return index + 1 < (int) states.size(); }

    juce::ValueTree undo() { return canUndo() ? states[(size_t) --index].createCopy() : juce::ValueTree(); }
    juce::ValueTree redo() { return canRedo() ? states[(size_t) ++index].createCopy() : juce::ValueTree(); }

    const juce::ValueTree& current() const { return states[(size_t) index]; }
    int getNumUndoSteps() const noexcept { return index; }

private:
    std::vector<juce::ValueTree> states;
    int index = 0;
};

} // namespace rk
