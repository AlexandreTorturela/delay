/*
  ==============================================================================

    Tempo.h
    Created: 24 May 2026 12:33:53am
    Author:  tortu

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class Tempo
{
public:
    void reset() noexcept;

    void update(const juce::AudioPlayHead* playHead) noexcept;

    double getMillisecondsForNoteLength(int index) const noexcept;

    double getTempo() const noexcept { return bpm; }

private:
    double bpm = 120.0;
};
