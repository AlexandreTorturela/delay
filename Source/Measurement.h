/*
  ==============================================================================

    Measurement.h
    Created: 26 May 2026 11:43:23am
    Author:  tortu

  ==============================================================================
*/

#pragma once

#include <atomic>

struct Measurement
{
	std::atomic<float> value;

	void reset() noexcept
	{
		value.store(0.0f);
	}

	void updateIfGreater(float newValue) noexcept
	{
		auto oldValue = value.load();
		while (newValue > oldValue && !value.compare_exchange_weak(oldValue, newValue));
	}

	float readAndReset() noexcept
	{
		return value.exchange(0.0f);
	}
};