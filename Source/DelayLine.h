/*
  ==============================================================================

    DelayLine.h
    Created: 25 May 2026 11:36:28am
    Author:  tortu

  ==============================================================================
*/

#pragma once

#include <memory>

class DelayLine
{
public:
	void setMaximumDelayInSamples(int maxLengthInSamples);
	void reset() noexcept;

	void write(float input) noexcept;
	float read(int delayInSamples) const noexcept;

	int getBufferLength() const noexcept { return bufferLength; }

private:
	std::unique_ptr<float[]> buffer;
	int bufferLength = 0;
	int writeIndex = 0;
};
