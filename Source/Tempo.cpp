/*
  ==============================================================================

    Tempo.cpp
    Created: 24 May 2026 12:33:53am
    Author:  tortu

  ==============================================================================
*/

#include "Tempo.h"

static std::array<double, 16> noteLengthMultipliers =
{
	0.125,			// 1/32
	0.5 / 3.0,		// 1/16 triplet
	0.1875,			// 1/32 dotted
	0.25,			// 1/16
	1.0 / 3.0,		// 1/8 triplet
	0.375,			// 1/16 dotted
	0.5,			// 1/8
	2.0 / 3.0,		// 1/4 triplet
	0.75,			// 1/8 dotted
	1.0,			// 1/4
	4.0 / 3.0,		// 1/2 triplet
	1.5,			// 1/4 dotted
	2.0,			// 1/2
	8.0 / 3.0,		// whole triplet
	3.0,			// 1/2 dotted
	4.0				// whole
};

void Tempo::reset() noexcept
{
	bpm = 120.0;
}

void Tempo::update(const juce::AudioPlayHead* playHead) noexcept
{
	reset();
	
	if (playHead == nullptr)
		return;

	const auto opt = playHead->getPosition();

	if (!opt.hasValue())
		return;

	const auto& pos = *opt;

	if (pos.getBpm().hasValue())
		bpm = *pos.getBpm();
}

double Tempo::getMillisecondsForNoteLength(int index) const noexcept
{
	return 60000.0 * noteLengthMultipliers[size_t(index)] / bpm;
}
